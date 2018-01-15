#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <fstream>
#include <queue>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include "serialib.h"


#if defined (_WIN32) || defined( _WIN64)
#define DEVICE_PORT "COM1"
#endif
#ifdef __linux__
#define DEVICE_PORT "/dev/ttyACM0"
#endif


#define emptySpace 0
#define XMark 2
#define OMark 1
#define vertical 180
#define horizontal 127

using namespace cv;
using namespace std;


Mat frame, templ, result;

string cameraWindow = "Kamera";
string resultWindow = "Rezultat";

int matchMethod;
int thresholdMethod;

int8_t place;

queue<Point> positionsOMarks;

int8_t board[] = { 0, 0, 0,
                   0, 0, 0,
                   0, 0, 0};

bool tempBoard[] = { 0, 0, 0,
                     0, 0, 0,
                     0, 0, 0};

/**< Struktura do komunikacji z arduino >**/
struct data{
  bool board[9];
  bool playerCanMove;
} order;


void MultipleMatching(int, void*);
void TakeOMarksPositons(void);
bool exists(const char* fileName);
int minMaxAlgorithm(int state, int8_t player);
bool isWin(int8_t player);
void savePositions(void);
bool kbhit(void);
bool canPlay(void);

 /**< Program glowny >**/

int main(int argc, char *argv[])
{
    char keyDown;
    bool humanMove = true;

    serialib Arduino;
    char Buffer[sizeof(order)];
    unsigned short Ret;


    Ret = Arduino.Open(DEVICE_PORT, 9600);
    if(Ret != 1){
        cout << "BLAD! : Nie mozna otworzyc portu!    Nr bledu: " << Ret << "  PORT: " << DEVICE_PORT;
        exit(EXIT_FAILURE);
    }

    VideoCapture capture = VideoCapture(1); //Wybor kamery
    templ = exists("kolko.jpg") ? imread("kolko.jpg", CV_LOAD_IMAGE_COLOR) : imread(argv[1], CV_LOAD_IMAGE_COLOR);

    if(templ.data == NULL){
        cout << "BLAD! : Nie znaleziono obrazu wzorca!";
        exit(EXIT_FAILURE);
    }

    namedWindow( cameraWindow, CV_WINDOW_AUTOSIZE );
    namedWindow( resultWindow, CV_WINDOW_AUTOSIZE );

    string trackbar_matchingMethod = "Method: \n 0: SQDIFF \n 1: SQDIFF NORMED \n 2: TM CCORR \n 3: TM CCORR NORMED \n 4: TM COEFF \n 5: TM COEFF NORMED";
    string trackbar_thresholdMethod = "Type: \n 0: Binary \n 1: Binary Inverted \n 2: Truncate \n 3: To Zero \n 4: To Zero Inverted";

    createTrackbar( trackbar_matchingMethod, cameraWindow, &matchMethod, 5, MultipleMatching);
    createTrackbar( trackbar_thresholdMethod, cameraWindow, &thresholdMethod, 4, MultipleMatching);

    do{
        for(int i = 0; i < 9; i++)
            board[i] = emptySpace;

        system("clear");
        cout << "Wyczysc plansze, wcisnij N aby gdy wykonasz swoj ruch." << flush;
        sleep(3);

        while(waitKey(20) != 27){
            capture >> frame;

            MultipleMatching(0, 0);

            for(int i = 0; i<9;i++)
                tempBoard[i]= false;

            TakeOMarksPositons();

            system("clear");
            cout << "Kamera:" << endl << flush;

            for(int i = 1; i <= 9; i++){
                cout << (tempBoard[i-1] ? 'O' : '_') << " " << flush;
                if(i%3 == 0)
                    cout << endl;
            }

            cout << endl << "Plansza gry:" << endl << flush;

            for(int i = 1; i <= 9; i++){
                cout << (board[i-1] == OMark ? 'O' : board[i-1] == XMark ? 'X' : '_') << flush;
                    if(i%3 == 0)
                        cout << endl;
            }

            if(humanMove == false){
                 /**< Wczytywanie danych z Arduino >**/
                Arduino.Read(Buffer,sizeof(order));
                memcpy(&order, Buffer, sizeof(order));
                humanMove = order.playerCanMove;
            }


            if(kbhit() && humanMove == true){
                keyDown = getchar();
                if(keyDown == 'n' && canPlay() == true){
                    humanMove = false;
                    savePositions();
                    minMaxAlgorithm(0, XMark);

                     /**< Wysylanie danych do Arduino>**/
                    order.board[place] = true;
                    order.playerCanMove = false;
                    memcpy(Buffer, &order, sizeof(order));
                    Arduino.Write(Buffer, sizeof(order));
                }
            }

            if(isWin(OMark)){
                cout << endl << "Wygrales, gratulacje!" << flush;
                break;
            }

            if(isWin(XMark)){
                cout << endl << "Maszyna wygrala." << flush;
                break;
            }

            if(!canPlay()){
                cout << endl << "Remis" << flush;
                break;
            }
        }
        cout << endl << "Chcesz zagrac jeszcze raz? (t/n)" << flush;
        keyDown = getchar();
    }while(keyDown == 't');

    Arduino.Close();

    return EXIT_SUCCESS;
}

/**< Szukanie wzokru kolka na kamerze  >**/
void MultipleMatching(int, void*){

    matchTemplate(frame, templ, result, matchMethod);

    double thresh = 0.4;
    threshold(result, result, thresh, 1., thresholdMethod);

    Mat resb;
    result.convertTo(resb, CV_8U, 255);

    vector<vector<Point> > contours;
    findContours(resb, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);

    for(int i = 0; i < contours.size(); ++i){
        Mat mask(result.rows, result.cols, uchar(0));
        drawContours(mask, contours, i, Scalar(255), CV_FILLED);

        Point maxPoint;
        double maxVal;
        minMaxLoc(result, NULL, &maxVal, NULL, &maxPoint, mask);

        positionsOMarks.push(maxPoint);

        rectangle(frame, Rect(maxPoint.x, maxPoint.y, templ.cols, templ.rows), Scalar::all(0), 2);
    }

    imshow(cameraWindow, frame);
    imshow(resultWindow, result);

    return;
}

/**< Rzutowanie pozycji z kamery na plansze oraz zapisanie danych w tablicy tymczasowej >**/
void TakeOMarksPositons(void){
    Point markerPosition;

    while(!positionsOMarks.empty()){
       markerPosition = positionsOMarks.front();
       positionsOMarks.pop();

       if(markerPosition.x <= vertical && markerPosition.y <= horizontal)
            tempBoard[0] = true;
       else if((markerPosition.x > vertical && markerPosition.x <= 2*vertical) && markerPosition.y <= horizontal)
            tempBoard[1] = true;
       else if((markerPosition.x > 2*vertical) && markerPosition.y <= horizontal)
            tempBoard[2] = true;

       else if(markerPosition.x <= vertical && (markerPosition.y > horizontal && markerPosition.y <= 2*horizontal))
            tempBoard[3] = true;
       else if((markerPosition.x > vertical && markerPosition.x <= 2*vertical) && (markerPosition.y > horizontal && markerPosition.y <= 2*horizontal))
            tempBoard[4] = true;
       else if((markerPosition.x > 2*vertical) && (markerPosition.y > horizontal && markerPosition.y <= 2*horizontal))
            tempBoard[5] = true;

       else if(markerPosition.x <= vertical && markerPosition.y > 2*horizontal)
            tempBoard[6] = true;
       else if((markerPosition.x > vertical && markerPosition.x <= 2*vertical) &&  markerPosition.y > 2*horizontal)
            tempBoard[7] = true;
       else if((markerPosition.x > 2*vertical) && markerPosition.y > 2*horizontal)
            tempBoard[8] = true;
    }

    return;
}

/**< Sprawdzanie czy gracz wygrał >**/
bool isWin(int8_t player){

    /**< Sprawdzanie wierszy i kolumn >**/

    for(int i = 0, j = 0; i < 9; i+=3, j++){
        if(board[i] == player && board[i+1] == player && board[i+2] == player) return true;
        if(board[j] == player && board[j+3] == player && board[j+6] == player) return true;
    }

    /**< Sprawdzanie przekątnych >**/

    if(board[0] == player && board[4] == player && board[8] == player) return true;
    if(board[2] == player && board[4] == player && board[6] == player) return true;

    return false;
}

/**< Algorytm minMax >**/
int minMaxAlgorithm(int state, int8_t player){
    short int counter = 0;
    short int w;

    /**< Sprawdzamy w petli czy jest mozliwa wygrana >**/
    for(int i = 0; i < 9; i++){
        if(board[i] == emptySpace){
            board[i] = player;
            w = i;
            counter++;

            bool test = isWin(player);

            board[i] = emptySpace;

            if(test == true){
                if(!state){
                    board[i] = player;
                    place = i;
                }

                return player == XMark ? -1 : 1;
            }
        }
    }

    /**< Sprawdzamy czy jest mozliwy remis >**/

    if(counter == 1){
        if(!state){
            board[w] = player;
            place = w;
        }
        return 0;
    }

    /**< Wypieramy najlepszy mozliwy ruch dla robota >**/

    int v;
    int vmax;

    vmax = player == XMark ? 2 : -2;

    for(int i = 0; i < 9; i++){
        if(board[i] == emptySpace){
            board[i] = player;

            v = minMaxAlgorithm(state + 1,(player == XMark ? OMark : XMark));

            board[i] = emptySpace;
            if((player == XMark && v < vmax) || (player == OMark && v > vmax)){
                vmax = v;
                w = i;
            }
        }
    }

    if(!state){
        board[w] = player;
        place = w;
    }

    return vmax;
}

 /**< Porownanie oraz zapisanie nowych danych do tablicy >**/
void savePositions(void){
    for(int i = 0; i < 9; i++){
        if(board[i] == emptySpace && tempBoard[i] == true)
            board[i] = OMark;
    }

    return;
}


 /**< Funkcje pomocnicze >**/

bool kbhit(void){
    termios term;
    tcgetattr(0, &term);

    termios term2 = term;
    term2.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &term2);

    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);

    tcsetattr(0, TCSANOW, &term);

    return byteswaiting > 0;
}

bool canPlay(void){
    for(int i = 0; i < 9; i++)
        if(board[i] == emptySpace) return true;
    return false;
}

bool exists(const char* fileName){
    ifstream ifs(fileName);
    if(!ifs.good()) return false;
    ifs.close();
    return true;
}

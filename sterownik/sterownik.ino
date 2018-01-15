#include <Servo.h>

#define defaultRotation 82
#define defaultBase 0
#define defaultArm 180
//#define defaultStabilizer 0


Servo rotation, base, arm, stabilizer;

int rotationPosition = 82; //Domyślna pozycja serva odposiedzialnego za obrot ramienia
int basePosition = 0; //Domyślna pozycja serva na podstawie
int armPosition = 180; //Domyślna pozycja serva na ramieniu
//int stabilizerPosition = 0; //Domyślna pozycja serva odpowiedzialnego za stabilizacje mazaka

void moveServo(Servo servo, int &currentPosition, int targetPosition, int delayTime);
void backToDefaultPosit(void);

struct data{
  bool board[9];
  bool playerCanMove;
} order;

char var[sizeof(order)];

void serialEvent();
void placeXMark(const int);


void setup() {
 Serial.begin(9600);
 rotation.attach(9);
 base.attach(10);
 arm.attach(6);
 //stabilizer.attach(5);

 rotation.write(rotationPosition);            
 delay(15);                     

 base.write(basePosition);            
 delay(15); 

 arm.write(armPosition);            
 delay(15); 

 //stabilizer.write(stabilizerPosition);            
 //delay(15); 

 delay(2000);
}

void loop() {
 ;
}

void serialEvent(){
  if(Serial.available() == sizeof(data)){
      Serial.readBytes(var, sizeof(order));
      memcpy(&order, var, sizeof(order));

      int i;
      short int k = 0;
      
      for(i = 0; i < 9; i++){
        if(order.board[i] == true){
          k = i + 1; break;
        }
      }
      
      placeXMark(k);

      order.playerCanMove = true;

      for(i = 0; i < 9; i++)
        order.board[i] = false;

      memcpy(var, &order, sizeof(order));
      Serial.write(var, sizeof(order));
  }
}

void placeXMark(const int place){
  switch(place){
    case 1:
    /*  X| | 
     *   | |
     *   | |
     */

      moveServo(arm, armPosition, 30, 50);
      moveServo(rotation, rotationPosition, 89, 50);
      moveServo(base, basePosition, 80, 50);
      backToDefaultPosit();
    
    break;

    case 2:
    /*   |X| 
     *   | |
     *   | |
     */

      moveServo(arm, armPosition, 30, 50);
      moveServo(rotation, rotationPosition, 75, 50);
      moveServo(base, basePosition, 80, 50);
      backToDefaultPosit();

    break;

    case 3:
    /*   | |X 
     *   | |
     *   | |
     */
  
      moveServo(arm, armPosition, 30, 50);
      moveServo(rotation, rotationPosition, 60, 50);
      moveServo(base, basePosition, 80, 50);
      backToDefaultPosit();

    break;

    case 4:
    /*   | | 
     *  X| |
     *   | |
     */

      moveServo(arm, armPosition, 90, 50);
      moveServo(rotation, rotationPosition, 89, 50);
      moveServo(base, basePosition, 70, 50);
      backToDefaultPosit();

    break;

    case 5:
    /*   | | 
     *   |X|
     *   | |
     */

      moveServo(arm, armPosition, 90, 50);
      moveServo(rotation, rotationPosition, 70, 50);
      moveServo(base, basePosition, 80, 50);
      backToDefaultPosit();

    break;

    case 6:
    /*   | | 
     *   | |X
     *   | |
     */
    
      moveServo(arm, armPosition, 90, 50);
      moveServo(rotation, rotationPosition, 53, 50);
      moveServo(base, basePosition, 80, 50);
      backToDefaultPosit();

    break;

    case 7:
    /*   | | 
     *   | |
     *  X| |
     */
      moveServo(base, basePosition, 60, 50);
      moveServo(rotation, rotationPosition, 90, 50);
      moveServo(arm, armPosition, 120, 50);
      moveServo(base, basePosition, 80, 50);
      backToDefaultPosit();
      
    break;

    case 8:
    /*   | | 
     *   | |
     *   |X|
     */

      moveServo(base, basePosition, 60, 50);
      moveServo(rotation, rotationPosition, 60, 50);
      moveServo(arm, armPosition, 130, 50);
      moveServo(base, basePosition, 90, 50);
      backToDefaultPosit();

    break;

    case 9:
    /*   | | 
     *   | |
     *   | |X
     */
      moveServo(base, basePosition, 60, 50);
      moveServo(rotation, rotationPosition, 30, 50);
      moveServo(arm, armPosition, 120, 50);
      moveServo(base, basePosition, 90, 50);
      backToDefaultPosit();

    break;

  }
}

void moveServo(Servo servo, int &currentPosition, int targetPosition, int delayTime){
  
  if(currentPosition > targetPosition){
    while(currentPosition > targetPosition){
      servo.write(currentPosition);
      delay(delayTime);
      currentPosition--;
    }
  } else if(currentPosition < targetPosition){
    while(currentPosition < targetPosition){
      servo.write(currentPosition);
      delay(delayTime);
      currentPosition++;
    }
  }

  return;
}

void backToDefaultPosit(void){
    moveServo(rotation, rotationPosition, defaultRotation, 50);
    moveServo(base, basePosition, defaultBase, 50);
   //moveServo(stabilizer, stabilizerPosition, defaultStabilizer, 50);
    moveServo(arm, armPosition, defaultArm, 50);

    return;
}


#include "Timer.h"
#include <Servo.h>
#include <SoftwareSerial.h>

#define BUZZER 10 
#define ECHOPIN 6 
#define TRIGGERPIN 8
#define SERVOPIN 5 


#define OFF 180 
#define SIMMER 0
#define MAX 90 

#define SERVO_MAX_ROTATION_TIME 1500

SoftwareSerial BTserial(3,2); // RX, TX
Servo myservo;
int knobPosition = OFF;
int alarmTime = 0;
long duration, distanceInch, distanceCm;
String command;
int data;
int separatorPos;
int tickEvent;
int endEvent;
boolean opComplete = false;
Timer t;

void setup() {
  // put your setup code here, to run once:
  pinMode(BUZZER, OUTPUT);
  pinMode(ECHOPIN, INPUT);
  pinMode(TRIGGERPIN, OUTPUT);

  BTserial.begin(9600);
  Serial.begin(9600);

  positionServo(OFF);
}


void loop() 
{
  readBluetoothChannel();
  t.update();
}


void readBluetoothChannel()
{
  if(BTserial.available())
  {
    command = BTserial.readString();
    int data = getData(command);   

    //Serial.println(command);

    if (command.startsWith("SET", 0)) 
    {
      if(data >= 0)
      {
        clearOpComplete();
        
        alarmTime = data;
        releaseServo();        
        startTimer();
      }
    } 
    else if (command.startsWith("RESET", 0)) 
    {
      clearOpComplete();
      
      alarmTime = data;
      cancelTimer();   
      resetServo();         
    }
    else if (command.startsWith("POSITION", 0)) 
    {
      clearOpComplete();
      
      if(data >= 0)
      positionServo(data);
    } 
  }
}



void doTimer()
{
  //Serial.println("doTimer");
  
  if(alarmTime > 0)
  {
    alarmTime--;
    //Serial.println("Remaining");
    //Serial.println(alarmTime);
  }
  else
  {
    cancelTimer();
    switchOffGas();
  }
}



void startTimer()
{
  tickEvent = t.every(1000, doTimer);  
  beep();
}



void cancelTimer()
{
   t.stop(tickEvent);
   beep();
}



void switchOffGas()
{
  positionServo(180);
  doOpComplete();
}


int getData(String command)
{
  separatorPos = command.indexOf(':');
  if(separatorPos >= 0){
  data = command.substring(separatorPos+1).toInt();
  }
  return data;
}



void evaluateDistance()
{

  digitalWrite(TRIGGERPIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIGGERPIN, HIGH);
  delayMicroseconds(2);

  duration = pulseIn(ECHOPIN, HIGH);

  distanceCm = duration * 0.034/2;
  distanceInch = duration * 0.0133/2;
  
}


void doOpComplete()
{
  if(!opComplete){
  endEvent =  t.oscillate(BUZZER, 500, LOW);  
  }

  opComplete = true;
}



void clearOpComplete()
{
  if(opComplete){
   t.stop(endEvent);
  }

  opComplete = false;
}



void beep()
{
  digitalWrite(BUZZER, HIGH);
  delay(150);
  digitalWrite(BUZZER, LOW);
}



void acquireServo()
{
  if(!myservo.attached())
  {
    myservo.attach(SERVOPIN); 
  }
}



void releaseServo()
{
  if(myservo.attached())
  {
    myservo.detach(); 
  }
}


void positionServo(int pos)
{
  releaseServo();
  acquireServo();
  myservo.write(pos);
  delay(SERVO_MAX_ROTATION_TIME); // give servo time to position
  releaseServo();
  beep();
}


void resetServo()
{
  releaseServo();
  acquireServo();
  myservo.write(0);
  delay(SERVO_MAX_ROTATION_TIME); // give servo time to position
  releaseServo();
  beep();
}


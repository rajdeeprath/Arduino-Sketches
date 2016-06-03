#include <SoftwareSerial.h>

SoftwareSerial BTserial(2,3);
char c;
int LED_PIN = 13;
boolean LED_ON = false;

void setup(){
  // put your setup code here, to run once:
  Serial.begin(9600);
  BTserial.begin(9600);

  pinMode(LED_PIN, OUTPUT);
  
  Serial.println("Arduino ready for bluetooth");
}

void loop() 
{
  // from bt device to arduino
  if(BTserial.available())
  {
    c = BTserial.read();
    Serial.write(c);
    
    int ia = c - '0';

    if(ia == 1)
    {
      // if led was off turn it on
      if(LED_ON == false)
      {
        digitalWrite(LED_PIN, HIGH);
        LED_ON = true;
      }
    }
    else
    {
      // if led was on  turn it off
      if(LED_ON == true)
      {
        digitalWrite(LED_PIN, LOW);
        LED_ON = false;
      }
    }    
  }
}

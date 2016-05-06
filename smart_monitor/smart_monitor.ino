#include <DS3232RTC.h>    //http://github.com/JChristensen/DS3232RTC
#include <Time.h>         //http://www.arduino.cc/playground/Code/Time  
#include <Wire.h>         //http://arduino.cc/en/Reference/Wire (included with Arduino IDE)




int LIGHT_SENSOR_PIN = A0;

int PIR_SENSOR_PIN = 2;

int LED_PIN = 13;

int RELAY_PIN_1 = 7;

int RELAY_PIN_2 = 9;

int RTC_SDA_SENSOR_PIN = A4;

int RTC_SCL_SENSOR_PIN = A5;


int callibrationTime = 35; // seconds

boolean callibrationDone = false;

int lightVal = 0;

int pirVal = 0;
int pirHistoryIndex = 0;
int MAX_RECORDS = 2;

int PIRSTATE = LOW;
int LIGHTSTATE = LOW;

boolean CONDITION = false;

boolean LIGHT_ALARM_ACTIVE = false;
boolean lightAlarmActive = false;

boolean PUMP_ALARM_ACTIVE = false;
boolean pumpAlarmActive = false;


long lastPirHigh = 0;
long timeNow = 0;

tmElements_t tm;

long CONDITION_TIMEOUT = 30; // seconds
int LIGHT_VAL_THRESHOLD = 700;


tmElements_t lightOnTime;
tmElements_t lightOffTime;

tmElements_t pumpOnTime;
tmElements_t pumpOffTime;

void initClock()
{
  setSyncProvider(RTC.get);   // the function to get the time from the RTC


  if (timeStatus() != timeSet)
    Serial.println("Unable to sync with the RTC");
  else
    Serial.println("RTC has set the system time");
}



void setup() {

  pinMode(LIGHT_SENSOR_PIN, INPUT);

  pinMode(PIR_SENSOR_PIN, INPUT);

  pinMode(RELAY_PIN_1, OUTPUT);

  pinMode(RELAY_PIN_2, OUTPUT);

  // init light on time
  lightOnTime.Hour = 17;
  lightOnTime.Minute = 00;
  lightOnTime.Second = 0;


  // init light off time
  lightOffTime.Hour = 19;
  lightOffTime.Minute = 00;
  lightOffTime.Second = 0;


  // init pump on time
  pumpOnTime.Hour = 20;
  pumpOnTime.Minute = 00;
  pumpOnTime.Second = 0;


  // init pump off time
  pumpOffTime.Hour = 23;
  pumpOffTime.Minute = 00;
  pumpOffTime.Second = 0;
  

  Serial.begin(9600);

  initClock();
}




void loop() {


  if (callibrationDone == false)
  {
    int counter = 0;

    while (counter < callibrationTime)
    {
      Serial.println("callibrating");
      delay(1000);
      counter++;
    }

    callibrationDone = true;
  }


  // read light sensor value
  lightVal = analogRead(LIGHT_SENSOR_PIN);


  // read pir sensor value
  pirVal = digitalRead(PIR_SENSOR_PIN);


  // evaluate pir data
  evaluateMotionState(pirVal, PIRSTATE);


  // evaluate light data
  evaluateLightState(lightVal, LIGHTSTATE);


  // delay
  delay(1000);
  timeNow = now();
  RTC.read(tm);
  

  /********************************************
     Check for PIR sensor + LIGHT sensor COMBO
   *******************************************/

  if (PIRSTATE == HIGH && LIGHTSTATE == LOW && LIGHT_ALARM_ACTIVE == false)
  {
    if (CONDITION == false && pirHistoryIndex >= MAX_RECORDS)
    {
      CONDITION = true;
      digitalWrite(RELAY_PIN_1, HIGH);
    }
  }
  else
  {
    if (CONDITION == true)
    {
      Serial.print("timeNow - lastPirHigh = ");
      Serial.print(timeNow - lastPirHigh);
      Serial.print("\n");

      if (timeNow - lastPirHigh > CONDITION_TIMEOUT)
      {
        CONDITION = false;
        digitalWrite(RELAY_PIN_1, LOW);
      }
    }
    else
    {
      // evaluate light alarm
      lightAlarmActive = isAlarmPeriodActive(tm, lightOnTime, lightOffTime);  // do when on alarm
      if(lightAlarmActive)
      {
        if (LIGHT_ALARM_ACTIVE == false)
        {
          LIGHT_ALARM_ACTIVE = true;
          digitalWrite(RELAY_PIN_1, HIGH);
        }
      }
      else
      {
        if (LIGHT_ALARM_ACTIVE == true)
        {
          LIGHT_ALARM_ACTIVE = false;
          digitalWrite(RELAY_PIN_1, LOW);
        }
      }


      // evaluate pump alarm
      pumpAlarmActive = isAlarmPeriodActive(tm, pumpOnTime, pumpOffTime);  // do when on alarm
      if(pumpAlarmActive)
      {
        if (PUMP_ALARM_ACTIVE == false)
        {
          PUMP_ALARM_ACTIVE = true;
          digitalWrite(RELAY_PIN_2, HIGH);
        }
      }
      else
      {
        if (PUMP_ALARM_ACTIVE == true)
        {
          PUMP_ALARM_ACTIVE = false;
          digitalWrite(RELAY_PIN_2, LOW);
        }      
      }
    }
  }
}





void evaluateMotionState(int pirVal, int &PIRSTATE)
{
  if (pirVal == LOW)
  {
    if (PIRSTATE == HIGH)
    {
      PIRSTATE = LOW;

      // reset history tracker
      pirHistoryIndex = 0;
    }
  }
  else
  {
    // record in history tracker
    if (pirHistoryIndex < MAX_RECORDS)
    {
      // record when pir state was high recently
      lastPirHigh = now();
      pirHistoryIndex++;
    }

    if (PIRSTATE == LOW)
    {
      PIRSTATE = HIGH;
    }
  }
}





void evaluateLightState(int lightVal, int &LIGHTSTATE)
{
  if (lightVal > LIGHT_VAL_THRESHOLD)
  {
    if (LIGHTSTATE == HIGH)
    {
      LIGHTSTATE = LOW;
    }
  }
  else
  {
    if (LIGHTSTATE == LOW)
    {
      LIGHTSTATE = HIGH;
    }
  }
}



boolean isAlarmPeriodActive(tmElements_t tm, tmElements_t startTime, tmElements_t endTime)
{
  if ((tm.Hour >= startTime.Hour && tm.Minute >= startTime.Minute) && (tm.Hour <= endTime.Hour && tm.Minute <= endTime.Minute))
  return true;
  else
  return false;
}


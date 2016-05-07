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
time_t current_t;

tmElements_t lightOnTime;
time_t lightOn_t;

tmElements_t lightOffTime;
time_t lightOff_t;

tmElements_t pumpOnTime;
time_t pumpOn_t;

tmElements_t pumpOffTime;
time_t pumpOff_t;


long CONDITION_TIMEOUT = 30; // seconds
int LIGHT_VAL_THRESHOLD = 700;

boolean rtcOk;


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

  
  // init light on alarm time
  lightOnTime.Hour = 18;
  lightOnTime.Minute = 00;
  lightOnTime.Second = 00;


  // init light off alarm time
  lightOffTime.Hour = 20;
  lightOffTime.Minute = 00;
  lightOffTime.Second = 00;

 
  // init pump on alarm time
  pumpOnTime.Hour = 20;
  pumpOnTime.Minute = 30;
  pumpOnTime.Second = 00;


  // init pump off alarm time
  pumpOffTime.Hour = 23;
  pumpOffTime.Minute = 00;
  pumpOffTime.Second = 00;
  

  Serial.begin(9600);

  initClock();
}




void loop() {

  // calliberate sensors for the first time during startup / program reset
  if (callibrationDone == false)
  {
    int counter = 0;

    while (counter < callibrationTime)
    {
      //Serial.println("callibrating");
      delay(1000);
      counter++;
    }

    callibrationDone = true;
  }

  
  // Delay
  delay(1000);
  

  // Read light sensor value
  lightVal = analogRead(LIGHT_SENSOR_PIN);


  // Read pir sensor value
  pirVal = digitalRead(PIR_SENSOR_PIN);


  // Evaluate pir data
  evaluateMotionState(pirVal, PIRSTATE);


  // Evaluate light data
  evaluateLightState(lightVal, LIGHTSTATE);


  // check RTC - If not ok skip time related code execution
  rtcOk = !RTC.oscStopped();
  if(!rtcOk) return;
  

  // Calculate alarm times W.R.T today
  timeNow = now();
  breakTime(timeNow, tm);
  
  lightOn_t = getTimePostSyncAlarmYearMonthDate(tm, lightOnTime);
  lightOff_t = getTimePostSyncAlarmYearMonthDate(tm, lightOffTime);
  pumpOn_t = getTimePostSyncAlarmYearMonthDate(tm, pumpOnTime);
  pumpOff_t = getTimePostSyncAlarmYearMonthDate(tm, pumpOffTime);
  

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
      //Serial.print("timeNow - lastPirHigh = ");
      //Serial.print(timeNow - lastPirHigh);
      //Serial.print("\n");

      if (timeNow - lastPirHigh > CONDITION_TIMEOUT)
      {
        CONDITION = false;
        digitalWrite(RELAY_PIN_1, LOW);
      }
    }
    else
    {
      // evaluate light alarm
      lightAlarmActive = isAlarmPeriodActive(timeNow, lightOn_t, lightOff_t);  // do when on alarm
      //Serial.print("lightAlarmActive = ");
      //Serial.println(lightAlarmActive);
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
      pumpAlarmActive = isAlarmPeriodActive(timeNow, pumpOn_t, pumpOff_t);  // do when on alarm
      //Serial.print("pumpAlarmActive = ");
      //Serial.println(pumpAlarmActive);
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



time_t getTimePostSyncAlarmYearMonthDate(tmElements_t source, tmElements_t &destination)
{
  destination.Year = source.Year;
  destination.Month = source.Month;
  destination.Day = source.Day;
  destination.Wday = source.Wday;

  return makeTime(destination);
}



/* 
 *  Calculate to see if current time is within the range of requested start and end alarm times
 */
boolean isAlarmPeriodActive(time_t currentTime, time_t startTime, time_t endTime)
{ 

  long int diff_start = currentTime - startTime;
  long int diff_end = currentTime - endTime;

  if(diff_start >= 0 && diff_end <= 0)
  {
    return true;
  }

  return false;
}



void print_time(time_t t)
{
  Serial.print(hour(t));
  Serial.print(" : ");
  Serial.print(minute(t));
  Serial.print(" : ");
  Serial.print(second(t));
  Serial.print(" : ");
  Serial.print(day(t));
  Serial.print(" : ");
  Serial.print(weekday(t));
  Serial.print(" : ");
  Serial.print(month(t));
  Serial.print(" : ");
  Serial.print(year(t));
}


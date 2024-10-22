boolean printDiagnostics = false;

union CubeData
{
  struct
  {
    int16_t state;
    int16_t watchdog;
    int16_t chipTemp;
    int16_t switchOpenAlarm;
    int16_t newData;
    int16_t switchState[3];
  };
  byte buffer[16];
};
CubeData cubeData;

#include "BlinkyPicoWCube.h"


int commLEDPin = 2;
int commLEDBright = 255; 
int resetButtonPin = 3;

int powerPin[]  = {10, 13, 16};
int switchPin[] = {11, 14, 17};
int signalPin[] = {12, 15, 18};
unsigned long switchTime[] = {0, 0, 0};

unsigned long lastPublishTime;
unsigned long publishInterval = 2000;
unsigned long debounceInterval = 20;

void setupServerComm()
{
  // Optional setup to overide defaults
  if (printDiagnostics)
  {
    Serial.begin(115200);
    delay(10000);
  }
  BlinkyPicoWCube.setChattyCathy(printDiagnostics);
  BlinkyPicoWCube.setWifiTimeoutMs(20000);
  BlinkyPicoWCube.setWifiRetryMs(20000);
  BlinkyPicoWCube.setMqttRetryMs(3000);
  BlinkyPicoWCube.setResetTimeoutMs(10000);
  BlinkyPicoWCube.setHdwrWatchdogMs(8000);
  BlinkyPicoWCube.setBlMqttKeepAlive(8);
  BlinkyPicoWCube.setBlMqttSocketTimeout(4);
  BlinkyPicoWCube.setMqttLedFlashMs(10);
  BlinkyPicoWCube.setWirelesBlinkMs(100);
  BlinkyPicoWCube.setMaxNoMqttErrors(5);
  
  // Must be included
  BlinkyPicoWCube.init(commLEDPin, commLEDBright, resetButtonPin);
}

void setupCube()
{
  lastPublishTime = 0;
  for (int ii = 0; ii < 3; ++ii)
  {
    pinMode(powerPin[ii], OUTPUT);
    pinMode(switchPin[ii], INPUT);
    pinMode(signalPin[ii], OUTPUT);
    digitalWrite(powerPin[ii], HIGH);
    digitalWrite(signalPin[ii], LOW);
    cubeData.switchState[ii] = -1;
  }

  cubeData.state = 1;
  cubeData.watchdog = 0;
  cubeData.switchOpenAlarm = 1;
  cubeData.newData = 0;
}

void cubeLoop()
{
  unsigned long nowTime = millis();
  boolean stateChange = false;
  for (int ii = 0; ii < 3; ++ii)
  {
    int16_t pinValue = (int16_t) digitalRead(switchPin[ii]);
    if (pinValue != cubeData.switchState[ii])
    {
      if((nowTime - switchTime[ii]) > debounceInterval)
      {
        cubeData.switchState[ii] = pinValue;
        switchTime[ii] = nowTime;
        stateChange = true;
        boolean signalVal = false;
        if (pinValue > 0) signalVal = true;
        if (cubeData.switchOpenAlarm == 1) signalVal = !signalVal;
        digitalWrite(signalPin[ii], signalVal);
      }
    }
  }
  cubeData.newData = 0;
  if (stateChange)
  {
    cubeData.newData = 1;
    publishData(nowTime);  
  }
  if ((nowTime - lastPublishTime) > publishInterval) publishData(nowTime);
  
}

void publishData(unsigned long nowTime)
{
  lastPublishTime = nowTime;
  cubeData.watchdog = cubeData.watchdog + 1;
  if (cubeData.watchdog > 32760) cubeData.watchdog= 0 ;
  cubeData.chipTemp = (int16_t) (analogReadTemp() * 100.0);
  
  BlinkyPicoWCube.publishToServer();
  if (printDiagnostics)
  {
    Serial.print(cubeData.switchState[0]);
    Serial.print(",");
    Serial.print(cubeData.switchState[1]);
    Serial.print(",");
    Serial.println(cubeData.switchState[2]);
  }
}
void handleNewSettingFromServer(uint8_t address)
{
  switch(address)
  {
    case 0:
      break;
    case 1:
      break;
    case 2:
      break;
    case 3:
      for (int ii = 0; ii < 3; ++ii) 
      {
        cubeData.switchState[ii] = -1;
        switchTime[ii] = 0;
      }
      break;
    case 4:
      break;
    default:
      break;
  }
}

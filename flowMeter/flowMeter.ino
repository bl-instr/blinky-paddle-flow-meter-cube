boolean printDiagnostics = false;

union CubeData
{
  struct
  {
    int16_t state;
    int16_t watchdog;
    int16_t publishInterval;
    int16_t nsamples;
    int16_t timeScale;
    int16_t rate[3];
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
int switchSignal[] = {0, 0, 0};
int oldSwitchSignal[] = {0, 0, 0};
unsigned long oldSwitchTime[] = {0, 0, 0};
float switchTime[] = {0, 0, 0};


unsigned long lastPublishTime;
unsigned long publishInterval = 2000;


void setupServerComm()
{
  // Optional setup to overide defaults
  if (printDiagnostics)
  {
    Serial.begin(115200);
    delay(7000);
  }
  delay(3000);
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
  lastPublishTime = millis();
  for (int ii = 0; ii < 3; ++ii)
  {
    pinMode(powerPin[ii], OUTPUT);
    pinMode(switchPin[ii], INPUT);
    pinMode(signalPin[ii], OUTPUT);
    digitalWrite(powerPin[ii], HIGH);
    digitalWrite(signalPin[ii], LOW);
    switchSignal[ii] = LOW;
    oldSwitchSignal[ii] = LOW;
    oldSwitchTime[ii] = lastPublishTime;
    switchTime[ii] = 32765;
    cubeData.rate[ii] = 0;
    
  }

  cubeData.state = 1;
  cubeData.watchdog = 0;
  cubeData.nsamples = 1;
  cubeData.timeScale = 3600;
  cubeData.publishInterval = 2000;
  publishInterval = (unsigned long) cubeData.publishInterval;

}

void cubeLoop()
{
  unsigned long nowTime = millis();
  for (int ii = 0; ii < 3; ++ii)
  {
    switchSignal[ii] = digitalRead(switchPin[ii]); 
    delay(10);
    if ((switchSignal[ii] == 1) && (oldSwitchSignal[ii] == 0) )
    {
      float switchTimeF = (float) (nowTime - oldSwitchTime[ii]);
      switchTime[ii] = switchTime[ii] + (switchTimeF - switchTime[ii]) / ((float) cubeData.nsamples);
      oldSwitchTime[ii] = nowTime;
    }
    oldSwitchSignal[ii] = switchSignal[ii];
  }
  if ((nowTime - lastPublishTime) > publishInterval) 
  {
    for (int ii = 0; ii < 3; ++ii)
    {
      float rate  = 1000 * ((float) cubeData.timeScale) / (switchTime[ii] + 0.1);
      if (rate > 32765.0) rate = 32765.0;
      cubeData.rate[ii] = (int16_t) rate;
      if (printDiagnostics)
      {
        Serial.print(cubeData.rate[ii]);
        if ( ii < 2) Serial.print(",");
      }
    }
    publishData(nowTime);
    if (printDiagnostics) Serial.println(" ");

  }
  
}

void publishData(unsigned long nowTime)
{
  lastPublishTime = nowTime;
  cubeData.watchdog = cubeData.watchdog + 1;
  if (cubeData.watchdog > 32760) cubeData.watchdog= 0 ;
  
  BlinkyPicoWCube.publishToServer();
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
      if (cubeData.publishInterval < 500) cubeData.publishInterval = 500;
      publishInterval = (unsigned long) cubeData.publishInterval;
      break;
    case 3:
      if (cubeData.nsamples < 1) cubeData.nsamples = 1;
      break;
    case 4:
      if (cubeData.timeScale < 1) cubeData.timeScale = 1;
      break;
    default:
      break;
  }
}

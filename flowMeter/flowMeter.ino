#define BLINKY_DIAG        0
#define CUBE_DIAG          0
#define COMM_LED_PIN       2
#define RST_BUTTON_PIN     3
#include <BlinkyPicoW.h>

struct CubeSetting
{
  uint16_t publishInterval;
  uint16_t nsamples;
  uint16_t timeScale;
};
CubeSetting setting;

struct CubeReading
{
  uint16_t rate[3];;
};
CubeReading reading;

unsigned long lastPublishTime;

int powerPin[]  = {10, 13, 16};
int switchPin[] = {11, 14, 17};
int signalPin[] = {12, 15, 18};
int switchSignal[] = {0, 0, 0};
int oldSwitchSignal[] = {0, 0, 0};
unsigned long oldSwitchTime[] = {0, 0, 0};
float switchTime[] = {0, 0, 0};

void setupBlinky()
{
  if (BLINKY_DIAG > 0) Serial.begin(9600);

  BlinkyPicoW.setMqttKeepAlive(15);
  BlinkyPicoW.setMqttSocketTimeout(4);
  BlinkyPicoW.setMqttPort(1883);
  BlinkyPicoW.setMqttLedFlashMs(100);
  BlinkyPicoW.setHdwrWatchdogMs(8000);

  BlinkyPicoW.begin(BLINKY_DIAG, COMM_LED_PIN, RST_BUTTON_PIN, true, sizeof(setting), sizeof(reading));
}

void setupCube()
{
  if (CUBE_DIAG > 0) Serial.begin(9600);
  setting.publishInterval = 2000;
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
    reading.rate[ii] = 0;
    
  }
  setting.nsamples = 1;
  setting.timeScale = 3600;

}

void loopCube()
{
  unsigned long now = millis();
  for (int ii = 0; ii < 3; ++ii)
  {
    switchSignal[ii] = digitalRead(switchPin[ii]); 
    delay(10);
    if ((switchSignal[ii] == 1) && (oldSwitchSignal[ii] == 0) )
    {
      float switchTimeF = (float) (now - oldSwitchTime[ii]);
      switchTime[ii] = switchTime[ii] + (switchTimeF - switchTime[ii]) / ((float) setting.nsamples);
      oldSwitchTime[ii] = now;
    }
    oldSwitchSignal[ii] = switchSignal[ii];
  }
  if ((now - lastPublishTime) > setting.publishInterval)
  {
    for (int ii = 0; ii < 3; ++ii)
    {
      float rate  = 1000 * ((float) setting.timeScale) / (switchTime[ii] + 0.1);
      if (rate > 65534.0) rate = 65534.0;
      reading.rate[ii] = (uint16_t) rate;
      if (CUBE_DIAG > 0)
      {
        Serial.print(reading.rate[ii]);
        if ( ii < 2) Serial.print(",");
      }
    }
    if (CUBE_DIAG > 0) Serial.println(" ");
    lastPublishTime = now;
    boolean successful = BlinkyPicoW.publishCubeData((uint8_t*) &setting, (uint8_t*) &reading, false);
  }
  boolean newSettings = BlinkyPicoW.retrieveCubeSetting((uint8_t*) &setting);
  if (newSettings)
  {
    if (setting.publishInterval < 1000) setting.publishInterval = 1000;
    if (setting.nsamples < 1) setting.nsamples = 1;
    if (setting.timeScale < 1) setting.timeScale = 1;
  }
  
}

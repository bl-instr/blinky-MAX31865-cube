#include <SPI.h>

union CubeData
{
  struct
  {
    int16_t state;
    int16_t watchdog;
    int16_t raw1;
    int16_t raw2;
    int16_t nsample;
    int16_t publishInterval;
    int16_t measInterval;
    
  };
  byte buffer[14];
};
CubeData cubeData;

#include "BlinkyPicoWCube.h"

int commLEDPin = 14;
int commLEDBright = 255; 
int resetButtonPin = 7;
int csPin1 = 17;
int csPin2 = 21;

unsigned long lastPublishTime;
unsigned long publishInterval;
unsigned long lastMeasureTime;
unsigned long measInterval
;
SPISettings spiSetting(4000000, MSBFIRST, SPI_MODE3);

void intSpi(int cspin)
{
  SPI.setCS(cspin);
  SPI.beginTransaction(spiSetting);
  changeCsPin(cspin, 0);
  SPI.transfer(0x80);
  SPI.transfer(0x03);
  changeCsPin(cspin, 0);
  SPI.endTransaction();
}

void changeCsPin(int cspin, int val)
{
//  delayMicroseconds(10);
  digitalWrite(cspin, val);
//  delayMicroseconds(10);
}
uint16_t readSpi(int cspin)
{
  SPI.setCS(cspin);
  byte spiReadBuffer[4];
  int len = 4;
  for (int ii = 0; ii < len; ++ii) spiReadBuffer[ii] = 0x00;
  
  SPI.beginTransaction(spiSetting);
  changeCsPin(cspin, 0);
  SPI.transfer(0x80);
  SPI.transfer(0x83);
  changeCsPin(cspin, 1);
  SPI.endTransaction();
  
  delay(100);
  
  SPI.beginTransaction(spiSetting);
  changeCsPin(cspin, 0);
  SPI.transfer(0x80);
  SPI.transfer(0xA3);
  changeCsPin(cspin, 1);
  SPI.endTransaction();
  
  SPI.beginTransaction(spiSetting);
  changeCsPin(cspin, 0);
  SPI.transfer(spiReadBuffer,len);
  changeCsPin(cspin, 1);
  SPI.endTransaction();
  
  SPI.beginTransaction(spiSetting);
  changeCsPin(cspin, 0);
  SPI.transfer(0x80);
  SPI.transfer(0x03);
  changeCsPin(cspin, 1);
  SPI.endTransaction();
/*
  Serial.print(spiReadBuffer[0]);
  Serial.print(",");
  Serial.print(spiReadBuffer[1]);
  Serial.print(",");
  Serial.print(spiReadBuffer[2]);
  Serial.print(",");
  Serial.println(spiReadBuffer[3]);
*/
  uint16_t msb = (int16_t) spiReadBuffer[2];
  uint16_t lsb = (int16_t) spiReadBuffer[3];
  msb = msb * 256;
  uint16_t raw = msb + lsb;
  raw = raw / 2;
  return raw;
}


void setupServerComm()
{
// Optional setup to overide defaults
//  Serial.begin(115200);
  BlinkyPicoWCube.setChattyCathy(false);
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

float fraw1 = -1;
float fraw2 = -1;
float nsample = 1.0;

void setupCube()
{
  lastPublishTime = millis();
  cubeData.state = 1;
  cubeData.watchdog = 0;
  cubeData.raw1 = 0;
  cubeData.raw2 = 0;
  cubeData.nsample = 1;
  nsample = 1.0;
  cubeData.measInterval = 200;
  cubeData.publishInterval = 2000;
  publishInterval = (unsigned long) cubeData.publishInterval;
  measInterval = (unsigned long) cubeData.measInterval;
  pinMode(15, INPUT_PULLDOWN);
  pinMode(20, INPUT_PULLDOWN);
  pinMode(csPin1, OUTPUT);
  digitalWrite(csPin1, 1);
  pinMode(csPin2, OUTPUT);
  digitalWrite(csPin2, 1);


  SPI.setRX(16);
  SPI.setCS(csPin1);
  SPI.setSCK(18);
  SPI.setTX(19);  
  SPI.begin(false);
  SPI.setCS(csPin2);
  SPI.begin(false);
  intSpi(csPin1);
  intSpi(csPin2);
  lastPublishTime = millis();
  lastMeasureTime = millis();
}
void cubeLoop()
{
  unsigned long nowTime = millis();
  
  if ((nowTime - lastPublishTime) > publishInterval)
  {
    lastPublishTime = nowTime;
    cubeData.watchdog = cubeData.watchdog + 1;
    if (cubeData.watchdog > 32760) cubeData.watchdog= 0 ;

    BlinkyPicoWCube.publishToServer();
  }  
  if ((nowTime - lastMeasureTime) > measInterval)
  {
    lastMeasureTime = nowTime;

    if (fraw1 < 0)
    {
      fraw1 = (float) readSpi(csPin1);
    }
    else
    {
      fraw1 = fraw1 + (((float) readSpi(csPin1)) - fraw1) / nsample;
    }
    if (fraw2 < 0)
    {
      fraw2 = (float) readSpi(csPin2);
    }
    else
    {
      fraw2 = fraw2 + (((float) readSpi(csPin2)) - fraw2) / nsample;
    }
    cubeData.raw1 = (int16_t) fraw1;
    cubeData.raw2 = (int16_t) fraw2;
//    Serial.print(cubeData.raw1);
//    Serial.print(",");
//    Serial.println(cubeData.raw2);
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
      break;
    case 4:
      if (cubeData.nsample < 1) cubeData.nsample = 1;
      nsample = (float) cubeData.nsample;
      fraw1 = -1.0;
      fraw2 = -1.0;
      break;
    case 5:
      if (cubeData.publishInterval < 500) cubeData.publishInterval = 500;
      publishInterval = (unsigned long) cubeData.publishInterval;
      break;
    case 6:
      if (cubeData.measInterval < 200) cubeData.measInterval = 200;
      measInterval = (unsigned long) cubeData.measInterval;
      fraw1 = -1.0;
      fraw2 = -1.0;
      break;
    default:
      break;
  }
}

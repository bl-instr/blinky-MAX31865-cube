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
    
  };
  byte buffer[10];
};
CubeData cubeData;

#include "BlinkyPicoWCube.h"

int commLEDPin = 14;
int commLEDBright = 255; 
int resetButtonPin = 7;
int csPin1 = 17;
int csPin2 = 21;

unsigned long lastPublishTime;
unsigned long publishInterval = 2000;
unsigned long lastMeasureTime;
unsigned long measInterval = 200;
SPISettings spiSetting(400000, MSBFIRST, SPI_MODE1);

void intSpi(int cspin)
{
  byte spiWriteBuffer[] = {0x80,0xC3};
  SPI.beginTransaction(spiSetting);
  digitalWrite(cspin, 0);
//  delayMicroseconds(10);
  SPI.transfer(0x80);
  SPI.transfer(0xC3);
  digitalWrite(cspin, 1);
//  delayMicroseconds(10);
  SPI.endTransaction();
}
uint16_t readSpi(int cspin)
{
  byte spiReadBuffer[4];
  int len = 4;
  for (int ii = 0; ii < len; ++ii) spiReadBuffer[ii] = 0x00;
  SPI.beginTransaction(spiSetting);
  digitalWrite(cspin, 0);
//  delayMicroseconds(10);
  SPI.transfer(spiReadBuffer,len);
  digitalWrite(cspin, 1);
//  delayMicroseconds(10);
  SPI.endTransaction();
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

float fraw1;
float fraw2;

void setupCube()
{
  lastPublishTime = millis();
  cubeData.state = 1;
  cubeData.watchdog = 0;
  cubeData.raw1 = 0;
  cubeData.raw2 = 0;
  cubeData.nsample = 10;
  pinMode(csPin1, OUTPUT);
  digitalWrite(csPin1, 1);
  pinMode(csPin2, OUTPUT);
  digitalWrite(csPin2, 1);
  
  SPI.setRX(16);
  SPI.setCS(17);
  SPI.setSCK(18);
  SPI.setTX(19);  
  SPI.begin(true);
  intSpi(csPin1);
  intSpi(csPin2);
  delay(1000);
  fraw1 = (float) readSpi(csPin1);
  fraw2 = (float) readSpi(csPin2);
}
void cubeLoop()
{
  unsigned long nowTime = millis();
  
  if ((nowTime - lastPublishTime) > publishInterval)
  {
    lastPublishTime = nowTime;
    cubeData.watchdog = cubeData.watchdog + 1;
    if (cubeData.watchdog > 32760) cubeData.watchdog= 0 ;
    BlinkyPicoWCube::publishToServer();
  }  
  if ((nowTime - lastMeasureTime) > measInterval)
  {
    lastMeasureTime = nowTime;
    fraw1 = fraw1 + (((float) readSpi(csPin1)) - fraw1) / ((float) cubeData.nsample);
    fraw2 = fraw2 + (((float) readSpi(csPin2)) - fraw2) / ((float) cubeData.nsample);
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
      fraw1 = (float) readSpi(csPin1);
      fraw2 = (float) readSpi(csPin2);
      break;
    default:
      break;
  }
}

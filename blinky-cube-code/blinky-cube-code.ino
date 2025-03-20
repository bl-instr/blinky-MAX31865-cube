#define BLINKY_DIAG         0
#define COMM_LED_PIN       14
#define RST_BUTTON_PIN     7
#include <BlinkyPicoW.h>
#include <SPI.h>

struct CubeSetting
{
  uint16_t nsample;
  uint16_t publishInterval;
  uint16_t measInterval;
};
CubeSetting setting;

struct CubeReading
{
  uint16_t raw1;
  uint16_t raw2;
};
CubeReading reading;

int csPin1 = 17;
int csPin2 = 21;
unsigned long lastMeasureTime;
unsigned long lastPublishTime;
float fraw1 = -1;
float fraw2 = -1;
float nsample = 1.0;
uint16_t oldNsample;
uint16_t oldMeasInterval;

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
  setting.measInterval = 200;
  oldMeasInterval = setting.measInterval;
  setting.publishInterval = 2000;
  setting.nsample = 1;
  oldNsample = setting.nsample;
  nsample = 1.0;
  reading.raw1 = 0;
  reading.raw2 = 0;
    

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
void loopCube()
{
  unsigned long now = millis();
  if ((now - lastPublishTime) > setting.publishInterval)
  {
    lastPublishTime = now;
     boolean successful = BlinkyPicoW.publishCubeData((uint8_t*) &setting, (uint8_t*) &reading, false);
  }
  if ((now - lastMeasureTime) > ((unsigned long) setting.measInterval))
  {
    lastMeasureTime = now;

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
    reading.raw1 = (uint16_t) fraw1;
    reading.raw2 = (uint16_t) fraw2;
  }  
  if (BlinkyPicoW.retrieveCubeSetting((uint8_t*) &setting) )
  {
      if (setting.publishInterval < 1000) setting.publishInterval = 1000;
      if (oldNsample != setting.nsample)
      {
        oldNsample = setting.nsample;
        if (setting.nsample < 1) setting.nsample = 1;
        nsample = (float) setting.nsample;
        fraw1 = -1.0;
        fraw2 = -1.0;
      }
      if (oldMeasInterval != setting.measInterval)
      {
        oldMeasInterval = setting.measInterval;
        if (setting.measInterval < 200) setting.measInterval = 200;
        fraw1 = -1.0;
        fraw2 = -1.0;
      }
  }
}

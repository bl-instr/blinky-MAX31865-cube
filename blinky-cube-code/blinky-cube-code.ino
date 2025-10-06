#define BLINKY_DIAG       0
#define CUBE_DIAG         0
#define COMM_LED_PIN     14
#define RST_BUTTON_PIN    7
#define POLYSIZE          7
#include <BlinkyPicoW.h>
#include <SPI.h>

struct CubeSetting
{
  uint16_t nsample;
  uint16_t publishInterval;
  uint16_t measInterval;

/* 
 *  fitType = 0  lin   T, lin   R
 *  fitType = 1  lin   T, log10 R
 *  fitType = 2  log10 T, lin   R
 *  fitType = 3  log10 T, log10 R  
 */

  uint8_t fitTypeA;
  uint8_t fitTypeB;

  float refResA;
  float resScaleA;
  float tempScaleA;
  float coefA[POLYSIZE];
  float refResB;
  float resScaleB;
  float tempScaleB;
  float coefB[POLYSIZE];

};
CubeSetting setting;

struct CubeReading
{
  float resA;
  float resB;
  float tempA;
  float tempB;
};
CubeReading reading;

int csPin1 = 17;
int csPin2 = 21;
unsigned long lastMeasureTime;
unsigned long lastPublishTime;
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
float readSpi(int cspin)
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
  float fresNorm = (float) raw;
  fresNorm = fresNorm / 32768.0;
  return fresNorm;
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
  if (CUBE_DIAG > 0)
  {
    Serial.begin(9600);
    delay(5000);
  }
  setting.measInterval = 200;
  oldMeasInterval = setting.measInterval;
  setting.publishInterval = 2000;
  setting.nsample = 1;
  oldNsample = setting.nsample;
  nsample = 1.0;
  setting.fitTypeA = 0;
  setting.fitTypeB = 0;
  setting.refResA = 4302.0;
  setting.refResB = 4302.0;
  setting.resScaleA = 1000.0;
  setting.resScaleB = 1000.0;
  setting.tempScaleA = 293.0;
  setting.tempScaleB = 293.0;
  setting.coefA[0] = 1.0;
  setting.coefB[0] = 1.0;
  for (int ii = 1; ii < POLYSIZE; ++ii)
  {
    setting.coefA[ii] = 0.0;
    setting.coefB[ii] = 0.0;
  }
  reading.resA = 0.0;
  reading.resB = 0.0;
    
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
  delay(100);
  
  lastPublishTime = millis();
  lastMeasureTime = millis();

  reading.resA = (setting.refResA * readSpi(csPin1));
  reading.resB = (setting.refResB * readSpi(csPin2));

}
float calcTemp(float res, float resScale, float tempScale, uint8_t fitType, float* coef)
{
  float fresPoly = res / resScale;
  if ( (fitType == 1) || (fitType == 3) ) fresPoly = log10(fresPoly);
  float fxton = 1.0;
  float tempNorm = 0.0;
  for (int ipoly = 0; ipoly < POLYSIZE; ++ipoly)
  {
    tempNorm = tempNorm + coef[ipoly] * fxton;
    fxton = fxton * fresPoly;
  }
  if ( (fitType == 2) || (fitType == 3) )
  {
      tempNorm = pow(10.0, tempNorm);
  }
  tempNorm = tempNorm * tempScale;
  return tempNorm;
}
void loopCube()
{
  unsigned long now = millis();
  if ((now - lastPublishTime) > setting.publishInterval)
  {
    lastPublishTime = now;
    reading.tempA = calcTemp(reading.resA, setting.resScaleA, setting.tempScaleA, setting.fitTypeA, setting.coefA);
    reading.tempB = calcTemp(reading.resB, setting.resScaleB, setting.tempScaleB, setting.fitTypeB, setting.coefB);
    boolean successful = BlinkyPicoW.publishCubeData((uint8_t*) &setting, (uint8_t*) &reading, false);
  }
  if ((now - lastMeasureTime) > ((unsigned long) setting.measInterval))
  {
    lastMeasureTime = now;

    reading.resA = reading.resA + ((setting.refResA * readSpi(csPin1)) - reading.resA) / nsample;
    reading.resB = reading.resB + ((setting.refResB * readSpi(csPin2)) - reading.resB) / nsample;
  }  
  if (BlinkyPicoW.retrieveCubeSetting((uint8_t*) &setting) )
  {
      if (setting.publishInterval < 1000) setting.publishInterval = 1000;
      if (oldNsample != setting.nsample)
      {
        oldNsample = setting.nsample;
        if (setting.nsample < 1) setting.nsample = 1;
        nsample = (float) setting.nsample;
        reading.resA = (setting.refResA * readSpi(csPin1));
        reading.resB = (setting.refResB * readSpi(csPin2));
        lastMeasureTime = now;
      }
      if (oldMeasInterval != setting.measInterval)
      {
        oldMeasInterval = setting.measInterval;
        if (setting.measInterval < 200) setting.measInterval = 200;
        reading.resA = (setting.refResA * readSpi(csPin1));
        reading.resB = (setting.refResB * readSpi(csPin2));
        lastMeasureTime = now;
      }
  }
}

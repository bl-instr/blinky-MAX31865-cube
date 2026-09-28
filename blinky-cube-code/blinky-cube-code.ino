#include <BlinkyPicoW.h>
#include <SPI.h>

// --- Configuration Constants ---
constexpr int BLINKY_DIAG    = 0;
constexpr int CUBE_DIAG      = 0;
constexpr int COMM_LED_PIN   = 14;
constexpr int RST_BUTTON_PIN = 7;
constexpr int NUMCHAN        = 2;

constexpr int CS_PINS[NUMCHAN] = {17, 21};
constexpr int RDY_PINS[NUMCHAN] = {15, 20};
constexpr int POLYSIZE        = 7;

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
  uint8_t fitType[NUMCHAN];

  float refRes[NUMCHAN];
  float resScale[NUMCHAN];
  float tempScale[NUMCHAN];
  float coef[NUMCHAN][POLYSIZE];
};

struct CubeReading
{
  float res[NUMCHAN];
  float temp[NUMCHAN];
};

struct CubeArm {
  bool res[NUMCHAN];
  bool temp[NUMCHAN];
};

// --- Global Variables ---
CubeSetting setting;
CubeReading reading;
CubeReading readingLow;
CubeReading readingHigh;
CubeArm readingArm;


unsigned long lastMeasureTime;
unsigned long lastPublishTime;
float nsample = 1.0;
uint16_t oldNsample;
uint16_t oldMeasInterval;

SPISettings spiSetting(4000000, MSBFIRST, SPI_MODE3);

// --- Helper Functions ---
template <typename T>
inline bool outsideLimits(T current, T low, T high) {
  return (current < low) || (current > high);
}

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
  
  for (int i = 0; i < NUMCHAN; ++i) 
  {
    setting.fitType[i] = 0;
    setting.refRes[i] = 4302.0;
    setting.resScale[i] = 1000.0;
    setting.tempScale[i] = 293.0;
    setting.coef[i][0] = 1.0;
    readingArm.res[i] = true;
    readingArm.temp[i] = true;
    for (int j = 1; j < POLYSIZE; ++j)
    {
      setting.coef[i][j] = 0.0;
      reading.res[i] = 0.0;
    }
    pinMode(RDY_PINS[i], INPUT_PULLDOWN);
    pinMode(CS_PINS[i], OUTPUT);
    digitalWrite(CS_PINS[i], 1);
  }
    
  SPI.setRX(16);
  SPI.setCS(CS_PINS[0]);
  SPI.setSCK(18);
  SPI.setTX(19);  
  SPI.begin(false);
  SPI.setCS(CS_PINS[1]);
  SPI.begin(false);
  intSpi(CS_PINS[0]);
  intSpi(CS_PINS[1]);
  delay(100);
  
  lastPublishTime = millis();
  lastMeasureTime = millis();

  for (int i = 0; i < NUMCHAN; ++i) reading.res[i] = (setting.refRes[i] * readSpi(CS_PINS[i]));
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
    if (BlinkyPicoW.publishCubeData(reinterpret_cast<uint8_t*>(&setting), reinterpret_cast<uint8_t*>(&reading), false)) {
      for (int i = 0; i < NUMCHAN; ++i) {
        if (!outsideLimits(reading.temp[i], readingLow.temp[i], readingHigh.temp[i])) {
          readingArm.temp[i] = true;
        }
      }
    }
  }
  if ((now - lastMeasureTime) > ((unsigned long) setting.measInterval))
  {
    lastMeasureTime = now;
    for (int i = 0; i < NUMCHAN; ++i)
    {
      reading.res[i] = reading.res[i] + ((setting.refRes[i] * readSpi(CS_PINS[i])) - reading.res[i]) / nsample;
      reading.temp[i] = calcTemp(reading.res[i], setting.resScale[i], setting.tempScale[i], setting.fitType[i], setting.coef[i]);
      if (BlinkyPicoW.isInitialized()) 
      {  
        if (outsideLimits(reading.temp[i], readingLow.temp[i], readingHigh.temp[i])) 
        {
          if (readingArm.temp[i]) {
            const bool published = BlinkyPicoW.publishCubeData(
              reinterpret_cast<uint8_t*>(&setting), 
              reinterpret_cast<uint8_t*>(&reading), 
              true
            );
            
            readingArm.temp[i] = !published;
            if (published) {
              lastPublishTime = now;
            }
          }
        }
      }
    }
  }  
  // 3. Check for New MQTT Settings
  const bool newSettings = BlinkyPicoW.retrieveCubeSetting(
    reinterpret_cast<uint8_t*>(&setting), 
    reinterpret_cast<uint8_t*>(&readingLow), 
    reinterpret_cast<uint8_t*>(&readingHigh)
  );

  if (newSettings) 
  {
    if (setting.publishInterval < 1000) setting.publishInterval = 1000;
    if (oldNsample != setting.nsample)
    {
      oldNsample = setting.nsample;
      if (setting.nsample < 1) setting.nsample = 1;
      nsample = (float) setting.nsample;
      for (int i = 0; i < NUMCHAN; ++i) reading.res[i] = (setting.refRes[i] * readSpi(CS_PINS[i]));
      lastMeasureTime = now;
    }
    if (oldMeasInterval != setting.measInterval)
    {
      oldMeasInterval = setting.measInterval;
      if (setting.measInterval < 200) setting.measInterval = 200;
      for (int i = 0; i < NUMCHAN; ++i) reading.res[i] = (setting.refRes[i] * readSpi(CS_PINS[i]));
      lastMeasureTime = now;
    }
  }
}

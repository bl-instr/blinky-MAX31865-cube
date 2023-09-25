from machine import Pin
from time import sleep, sleep_us 

class Ada31865:
#   Configuration bits:
#        Vbias           1=on
#        Conversion mode 1=auto,0=normally off
#        1-shot          1=1-shot (auto-clear)
#        3-wire          1=3-wire,0=2/4 wire
#        Fault detection
#        Fault detection
#        Fault Status    1=clear
#        Filter          1=50Hz,2=60Hz
    def __init__(self,spi,chipSelect,readyPin,r0,nfilterLen,config):
        self.spi = spi
        self.nfilterLen = nfilterLen
        self.r0 = r0
        self.chipSelect = chipSelect
        self.ready = Pin(readyPin, Pin.IN)
        self.rawReading = 0
        buf = bytearray(2)
        buf[0] = 0x80 #configuration write addr
        buf[1] = config
        self.chipSelect(0)
        sleep_us(1)
        self.spi.write(buf)
        self.chipSelect(1)
        sleep(1)
        self.setNfilterLen(nfilterLen)
    def __readRaw(self):
        self.chipSelect(0)
        sleep_us(1)
        MSBLSB = self.spi.read(4,0x00)
        self.chipSelect(1)
        MSB = MSBLSB[2]
        LSB = MSBLSB[3]
        MSB = MSB<<8
        raw = MSB+LSB
        raw = raw>>1 #remove fault bit
        return raw
    def setNfilterLen(self,nfilterLen):
        self.nfilterLen = nfilterLen
        self.rawReading = self.__readRaw()
    def read(self):
        raw = self.__readRaw()
        self.rawReading = self.rawReading + (self.__readRaw() - self.rawReading) / self.nfilterLen
        return self.rawReading

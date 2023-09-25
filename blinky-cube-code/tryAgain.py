from machine import Pin, SPI
from time import sleep
from Ada31865 import Ada31865

# Initalize SPI
chipSelect_0 = Pin(17, mode=Pin.OUT, value=1)
chipSelect_1 = Pin(21, mode=Pin.OUT, value=1)
sleep(0.1)
spi = SPI(0,baudrate=4000000, polarity=0, phase=1, sck=Pin(18), mosi=Pin(19), miso=Pin(16), firstbit=SPI.MSB, bits=8)
sleep(0.1)

ada31865_0 =  Ada31865(spi=spi ,chipSelect=chipSelect_0,readyPin=15,r0=4300,nfilterLen=10,config=0b11000011)
ada31865_1 =  Ada31865(spi=spi ,chipSelect=chipSelect_1,readyPin=20,r0=4300,nfilterLen=10,config=0b11000011)


sleep(1)
while True:
    res0 = ada31865_0.read()
    res1 = ada31865_1.read()
    print(res0, res1,sep=',')
    sleep(0.5)


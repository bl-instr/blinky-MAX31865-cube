from utime import sleep, ticks_ms
from machine import Pin, SPI
from Ada31865 import Ada31865
from BlinkyPicoWifi import BlinkyPicoWifi

g_chitChat = True
g_measInterval = 0.5
g_lastMeasTick = 0
g_pubInterval = 5
g_lastPubTick = 0
g_watchdog = 0


g_blinkyPicoWifi = BlinkyPicoWifi(chitChat=True,wifiApButtonPinNo=7,comLedPinNo=14,mqttRetryInterval=1)

print("Starting cube")

g_chipSelect_1 = Pin(17, mode=Pin.OUT, value=1)
g_chipSelect_2 = Pin(21, mode=Pin.OUT, value=1)
sleep(0.1)
g_spi = SPI(0,baudrate=4000000, polarity=0, phase=1, sck=Pin(18), mosi=Pin(19), miso=Pin(16), firstbit=SPI.MSB, bits=8)
sleep(0.1)

g_ada31865_1 =  Ada31865(spi=g_spi ,chipSelect=g_chipSelect_1,readyPin=15,r0=4300,nfilterLen=10,config=0b11000011)
g_ada31865_2 =  Ada31865(spi=g_spi ,chipSelect=g_chipSelect_2,readyPin=20,r0=4300,nfilterLen=10,config=0b11000011)

g_raw1 = g_ada31865_1.read()
g_raw2 = g_ada31865_2.read()
while True:
    new_time = ticks_ms()
    g_blinkyPicoWifi.check()
    if (new_time - g_lastMeasTick) > (g_measInterval * 1000):
        g_raw1 = g_ada31865_1.read()
        g_raw2 = g_ada31865_2.read()
        g_lastMeasTick = new_time
    if (new_time - g_lastPubTick) > (g_pubInterval * 1000):
        g_lastPubTick = new_time
        g_watchdog = g_watchdog + 1
        if g_watchdog > 32765: g_watchdog = 0
        readings = {'state' : 0, 'watchdog': g_watchdog, 'raw1' : g_raw1, 'raw2' : g_raw2}
        g_blinkyPicoWifi.publish(readings)

        
            
    

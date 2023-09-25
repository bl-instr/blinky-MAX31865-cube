import network
from machine import Pin
from ubinascii import hexlify
from time import sleep

commLed = Pin('LED', Pin.OUT)
commLed.on()
wlan = network.WLAN(network.STA_IF)
wlan.active(True)
mac = hexlify(network.WLAN().config('mac'),':').decode()
print('mac = ' + mac)
while True:
    sleep(1)
    commLed.off()
    sleep(1)
    commLed.on()
    

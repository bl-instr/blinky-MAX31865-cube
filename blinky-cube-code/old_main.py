import ujson as json
from utime import sleep, ticks_ms
from machine import Pin, SPI
from BlinkyCubeCredsAccessPoint import BlinkyCubeCredsAccessPoint
import rp2
import network
from ubinascii import hexlify
from umqttsimple import MQTTClient
from Ada31865 import Ada31865


g_new_time = ticks_ms()
g_chitChat = True
g_wifiTimeout = 20
g_wifiRetry = 30
g_lastTimeWifiApButtonPressed = g_new_time 
g_wifiApButtonDownTime = g_new_time
g_wifiApButtonDown = False
g_commLED = Pin(14, mode=Pin.OUT, value=0)
g_mqttClient = None
g_mqttConnected = False
g_mqttLastTry = g_new_time
g_mqttRetry = 30
g_creds = {}
g_measInterval = 500
g_lastMeas = g_new_time
g_wlan


def wifiApButtonHandler(pin):
    global g_lastTimeWifiApButtonPressed, g_wifiApButtonDownTime, g_wifiApButtonDown
    new_time = ticks_ms()
    if (new_time - g_lastTimeWifiApButtonPressed) > 200:
        if not g_wifiApButtonDown: 
            if pin.value() > 0:
                g_lastTimeWifiApButtonPressed = new_time
                g_wifiApButtonDown = True
                g_wifiApButtonDownTime = new_time
                if (g_chitChat): print("WifiApButton down")
        else:
            if pin.value() < 1:
                g_lastTimeWifiApButtonPressed = new_time
                g_wifiApButtonDown = False
                if (g_chitChat): print("WifiApButton up")
                
def connectMqtt(creds,chitChat):
    client_id   = creds['box'] + "-" + creds['trayName'] + "-" + creds['trayType']
    sleep(1)
    g_mqttClient = MQTTClient(client_id, creds['mqttServer'], 1883, creds['mqttUsername'], creds['mqttPassword'], keepalive=1)
    try:
        g_mqttClient.connect()
        if (chitChat): print('Connected to %s MQTT broker' % (creds['mqttServer']))
        g_mqttConnected = True
    except:
        if (chitChat): print('MQTT connection failed')
        g_mqttConnected = False
    g_new_time = ticks_ms()
    g_wifiLastTry = g_new_time
    return 

def checkWifi(creds, timeoutIn=20, chitChat=False):
    if (g_new_time - g_wifiLastTry) < (g_wifiRetry * 1000): return
    if g_wlan != None:
        if  g_wlan.isconnected():
    rp2.country(creds['country'])
    Pin(23, Pin.OUT).high() # turn on WiFi module
    g_wlan = network.WLAN(network.STA_IF)
    g_wlan.active(True)
    mac = hexlify(network.WLAN().config('mac'),':').decode()
    if (chitChat):
        print('mac = ' + mac)
        print('wifi channel = ' + str(g_wlan.config('channel')))
        print('wifi essid = ' + g_wlan.config('essid'))
        print('wifi txpower = ' + str(g_wlan.config('txpower')))
    g_wlan.connect(creds['ssid'], creds['wifiPassword'])
    timeout = timeoutIn
    while timeout > 0:
        if g_wlan.status() < 0 or g_wlan.status() >= 3:
            break
        timeout -= 1
        if (chitChat): print('Waiting for connection...')
        sleep(1)
    wlan_status = g_wlan.status()
    if wlan_status != 3:
        if (chitChat): print('Wi-Fi connection failed')
        g_wlan.active(False)
    else:
        if (chitChat): print('Wifi Connected')
        status = g_wlan.ifconfig()
        if (chitChat): print('ip = ' + status[0])
        connectMqtt(creds, chitChat)
    g_new_time = ticks_ms()
    g_wifiLastTry = g_new_time
    return
        

g_wifiApButtonPin = Pin(7, Pin.IN)
g_wifiApButtonPin.irq(trigger= (Pin.IRQ_RISING | Pin.IRQ_FALLING), handler = wifiApButtonHandler)
with open('creds.json', 'r') as f: g_creds = json.load(f)
g_credAp = BlinkyCubeCredsAccessPoint(g_creds,'blinky-lite')
g_wlan = connectWifi(g_creds, timeoutIn=20, chitChat=g_chitChat)
g_new_time = ticks_ms()
g_wifiLastTry = g_new_time 

print("Starting cube")

chipSelect_0 = Pin(17, mode=Pin.OUT, value=1)
chipSelect_1 = Pin(21, mode=Pin.OUT, value=1)
sleep(0.1)
spi = SPI(0,baudrate=4000000, polarity=0, phase=1, sck=Pin(18), mosi=Pin(19), miso=Pin(16), firstbit=SPI.MSB, bits=8)
sleep(0.1)

g_ada31865_0 =  Ada31865(spi=spi ,chipSelect=chipSelect_0,readyPin=15,r0=4300,nfilterLen=10,config=0b11000011)
g_ada31865_1 =  Ada31865(spi=spi ,chipSelect=chipSelect_1,readyPin=20,r0=4300,nfilterLen=10,config=0b11000011)

while True:
    g_new_time = ticks_ms()
    while g_wifiApButtonDown:
        g_new_time = ticks_ms()
        if ((new_time - g_lastTimeWifiApButtonPressed) > 10000):
            g_wifiApButtonDown = False
            g_commLED(1)
            if (g_chitChat): print("wifiApWebsite up")
            if g_wlan.isconnected() :
                g_wlan.disconnect()
                g_wlan.active(False)
            g_creds = g_credAp.serveWebSite()
            with open('creds.json', 'w') as f: json.dump(g_creds, f)
            if (g_chitChat): print("New Creds. Restarting wifi...")
            sleep(5)
            g_commLED(0)
            sleep(2)
            g_wlan = connectWifi(g_creds, timeoutIn=g_wifiTimeout, chitChat=g_chitChat)
    checkWifi()
    if (g_new_time - g_wifiLastTry) > (g_wifiRetry * 1000):
        if not g_wlan.isconnected():
            g_wlan = connectWifi(g_creds, timeoutIn=g_wifiTimeout, chitChat=g_chitChat)
        else:
            if not g_mqttConnected: connectMqtt(g_creds,g_chitChat)
            
    

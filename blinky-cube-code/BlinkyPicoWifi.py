import ujson as json
from utime import sleep, ticks_ms
import network
from ubinascii import hexlify
from BlinkyCubeCredsAccessPoint import BlinkyCubeCredsAccessPoint
from machine import Pin
from umqttsimple import MQTTClient

g_chitChat = True
g_lastTimeWifiApButtonPressed = 0 
g_wifiApButtonDownTime = 0
g_wifiApButtonDown = False

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
    

class BlinkyPicoWifi:
    def __init__(self, comLedPinNo, wifiApButtonPinNo, chitChat=False, retryInterval=30, connectTimeout=20,  mqttRetryInterval=20):
        self.creds = {}
        with open('creds.json', 'r') as f: self.creds = json.load(f)
        self.wlan = network.WLAN(network.STA_IF)
        self.wlan.disconnect()
        self.wlan.active(False)
        sleep(1)
        self.wlan.active(True)
        self.mac = hexlify(network.WLAN().config('mac'),':').decode()
        self.chitChat = chitChat
        self.retryInterval = retryInterval
        self.connectTimeout = connectTimeout
        self.mqttClient = None
        self.mqttLastTryTick = 0
        self.mqttRetryInterval = mqttRetryInterval
        self.lastConnectTryTick = 0
        if (self.chitChat):
            print('mac = ' + self.mac)
            print('wifi channel = ' + str(self.wlan.config('channel')))
            print('wifi txpower = ' + str(self.wlan.config('txpower')))
        self.credAp = BlinkyCubeCredsAccessPoint(self.creds,'blinky-lite')
        self.wifiApButtonPin = Pin(wifiApButtonPinNo, Pin.IN)
        self.wifiApButtonPin.irq(trigger= (Pin.IRQ_RISING | Pin.IRQ_FALLING), handler = wifiApButtonHandler)
        self.commLED = Pin(comLedPinNo, mode=Pin.OUT, value=0)
        self.connect()
    def disconnect(self):
        if self.wlan.isconnected():
            self.wlan.disconnect()
            self.wlan.active(False)
    def connect(self):
        if self.wlan.isconnected():
            self.lastConnectTryTick = ticks_ms()
            return
        self.wlan.connect(self.creds['ssid'], self.creds['wifiPassword'])
        timeout = self.connectTimeout
        while timeout > 0:
            if self.wlan.status() < 0 or self.wlan.status() >= 3:
                break
            timeout -= 1
            if (self.chitChat): print('Waiting for connection...')
            sleep(1)
        wlan_status = self.wlan.status()
        if wlan_status != 3:
            if (self.chitChat): print('Wi-Fi connection failed')
            self.wlan.active(False)
        else:
            if (self.chitChat): print('Wifi Connected')
            status = self.wlan.ifconfig()
            if (self.chitChat): print('ip = ' + status[0])
            sleep(1)
            client_id   = self.creds['box'] + "-" + self.creds['trayName'] + "-" + self.creds['trayType']
            self.mqttClient = MQTTClient(client_id, self.creds['mqttServer'], 1883, self.creds['mqttUsername'], self.creds['mqttPassword'], keepalive=1)
            self.connectMqtt()
        self.lastConnectTryTick = ticks_ms()
    def check(self):
        self.checkWifiApButton()
        if (ticks_ms() - self.lastConnectTryTick) > (self.retryInterval * 1000): self.connect()
        if (ticks_ms() - self.mqttLastTryTick)    > (self.mqttRetryInterval * 1000):
            if self.wlan.isconnected():
                self.mqttLastTryTick = ticks_ms()
                try:
                    if (self.chitChat): print('Start Ping')
                    self.mqttClient.ping()
                    if (self.chitChat): print('End Ping')
                except:
                    if (self.chitChat): print("Reconnectig MQTT")
                    self.connectMqtt()
    def checkWifiApButton(self):
        global g_lastTimeWifiApButtonPressed, g_wifiApButtonDownTime, g_wifiApButtonDown
        while g_wifiApButtonDown:
            new_time = ticks_ms()
            if ((new_time - g_lastTimeWifiApButtonPressed) > 10000):
                if self.wlan.isconnected() :
                    if (self.chitChat): print("Disconnecting from external Wifi")
                    self.wlan.disconnect()
                    self.wlan.active(False)
                    sleep(2)
                self.commLED(1)
                if (self.chitChat): print("wifiApWebsite up")
                self.creds = self.credAp.serveWebSite()
                with open('creds.json', 'w') as f: json.dump(self.creds, f)
                if (self.chitChat): print("New Creds. Restarting wifi...")
                sleep(2)
                self.commLED(0)
                sleep(2)
                self.connect()
                g_lastTimeWifiApButtonPressed = ticks_ms()
                g_wifiApButtonDown = False
    def connectMqtt(self):
        try:
            self.mqttClient.connect(clean_session=True)
            if (self.chitChat): print('Connected to %s MQTT broker' % (self.creds['mqttServer']))
        except:
            if (self.chitChat): print('MQTT connection failed')
            self.commLED.value(0)
        self.mqttLastTrytick = ticks_ms()
        return 
    def publish(self,msg):
        topic = self.creds['box']
        topic = topic + "/" + self.creds['cubeType']
        topic = topic + "/" + self.creds['trayType']
        topic = topic + "/" + self.creds['trayName']
        topic = topic + "/reading"
        if (self.chitChat): print('MQTT topic: ' + str(topic))
        if (self.chitChat): print('MQTT   msg: ' + str(msg))
        try:
            self.mqttClient.publish(topic,json.dumps(msg))
            if (self.chitChat): print('Publish complete')
            self.commPinToggle()
        except:
            if (self.chitChat): print('MQTT publish failed')
            self.commLED.value(0)
    def commPinToggle(self):
        pinVal = self.commLED.value()
        if pinVal > 0:
            self.commLED.value(0)
        else:
            self.commLED.value(1)
    
        

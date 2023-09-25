import rp2
import network
import ubinascii
from machine import Pin, unique_id, ADC, SPI, reset
from umqttsimple import MQTTClient
from time import sleep as sleep, ticks_ms, sleep_us 
from ubinascii import hexlify
import ujson as json
import gc
gc.collect()

chitChat = True

commLed = Pin('LED', Pin.OUT)
#commLed = Pin(2, Pin.OUT)
commLed.on()

    
def connectWifi(timeoutIn=20, chitChat=False, country='SE'):
    rp2.country(country)
    Pin(23, Pin.OUT).high() # turn on WiFi module
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    mac = hexlify(network.WLAN().config('mac'),':').decode()
    if (chitChat):
        print('mac = ' + mac)
        print('wifi channel = ' + str(wlan.config('channel')))
        print('wifi essid = ' + wlan.config('essid'))
        print('wifi txpower = ' + str(wlan.config('txpower')))
    creds = {}
    with open('creds.json', 'r') as f: creds = json.load(f)
    wlan.connect(creds['ssid'], creds['wifiPassword'])
    
    timeout = timeoutIn
    while timeout > 0:
        if wlan.status() < 0 or wlan.status() >= 3:
            break
        timeout -= 1
        if (chitChat): print('Waiting for connection...')
        sleep(1)
    wlan_status = wlan.status()
    if wlan_status != 3:
        if (chitChat): print('Wi-Fi connection failed')
        return None
    else:
        if (chitChat): print('Wifi Connected')
        status = wlan.ifconfig()
        if (chitChat): print('ip = ' + status[0])
        return wlan
    return None

def oneShotMqttPublish(wlan, msg, chitChat=False, keepalive=0):
    if wlan == None: return None
    client_id   = creds['box'] + "-" + creds['trayName'] + "-" + creds['trayType']
    topic = creds['box']
    topic = topic + "/" + creds['cubeType']
    topic = topic + "/" + creds['trayType']
    topic = topic + "/" + creds['trayName']
    topic = topic + "/reading"
    if (chitChat): print('MQTT topic: ' + str(topic))
    if (chitChat): print('MQTT msg: ' + str(msg))
    try:
        client = MQTTClient(client_id, creds['mqttServer'], 1883, creds['mqttUsername'], creds['mqttPassword'], keepalive=keepalive)
        client.connect()
        if (chitChat): print('Connected to %s MQTT broker' % (creds['mqttServer']))
        client.publish(topic, msg)
        client.disconnect()
        return client
    except:
        if (chitChat): print('Failed to connect to MQTT broker.')
        return None
    return None
    
def readAdc(pin):
    vread = pin.read_u16() * (3.3 / 65535)
    return vread
    
sleep(1)

tstart = ticks_ms()

vsys = readAdc(ADC(29))
vsys = round(vsys * 3.0,4)
ictemp = 27 - (readAdc(ADC(4)) - 0.706)/0.001721
ictemp = round(ictemp,4)
deltatT = ticks_ms() - tstart;
if (chitChat): print("")
if (chitChat): print("vsys    (V): " + str(vsys))
if (chitChat): print("ictemp  (C): " + str(ictemp))
if (chitChat): print("")


#wlan = connectWifi(timeoutIn=30, chitChat=chitChat, country='SE')    # need to check that wlan is connected

commLed.off()
          

ready = Pin(20, Pin.IN)

# Initalize SPI
cs = Pin(21, mode=Pin.OUT, value=1)
spi = SPI(0,baudrate=4000000, polarity=1, phase=1, sck=Pin(18), mosi=Pin(19), miso=Pin(16), firstbit=SPI.MSB, bits=8)

cs(0)
spi.write(0x80)
spi.write(0xB0)
cs(1)

sleep_us(1000)
cs(0)
spi.write(1)
reg1 = spi.write(0xFF)
reg2 = spi.write(0xFF)
cs(1)

print(reg1)
print(reg2)
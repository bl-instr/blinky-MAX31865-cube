import usocket as socket    
import network          
from ubinascii import hexlify
from time import sleep


    
htmlUrlCode = {
    '20' : ' ',
    '21' : '!',
    '22' : '"',
    '23' : '#',
    '24' : '$',
    '25' : '%',
    '26' : '&',
    '27' : '\'',
    '28' : '(',
    '29' : ')',
    '2A' : '*',
    '2B' : '+',
    '2C' : ',',
    '2D' : '-',
    '2E' : '.',
    '2F' : '/',
    '3A' : ':',
    '3B' : ';',
    '3C' : '<',
    '3D' : '=',
    '3E' : '>',
    '3F' : '?',
    '40' : '@',
    '5B' : '[',
    '5C' : '\\',
    '5D' : ']',
    '5E' : '^',
    '5F' : '_',
    '60' : '`',
    '7B' : '{',
    '7C' : '|',
    '7D' : '}',
    '7E' : '~'
}
class BlinkyCubeCredsAccessPoint:
    def __init__(self, credList, apPassword):
        self.credList = credList
        ssid = credList['trayType'] + '-' + credList['trayName']  
        self.ap = network.WLAN(network.AP_IF)
        self.ap.config(essid=ssid, password=apPassword)
    def serveWebSite(self):
        self.ap.active(True) 
        while self.ap.active() == False:
            pass
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)   #creating socket object
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(('', 80))
        s.listen(5)
        post = False
        while not post:
          conn, addr = s.accept()
          request = conn.recv(1024)
          index = str(request).find("data=begin");
          if index >= 0:
              post = True
              dataString = str(request)[index:len(str(request))-1]
              lastIndex = dataString.find("data=end")
              while lastIndex < 0:
                  request = conn.recv(1024)
                  dataString = dataString + str(request)[2:len(str(request))-1]
                  lastIndex = dataString.find("data=end")

              newCredData = dataString.split("&")
              for cred in newCredData:
                  keyValue = cred.split("=")
                  if keyValue[0] != "data":
                      plusIndex = keyValue[1].find("+")
                      while plusIndex > -1:
                          keyValue[1] = keyValue[1].replace("+"," ")
                          plusIndex = keyValue[1].find("+")
                      percIndex = keyValue[1].find("%")
                      while percIndex > -1:
                          frontString = keyValue[1][:percIndex]
                          backString = keyValue[1][percIndex + 3:]
                          htmlcode = keyValue[1][percIndex + 1:percIndex + 3]
                          keyValue[1] = keyValue[1][:percIndex] + htmlUrlCode[keyValue[1][percIndex + 1:percIndex + 3]] + keyValue[1][percIndex + 3:]
                          percIndex = keyValue[1].find("%")
                      self.credList[keyValue[0]] = keyValue[1]

              conn.send(self.__acceptedWebPage())
              conn.close()
              sleep(1)
          else:
              response = self.__web_page()
              conn.send(response)
              conn.close()
        sleep(1)
        self.ap.active(False)
        while self.ap.active() == True:
            pass
        s.close()
        sleep(1)
        return self.credList
    def __web_page(self):
        mac = hexlify(network.WLAN().config('mac'),':').decode()
        html = """
<html>
  <head>
    <title>Blinky-Lite Cube Credentials</title>
    <style>
      body{background-color: #083357 !important;font-family: Arial;}
      .labeltext{color: white;font-size:250%;}
      .formtext{color: black;font-size:250%;}
      .cell{padding-bottom:25px;}
    </style>
  </head>
  <body>
    <h1 style=\"color:white;font-size:300%;text-align: center;\">Blinky-Lite Cube Credentials</h1>
    <h2 style=\"color:yellow;font-size:250%;text-align: center;\">Device MAC:  """ + mac + """</h2>
    <hr>
    <div>
      <form action=\"/disconnected\" method=\"POST\">
        <input type=\"hidden\" name=\"data\" value=\"begin\" class=\"formtext\"/>
        <table align=\"center\">"""
        html = html + self.__credLine("ssid",         "SSID")
        html = html + self.__credLine("wifiPassword", "Wifi Password")
        html = html + self.__credLine("country", "Country")
        html = html + self.__credLine("mqttServer", "MQTT Server")
        html = html + self.__credLine("mqttUsername", "MQTT Username")
        html = html + self.__credLine("mqttPassword", "MQTT Password")
        html = html + self.__credLine("box", "Box")
        html = html + self.__credLine("trayType", "Tray Type")
        html = html + self.__credLine("trayName", "Tray Name")
        html = html + self.__credLine("cubeType", "Cube Type")
        html = html + """    
          <tr>
            <td><input type=\"hidden\" name=\"data\" value=\"end\" class=\"formtext\"/></td>
            <td><input type=\"submit\" class=\"formtext\"/></td>
          </tr>
        </table>
      </form>
    </div>
  </body>
</html>
"""
        return html
    def __credLine(self, credName, credLabel):
        html = """
          <tr>
            <td class=\"cell\"><label class=\"labeltext\">""" + credLabel + """</label></td>
            <td class=\"cell\"><input name=\"""" + credName + """\" type=\"text\" value=\"""" + self.credList[credName] + """\" class=\"formtext\"/></td>
          </tr>"""
        return html    
    def __acceptedWebPage(self):
        html = """
<html>
  <head>
    <title>Blinky-Lite Cube Credentials</title>
    <style>
      body{background-color: #083357 !important;font-family: Arial;}
      .labeltext{color: white;font-size:250%;}
      .formtext{color: black;font-size:250%;}
      .cell{padding-bottom:25px;}
    </style>
  </head>
  <body>
    <h1 style=\"color:white;font-size:300%;text-align: center;\">Blinky-Lite Cube Credentials</h1>
    <h1 style=\"color:yellow;font-size:300%;text-align: center;\">Accepted</h1>
  </body>
</html>
"""
        return html
        



  

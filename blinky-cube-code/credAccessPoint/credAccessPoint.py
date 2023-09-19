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
def credLine(credName, credLabel,credList):
    html = """
          <tr>
            <td class=\"cell\"><label class=\"labeltext\">""" + credLabel + """</label></td>
            <td class=\"cell\"><input name=\"""" + credName + """\" type=\"text\" value=\"""" + credList[credName] + """\" class=\"formtext\"/></td>
          </tr>"""
    return html    
def web_page(credList):
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
    html = html + credLine("ssid",         "SSID",         credList)
    html = html + credLine("wifiPassword", "Wifi Password",credList)
    html = html + credLine("mqttServer", "MQTT Server",credList)
    html = html + credLine("mqttUsername", "MQTT Username",credList)
    html = html + credLine("mqttPassword", "MQTT Password",credList)
    html = html + credLine("box", "Box",credList)
    html = html + credLine("trayType", "Tray Type",credList)
    html = html + credLine("trayName", "Tray Name",credList)
    html = html + credLine("cubeType", "Cube Type",credList)
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
def acceptedWebPage():
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
def credWebSite(credList, apPassword):
    ssid = credList['trayType'] + '-' + credList['trayName']  

    ap = network.WLAN(network.AP_IF)
    ap.config(essid=ssid, password=apPassword)
    ap.active(True) 

    while ap.active() == False:
        pass

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)   #creating socket object
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
                  credList[keyValue[0]] = keyValue[1]

          conn.send(acceptedWebPage())
          conn.close()
          sleep(5)
          ap.active(False)
      else:
          response = web_page(credList)
          conn.send(response)
          conn.close()
    return
  

import ujson as json
from BlinkyCubeCredsAccessPoint import BlinkyCubeCredsAccessPoint

creds = {}
with open('creds.json', 'r') as f:
    creds = json.load(f)
    
credAp = BlinkyCubeCredsAccessPoint(creds,'blinky-lite')
creds = credAp.serveWebSite()
print(creds)
with open('creds.json', 'w') as f:
    json.dump(creds, f)

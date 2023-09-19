import ujson as json
import credAccessPoint

creds = {}
with open('creds.json', 'r') as f:
    creds = json.load(f)

credAccessPoint.credWebSite(creds,'blinky-lite')
print(creds)
with open('creds.json', 'w') as f:
    json.dump(creds, f)

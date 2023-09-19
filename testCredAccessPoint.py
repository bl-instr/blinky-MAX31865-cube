import ujson as json
import webdev

creds = {}
with open('creds.json', 'r') as f:
    creds = json.load(f)

webdev.credWebSite(creds,'blinky-lite')
print(creds)
with open('creds.json', 'w') as f:
    json.dump(creds, f)

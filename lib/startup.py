import requests
import threading
from . import log
from . import ext

def check_antisniper_key(key):
    data = requests.get(f'https://api.antisniper.net/stats?key={key}').json()
    return data.get('success', False)

def check_inject(self):
    result = ext.send("id")
    if result.startswith("id: "):
        self.mcid = result[4:].rstrip("\x00").strip()
        self.status.set_log(f"injected")
        self.status.injected = True
        return True

def startup(self):
    if not check_antisniper_key(self.config.get("antisniper_key")):
        self.status.set_log("invalid antisniper apikey")
    if not check_inject(self):
        self.status.set_log("failed to inject")
    threading.Thread(target=log.reader, args=[self], daemon=True).start()
    threading.Thread(target=log.checker, args=[self], daemon=True).start()
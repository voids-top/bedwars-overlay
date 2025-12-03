import requests
import threading
import traceback
import time
from . import log
from . import ext
from . import utils


def check_antisniper_key(key):
    data = requests.get(f'https://api.antisniper.net/stats?key={key}').json()
    return data.get('success', False)

def check_inject(self):
    while True:
        time.sleep(5)
        try:
            injected_pids = ext.injected_pids()
            self.status.injected = True if injected_pids else False
            if self.status.injected:
                if len(injected_pids) == 1:
                    ext.send("id")
                    result = ext.recv()
                    if result.startswith("id: "):
                        self.mcid = result[4:].rstrip("\x00").strip()
                        self.status.set_log(f"injected")
                        self.status.injected = True
                    ext.send("score")
                    ext.recv()
                    ext.send("tab")
                    ext.recv()
            pids = ext.get_pids_by_name("javaw.exe")
            for pid in pids:
                if not pid in injected_pids:
                    injected_pids.append(pid)
                    titles = ext.get_window_titles_by_pid(pid)
                    if [True for title in titles if "1.8" in title]:
                        print("[inject]", ext.inject(utils.resource_path("injector.exe"), pid, utils.resource_path("util.dll")))
        except:
            traceback.print_exc()
            pass

def startup(self):
    if not check_antisniper_key(self.config.get("antisniper_key")):
        self.status.set_log("invalid antisniper apikey")
    #if not check_inject(self):
    #    self.status.set_log("failed to inject")
    threading.Thread(target=log.reader, args=[self], daemon=True).start()
    threading.Thread(target=log.checker, args=[self], daemon=True).start()
    threading.Thread(target=check_inject, args=[self], daemon=True).start()
    threading.Thread(target=ext.accept_loop, args=[self], daemon=True).start()
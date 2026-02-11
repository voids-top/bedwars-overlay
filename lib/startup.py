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
                    # One-time chat test to verify send_chat on Badlion
                    if not self.status.test_chat_sent:
                        #ext.send("chat [overlay] Hello from overlay")
                        #print("[test] chat send ->", ext.recv())
                        self.status.test_chat_sent = True
                    ext.send("id")
                    result = ext.recv()
                    #print(result)
                    if result and result.startswith("id: ") and result.strip() != "id: (unknown)":
                        self.mcid = result[4:].rstrip("\x00").strip()
                        self.status.set_log(f"injected")
                        self.status.injected = True
                    else:
                        # If ID not resolved, allow re-sending chat test on next loop
                        self.status.test_chat_sent = False
                    #ext.send("score")
                    #ext.recv()
                    #ext.send("tab")
                    #ext.recv()
            pids = ext.get_pids_by_name("javaw.exe")
            for pid in pids:
                if not pid in injected_pids:
                    injected_pids.append(pid)
                    titles = ext.get_window_titles_by_pid(pid)
                    if [True for title in titles if "1.8" in title]:
                        dll_candidates = (
                            "build\\util8b.dll",
                            "build\\util8.dll",
                            "build\\util7.dll",
                            "build\\util6.dll",
                            "build\\util.dll",
                        )
                        dll_path = None
                        for rel in dll_candidates:
                            p = utils.resource_path(rel)
                            if utils.path.exists(p):
                                dll_path = p
                                break
                        if dll_path:
                            print("[inject]", ext.inject(utils.resource_path("injector.exe"), pid, dll_path))
                        else:
                            print("[inject] no util dll found")
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

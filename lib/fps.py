import time
import threading

frames = 0
fps = 0

def fps_checker():
    global frames, fps
    while True:
        time.sleep(1)
        fps = frames
        frames = 0

threading.Thread(target=fps_checker, daemon=True).start()

class FPSController:
    ERR_COR: float = 0.000001
    AVG_DEPTH: int = 10

    def __init__(self, target_fps=30) -> None:
        self.target_fps = target_fps
        self.refresh_rate_ms = 1000 / self.target_fps
        self.refresh_rate_s = self.refresh_rate_ms / 1000

        self.correction = 0.0
        self.refreshes = []
        self.last_refresh = time.time()
    @property
    def fps(self):
        return fps
    def pause(self) -> int:
        global frames
        frames += 1
        now = time.time()
        self.refreshes.append(now - self.last_refresh)
        self.refreshes = self.refreshes[-self.AVG_DEPTH:]
        self.last_refresh = now

        average = sum(self.refreshes) / len(self.refreshes)
        if float('%.5f' % average) < self.refresh_rate_s:
            self.correction += self.ERR_COR
        elif float('%.5f' % average) > self.refresh_rate_s:
            self.correction -= self.ERR_COR

        return int(1000 * min(self.refresh_rate_s, (max(0, (2 * self.refresh_rate_s - average + self.correction)))))

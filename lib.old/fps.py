import time

class FPSController():
    ERR_COR: float = 0.000001
    AVG_DEPTH: int = 10

    def __init__(self, fps=30) -> None:
        self.fps = fps
        self.refresh_rate_ms = 1000 / self.fps
        self.refresh_rate_s = self.refresh_rate_ms / 1000

        self.correction = 0.0
        self.refreshes = []

        self.last_refresh = time.time()

    def pause(self) -> int:
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

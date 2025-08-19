import time

class TempValue:
    def __init__(self):
        self._offsetx = 0
        self._offsety = 0
        self.moving = None

class Status:
    def __init__(self):
        self.updated = True
        self.hidden = False
        self.add_width = 0
        self.add_height = 0
        self.client = None
        self.mode = None
        self.players = []
        self.noticed = []
        self.moving = False
        self.log = "Started"
        self.logfile = (None, None)
        self.last_update = 0
        self.last_notice = 0
        self.temp = TempValue()
        self.injected = False
    def set_log(self, log):
        self.log = log
        self.updated = True
    def update(self):
        self.updated = True
        self.last_update = time.time()
        self.show()
    def done(self):
        self.updated = False
    def hide(self):
        self.hidden = True
        self.updated = True
    def show(self):
        self.hidden = False
        self.updated = True
    def set_mode(self, mode):
        self.mode = mode
        self.updated = True
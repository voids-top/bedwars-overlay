from lib import utils
from lib.data import colors
from lib.fps import FPSController
from modules.popup import AntiSniperKeyInput

positions = {}

def render(self, rewrite=False):
    if not positions:
        positions["opacity"] = utils.calcpos(self.config.get('opacity'), 150, 10, 100)
        positions["fps"] = utils.calcpos(self.config.get('fps'), 150, 1, 100)
        positions["refresh"] = utils.calcpos(self.config.get('refresh'), 150, 1000, 10)
        positions["fkdr"] = utils.calcpos(self.config.get('fkdr'), 150, 10, 200)
    if rewrite:
        self.canvas.create_text(250, 45, text='Settings', anchor='center', font=('Minecraftia', 15), fill='#FFFFFF', tag='mode')
        self.canvas.create_line(299, 80, 451, 80, fill='#FFFFFF', width=3, tag='mode')
        self.canvas.create_line(299, 100, 451, 100, fill='#FFFFFF', width=3, tag='mode')
        self.canvas.create_line(299, 120, 451, 120, fill='#FFFFFF', width=3, tag='mode')
        self.canvas.create_text(125, 265, text='Custom Logfile', anchor='center', font=('Minecraftia', 8), fill='#FFFFFF', tag='mode')
        self.canvas.create_text(125, 245, text='Mode', anchor='center', font=('Minecraftia', 8), fill='#FFFFFF', tag='mode')
        self.canvas.create_text(125, 285, text='Reset API Key', anchor='center', font=('Minecraftia', 8), fill='#FFFFFF', tag='mode')
        self.canvas.create_text(125, 150, text='Transparent Background'.format(int(self.config.get('refresh'))), anchor='center', font=('Minecraftia', 8), fill='#FFFFFF', tag='mode')
        self.canvas.create_line(350, 150, 365, 150, fill='#FFFFFF', width=3, tag='mode')
        x = 280
        y = 285
        self.canvas.create_line(x - 26, y - 8, x + 25, y - 8, fill='#FFFFFF', width=2, tag='mode')
        self.canvas.create_line(x - 26, y + 8, x + 25, y + 8, fill='#FFFFFF', width=2, tag='mode')
        self.canvas.create_line(x - 25, y - 8, x - 25, y + 8, fill='#FFFFFF', width=2, tag='mode')
        self.canvas.create_line(x + 24, y - 8, x + 24, y + 8, fill='#FFFFFF', width=2, tag='mode')
        self.canvas.create_text(x, y, text='Hypixel', anchor='center', font=('Minecraftia', 7), fill='#FFFFFF', tag='mode')
        x = 370
        y = 285
        self.canvas.create_line(x - 36, y - 8, x + 34, y - 8, fill='#FFFFFF', width=2, tag='mode')
        self.canvas.create_line(x - 36, y + 8, x + 34, y + 8, fill='#FFFFFF', width=2, tag='mode')
        self.canvas.create_line(x - 35, y - 8, x - 35, y + 8, fill='#FFFFFF', width=2, tag='mode')
        self.canvas.create_line(x + 33, y - 8, x + 33, y + 8, fill='#FFFFFF', width=2, tag='mode')
        self.canvas.create_text(x, y, text='Antisniper', anchor='center', font=('Minecraftia', 7), fill='#FFFFFF', tag='mode')
        x = 325
        y = 265
        self.canvas.create_line(x - 26, y - 8, x + 25, y - 8, fill='#FFFFFF', width=2, tag='mode')
        self.canvas.create_line(x - 26, y + 8, x + 25, y + 8, fill='#FFFFFF', width=2, tag='mode')
        self.canvas.create_line(x - 25, y - 8, x - 25, y + 8, fill='#FFFFFF', width=2, tag='mode')
        self.canvas.create_line(x + 24, y - 8, x + 24, y + 8, fill='#FFFFFF', width=2, tag='mode')
        self.canvas.create_text(x, y, text='Change', anchor='center', font=('Minecraftia', 7), fill='#FFFFFF', tag='mode')
        self.canvas.create_line(350, 170, 365, 170, fill='#FFFFFF', width=3, tag='mode')
        self.canvas.create_line(350, 170, 365, 170, fill='#FFFFFF', width=3, tag='rewrite')
        self.canvas.create_text(125, 170, text='Auto Requeue (bw only, beta)', anchor='center', font=('Minecraftia', 8), fill='#FFFFFF', tag='mode')
    self.canvas.create_text(125, 80, text='Opacity : {}%'.format(int(self.config.get('opacity'))), anchor='center', font=('Minecraftia', 8), fill='#FFFFFF', tag='rewrite')
    self.canvas.create_oval(295 + positions["opacity"], 75, 305 + positions["opacity"], 85, fill='#FFFFFF', tag='rewrite')
    self.canvas.create_text(125, 100, text='FPS : {}'.format(int(self.config.get('fps'))), anchor='center', font=('Minecraftia', 8), fill='#FFFFFF', tag='rewrite')
    self.canvas.create_oval(295 + positions["fps"], 95, 305 + positions["fps"], 105, fill='#FFFFFF', tag='rewrite')
    self.canvas.create_text(125, 120, text='Refresh Log : {}ms'.format(int(self.config.get('refresh'))), anchor='center', font=('Minecraftia', 8), fill='#FFFFFF', tag='rewrite')
    self.canvas.create_oval(295 + positions["refresh"], 115, 305 + positions["refresh"], 125, fill='#FFFFFF', tag='rewrite')
    if self.config.get('background'):
        self.canvas.create_oval(360, 145, 370, 155, fill='#FFFFFF', tag='rewrite')
        self.canvas.create_text(390, 150, text='On', anchor='center', font=('Minecraftia', 8), fill='#00FF1A', tag='rewrite')
    else:
        self.canvas.create_oval(345, 145, 355, 155, fill='#FFFFFF', tag='rewrite')
        self.canvas.create_text(390, 150, text='Off', anchor='center', font=('Minecraftia', 8), fill='#FF0000', tag='rewrite')
    if self.config.get('background'):
        self.canvas.create_oval(360, 145, 370, 155, fill='#FFFFFF', tag='rewrite')
        self.canvas.create_text(390, 150, text='On', anchor='center', font=('Minecraftia', 8), fill='#00FF1A', tag='rewrite')
    else:
        self.canvas.create_oval(345, 145, 355, 155, fill='#FFFFFF', tag='rewrite')
        self.canvas.create_text(390, 150, text='Off', anchor='center', font=('Minecraftia', 8), fill='#FF0000', tag='rewrite')
    if False: #self.config.get('autorequeue'):
        self.canvas.create_oval(360, 165, 370, 175, fill='#FFFFFF', tag='rewrite')
        self.canvas.create_text(390, 170, text='On', anchor='center', font=('Minecraftia', 8), fill='#00FF1A', tag='rewrite')
        color = colors["enabled"]
    else:
        self.canvas.create_oval(345, 165, 355, 175, fill='#FFFFFF', tag='rewrite')
        self.canvas.create_text(390, 170, text='Off', anchor='center', font=('Minecraftia', 8), fill='#FF0000', tag='rewrite')
        color = colors["not_available"]
    self.canvas.create_line(299, 190, 451, 190, fill=color, width=2, tag='rewrite')
    self.canvas.create_text(125, 190, text='Requeue fkdr : {:.1f}+'.format(self.config.get('fkdr') / 10), anchor='center', font=('Minecraftia', 8), fill=color, tag='rewrite')
    self.canvas.create_oval(300 + positions["fkdr"] - 5, 185, 300 + positions["fkdr"] + 5, 195, fill=color, tag='rewrite')
    x = 100
    y = 210
    if True: #not self.config.get('autorequeue'):
        color = colors["not_available"]
    elif self.config.get('queuemode') == 'solo':
        color = colors["enabled"]
    else:
        color = colors["disabled"]
    self.canvas.create_line(x - 16, y - 8, x + 15, y - 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x - 16, y + 8, x + 15, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x - 15, y - 8, x - 15, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x + 14, y - 8, x + 14, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_text(x, y, text='Solo', anchor='center', font=('Minecraftia', 7), fill=color, tag='rewrite')
    x = 175
    y = 210
    if True: # self.config.get('autorequeue'):
        color = colors["not_available"]
    elif self.config.get('queuemode') == 'doubles':
        color = colors["enabled"]
    else:
        color = colors["disabled"]
    self.canvas.create_line(x - 26, y - 8, x + 25, y - 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x - 26, y + 8, x + 25, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x - 25, y - 8, x - 25, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x + 24, y - 8, x + 24, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_text(x, y, text='Doubles', anchor='center', font=('Minecraftia', 7), fill=color, tag='rewrite')
    x = 250
    y = 210
    if True: # self.config.get('autorequeue'):
        color = colors["not_available"]
    elif self.config.get('queuemode') == '3v3v3v3':
        color = colors["enabled"]
    else:
        color = colors["disabled"]
    self.canvas.create_line(x - 29, y - 8, x + 28, y - 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x - 29, y + 8, x + 28, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x - 28, y - 8, x - 28, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x + 27, y - 8, x + 27, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_text(x, y, text='3v3v3v3', anchor='center', font=('Minecraftia', 7), fill=color, tag='rewrite')
    x = 325
    y = 210
    if True: # self.config.get('autorequeue'):
        color = colors["not_available"]
    elif self.config.get('queuemode') == '4v4v4v4':
        color = colors["enabled"]
    else:
        color = colors["disabled"]
    self.canvas.create_line(x - 29, y - 8, x + 28, y - 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x - 29, y + 8, x + 28, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x - 28, y - 8, x - 28, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x + 27, y - 8, x + 27, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_text(x, y, text='4v4v4v4', anchor='center', font=('Minecraftia', 7), fill=color, tag='rewrite')
    x = 400
    y = 210
    if True: # self.config.get('autorequeue'):
        color = colors["not_available"]
    elif self.config.get('queuemode') == '4v4':
        color = colors["enabled"]
    else:
        color = colors["disabled"]
    self.canvas.create_line(x - 16, y - 8, x + 15, y - 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x - 16, y + 8, x + 15, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x - 15, y - 8, x - 15, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x + 14, y - 8, x + 14, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_text(x, y, text='4v4', anchor='center', font=('Minecraftia', 7), fill=color, tag='rewrite')
    x = 275
    y = 245
    color = '#FFFFFF'
    self.canvas.create_line(x - 16, y - 8, x + 15, y - 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x - 16, y + 8, x + 15, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x - 15, y - 8, x - 15, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x + 14, y - 8, x + 14, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_text(x, y, text='bw', anchor='center', font=('Minecraftia', 7), fill=color, tag='rewrite')
    x = 325
    y = 245
    color = colors["not_available"]
    self.canvas.create_line(x - 16, y - 8, x + 15, y - 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x - 16, y + 8, x + 15, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x - 15, y - 8, x - 15, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x + 14, y - 8, x + 14, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_text(x, y, text='sw', anchor='center', font=('Minecraftia', 7), fill=color, tag='rewrite')
    x = 375
    y = 245
    color = colors["not_available"]
    self.canvas.create_line(x - 18, y - 8, x + 17, y - 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x - 18, y + 8, x + 17, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x - 17, y - 8, x - 17, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_line(x + 16, y - 8, x + 16, y + 8, fill=color, width=2, tag='rewrite')
    self.canvas.create_text(x, y, text='uhc', anchor='center', font=('Minecraftia', 7), fill=color, tag='rewrite')

def event(self, mode, event):
    if mode == 1:
        if 300 <= event.x <= 450:
            if 75 <= event.y <= 85:
                self.status.temp.moving = "opacity"
            if 95 <= event.y <= 105:
                self.status.temp.moving = "fps"
            if 115 <= event.y <= 125:
                self.status.temp.moving = "refresh"
            if 185 <= event.y <= 195:
                self.status.temp.moving = "fkdr"
        if 300 <= event.x <= 350 and 257 <= event.y <= 273:
            utils.logfile(self.config)
        if 350 <= event.x <= 365:
            if 145 <= event.y <= 155:
                if not self.config.get('background'):
                    self.config.set('background', True)
                    self.tk.wm_attributes('-transparentcolor', 'black')
                else:
                    self.config.set('background', False)
                    self.tk.wm_attributes('-transparentcolor', '')
            if 165 <= event.y <= 175:
                if not self.config.get('autorequeue'):
                    self.config.set('autorequeue', True)
                else:
                    self.config.set('autorequeue', False)
        if 202 <= event.y <= 218:
            if 84 <= event.x <= 116:
                self.config.set('queuemode', 'solo')
            if 149 <= event.x <= 201:
                self.config.set('queuemode', 'doubles')
            if 221 <= event.x <= 279:
                self.config.set('queuemode', '3v3v3v3')
            if 296 <= event.x <= 354:
                self.config.set('queuemode', '4v4v4v4')
            if 384 <= event.x <= 416:
                self.config.set('queuemode', '4v4')
        if 277 <= event.y <= 293:
            if 255 <= event.x <= 305:
                question = utils.askwindow('Hypixel', 'https://api.hypixel.net/key')
                self.config.set('hypixel_key', question.result)
                self.config.save_config()
                self.status.set_log('Saved')
            if 335 <= event.x <= 405:
                question = AntiSniperKeyInput()
                self.config.set('antisniper_key', question.result)
                self.config.save_config()
                self.status.set_log('Saved')
    if mode == 0:
        if self.status.temp.moving == 'opacity':
            if event.x < 300:
                self.config.set('opacity', 10)
                positions["opacity"] = 0
            if 300 <= event.x <= 450:
                value = event.x - 300
                self.config.set('opacity', utils.calcvalue(value, 150, 10, 100))
                self.tk.attributes('-alpha', self.config.get('opacity') / 100)
                positions["opacity"] = value
            if event.x > 450:
                self.config.set('opacity', 100)
                positions["opacity"] = 150
        if self.status.temp.moving == 'fps':
            if event.x < 300:
                self.config.set('fps', 1)
                positions["fps"] = 0
            if 300 <= event.x <= 450:
                value = event.x - 300
                self.config.set('fps', utils.calcvalue(value, 150, 1, 100))
                self.fps_controller = FPSController(target_fps=self.config.get('fps'))
                positions["fps"] = value
            if event.x > 450:
                self.config.set('fps', 100)
                positions["fps"] = 150
        if self.status.temp.moving == 'refresh':
            if event.x < 450:
                self.config.set('refresh', 1000)
                positions["refresh"] = 0
            if 300 <= event.x <= 450:
                value = event.x - 300
                self.config.set('refresh', utils.calcvalue(value, 150, 1000, 10))
                positions["refresh"] = value
            if event.x > 450:
                self.config.set('refresh', 10)
                positions["refresh"] = 150
        if self.status.temp.moving == 'fkdr':
            if event.x < 450:
                self.config.set('fkdr', 10)
                positions["fkdr"] = 0
            if 300 <= event.x <= 450:
                value = event.x - 300
                self.config.set('fkdr', utils.calcvalue(value, 150, 10, 200))
                positions["fkdr"] = value
            if event.x > 450:
                self.config.set('fkdr', 200)
                positions["fkdr"] = 150
    if mode == 2:
        if self.status.temp.moving == 'opacity':
            if event.x < 300:
                self.config.set('opacity', 10)
                positions["opacity"] = 0
            if 300 <= event.x <= 450:
                value = event.x - 300
                self.config.set('opacity', utils.calcvalue(value, 150, 10, 100))
                self.tk.attributes('-alpha', self.config.get('opacity') / 100)
                positions["opacity"] = value
            if event.x > 450:
                self.config.set('opacity', 100)
                positions["opacity"] = 150
        if self.status.temp.moving == 'fps':
            if event.x < 300:
                self.config.set('fps', 1)
                positions["fps"] = 0
            if 300 <= event.x <= 450:
                value = event.x - 300
                self.config.set('fps', utils.calcvalue(value, 150, 1, 100))
                self.fps_controller = FPSController(target_fps=self.config.get('fps'))
                positions["fps"] = value
            if event.x > 450:
                self.config.set('fps', 100)
                positions["fps"] = 150
        if self.status.temp.moving == 'refresh':
            if event.x < 300:
                self.config.set('refresh', 1000)
                positions["refresh"] = 0
            if 300 <= event.x <= 450:
                value = event.x - 300
                self.config.set('refresh', utils.calcvalue(value, 150, 1000, 10))
                positions["refresh"] = value
            if event.x > 450:
                self.config.set('refresh', 10)
                positions["refresh"] = 150
        if self.status.temp.moving == 'fkdr':
            if event.x < 300:
                self.config.set('fkdr', 10)
                positions["fkdr"] = 0
            if 300 <= event.x <= 450:
                value = event.x - 300
                self.config.set('fkdr', utils.calcvalue(value, 150, 10, 200))
                positions["fkdr"] = value
            if event.x > 450:
                self.config.set('fkdr', 200)
                positions["fkdr"] = 150
        if self.status.temp.moving:
            self.status.temp.moving = None
        self.config.save_config()

    

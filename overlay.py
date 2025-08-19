import json
import sys
import traceback
import time
import threading
import os
os.environ['PYGAME_HIDE_SUPPORT_PROMPT'] = "hide"
from lib.fps import FPSController
from lib.config import Config
from lib.player import player
from lib.startup import startup
from lib.status import Status
from lib import utils, data
from os import getenv
from tkinter import Canvas, Tk, Entry, Label, Button, filedialog, messagebox, Toplevel
from modules import stats, settings, about, music, compact

version = 'v0.1.3'
button_offset = 0
utils.load_font()

class Overlay:
    def __init__(self):
        self.config = Config()
        self.config.load_config()
        self.status = Status()
        self.tk = Tk()
        self.tk.attributes('-alpha', 0.0)
        self.tk.update()
        self.tk.configure(background='#000000', highlightbackground='#000000')
        self.canvas = Canvas(self.tk, width=1000, height=500)
        self.canvas.configure(background='#000000')
        self.canvas.pack()
        self.fps_controller = FPSController(target_fps=self.config.get('fps'))
        self.player = player(self.config)
        utils.make_borderless(self.tk, self.config)
        self.font = ('Minecraftia', 8)
        self.canvas.bind('<B1-Motion>', lambda event: self.event(0, event))
        self.canvas.bind('<Button-1>', lambda event: self.event(1, event))
        self.canvas.bind('<ButtonRelease-1>', lambda event: self.event(2, event))
        self.canvas.create_line(0, 25, 1000, 25, fill='#FFFFFF')
        self.canvas.create_text(10, 13, text=f'Void Overlay {version}', anchor='w', font=self.font, fill='#FFFFFF')
        self.canvas.create_text(155 + button_offset, 12, text='⚙', anchor='center', font=('Minecraftia', 7), fill='#FFFFFF')
        self.canvas.create_text(170 + button_offset, 12, text='ℹ', anchor='center', font=('Minecraftia', 7), fill='#FFFFFF')
        self.canvas.create_text(185 + button_offset, 12, text='♪', anchor='center', font=('Minecraftia', 7), fill='#FFFFFF')
        threading.Thread(target=startup, args=[self], daemon=True).start()
        self.mcid = None
        self.players = []
        self.player_data = {}
        self.limits = {'Player': 14, 'TAG': 9, 'WS': 3, 'FKDR': 5, 'WLR': 5, 'Finals': 5, 'Wins': 5}
        self.places = {'Player': 135, 'TAG': 235, 'WS': 290, 'FKDR': 330, 'WLR': 375, 'Finals': 420, 'Wins': 465}

    def event(self, mode, event):
        music.default_event(self, mode, event)
        if self.status.mode == 'settings':
            settings.event(self, mode, event)
        if self.status.mode == 'about':
            about.event(self, mode, event)
        if self.status.mode == 'music':
            music.event(self, mode, event)
        if mode == 0:
            if self.status.moving:
                x = self.tk.winfo_pointerx() - self.status.temp._offsetx
                y = self.tk.winfo_pointery() - self.status.temp._offsety
                self.tk.geometry('+{x}+{y}'.format(x=x, y=y))
        if mode == 1:
            if 6 <= event.y <= 19:
                if 155 + button_offset - 5 <= event.x <= 155 + button_offset + 5:
                    self.status.set_mode('settings' if self.status.mode != 'settings' else None)
                    self.config.save_config()
                if 170 + button_offset - 5 <= event.x <= 170 + button_offset + 5:
                    self.status.set_mode('about' if self.status.mode != 'about' else None)
                if 185 + button_offset - 5 <= event.x <= 185 + button_offset + 5:
                    self.status.set_mode('music' if self.status.mode != 'music' else None)
            if event.y < 25:
                self.status.moving = True
                self.status.temp._offsetx = event.x
                self.status.temp._offsety = event.y
            if 500 + self.status.add_width - 20 <= event.x <= 500 + self.status.add_width - 10 and 8 <= event.y <= 16:
                self.config.save_config()
                self.tk.quit()
                self.tk.destroy()
        if mode == 2:
            if self.status.moving:
                self.config.set('geometry', '+' + '+'.join(self.tk.geometry().split('+')[1:]))
                self.config.save_config()
                self.status.moving = False

    def loop(self):
        self.canvas.delete('rewrite')
        rewrite = False
        if self.status.updated:
            self.canvas.delete('mode')
            self.status.updated = False
            rewrite = True
        if not self.status.mode and self.status.hidden:
            self.status.add_width = 0
            self.tk.geometry('500x50')
            compact.render(self)
            self.canvas.create_text(488, 13, text='×', anchor='center', font=('Minecraftia', 9), fill='#FFFFFF', tag='rewrite')
            self.tk.after(self.fps_controller.pause(), self.loop)
            return
        if self.status.mode == 'settings':
            self.status.add_height = 85
            self.status.add_width = 0
            settings.render(self, rewrite)
            height, width = (262 + self.status.add_height, 500 + self.status.add_width)
            self.tk.geometry('{}x{}'.format(width, height))
        elif self.status.mode == 'about':
            self.status.add_height = 65
            self.status.add_width = 0
            about.render(self, rewrite)
            height, width = (262 + self.status.add_height, 500 + self.status.add_width)
            self.tk.geometry('{}x{}'.format(width, height))
        elif self.status.mode == 'music':
            self.status.add_height = 65
            self.status.add_width = 0
            music.render(self, rewrite)
            height, width = (262 + self.status.add_height, 500 + self.status.add_width)
            self.tk.geometry('{}x{}'.format(width, height))
        else:
            self.status.add_height = 0
            self.status.add_width = 0
            stats.render(self, rewrite)
        height, width = (262 + self.status.add_height, 500 + self.status.add_width)
        self.canvas.create_line(0, height - 25, 1000, height - 25, fill='#FFFFFF', tag='rewrite')
        self.tk.geometry('{}x{}'.format(width, height))
        self.canvas.create_text(width - 8, height - 15, text='{} FPS'.format(self.fps_controller.fps), anchor='e', font=self.font, fill='#FFFFFF', tag='rewrite')
        self.canvas.create_text(width - 12, 13, text='×', anchor='center', font=('Minecraftia', 9), fill='#FFFFFF', tag='rewrite')
        self.canvas.create_oval(width - 6 - 2, height - 6 - 2, width - 6 + 3, height - 6 + 3, fill='#1fff1f' if self.status.injected else "#ff1f1f" , tag='rewrite')
        self.canvas.create_text(8, height - 15, text='Log : {}'.format(self.status.log), anchor='w', font=self.font, fill='#FFFFFF', tag='rewrite')
        if not self.status.mode == 'music':
            music.default_render(self, height, width)
        self.tk.after(self.fps_controller.pause(), self.loop)

m = Overlay()
m.loop()
m.tk.mainloop()
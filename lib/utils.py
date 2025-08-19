from tkinter import Tk, Button, Entry, Label, messagebox, filedialog, font
import sys
import pyglet
import json
import socket
from os import path, getenv, mkdir
from requests import get
from threading import Thread
from time import sleep, time
from . import data
import pygame
import pygame._sdl2.audio as sdl2_audio
from ctypes import windll
from typing import Tuple

def get_devices(capture_devices: bool=False) -> Tuple[str, ...]:
    init_by_me = not pygame.mixer.get_init()
    if init_by_me:
        pygame.mixer.init()
    devices = tuple(sdl2_audio.get_audio_device_names(capture_devices))
    if init_by_me:
        pygame.mixer.quit()
    return devices

def util_checker(config):
    before = ""
    game_mode = ""
    while True:
        sleep(3)
        try:
            result = send("score").lstrip("score: ").rstrip("\x00").strip().split("\n")
        except:
            result = None
        if not result:
            continue
        #print(result)
        if len(result[-1].split(" ")[0].split("/")) == 3 and len(result[-1].split(" ")) == 5:
            server_id = result[-1].split(" ")[2]
            game_type = result[0].split(" (")[0].lower()
            if result[3].startswith("Mode: "):
                game_mode = result[3].lstrip("Mode: ").split(" : ")[0].strip()
                log = "You are queued in {} game ({}, {})".format(game_type, game_mode if game_mode else "Unknown", server_id)
                if before != log:
                    before = log
                    config.log = log
            elif result[-3].startswith("Level"):
                log = "You are at {} lobby ({})".format(game_type, server_id)
                game_mode = None
                if before != log:
                    before = log
                    config.log = log
            else:
                log = "You are in {} game ({}, {})".format(game_type, game_mode if game_mode else "Unknown", server_id)
                if before != log:
                    before = log
                    config.log = log

def logfile(config):
    typ = [('Log File', '*.log')]
    appdata = getenv('appdata')
    dir = f'{appdata}\\.minecraft'
    file = filedialog.askopenfilename(filetypes=typ, initialdir=dir)
    if not file:
        messagebox.showwarning('Warning', 'Logfile was not changed.')
        return
    config.set('log_file', file)
    config.save_config()
    messagebox.showinfo('Info', 'Successfully changed')

def calcvalue(value, value_range, min, max):
    value = value / value_range * 100
    all = (max - min) / 100
    return int(value * all + min)

def calcpos(value, value_range, min, max):
    siki = value / value_range * 100 * (max / 100 - min / 100)
    return (value - min) * value_range / 100 / (max / 100 - min / 100)

def calc(value):
    if value >= 3600:
        hours, value = divmod(int(value), 3600)
        minutes, value = divmod(value, 60)
        minutes = str(minutes).zfill(2)
        seconds = str(value).zfill(2)
        return f'{hours}:{minutes}:{seconds}'
    if value >= 60:
        minutes, value = divmod(int(value), 60)
        seconds = str(value).zfill(2)
        return f'{minutes}:{seconds}'
    seconds = str(int(value)).zfill(2)
    return f'0:{seconds}'

def load_font():
    def resource_path(relative_path):
        try:
            base_path = sys._MEIPASS
        except:
            base_path = path.abspath('.')
        return path.join(base_path, relative_path)
    pyglet.font.add_file(resource_path('Minecraftia.ttf'))

def set_appwindow(root):
    GWL_EXSTYLE = -20
    WS_EX_APPWINDOW = 262144
    WS_EX_TOOLWINDOW = 128
    hwnd = windll.user32.GetParent(root.winfo_id())
    style = windll.user32.GetWindowLongW(hwnd, GWL_EXSTYLE)
    style = style & ~WS_EX_TOOLWINDOW
    style = style | WS_EX_APPWINDOW
    res = windll.user32.SetWindowLongW(hwnd, GWL_EXSTYLE, style)
    root.withdraw()
    root.after(10, root.deiconify)

def make_borderless(tk, config):
    tk.option_add('*font', font.Font(font='TkDefaultFont'))
    tk.overrideredirect(True)
    tk.title('Void Overlay')
    tk.attributes('-topmost', 1)
    tk.attributes('-alpha', config.get('opacity') / 100)
    tk.geometry('500x300{}'.format(config.get('geometry')))
    tk.resizable(width=False, height=False)
    tk.wm_attributes('-disabled', False)
    tk.withdraw()
    if config.get('background'):
        tk.wm_attributes('-transparentcolor', '#000000')
    set_appwindow(tk)
    tk.update()
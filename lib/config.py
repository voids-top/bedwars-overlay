from modules.popup import AntiSniperKeyInput
from os import getenv, mkdir
import json
from tkinter import messagebox

default = {'hypixel_key': None, 'antisniper_key': None, "fkdr": 100, 'refresh': 100, 'mode': 'bedwars', 'fps': 30, 'opacity': 50, 'background': False, 'autochat': False, 'geometry': '', 'volume': 10, 'position': 0, 'log_file': 'custom'}

validator = {
    "mode": lambda a: a in ["bedwars", "skywars"],
    "opacity": lambda a: 0 <= a <= 100,
    "fps": lambda a: a > 0,
    "background": lambda a: isinstance(a, bool),
    "autorequeue": lambda a: isinstance(a, bool),
    "fkdr": lambda a: a > 0,
    "queuemode": lambda a: a in ["solo", "duo", "squad"],
    "geometry": lambda a: isinstance(a, str),
    "volume": lambda a: 0 <= a <= 100,
    "position": lambda a: isinstance(a, int) or isinstance(a, float),
    "log_file": lambda a: isinstance(a, str),
}

"""
try:
    data = get('https://api.hypixel.net/key?key={}'.format(self.config['hypixel_key'])).json()
    if not data['success']:
        messagebox.showerror('Error', data['cause'])
        self.config['hypixel_key'] = utils.askwindow('Hypixel', 'https://api.hypixel.net/key').result
        self.save_config()
except:
    pass
"""

class Config:
    def __init__(self):
        self.config = {}
    def get(self, arg):
        return self.config[arg]
    def set(self, arg, arg2):
        self.config[arg] = arg2
    def save_config(self):
        appdata = getenv('appdata')
        try:
            mkdir(f'{appdata}\\voidoverlay')
        except:
            pass
        try:
            mkdir(f'{appdata}\\voidoverlay\\music')
        except:
            pass
        open(f'{appdata}\\voidoverlay\\settings.json', 'w', encoding='utf_8').write(json.dumps(self.config))
    def load_config(self):
        try:
            mkdir(f'{appdata}\\voidoverlay')
        except:
            pass
        try:
            mkdir(f'{appdata}\\voidoverlay\\music')
        except:
            pass
        appdata = getenv('appdata')
        try:
            self.config = json.loads(open(f'{appdata}\\voidoverlay\\settings.json', 'r', encoding='utf_8').read())
            for obj in default:
                if obj not in self.config:
                    self.config[obj] = default[obj]
                if not validator.get(obj):
                    continue
                if not validator[obj](self.config[obj]):
                    print(f"validation failure of {obj}, set to default")
                    self.config[obj] = default[obj]
        except:
            self.config = default
            self.config['antisniper_key'] = "60ff143a-e0fa-4eaf-8382-e9eda046325c" #AntiSniperKeyInput().result
            self.save_config()
            return
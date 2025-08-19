from tkinter import Tk, Button, Entry, Label, messagebox
import sys, pyglet, json
from os import path, getenv, mkdir
from requests import get
from threading import Thread
from time import sleep, time

appdata = getenv("appdata")
userprofile = getenv("userprofile")
file_path = f"{appdata}\\voidoverlay\\settings.json"

try:
    mkdir(f"{appdata}\\voidoverlay")
except:
    pass
class askwindow:
    def __init__(self, name, url):
        self.tk = Tk()
        self.tk.attributes('-topmost', 1)
        self.label = Label(self.tk, text=f'Enter a {name} API Key')
        self.label.pack()
        self.url = url
        if name == "Antisniper":
            link = Label(self.tk, text="You don't know about antisniper?\nClick here to Discord Server and Get API Key", fg="blue")
            link.pack()
            link.bind("<Button-1>", lambda e: webbrowser.open("https://antisniper.net", new=2, autoraise=True))
        self.entry = Entry(self.tk, width=50)
        self.entry.focus_set()
        self.entry.pack()
        self.button = Button(self.tk, text='OK', command=self.entried)
        self.button.pack()
        self.tk.lift()
        self.result = None
        self.tk.protocol("WM_DELETE_WINDOW", self.on_closing)
        self.tk.mainloop()
        if self.result:
            return
        else:
            raise Exception("Invalid api key")
    def on_closing(self):
        if messagebox.askokcancel("Quit", "Do you want to quit?"):
            self.tk.quit()
            self.tk.destroy()
    def entried(self):
        self.result = self.entry.get()
        data = get(f"{self.url}?key={self.result}").json()
        if data["success"]:
            self.tk.quit()
            self.tk.destroy()
        else:
            messagebox.showerror("Error", data["cause"])

def key_checker(url, key):
    return get(f"{url}?key={key}").json()

def save_settings(data):
    open(file_path, "w", encoding="utf_8").write(json.dumps(data))

def load_settings():
    try:
        temp = json.loads(open(file_path, "r", encoding="utf_8").read())
        print(temp)
    except:
        temp = {"hypixel_key": None, "antisniper_key": None, "refresh": 100, "fps": 30, "opacity": 40, "geometry": "","log_file": "custom"}
        temp["hypixel_key"] = askwindow("Hypixel", "https://api.hypixel.net/key").result
        temp["antisniper_key"] = askwindow("Antisniper", "https://api.antisniper.net/stats").result
        save_settings(temp)
        return temp
    try:
        data = key_checker("https://api.hypixel.net/key", temp["hypixel_key"])
        if not data["success"]:
            #messagebox.showerror("Error", data["cause"])
            #temp["hypixel_key"] = askwindow("Hypixel", "https://api.hypixel.net/key").result
            print(temp)
            save_settings(temp)
    except:
        pass
    try:
        data = key_checker("https://api.antisniper.net/stats", temp["antisniper_key"])
        if not data["success"]:
            #messagebox.showerror("Error", data["cause"])
            #temp["antisniper_key"] = askwindow("Antisniper", "https://api.antisniper.net/stats").result
            print(temp)
            save_settings(temp)
    except:
        pass
    return temp

import socket

HOST = "127.0.0.1"
PORT = 46001

def send(cmd):
    try:
        with socket.create_connection((HOST, PORT), timeout=1) as sock:
            sock.recv(6)
            sock.sendall((cmd + "\n").encode("utf-8"))
            try:
                return sock.recv(1000).decode("utf-8").strip()
            except:
                pass
            try:
                sock.close()
            except:
                pass
    except:
        pass

config = load_settings()
clients = {
    'Vanilla/Forge Client': f'{appdata}\\.minecraft\\logs\\latest.log',
    'Lunar Client': f'{userprofile}\\.lunarclient\\offline\\multiver\\logs\\latest.log',
    'Lunar Client ': f'{userprofile}\\.lunarclient\\logs\\launcher\\renderer.log',
    'Lunar Client (1.8-1)': f'{userprofile}\\.lunarclient\\offline\\1.8\\logs\\latest.log',
    'Lunar Client (1.7-1)': f'{userprofile}\\.lunarclient\\offline\\1.7\\logs\\latest.log',
    'Lunar Client (1.12-1)': f'{userprofile}\\.lunarclient\\offline\\1.12\\logs\\latest.log',
    'Lunar Client (1.15-1)': f'{userprofile}\\.lunarclient\\offline\\1.15\\logs\\latest.log',
    'Lunar Client (1.16-1)': f'{userprofile}\\.lunarclient\\offline\\1.16\\logs\\latest.log',
    'Lunar Client (1.8-2)': f'{userprofile}\\.lunarclient\\offline\\files\\1.8\\logs\\latest.log',
    'Lunar Client (1.7-2)': f'{userprofile}\\.lunarclient\\offline\\files\\1.7\\logs\\latest.log',
    'Lunar Client (1.12-2)': f'{userprofile}\\.lunarclient\\offline\\files\\1.12\\logs\\latest.log',
    'Lunar Client (1.15-2)': f'{userprofile}\\.lunarclient\\offline\\files\\1.15\\logs\\latest.log',
    'Lunar Client (1.16-2)': f'{userprofile}\\.lunarclient\\offline\\files\\1.16\\logs\\latest.log',
    'Badlion Client': f'{appdata}\\.minecraft\\logs\\blclient\\minecraft\\latest.log',
    'PVP Lounge Client': f'{appdata}\\.pvplounge\\logs\\latest.log',
    'Custom Client': config["log_file"]
}

def client_check():
    while True:
        for client in clients:
            try:
                if time() - path.getmtime(clients[client]) <= 30:
                    for encoding in ["ansi", "utf_8", "cp932"]:
                        try:
                            open(clients[client], "r", encoding=encoding).read()
                            return client, clients[client], encoding
                        except:
                            pass
            except:
                pass
        sleep(0.5)

def load_font():
    def resource_path(relative_path):
        try:
            base_path = sys._MEIPASS
        except:
            base_path = path.abspath(".")

        return path.join(base_path, relative_path)
    pyglet.font.add_file(resource_path('Minecraftia.ttf'))

class stats:
    def __init__(self, hypixel_key, antisniper_key):
        self.hypixel_key = hypixel_key
        self.antisniper_key = antisniper_key
        self.processed = []
        self.players = []
        self.data = {}
        self.log = "Checking Client..."
        self.write = False
        # self.botlist = requests.get("https://api.antisniper.net/botlist?key={}".format(antisniper_key)).json()["botlist"]
    def logger(self, refresh):
        client, logfile, encoding = client_check()
        self.log = f"Connected to {client}"
        processed = []
        while True:
            logs = open(logfile, "r", encoding=encoding).read().split("\n")[-20:]
            for log in logs:
                try:
                    if not log in processed:
                        processed.append(log)
                        log_chat = log.split("[Client thread/INFO]: [CHAT] ")[1]
                        if log_chat.startswith("Can't find a player by the name of '."):
                            player = log_chat.split("Can't find a player by the name of '.")[1].split("'")[0]
                            Thread(target=self.get_player, args=[player], daemon=True).start()
                        if log_chat.startswith("ONLINE: "):
                            self.log = "autowho detected, checking players"
                            self.players.clear()
                            players = log_chat.split("ONLINE: ")[1].split(" [x")[0].split(", ")
                            for player in players:
                                Thread(target=self.get_player, args=[player], daemon=True).start()
                        #if " has quit!" in log_chat:
                        #    player = log_chat.split(" has quit")[0]
                        #    self.players.remove(player)
                        #    self.log = f"{player} has quit"
                        #    self.write = True
                        #if " has joined " in log_chat:
                        #    player = log_chat.split(" has joined ")[0]
                        #    Thread(target=self.get_player, args=[player], daemon=True).start()
                        #    self.log = f"{player} has joined"
                except Exception as e:
                    pass
            sleep(refresh / 1000)
            if len(processed) >= 200:
                del processed[:100]
                self.log = "buffer reset"
    def get_player(self, mcid):
        self.players.append(mcid)
        self.write = True
        while True:
            status, data = self.get_data(mcid, "name")
            self.data[mcid] = data
            self.write = True
            if status == "Winstreak Hidden":
                winstreak_data = get("https://api.antisniper.net/winstreak?key={}&name={}".format(self.antisniper_key, mcid)).json()
                if winstreak_data["success"]:
                    try:
                        self.data[mcid]["winstreak"] = winstreak_data["player"]["data"]["overall_winstreak"]
                        self.write = True
                    except:
                        self.data[mcid]["winstreak"] = 0
                        self.write = True
                return
            elif status == "Nicked":
                self.data[mcid] = {"name": mcid, "tag": "Nicked", "nick": 2, "rank": "-", "fkdr": "-", "wlr": "-","finals": "-", "wins": "-", "winstreak": "-", "star": "-"}
                self.write = True
                nick_data = get("https://api.antisniper.net/denick?key={}&nick={}".format(self.antisniper_key, mcid)).json()
                if not nick_data["player"]:
                    raise Exception("Failed to denick")
                else:
                    status, data = self.get_data(nick_data["player"]["ign"], "name", mcid)
                    self.data[mcid] = data
                    self.write = True
                    if data == "Winstreak Hidden":
                        winstreak_data = get("https://api.antisniper.net/winstreak?key={}&name={}".format(self.antisniper_key,nick_data["player"]["ign"])).json()
                        if winstreak_data["success"]:
                            try:
                                self.data[mcid]["winstreak"] = winstreak_data["player"]["data"]["overall_winstreak"]
                                self.write = True
                            except:
                                self.data[mcid]["winstreak"] = 0
                                self.write = True
                    return
            elif status == "Success":
                return
            else:
                self.log = f"{mcid} check error ({status})"
                return

    def get_rank(self, player):
        if "rank" in player:
            rank = player["rank"]
        elif "newPackageRank" in player:
            if "monthlyPackageRank" in player:
                if not player["monthlyPackageRank"] == "NONE":
                    rank = "MVP++"
                else:
                    rank = player["newPackageRank"]
            else:
                rank = player["newPackageRank"]
        else:
            rank = "DEFAULT"
        return rank

    def get_data(self, mcid, mode="name", nick=None):
        while True:
            #data = get("https://api.hypixel.net/player?key={}&{}={}".format(self.hypixel_key, mode, mcid)).json()
            data = get("https://hypixel.voids.top/v2/player?{}={}".format(mode, mcid)).json()
            if data["success"]:
                if data["player"]:
                    if not "Bedwars" in data["player"]["stats"]:
                        return "Success", {"name": data["player"]["displayname"], "nick": 2,
                                           "rank": rank, "tag": "First Time", "fkdr": "-", "wlr": "-",
                                           "finals": "-", "wins": "-", "winstreak": "-",
                                           "star": 0}
                    try:
                        final_kills = data["player"]["stats"]["Bedwars"]["final_kills_bedwars"]
                    except:
                        final_kills = 0
                    try:
                        final_deaths = data["player"]["stats"]["Bedwars"]["final_deaths_bedwars"]
                    except:
                        final_deaths = 0
                    try:
                        total_wins = data["player"]["stats"]["Bedwars"]["wins_bedwars"]
                    except:
                        total_wins = 0
                    try:
                        total_games = data["player"]["stats"]["Bedwars"]["games_played_bedwars"]
                    except:
                        total_games = 0
                    try:
                        star = data["player"]["achievements"]["bedwars_level"]
                    except:
                        star = 0
                    try:
                        fkdr = "{:.2f}".format(int(final_kills) / int(final_deaths))
                    except:
                        fkdr = "{:.2f}".format(int(final_kills))
                    try:
                        wlr = "{:.2f}".format(int(total_wins) / int(int(total_games)-int(total_wins)))
                    except:
                        wlr = "{:.2f}".format(int(total_wins))
                    rank = self.get_rank(data["player"])
                    if "winstreak" in data["player"]["stats"]["Bedwars"]:
                        winstreak = data["player"]["stats"]["Bedwars"]["winstreak"]
                        if nick:
                            return "Success", {"name":"{} ({})".format(data["player"]["displayname"], nick), "nick":1, "rank":rank, "tag":"denicked", "fkdr":fkdr, "wlr":wlr, "finals":final_kills, "wins":total_wins, "winstreak":winstreak, "star":star}
                        else:
                            return "Success", {"name":data["player"]["displayname"], "fkdr":fkdr, "nick":0, "rank":rank, "tag":"-", "wlr":wlr, "finals":final_kills, "wins":total_wins, "winstreak":winstreak, "star":star}
                        return "Success"
                    else:
                        if nick:
                            return "Winstreak Hidden", {"name":"{} ({})".format(data["player"]["displayname"], nick), "nick":1, "rank":rank, "tag":"denicked", "fkdr":fkdr, "wlr":wlr, "finals":final_kills, "wins":total_wins, "winstreak":"-", "star":star}
                        else:
                            return "Winstreak Hidden", {"name":data["player"]["displayname"], "fkdr":fkdr, "nick":0, "rank":rank, "tag":"ws hidden", "wlr":wlr, "finals":final_kills, "wins":total_wins, "winstreak":"-", "star":star}
                else:
                    return "Nicked", {}
            else:
                if data["cause"] == 'Key throttle':
                    sleep(0.5)
                elif data["cause"] == "You have already looked up this name recently":
                    mode = "uuid"
                    data = get("https://api.mojang.com/users/profiles/minecraft/{}".format(mcid))
                    if data.status_code == 204:
                        return "Nicked", {}
                    else:
                        mcid = data.json()["id"]
                else:
                    return data["cause"], {}

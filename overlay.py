import json, webbrowser, pyglet, sys, traceback
from lib import utils, window, data, fps
from threading import Thread
from time import sleep, time
from os import getenv
from requests import get
from tkinter import Canvas, Tk, Entry, Label, Button, filedialog, messagebox, Toplevel
import time
from ctypes import windll

def set_appwindow(root):
    GWL_EXSTYLE=-20
    WS_EX_APPWINDOW=0x00040000
    WS_EX_TOOLWINDOW=0x00000080
    hwnd = windll.user32.GetParent(root.winfo_id())
    style = windll.user32.GetWindowLongPtrW(hwnd, GWL_EXSTYLE)
    style = style & ~WS_EX_TOOLWINDOW
    style = style | WS_EX_APPWINDOW
    res = windll.user32.SetWindowLongPtrW(hwnd, GWL_EXSTYLE, style)
    # re-assert the new window style
    root.withdraw()
    root.after(10, root.deiconify)

utils.load_font()

version = "v0.1.2"
appdata = getenv("appdata")

config = utils.config

class window_handler:
    def __init__(self):
        self.api = utils.stats(config["hypixel_key"],config["antisniper_key"])
        self.fps_controller = fps.FPSController(fps=config["fps"])
        self.tk = Tk()
        self.tk.overrideredirect(True)
        self.tk.title("Void Overlay")
        self.tk.attributes('-topmost', 1)
        self.font = ("Minecraftia", 8)
        self.x = 0
        self.y = 0
        self.xa = 0
        self.ya = 0
        self.now = None
        self.move = False
        self.moving = None
        self.transparency = int(config["transparency"])/100
        self.tk.configure(background='black')
        self.tk.wm_attributes("-transparentcolor", "")
        self.tk.resizable(width=False, height=False)
        # tk.withdraw()
        self.current = "Started"
        self.tk.geometry("500x330{}".format(config["geometry"]))
        self.canvas = Canvas(self.tk, width=1000, height=330)
        self.canvas.configure(background='black')
        self.canvas.pack()
        self.canvas.bind("<B1-Motion>", self.move_app)
        self.canvas.bind("<Button-1>", self.click_app)
        self.canvas.bind("<ButtonRelease-1>", self.release_app)
        self.tk.bind("<Unmap>", self.onRootIconify)
        self.tk.bind("<Map>", self.onRootDeiconify)
        self.transparency_position = int(config["transparency"]-10)*3*10/9
        self.fps_position = int(config["fps"]-1)*3/0.99
        self.refresh_position = 300-int(config["refresh"]-10)/3.3
        self.canvas.create_line(0, 25, 1000, 25, fill="#FFFFFF")
        self.canvas.create_text(10, 13, text=f"Void Overlay {version}", anchor="w", font=self.font, fill="#FFFFFF")
        self.canvas.create_text(150, 12, text="⚙", anchor="center", font=("Minecraftia", 7), fill="#FFFFFF")
        self.canvas.create_text(165, 12, text="ℹ", anchor="center", font=("Minecraftia", 7), fill="#FFFFFF")
        self.canvas.create_line(0, 305, 1000, 305, fill="#FFFFFF")
        self.fps = 0
        self.plus = 0
        self.frames = 0
        Thread(target=self.api.logger, args=[config["refresh"]], daemon=True).start()
        Thread(target=self.fps_checker, daemon=True).start()
        self.places = {"Player": 140, "TAG": 215, "WS": 270, "FKDR": 320, "WLR": 375, "Finals": 425, "Wins": 475}
        self.limits = {"name": 12, "fkdr": 5, "wlr": 5, "winstreak": 3, "finals": 4, "wins": 4}
        self.replace = {"name": "Player", "fkdr": "FKDR", "winstreak": "WS", "finals": "Finals", "wins": "Wins", "wlr": "WLR"}
        set_appwindow(self.tk)
        self.tk.update()
    def onRootIconify(self, event):
        print("iconify")
        #hwnd = windll.user32.GetParent(self.tk.winfo_id())
        #windll.user32.ShowWindow(hwnd, 6)
    def onRootDeiconify(self, event):
        print("deiconify")
        #hwnd = windll.user32.GetParent(self.tk.winfo_id())
        #windll.user32.ShowWindow(hwnd, 0)
    def move_app(self,event):
        if self.moving == "1":
            if event.x < 100:
                config["transparency"] = 10
                self.transparency_position = 0
            if 100 <= event.x <= 400:
                value = event.x-100
                config["transparency"] = float(value/3/10*9)+10
                self.transparency = int(config["transparency"]) / 100
                self.transparency_position = value
            if event.x > 400:
                config["transparency"] = 100
                self.transparency_position = 300
        if self.moving == "2":
            if event.x < 100:
                config["fps"] = 1
                self.fps_position = 0
            if 100 <= event.x <= 400:
                value = event.x-100
                config["fps"] = int(value/3*0.99)+1
                self.fps_controller = fps.FPSController(fps=config["fps"])
                self.fps_position = value
            if event.x > 400:
                config["fps"] = 100
                self.fps_position = 300
        if self.moving == "3":
            if event.x < 100:
                config["refresh"] = 1000
                self.refresh_position = 0
            if 100 <= event.x <= 400:
                value = event.x-100
                config["refresh"] = 1000-float(value*3.3)+10
                self.refresh_position = value
            if event.x > 400:
                config["refresh"] = 10
                self.refresh_position = 300
        if self.move:
            x = self.tk.winfo_pointerx() - self._offsetx
            y = self.tk.winfo_pointery() - self._offsety
            self.tk.geometry('+{x}+{y}'.format(x=x,y=y))
    def release_app(self,event):
        if self.moving == "1":
            if event.x < 100:
                config["transparency"] = 10
                self.transparency_position = 0
            if 100 <= event.x <= 400:
                value = event.x-100
                config["transparency"] = float(value/3/10*9)+10
                self.transparency = int(config["transparency"]) / 100
                self.transparency_position = value
            if event.x > 400:
                config["transparency"] = 100
                self.transparency_position = 300
        if self.moving == "2":
            if event.x < 100:
                config["fps"] = 1
                self.fps_position = 0
            if 100 <= event.x <= 400:
                value = event.x-100
                config["fps"] = int(value/3*0.99)+1
                self.fps_controller = fps.FPSController(fps=config["fps"])
                self.fps_position = value
            if event.x > 400:
                config["fps"] = 100
                self.fps_position = 300
        if self.moving == "3":
            if event.x < 100:
                config["refresh"] = 1000
                self.refresh_position = 0
            if 100 <= event.x <= 400:
                value = event.x-100
                config["refresh"] = 1000-float(value*3.3)+10
                self.refresh_position = value
            if event.x > 400:
                config["refresh"] = 10
                self.refresh_position = 300
        if self.moving:
            utils.save_settings(config)
        self.moving = None
        if self.move:
            config["geometry"] = self.tk.geometry().split("x330")[1]
            utils.save_settings(config)
        self.move = False
    def logfile(self):
        typ = [('Log File','*.log')] 
        dir = f'{appdata}\.minecraft'
        file = filedialog.askopenfilename(filetypes = typ, initialdir = dir)
        if not file:
            messagebox.showwarning("Warning", "Logfile was not changed.")
            return
        config["log_file"] = file
        utils.save_settings(config)
        messagebox.showinfo("Info", "Successfully changed")
    def click_app(self,event):
        if 145 <= event.x <= 155:
            if 6 <= event.y <= 19:
                if self.now == "settings":
                    self.now = None
                    utils.save_settings(config)
                else:
                    self.now = "settings"
        if 160 <= event.x <= 170:
            if 6 <= event.y <= 19:
                if self.now == "about":
                    self.now = None
                else:
                    self.now = "about"
            return
        if event.y < 25:
            self.move = True
            self._offsetx = event.x
            self._offsety = event.y
        if 500+(6*self.plus)-20 <= event.x <= 500+(6*self.plus)-10:
            if 8 <= event.y <= 16:
                utils.save_settings(config)
                self.tk.quit()
                self.tk.destroy()
        if self.now == "about":
            if 282 <= event.y <= 296:
                if 166 <= event.x <= 400:
                    webbrowser.open("https://discord.gg/r42t2kb3pC", new=2, autoraise=True)
        if self.now == "settings":
            if 100 <= event.x <= 400:
                if 110 <= event.y <= 120:
                    self.moving = "1"
                if 155 <= event.y <= 165:
                    self.moving = "2"
                if 200 <= event.y <= 210:
                    self.moving = "3"
            if 220 <= event.y <= 240:
                if 155 <= event.x <= 345:
                    self.logfile()
            if 275 <= event.y <= 295:
                if 125 <= event.x <= 225:
                    question = utils.askwindow("Hypixel", "https://api.hypixel.net/key")
                    config["hypixel_key"] = question.result
                    utils.save_settings(config)
                    messagebox.showinfo("Info", "Successfully changed")
                if 275 <= event.x <= 375:
                    question = utils.askwindow("Antisniper", "https://api.antisniper.net/stats")
                    config["antisniper_key"] = question.result
                    utils.save_settings(config)
                    messagebox.showinfo("Info", "Successfully changed")
    def fps_checker(self):
        while True:
            sleep(1)
            self.fps = self.frames
            self.frames = 0
    def canvas_handler(self):
        while True:
            if self.now != self.current:
                self.canvas.delete("stats")
                self.canvas.delete("rewrite")
                self.plus = 0
                if self.now == "about":
                    self.current = "about"
                    window.about(self.canvas)
                elif self.now == "settings":
                    self.current = "settings"
                    window.settings(self.canvas, config, self.transparency_position, self.fps_position, self.refresh_position)
                else:
                    self.now = None
                    self.current = None
                    self.canvas.create_line(0, 45, 1000, 45, fill="#FFFFFF", tag="rewrite")
                    self.canvas.create_text(35, 35, text="Star", anchor="center", font=self.font, fill="#FFFFFF",
                                            tag="rewrite")
                    self.canvas.create_text(int(self.places["TAG"] + 20) / 2, 35, text="Player", anchor="center", font=self.font,
                                            fill="#FFFFFF", tag="rewrite")
                    self.canvas.create_text(self.places["TAG"], 35, text="TAG", anchor="center", font=self.font, fill="#FFFFFF",
                                            tag="rewrite")
                    self.canvas.create_text(self.places["WS"], 35, text="WS", anchor="center", font=self.font, fill="#FFFFFF",
                                            tag="rewrite")
                    self.canvas.create_text(self.places["FKDR"], 35, text="FKDR", anchor="center", font=self.font,
                                            fill="#FFFFFF", tag="rewrite")
                    self.canvas.create_text(self.places["WLR"], 35, text="WLR", anchor="center", font=self.font, fill="#FFFFFF",
                                            tag="rewrite")
                    self.canvas.create_text(self.places["Finals"], 35, text="Finals", anchor="center", font=self.font,
                                            fill="#FFFFFF", tag="rewrite")
                    self.canvas.create_text(self.places["Wins"], 35, text="Wins", anchor="center", font=self.font,
                                            fill="#FFFFFF", tag="rewrite")
                    self.api.write = True

            sleep(0.01)
    def loop(self):
        self.frames += 1
        self.tk.attributes('-alpha', self.transparency)
        self.canvas.delete("log")
        self.canvas.create_text(10, 315, text="Log : {}".format(self.api.log), anchor="w", font=self.font, fill="#FFFFFF", tag="log")
        if self.now == "settings":
            self.canvas.delete("rewrite")
            window.settings(self.canvas, config, self.transparency_position, self.fps_position, self.refresh_position)
        if not self.now:
            if self.api.write:
                self.canvas.delete("rewrite")
                self.canvas.create_line(0, 45, 1000, 45, fill="#FFFFFF", tag="rewrite")
                self.canvas.create_text(35, 35, text="Star", anchor="center", font=self.font, fill="#FFFFFF",
                                            tag="rewrite")
                self.canvas.create_text(int(self.places["TAG"] + 20) / 2, 35, text="Player", anchor="center", font=self.font,
                                            fill="#FFFFFF", tag="rewrite")
                self.canvas.create_text(self.places["TAG"], 35, text="TAG", anchor="center", font=self.font, fill="#FFFFFF",
                                            tag="rewrite")
                self.canvas.create_text(self.places["WS"], 35, text="WS", anchor="center", font=self.font, fill="#FFFFFF",
                                            tag="rewrite")
                self.canvas.create_text(self.places["FKDR"], 35, text="FKDR", anchor="center", font=self.font,
                                            fill="#FFFFFF", tag="rewrite")
                self.canvas.create_text(self.places["WLR"], 35, text="WLR", anchor="center", font=self.font, fill="#FFFFFF",
                                            tag="rewrite")
                self.canvas.create_text(self.places["Finals"], 35, text="Finals", anchor="center", font=self.font,
                                            fill="#FFFFFF", tag="rewrite")
                self.canvas.create_text(self.places["Wins"], 35, text="Wins", anchor="center", font=self.font,
                                            fill="#FFFFFF", tag="rewrite")
                self.plus = 0
                self.api.write = False
                self.canvas.delete("stats")
                players = []
                moves = {"name": 0, "fkdr": 0, "wlr": 0, "winstreak": 0, "finals": 0, "wins": 0}
                self.places = {"Player": 140, "TAG": 215, "WS": 270, "FKDR": 320, "WLR": 375, "Finals": 425,"Wins": 475}
                for player in list(set(self.api.players)):
                    try:
                        players.append(self.api.data[player])
                        temp = self.api.data[player]
                        for a in self.limits:
                            if len(str(temp[a])) > self.limits[a]:
                                value = len(str(temp[a])) - self.limits[a]
                                if moves[a] < value:
                                    moves[a] = value
                    except Exception as e:
                        print(e)
                        players.append({"tag":"Loading", "star":"-", "nick":0, "rank":"-", "name":player,"fkdr":"-", "wlr":"-", "finals":"-", "wins":"-", "winstreak":"-"})
                for a in moves:
                    self.plus += moves[a]
                    for key in self.places:
                        if key == self.replace[a]:
                            self.places[key] += moves[a] * 6 / 2
                        else:
                            self.places[key] += moves[a] * 6
                players = sorted(players, key=lambda x:x['fkdr'])
                players = sorted(players, key=lambda x:x['nick'])
                players.reverse()
                try:
                    for n in range(len(players)):
                        try:
                            color = data.color(players[n]["star"])
                            data.star_color(self.canvas, color, n, self.font)
                        except Exception as e:
                            print(e)
                            pass
                        self.canvas.create_text(int(self.places["TAG"]+20)/2, 55+15*n, text=players[n]["name"], anchor="center", font=self.font, fill=data.rankcolor(players[n]["rank"]), tag="stats")
                        self.canvas.create_text(self.places["TAG"], 55+15*n, text=players[n]["tag"], anchor="center", font=self.font, fill="#FFFFFF", tag="stats")
                        self.canvas.create_text(self.places["WS"], 55+15*n, text=players[n]["winstreak"], anchor="center", font=self.font, fill=data.wscolor(players[n]["winstreak"]), tag="stats")
                        self.canvas.create_text(self.places["FKDR"], 55+15*n, text=players[n]["fkdr"], anchor="center", font=self.font, fill=data.fkdrcolor(players[n]["fkdr"]), tag="stats")
                        self.canvas.create_text(self.places["WLR"], 55+15*n, text=players[n]["wlr"], anchor="center", font=self.font, fill=data.wlrcolor(players[n]["wlr"]), tag="stats")
                        self.canvas.create_text(self.places["Finals"], 55+15*n, text=players[n]["finals"], anchor="center", font=self.font, fill=data.finalcolor(players[n]["finals"]), tag="stats")
                        self.canvas.create_text(self.places["Wins"], 55+15*n, text=players[n]["wins"], anchor="center", font=self.font, fill="#FFFFFF", tag="stats")
                except Exception as e:
                    print(e)
                    pass
        self.canvas.create_text((500+6*self.plus)-10, 315, text="{} FPS".format(self.fps), anchor="e", font=self.font, fill="#FFFFFF", tag="log")
        self.canvas.create_text((500+6*self.plus)-12, 11, text="×", anchor="center", font=("Minecraftia", 9), fill="#FFFFFF", tag="log")
        self.tk.geometry("{}x330".format(500+6*self.plus))
        self.tk.after(self.fps_controller.pause(), self.loop)


mainwindow = window_handler()
Thread(target=mainwindow.canvas_handler, daemon=True).start()
mainwindow.loop()
mainwindow.tk.mainloop()

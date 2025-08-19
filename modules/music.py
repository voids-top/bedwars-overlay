import pygame
import subprocess
from os import getenv
from lib import utils

def render(self, rewrite=False):
    if rewrite:
        self.canvas.create_text(250, 45, text='Music', anchor='center', font=('Minecraftia', 15), fill='#FFFFFF', tag='mode')
        self.canvas.create_line(50, 261, 450, 261, fill='#FFFFFF', width=2, tag='mode')
        self.canvas.create_text(475, 60, text='Vol', anchor='center', font=('Minecraftia', 10), fill='#FFFFFF', tag='mode')
        self.canvas.create_text(486, 265, text='➕', anchor='center', font=('Minecraftia', 10), fill='#FFFFFF', tag='mode')
        self.canvas.create_text(495, 285, text='🔄️', anchor='center', font=('Minecraftia', 10), fill='#FFFFFF', tag='mode')
        self.canvas.create_text(220, 275, text='⏮', anchor='center', font=('Minecraftia', 10), fill='#FFFFFF', tag='mode')
        self.canvas.create_text(280, 275, text='⏭', anchor='center', font=('Minecraftia', 10), fill='#FFFFFF', tag='mode')
    if self.player.albumart:
        self.canvas.create_image(250, 120, image=self.player.albumart, tag='rewrite')
    else:
        self.canvas.create_rectangle(200, 70, 300, 170, fill='#646464', tag='rewrite')
        self.canvas.create_text(250, 115, text='♪', font=('Minecraftia', 30), fill='#ACACAC', tag='rewrite')
    if len(self.player.title) >= 40:
        self.canvas.create_text(50, 200, text='Title : {:.40}...'.format(self.player.title), anchor='w', font=('Minecraftia', 10), fill='#FFFFFF', tag='rewrite')
    else:
        self.canvas.create_text(50, 200, text='Title : {}'.format(self.player.title), anchor='w', font=('Minecraftia', 10), fill='#FFFFFF', tag='rewrite')
    if len(self.player.artist) >= 40:
        self.canvas.create_text(50, 220, text='Artist : {:.40}...'.format(self.player.artist), anchor='w', font=('Minecraftia', 10), fill='#FFFFFF', tag='rewrite')
    else:
        self.canvas.create_text(50, 220, text='Artist : {}'.format(self.player.artist), anchor='w', font=('Minecraftia', 10), fill='#FFFFFF', tag='rewrite')
    if len(self.player.artist) >= 40:
        self.canvas.create_text(50, 240, text='Album : {:.40}...'.format(self.player.album), anchor='w', font=('Minecraftia', 10), fill='#FFFFFF', tag='rewrite')
    else:
        self.canvas.create_text(50, 240, text='Album : {}'.format(self.player.album), anchor='w', font=('Minecraftia', 10), fill='#FFFFFF', tag='rewrite')
    self.canvas.create_text(450, 275, text='{}/{}'.format(utils.calc(self.player.get_pos()), utils.calc(self.player.duration)), anchor='e', font=('Minecraftia', 10), fill='#FFFFFF', tag='rewrite')
    try:
        position = utils.calcvalue(self.player.get_pos(), self.player.duration, 0, 400)
    except:
        position = 0
    self.canvas.create_oval(position + 50 - 5, 255, position + 50 + 5, 265, fill='#FFFFFF', tag='rewrite')
    self.canvas.create_text(50, 275, text='{}/{}'.format(self.player.position + 1, len(self.player.queue)), anchor='w', font=('Minecraftia', 10), fill='#FFFFFF', tag='rewrite')
    self.canvas.create_text(475, 200, text='{}%'.format(int(pygame.mixer.music.get_volume() * 100)), anchor='center', font=('Minecraftia', 10), fill='#FFFFFF', tag='rewrite')
    position = utils.calcvalue(int(pygame.mixer.music.get_volume() * 100), 100, 100, 0)
    self.canvas.create_oval(470, 80 + position - 5, 480, 80 + position + 5, fill='#FFFFFF', tag='rewrite')
    if self.player.paused:
        self.canvas.create_text(250, 275, text='▶', anchor='center', font=('Minecraftia', 10), fill='#FFFFFF', tag='rewrite')
    else:
        self.canvas.create_text(250, 275, text='⏸', anchor='center', font=('Minecraftia', 10), fill='#FFFFFF', tag='rewrite')
    self.canvas.create_line(475, 80, 475, 180, fill='#FFFFFF', width=1, tag='rewrite')

def event(self, mode, event):
    if mode == 0:
        if self.status.temp.moving == 'music-seek':
            if 50 > event.x:
                self.player.seek(0)
            if 50 <= event.x <= 450:
                self.player.seek(utils.calcvalue(event.x - 50, 400, 0, self.player.duration))
            if event.x > 450:
                self.player.seek(self.player.duration)
        if self.status.temp.moving == 'music-volume':
            if 80 > event.y:
                pygame.mixer.music.set_volume(1)
            if 80 <= event.y <= 180:
                pygame.mixer.music.set_volume(utils.calcvalue(event.y - 80, 100, 100, 0) / 100)
            if event.y > 180:
                pygame.mixer.music.set_volume(0)
    if mode == 1:
        if 475 <= event.x <= 495 and 260 <= event.y <= 275:
            subprocess.Popen('start {}/voidoverlay/music'.format(getenv('appdata')), shell=True)
        if 475 <= event.x <= 495 and 275 <= event.y <= 295:
            self.player.load()
        if 240 <= event.x <= 260 and 265 <= event.y <= 285:
            self.player.pause()
        if 210 <= event.x <= 230 and 265 <= event.y <= 285:
            self.player.back = True
            pygame.mixer.music.stop()
        if 270 <= event.x <= 290 and 265 <= event.y <= 285:
            pygame.mixer.music.stop()
        if 255 <= event.y <= 265 and 50 <= event.x <= 450:
            self.player.seek(utils.calcvalue(event.x - 50, 400, 0, self.player.duration))
            self.status.temp.moving = 'music-seek'
        if 470 <= event.x <= 480 and 80 <= event.y <= 180:
            pygame.mixer.music.set_volume(utils.calcvalue(event.y - 80, 100, 100, 0) / 100)
            self.status.temp.moving = 'music-volume'
    if mode == 2:
        if self.status.temp.moving == 'music-volume':
            self.config.set('volume', pygame.mixer.music.get_volume())
        self.status.temp.moving = None

def default_event(self, mode, event):
    if mode == 0:
        if self.player.moving:
            if self.player.minialbumart:
                if 22 <= event.x <= 500 + self.status.add_width:
                    self.player.seek(utils.calcvalue(event.x - 22, 500 + self.status.add_width - 22, 0, self.player.duration))
            elif 0 <= event.x <= 500 + self.status.add_width:
                self.player.seek(utils.calcvalue(event.x, 500 + self.status.add_width, 0, self.player.duration))
    if mode == 1:
        if not self.status.mode == 'music':
            if 260 + self.status.add_height - 27 - 2 <= event.y <= 260 + self.status.add_height - 27 + 2:
                if self.player.minialbumart:
                    if 22 <= event.x <= 500 + self.status.add_width:
                        self.player.seek(utils.calcvalue(event.x - 26, 500 + self.status.add_width - 26, 0, self.player.duration))
                        self.player.moving = True
                else:
                    if 0 <= event.x <= 500 + self.status.add_width:
                        self.player.seek(utils.calcvalue(event.x, 500 + self.status.add_width, 0, self.player.duration))
                        self.player.moving = True
    if mode == 2:
        if self.player.moving:
            self.player.moving = False

def default_render(self, height, width):
    if self.player.minialbumart:
        more = 24
        self.canvas.create_image(13, height - 36, image=self.player.minialbumart, tag='rewrite')
    else:
        more = 0
    self.canvas.create_line(0, height - 47, 1000, height - 47, fill='#FFFFFF', tag='rewrite')
    if len(self.player.info) >= 50:
        self.canvas.create_text(8 + more, height - 37, text='{:.50}...'.format(self.player.info), anchor='w', font=self.font, fill='#FFFFFF', tag='rewrite')
    else:
        self.canvas.create_text(8 + more, height - 37, text='{}'.format(self.player.info), anchor='w', font=self.font, fill='#FFFFFF', tag='rewrite')
    self.canvas.create_text(width - 8, height - 37, text='{}/{} {}/{}'.format(self.player.position + 1, len(self.player.queue), utils.calc(int(self.player.get_pos())), utils.calc(int(self.player.duration))), anchor='e', font=self.font, fill='#FFFFFF', tag='rewrite')
    try:
        percent = int(float(self.player.get_pos() / self.player.duration * (500 - more))) + more
    except:
        percent = 0
    self.canvas.create_line(more, height - 26, percent, height - 26, fill='#FFFFFF', tag='rewrite')
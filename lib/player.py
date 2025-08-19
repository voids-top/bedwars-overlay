import subprocess
import pygame
from mutagen.mp3 import MP3
from mutagen.easyid3 import EasyID3
from . import utils
import threading
import time
from pathlib import Path
from tkinter import PhotoImage
from os import mkdir, getenv
from PIL import Image, ImageTk

class player:
    def __init__(self, config):
        try:
            pygame.mixer.init(frequency=48000, size=24, channels=2, buffer=16)
        except:
            pygame.mixer.init(frequency=48000, size=16, channels=2, buffer=16)
        self.duration = 0
        self.info = ''
        self.queue = []
        self.temp_disable = False
        self.status = 'Stopped'
        self.paused = False
        self.position = config.get('position') - 1
        self.add = 0
        self.back = False
        self.start = 0
        self.moving = False
        self.changed = True
        self.title = ''
        self.artist = ''
        self.album = ''
        self.albumart = None
        self.minialbumart = None
        self.config = config
        pygame.mixer.music.set_volume(config.get('volume'))
        threading.Thread(target=self.autoplay, daemon=True).start()
        for song in Path('{}/voidoverlay/music'.format(getenv('appdata'))).glob('*.mp3'):
            self.queue.append(str(song))
        if not self.queue:
            self.position = -1

    def load(self):
        try:
            playing = self.queue[self.position]
        except:
            playing = None
        self.queue.clear()
        for song in Path('{}/voidoverlay/music'.format(getenv('appdata'))).glob('*.mp3'):
            self.queue.append(str(song))
        for n in range(len(self.queue)):
            if self.queue[n] == playing:
                self.position = n
        self.config.set('position', self.position)

    def get_info(self, file):
        temp = EasyID3(file)
        self.album = ''
        self.artist = ''
        self.title = ''
        try:
            self.album = temp['album'][0]
        except:
            pass
        try:
            self.info = '{} - {}'.format(temp['artist'][0], temp['title'][0])
            self.title = temp['title'][0]
            self.artist = temp['artist'][0]
        except:
            try:
                self.info = temp['title'][0]
                self.title = temp['title'][0]
            except:
                self.info = file.split('\\')[-1]
                self.title = file.split('\\')[-1]
        temp = MP3(file)
        self.duration = temp.info.length
        try:
            open('albumart.png', 'wb').write(temp['APIC:'].data)
            image = Image.open('albumart.png')
            self.albumart = ImageTk.PhotoImage(image.resize((100, 100)))
            self.minialbumart = ImageTk.PhotoImage(image.resize((22, 22)))
        except:
            self.albumart = None
            self.minialbumart = None

    def next(self):
        if not self.back:
            self.position += 1
            self.add = 0
            if self.position >= len(self.queue):
                self.position = 0
            self.play(self.queue[self.position])
        else:
            self.back = False
            self.position -= 1
            self.add = 0
            if self.position < 0:
                self.position = len(self.queue) - 1
            self.play(self.queue[self.position])

    def seek(self, position):
        self.start = time.perf_counter()
        pygame.mixer.music.set_pos(position)
        self.add = position

    def pause(self):
        if not self.paused:
            self.paused = True
            pygame.mixer.music.pause()
            self.add = time.perf_counter() - self.start + self.add
            self.pausepos = time.perf_counter()
        else:
            pygame.mixer.music.unpause()
            self.paused = False
            self.start = time.perf_counter()

    def get_pos(self):
        if not self.queue:
            return 0
        if self.paused:
            return self.add
        return time.perf_counter() - self.start + self.add

    def autoplay(self):
        while True:
            while not pygame.mixer.music.get_busy() and (not self.paused) and self.queue:
                self.next()
            time.sleep(0.1)

    def play(self, file):
        try:
            pygame.mixer.music.load(file)
            self.get_info(file)
            self.start = time.perf_counter()
            pygame.mixer.music.play()
            self.config.set('position', self.position)
        except:
            self.next()
import os
import time
import threading
from . import ext
from . import data
from . import stats

def process(self, log):
    if not "[Client thread/INFO]: " in log:
        return
    log_chat = log.split('[Client thread/INFO]: ')[1]
    if log_chat == '[CHAT]                                   Bed Wars':
        #self.started = True
        #self.main.status['hidden'] = True
        #self.main.changed = True
        #self.config.log = 'Hidden'
        ext.send("chat /who")
        #ext.send("chat /shout check your stats using /bedwars stats")
    if log_chat == '[CHAT]                                   SkyWars':
        #self.started = True
        #self.main.status['hidden'] = True
        #self.main.changed = True
        #self.config.log = 'Hidden'
        ext.send("chat /who")
    if log_chat.startswith('[LC Accounts] Able to login '):
        self.mcid = log_chat.replace('[LC Accounts] Able to login ', '')
        self.status.set_log('account change')
    if log_chat.startswith("[CHAT] Can't find a player by the name of '."):
        player = log_chat.split("[CHAT] Can't find a player by the name of '.")[1].split("'")[0]
        if player == 'h':
            self.status.hide()
            self.status.set_log('Hidden')
        elif player == 's':
            self.status.show()
            #self.status.set_log('Shown')
        else:
            self.status.set_log('checking specified player')
            threading.Thread(target=stats.get_player, args=[self, player, True], daemon=True).start()
    if log_chat.startswith("[CHAT] Can't find a player by the name of '") and log_chat.endswith("?'"):
        player = log_chat.split("[CHAT] Can't find a player by the name of '")[1].split("?'")[0]
        threading.Thread(target=stats.get_player, args=[self, player], daemon=True).start()
    if log_chat == '[CHAT]        ':
        self.status.set_log('server change')
        self.status.noticed.clear()
        self.status.show()
        self.players.clear()
        self.status.noticed.clear()
        ext.send("chat /who")
    if log_chat.startswith('[CHAT] You deposited ') and log_chat.endswith("into your Team Chest!"):
        amount = log_chat.split('[CHAT] You deposited ')[1].split(' into your Team Chest!')[0].split(" ")[0].lstrip("x").strip()
        item = " ".join(log_chat.split('[CHAT] You deposited ')[1].split(' into your Team Chest!')[0].split(" ")[1:]).strip()
        if item in ["Emerald", "Diamond"]:
            ext.send("chat /pc deposited x{} {} into team chest".format(amount, item))
    if log_chat.startswith('[CHAT] The party was transferred to '):
        target = log_chat.split("[x")[0].split("by ")[1].strip().split("]")[-1].strip()
        if self.mcid and not self.mcid.lower() == target.lower():
            time.sleep(1)
            ext.send(f"chat /p transfer {target}")
    if log_chat.startswith('[CHAT] ') and log_chat.endswith("to Party Leader"):
        target = log_chat.split("[x")[0].split(" has promoted")[0].strip().split("]")[-1].strip()
        if self.mcid and not self.mcid.lower() == target.lower():
            time.sleep(1)
            ext.send(f"chat /p promote {target}")
    if log_chat.startswith('[CHAT] ONLINE: '):
        self.status.show()
        self.players.clear()
        self.status.set_log('checking players')
        players = log_chat.split('[CHAT] ONLINE: ')[1].split(' [x')[0].split(', ')
        for player in players:
            threading.Thread(target=stats.get_player, args=[self, player], daemon=True).start()
    if log_chat.startswith('[CHAT] Mode: '):
        self.status.show()
        self.players.clear()
        self.status.set_log('checking players')
    if log_chat.startswith('[CHAT] Team #'):
        self.status.show()
        #self.players.clear()
        self.status.set_log('checking players')
        #players = log_chat.split('[CHAT] ONLINE: ')[1].split(' [x')[0].split(', ')
        #for player in players:
        player = log_chat.split(': ')[1].split(' [x')[0].strip()
        threading.Thread(target=stats.get_player, args=[self, player], daemon=True).start()
    #if not log_chat == "[CHAT] Please don't spam the command!" and self.config.get('autorequeue') and self.started:
    #    utils.lobby(self)
    #    Thread(self.requeued, True, **('target', 'daemon')).start()
    #if ' has quit!' in log_chat:
    #    log_chat = log_chat.replace('[CHAT] ', '')
    #    player = log_chat.split(' has quit')[0]
    #    self.players.remove(player)
    #    self.config.log = log_chat
    #    self.write = True
    #if ' has joined ' in log_chat:
    #    log_chat = log_chat.replace('[CHAT] ', '')
    #    player = log_chat.split(' has joined ')[0]
    #    Thread(self.get_player, [
    #        player], True, **('target', 'args', 'daemon')).start()
    #    if self.started:
    #        self.player = player
    #        self.started = False
    #    self.config.log = log_chat

def reader(self):
    processed = []
    before = ''
    while True:
        try:
            (logfile, encoding) = self.status.logfile
            if before != logfile:
                if not logfile:
                    time.sleep(1)
                    continue
                before = logfile
                filesize = os.path.getsize(logfile)
                if filesize > 100000:
                    seek = filesize - 100000
                else:
                    seek = 0
                file = open(logfile, 'r', encoding=encoding)
                file.seek(seek)
                logs = file.read(100000).split('\n')[1:]
                processed = list(set(logs))
            filesize = os.path.getsize(logfile)
            if filesize > 100000:
                seek = filesize - 100000
            else:
                seek = 0
            file = open(logfile, 'r', encoding=encoding)
            file.seek(seek)
            logs = file.read(100000).split('\n')[1:]
            logs = list(set(logs))
            for log in logs:
                if log not in processed:
                    processed.append(log)
                    try:
                        process(self, log)
                    except:
                        pass
        except:
            pass
        time.sleep(self.config.get('refresh') / 1000)        
        if len(processed) >= 10000:
            try:
                del processed[:5000]
            finally:
                pass
            #self.status.set_log('buffer reset')
            continue

def check(self):
    clients = dict(data.clients)
    clients['Custom Client'] = self.config.get('log_file')
    appdata = os.getenv('appdata')
    userprofile = os.getenv('userprofile')
    for client in clients:
        try:
            if time.time() - os.path.getmtime(clients[client].format(appdata=appdata, userprofile=userprofile)) <= 30:
                for encoding in ['utf_8', 'ansi', 'cp932']:
                    try:
                        open(clients[client].format(appdata=appdata, userprofile=userprofile), 'r', encoding=encoding).read(100000)
                        if self.status.client == client:
                            return True
                        if not self.status.client == client:
                            self.status.client = client
                            self.status.logfile = (clients[client].format(appdata=appdata, userprofile=userprofile), encoding)
                            self.status.set_log(f'Connected to {client}')
                            return True
                    except:
                        pass
        except:
            pass
    else:
        return False

def checker(self):
    checked = False
    while True:
        checked = check(self)
        if checked:
            time.sleep(10)
        else:
            time.sleep(1)
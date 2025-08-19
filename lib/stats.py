import requests
import traceback
import time
from . import ext

session = requests.Session()
last_update = 0
mcid_cache = {}
data_cache = {}

def get_rank(player):
    if 'rank' in player:
        rank = player['rank']
    elif 'newPackageRank' in player:
        if 'monthlyPackageRank' in player:
            if not player['monthlyPackageRank'] == 'NONE':
                rank = 'MVP++'
            else:
                rank = player['newPackageRank']
        else:
            rank = player['newPackageRank']
    else:
        rank = 'DEFAULT'
    return rank

def bedwars_parse_from_hypixel_api(data, nick=None, custom=False):
    rank = get_rank(data['player'])
    if 'Bedwars' not in data['player']['stats']:
        return ('Success', {
            'Player': data['player']['displayname'],
            'rank': rank,
            'TAG': 'First Game',
            'FKDR': '-',
            'WLR': '-',
            'Finals': '-',
            'Wins': '-',
            'WS': '-',
            'star': 0,
            'score': 10000
        })

    tags = []
    try:
        final_kills = data['player']['stats']['Bedwars']['final_kills_bedwars']
    except:
        final_kills = 0
    try:
        final_deaths = data['player']['stats']['Bedwars']['final_deaths_bedwars']
    except:
        final_deaths = 0
    try:
        total_wins = data['player']['stats']['Bedwars']['wins_bedwars']
    except:
        total_wins = 0
    try:
        total_games = data['player']['stats']['Bedwars']['games_played_bedwars']
    except:
        total_games = 0
    try:
        star = data['player']['achievements']['bedwars_level']
    except:
        star = 0
    try:
        fkdr = '{:.2f}'.format(int(final_kills) / int(final_deaths))
    except:
        fkdr = '{:.2f}'.format(int(final_kills))
    try:
        wlr = '{:.2f}'.format(int(total_wins) / int(int(total_games) - int(total_wins)))
    except:
        wlr = '{:.2f}'.format(int(total_wins))
    
    if not final_deaths:
        tags.append('no death')
    if custom:
        tags.append('specified')
    if nick:
        tags.append('nicked')
        name = '{} ({})'.format(data['player']['displayname'], nick)
    else:
        name = data['player']['displayname']
    if 'winstreak' in data['player']['stats']['Bedwars']:
        winstreak = data['player']['stats']['Bedwars']['winstreak']
    else:
        tags.append('ws hidden')
        return ('Winstreak Hidden', {
            'Player': name,
            'FKDR': fkdr,
            'rank': rank,
            'TAG': ', '.join(tags),
            'WLR': wlr,
            'Finals': final_kills,
            'Wins': total_wins,
            'WS': '-',
            'star': star,
            'score': fkdr
        })
    return ("Success", {
        'Player': name,
        'FKDR': fkdr,
        'rank': rank,
        'TAG': ','.join(tags),
        'WLR': wlr,
        'Finals': final_kills,
        'Wins': total_wins,
        'WS': winstreak,
        'star': star,
        'score': fkdr
    })


def skywars_parse_from_hypixel_api(data, nick=None, custom=False):
    rank = get_rank(data['player'])
    if 'SkyWars' not in data['player']['stats']:
        return ('Success', {
            'Player': data['player']['displayname'],
            'rank': rank,
            'TAG': 'First Game',
            'FKDR': '-',
            'WLR': '-',
            'Finals': '-',
            'Wins': '-',
            'WS': '-',
            'star': 0,
            'score': 10000
        })

    normal_tags = []
    insane_tags = []
    try:
        kills_solo_normal = data['player']['stats']['SkyWars']['kills_solo_normal']
    except:
        kills_solo_normal = 0
    try:
        deaths_solo_normal = data['player']['stats']['SkyWars']['deaths_solo_normal']
    except:
        deaths_solo_normal = 0
    try:
        kills_solo_insane = data['player']['stats']['SkyWars']['kills_solo_insane']
    except:
        kills_solo_insane = 0
    try:
        deaths_solo_insane = data['player']['stats']['SkyWars']['deaths_solo_insane']
    except:
        deaths_solo_insane = 0
    try:
        wins_solo_normal = data['player']['stats']['SkyWars']['wins_solo_normal']
    except:
        wins_solo_normal = 0
    try:
        losses_solo_normal = data['player']['stats']['SkyWars']['losses_solo_normal']
    except:
        losses_solo_normal = 0
    try:
        wins_solo_insane = data['player']['stats']['SkyWars']['wins_solo_insane']
    except:
        wins_solo_insane = 0
    try:
        losses_solo_insane = data['player']['stats']['SkyWars']['losses_solo_insane']
    except:
        losses_solo_insane = 0
    try:
        star = data['player']['stats']['SkyWars']['levelFormattedWithBrackets']
    except:
        star = 0
    try:
        normal_kdr = '{:.2f}'.format(int(kills_solo_normal) / int(deaths_solo_normal))
    except:
        normal_kdr = '{:.2f}'.format(int(kills_solo_normal))
    try:
        normal_wlr = '{:.2f}'.format(int(wins_solo_normal) / int(int(losses_solo_normal) + int(wins_solo_normal)))
    except:
        normal_wlr = '{:.2f}'.format(int(wins_solo_normal))
    try:
        insane_kdr = '{:.2f}'.format(int(kills_solo_insane) / int(deaths_solo_insane))
    except:
        insane_kdr = '{:.2f}'.format(int(kills_solo_insane))
    try:
        insane_wlr = '{:.2f}'.format(int(wins_solo_insane) / int(int(losses_solo_insane) + int(wins_solo_insane)))
    except:
        insane_wlr = '{:.2f}'.format(int(wins_solo_insane))

    if not deaths_solo_normal:
        normal_tags.append('no death')
    if not deaths_solo_insane:
        insane_tags.append('no death')
    if custom:
        normal_tags.append('specified')
        insane_tags.append('specified')
    if nick:
        normal_tags.append('nicked')
        insane_tags.append('nicked')
        name = '{} ({})'.format(data['player']['displayname'], nick)
    else:
        name = data['player']['displayname']

    return ("Success", {
        'Player': name,
        'rank': rank,
        "modes": {
            "normal": {
                "kills": kills_solo_normal,
                "deaths": deaths_solo_normal,
                "wins": wins_solo_normal,
                "losses": losses_solo_normal,
                "KDR": normal_kdr,
                "WLR": normal_wlr,
                "TAG": ','.join(normal_tags),
                'score': normal_kdr
            },
            "insane": {
                "kills": kills_solo_insane,
                "deaths": deaths_solo_insane,
                "wins": wins_solo_insane,
                "losses": losses_solo_insane,
                "KDR": insane_kdr,
                "WLR": insane_wlr,
                "TAG": ','.join(insane_tags),
                'score': insane_kdr
            }
        },
        'star': star,
    })

def notify(self, reason, player, data={}):
    if not player in self.status.noticed:
        self.status.noticed.append(player)
        while self.status.last_notice + 0.5 > time.time():
            time.sleep(0.5)
        if reason == "fkdr":
            ext.send("chat /pc [!] {}{} -> FKDR {}, Lvl {}, Finals {}".format("[" + data["rank"].replace("_PLUS", "+") + "] " if data["rank"] and not data["rank"] in ["DEFAULT", "-"] else "", player, data["FKDR"], data["star"], data["Finals"]))
            self.status.last_notice = time.time()
        if reason == "nick":
            ext.send("chat /pc [!] {} -> Nicked".format(player))
            self.status.last_notice = time.time()

def get_player(self, mcid, custom = False):
    self.players.append(mcid)
    self.status.update()
    (status, data) = get_data(self, mcid, 'name', custom)
    try:
        if float(data.get("FKDR", 0)) >= 5:
            if not custom:
                notify(self, "fkdr", data["Player"], data)
    except:
        traceback.print_exc()
        pass
    if status == 'Cancelled':
        return None
    if status == 'Failed':
        self.player_data[mcid] = {
            'Player': mcid,
            'TAG': 'Failed',
            'rank': '-',
            'FKDR': '-',
            'WLR': '-',
            'Finals': '-',
            'Wins': '-',
            'WS': '-',
            'star': '-',
            'score': 100000 }
        #self.status.set_log(f'''Failed to check (network issue or voids api down)''')
    self.status.update()
    if status == 'Winstreak Hidden':
        self.player_data[mcid] = data
        winstreak_data = session.get('https://api.antisniper.net/winstreak?key={}&name={}'.format(self.config.get('antisniper_key'), mcid)).json()
        if winstreak_data['success']:
            try:
                self.player_data[mcid]['WS'] = winstreak_data['player']['data']['overall_winstreak']
            except:
                self.player_data[mcid]['WS'] = 0
            self.status.update()
    if status == 'Nicked':
        try:
            if not custom:
                notify(self, "nick", mcid)
        except:
            traceback.print_exc()
            pass
        self.player_data[mcid] = {
            'Player': mcid,
            'TAG': 'Nicked (in check)',
            'rank': '-',
            'FKDR': '-',
            'WLR': '-',
            'Finals': '-',
            'Wins': '-',
            'WS': '-',
            'star': '-',
            'score': 100000 }
        nick_data = session.get('https://api.antisniper.net/denick?key={}&nick={}'.format(self.config.get('antisniper_key'), mcid)).json()
        if not nick_data.get('player'):
            self.status.set_log(f'''Failed to denick from {mcid}''')
            self.player_data[mcid]['TAG'] = 'Nicked'
        else:
            try:
                (status, data) = get_data(self, nick_data['player']['ign'], 'name', custom, mcid)
                self.player_data[mcid] = data
                if data == 'Winstreak Hidden':
                    winstreak_data = session.get('https://api.antisniper.net/winstreak?key={}&name={}'.format(self.config.get('antisniper_key'), nick_data['player']['ign'])).json()
                    if winstreak_data['success']:
                        try:
                            self.player_data[mcid]['WS'] = winstreak_data['player']['data']['overall_winstreak']
                        except:
                            self.player_data[mcid]['WS'] = 0
                        self.status.update()
            except:
                self.status.set_log(f'''Failed to denick from {mcid}''')
                self.player_data[mcid]['TAG'] = 'Nicked'
                self.status.update()
    if status == 'Success':
        self.player_data[mcid] = data
        self.status.update()
        #self.status.set_log(f'''{mcid} ({status})''')
    time.sleep(11)
    if self.status.last_update < time.time() + 10:
        self.status.hide()

def get_data(self, mcid, mode, custom, nick = None):
    #key={}& self.config.get('hypixel_key'), 
    try:
        if mcid.lower() in data_cache:
            if data_cache[mcid.lower()]["last_update"] > time.time() - 60:
                return bedwars_parse_from_hypixel_api(data_cache[mcid.lower()], nick, custom)
        if mode == "name":
            if mcid.lower() in mcid_cache:
                mode = "uuid"
                mcid = mcid_cache[mcid.lower()]
        data = session.get('https://hypixel.voids.top/v2/player?{}={}'.format(mode, mcid)).json()
    except:
        return "Failed", {}
    if data.get('success'):
        if data['player']:
            data["last_update"] = time.time()
            return bedwars_parse_from_hypixel_api(data, nick, custom)
        return ('Nicked', {})
    if data['cause'] == 'Key throttle':
        time.sleep(0.1)
        if mcid not in self.players and nick not in self.players:
            return ('Cancelled', {})
    if data['cause'] == 'You have already looked up this name recently':
        while True:
            data = session.get('https://api.mojang.com/users/profiles/minecraft/{}'.format(mcid))
            if data.status_code == 204:
                return ('Nicked', {})
            if data.status_code != 429:
                try:
                    mcid = data.json()['id']
                    return get_data(self, mcid, "uuid", custom, nick)
                except:
                    return ('Nicked', {})
    return (data['cause'], { })


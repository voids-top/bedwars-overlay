colorlist = '#AAAAAA #AAAAAA #AAAAAA #AAAAAA #AAAAAA ✫\n#FFFFFF #FFFFFF #FFFFFF #FFFFFF #FFFFFF #FFFFFF ✫\n#FFAA00 #FFAA00 #FFAA00 #FFAA00 #FFAA00 #FFAA00 ✫\n#55FFFF #55FFFF #55FFFF #55FFFF #55FFFF #55FFFF ✫\n#00AA00 #00AA00 #00AA00 #00AA00 #00AA00 #00AA00 ✫\n#00AAAA #00AAAA #00AAAA #00AAAA #00AAAA #00AAAA ✫\n#AA0000 #AA0000 #AA0000 #AA0000 #AA0000 #AA0000 ✫\n#FF55FF #FF55FF #FF55FF #FF55FF #FF55FF #FF55FF ✫\n#5555FF #5555FF #5555FF #5555FF #5555FF #5555FF ✫\n#AA00AA #AA00AA #AA00AA #AA00AA #AA00AA #AA00AA ✫\n#FF5555 #FFAA00 #FFFF55 #55FF55 #55FFFF #FF55FF #AA00AA ✫\n#AAAAAA #FFFFFF #FFFFFF #FFFFFF #FFFFFF #AAAAAA #AAAAAA ✪\n#AAAAAA #FFFF55 #FFFF55 #FFFF55 #FFFF55 #FFAA00 #AAAAAA ✪\n#AAAAAA #55FFFF #55FFFF #55FFFF #55FFFF #00AAAA #AAAAAA ✪\n#AAAAAA #55FF55 #55FF55 #55FF55 #55FF55 #00AA00 #AAAAAA ✪\n#AAAAAA #00AAAA #00AAAA #00AAAA #00AAAA #5555FF #AAAAAA ✪\n#AAAAAA #FF5555 #FF5555 #FF5555 #FF5555 #AA0000 #AAAAAA ✪\n#AAAAAA #FF55FF #FF55FF #FF55FF #FF55FF #AA00AA #AAAAAA ✪\n#AAAAAA #5555FF #5555FF #5555FF #5555FF #0000AA #AAAAAA ✪\n#AAAAAA #AA00AA #AA00AA #AA00AA #AA00AA #FF55FF #AAAAAA ✪\n#555555 #AAAAAA #FFFFFF #FFFFFF #AAAAAA #AAAAAA #555555 ✪\n#FFFFFF #FFFFFF #FFFF55 #FFFF55 #FFAA00 #FFAA00 #FFAA00 ❀\n#FFAA00 #FFAA00 #FFFFFF #FFFFFF #55FFFF #00AAAA #00AAAA ❀\n#AA00AA #AA00AA #FF55FF #FF55FF #FFAA00 #FFFF55 #FFFF55 ❀\n#55FFFF #55FFFF #FFFFFF #FFFFFF #AAAAAA #AAAAAA #555555 ❀\n#FFFFFF #FFFFFF #55FF55 #55FF55 #00AA00 #00AA00 #00AA00 ❀\n#AA0000 #AA0000 #FF5555 #FF5555 #FF55FF #FF55FF #AA00AA ❀\n#FFFF55 #FFFF55 #FFFFFF #FFFFFF #555555 #555555 #555555 ❀\n#55FF55 #55FF55 #00AA00 #00AA00 #FFAA00 #FFAA00 #FFFF55 ❀\n#55FFFF #55FFFF #00AAAA #00AAAA #5555FF #5555FF #0000AA ❀\n#FFFF55 #FFFF55 #FFAA00 #FFAA00 #FF5555 #FF5555 #AA0000 ❀'.split('\n')
rankcolors = {'DEFAULT': '#a0a0a0', 'VIP': '#51f451', 'MVP': '#5afdfd', 'MVP++': '#e7a501', 'YOUTUBE': '#ed5454'}
clients = {
    'Vanilla/Forge Client': '{appdata}\\.minecraft\\logs\\latest.log',
    'Lunar Client': '{userprofile}\\.lunarclient\\offline\\multiver\\logs\\latest.log',
    'Lunar Client ': '{userprofile}\\.lunarclient\\logs\\launcher\\renderer.log',
    'Lunar Client (1.8-1)': '{userprofile}\\.lunarclient\\offline\\1.8\\logs\\latest.log',
    'Lunar Client (1.7-1)': '{userprofile}\\.lunarclient\\offline\\1.7\\logs\\latest.log',
    'Lunar Client (1.12-1)': '{userprofile}\\.lunarclient\\offline\\1.12\\logs\\latest.log',
    'Lunar Client (1.15-1)': '{userprofile}\\.lunarclient\\offline\\1.15\\logs\\latest.log',
    'Lunar Client (1.16-1)': '{userprofile}\\.lunarclient\\offline\\1.16\\logs\\latest.log',
    'Lunar Client (1.8-2)': '{userprofile}\\.lunarclient\\offline\\files\\1.8\\logs\\latest.log',
    'Lunar Client (1.7-2)': '{userprofile}\\.lunarclient\\offline\\files\\1.7\\logs\\latest.log',
    'Lunar Client (1.12-2)': '{userprofile}\\.lunarclient\\offline\\files\\1.12\\logs\\latest.log',
    'Lunar Client (1.15-2)': '{userprofile}\\.lunarclient\\offline\\files\\1.15\\logs\\latest.log',
    'Lunar Client (1.16-2)': '{userprofile}\\.lunarclient\\offline\\files\\1.16\\logs\\latest.log',
    'Badlion Client': '{appdata}\\.minecraft\\logs\\blclient\\minecraft\\latest.log',
    'PVP Lounge Client': '{appdata}\\.pvplounge\\logs\\latest.log'
}
colors = {
    "not_available": '#242424',
    "disabled": '#767676',
    "enabled": '#FFFFFF',
    "off": '#FF0000',
    "on": '#00FF1A',
}

def wscolor(ws):
    try:
        ws = int(ws)
        if ws < 10:
            return '#FFFFFF'
        if 10 <= ws < 20:
            return '#55FFFF'
        if 20 <= ws < 50:
            return '#55FF55'
        if 50 <= ws < 100:
            return '#FFFF55'
        if 100 <= ws < 1000:
            return '#FF5555'
        if 1000 <= ws:
            return '#AA0000'
    except:
        return '#FFFFFF'

def fkdrcolor(fkdr):
    try:
        fkdr = float(fkdr)
        if fkdr < 2:
            return '#FFFFFF'
        if 2 <= fkdr < 5:
            return '#55FFFF'
        if 5 <= fkdr < 10:
            return '#55FF55'
        if 10 <= fkdr < 50:
            return '#FFFF55'
        if 50 <= fkdr < 100:
            return '#FF5555'
        if 100 <= fkdr:
            return '#AA0000'
    except:
        return '#FFFFFF'

def wlrcolor(wlr):
    try:
        wlr = float(wlr)
        if wlr < 1:
            return '#FFFFFF'
        if 1 <= wlr < 2:
            return '#55FFFF'
        if 2 <= wlr < 5:
            return '#55FF55'
        if 5 <= wlr < 10:
            return '#FFFF55'
        if 10 <= wlr < 50:
            return '#FF5555'
        if 50 <= wlr:
            return '#AA0000'
    except:
        return '#FFFFFF'

def finalcolor(final):
    try:
        final = int(final)
        if final < 3000:
            return '#FFFFFF'
        if 3000 <= final < 5000:
            return '#55FFFF'
        if 5000 <= final < 10000:
            return '#55FF55'
        if 10000 <= final < 15000:
            return '#FFFF55'
        if 15000 <= final < 20000:
            return '#FF5555'
        if 20000 <= final:
            return '#AA0000'
    except:
        return '#FFFFFF'

def wincolor(win):
    try:
        win = int(win)
        if win < 1000:
            return '#FFFFFF'
        if 1000 <= win < 3000:
            return '#55FFFF'
        if 3000 <= win < 5000:
            return '#55FF55'
        if 5000 <= win < 10000:
            return '#FFFF55'
        if 10000 <= win < 20000:
            return '#FF5555'
        if 20000 <= win:
            return '#AA0000'
    except:
        return '#FFFFFF'

def rankcolor(rank):
    if rank == 'MVP++':
        return rankcolors['MVP++']
    if rank.startswith('VIP'):
        return rankcolors['VIP']
    if rank.startswith('MVP'):
        return rankcolors['MVP']
    if rank == 'DEFAULT':
        return rankcolors['DEFAULT']
    if rank == 'YOUTUBER':
        return rankcolors['YOUTUBE']
    return '#FFFFFF'

def color(stars):
    data = int(int(stars) / 100)
    if data >= 30:
        temp = colorlist[30].split(' ')
        result = []
        result.append('[:{}'.format(temp[0]))
        for n in range(len(str(stars))):
            result.append('{}:{}'.format(str(stars)[n], temp[n + 1]))
        result.append('{}:{}'.format(temp[7], temp[5]))
        result.append(']:{}'.format(temp[6]))
    else:
        temp = colorlist[data].split(' ')
        result = []
        result.append('[:{}'.format(temp[0]))
        for n in range(len(str(stars))):
            result.append('{}:{}'.format(str(stars)[n], temp[n + 1]))
        result.append('{}:{}'.format(temp[-1], temp[-3]))
        result.append(']:{}'.format(temp[-2]))
    return result

def star_color(canvas, color, n, font):
    c = 10
    if len(color) == 4:
        a = color[0].split(':')
        canvas.create_text(6 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
        a = color[1].split(':')
        canvas.create_text(13 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
        a = color[2].split(':')
        canvas.create_text(23 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
        a = color[3].split(':')
        canvas.create_text(31 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
    if len(color) == 5:
        a = color[0].split(':')
        canvas.create_text(6 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
        a = color[1].split(':')
        canvas.create_text(13 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
        a = color[2].split(':')
        canvas.create_text(22 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
        a = color[3].split(':')
        canvas.create_text(32 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
        a = color[4].split(':')
        canvas.create_text(40 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
    if len(color) == 6:
        a = color[0].split(':')
        canvas.create_text(6 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
        a = color[1].split(':')
        canvas.create_text(13 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
        a = color[2].split(':')
        canvas.create_text(22 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
        a = color[3].split(':')
        canvas.create_text(31 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
        a = color[4].split(':')
        canvas.create_text(41 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
        a = color[5].split(':')
        canvas.create_text(49 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
    if len(color) == 7:
        a = color[0].split(':')
        canvas.create_text(6 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
        a = color[1].split(':')
        canvas.create_text(13 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
        a = color[2].split(':')
        canvas.create_text(22 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
        a = color[3].split(':')
        canvas.create_text(31 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
        a = color[4].split(':')
        canvas.create_text(40 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
        a = color[5].split(':')
        canvas.create_text(50 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')
        a = color[6].split(':')
        canvas.create_text(58 + c, 55 + 15 * n, text=a[0], anchor='center', font=font, fill=a[1], tag='rewrite')

colorcodes = {
    "§0": "#000000",
    "§1": "#0000AA",
    "§2": "#00AA00",
    "§3": "#00AAAA",
    "§4": "#AA0000",
    "§5": "#AA00AA",
    "§6": "#FFAA00",
    "§7": "#AAAAAA",
    "§8": "#555555",
    "§9": "#5555FF",
    "§a": "#55FF55",
    "§b": "#55FFFF",
    "§c": "#FF5555",
    "§d": "#FF55FF",
    "§e": "#FFFF55",
    "§f": "#FFFFFF"
}

def parse_star_color(string):
    colorcode = "#FFFFFF"
    result = []
    string = string.strip().rstrip("§r")
    while True:
        if not string:
            break
        if string[0] == "§":
            colorcode = colorcodes.get(string[:2], "#FFFFFF")
            string = string[2:]
        result.append((colorcode, string[0]))
        string = string[1:]
    return result

colorlist = """#AAAAAA #AAAAAA #AAAAAA #AAAAAA #AAAAAA ✫
#FFFFFF #FFFFFF #FFFFFF #FFFFFF #FFFFFF #FFFFFF ✫
#FFAA00 #FFAA00 #FFAA00 #FFAA00 #FFAA00 #FFAA00 ✫
#55FFFF #55FFFF #55FFFF #55FFFF #55FFFF #55FFFF ✫
#00AA00 #00AA00 #00AA00 #00AA00 #00AA00 #00AA00 ✫
#00AAAA #00AAAA #00AAAA #00AAAA #00AAAA #00AAAA ✫
#AA0000 #AA0000 #AA0000 #AA0000 #AA0000 #AA0000 ✫
#FF55FF #FF55FF #FF55FF #FF55FF #FF55FF #FF55FF ✫
#5555FF #5555FF #5555FF #5555FF #5555FF #5555FF ✫
#AA00AA #AA00AA #AA00AA #AA00AA #AA00AA #AA00AA ✫
#FF5555 #FFAA00 #FFFF55 #55FF55 #55FFFF #FF55FF #AA00AA ✫
#AAAAAA #FFFFFF #FFFFFF #FFFFFF #FFFFFF #AAAAAA #AAAAAA ✪
#AAAAAA #FFFF55 #FFFF55 #FFFF55 #FFFF55 #FFAA00 #AAAAAA ✪
#AAAAAA #55FFFF #55FFFF #55FFFF #55FFFF #00AAAA #AAAAAA ✪
#AAAAAA #55FF55 #55FF55 #55FF55 #55FF55 #00AA00 #AAAAAA ✪
#AAAAAA #00AAAA #00AAAA #00AAAA #00AAAA #5555FF #AAAAAA ✪
#AAAAAA #FF5555 #FF5555 #FF5555 #FF5555 #AA0000 #AAAAAA ✪
#AAAAAA #FF55FF #FF55FF #FF55FF #FF55FF #AA00AA #AAAAAA ✪
#AAAAAA #5555FF #5555FF #5555FF #5555FF #0000AA #AAAAAA ✪
#AAAAAA #AA00AA #AA00AA #AA00AA #AA00AA #FF55FF #AAAAAA ✪
#555555 #AAAAAA #FFFFFF #FFFFFF #AAAAAA #AAAAAA #555555 ✪
#FFFFFF #FFFFFF #FFFF55 #FFFF55 #FFAA00 #FFAA00 #FFAA00 ❀
#FFAA00 #FFAA00 #FFFFFF #FFFFFF #55FFFF #00AAAA #00AAAA ❀
#AA00AA #AA00AA #FF55FF #FF55FF #FFAA00 #FFFF55 #FFFF55 ❀
#55FFFF #55FFFF #FFFFFF #FFFFFF #AAAAAA #AAAAAA #555555 ❀
#FFFFFF #FFFFFF #55FF55 #55FF55 #00AA00 #00AA00 #00AA00 ❀
#AA0000 #AA0000 #FF5555 #FF5555 #FF55FF #FF55FF #AA00AA ❀
#FFFF55 #FFFF55 #FFFFFF #FFFFFF #555555 #555555 #555555 ❀
#55FF55 #55FF55 #00AA00 #00AA00 #FFAA00 #FFAA00 #FFFF55 ❀
#55FFFF #55FFFF #00AAAA #00AAAA #5555FF #5555FF #0000AA ❀
#FFFF55 #FFFF55 #FFAA00 #FFAA00 #FF5555 #FF5555 #AA0000 ❀""".split("\n")

rankcolors = {"DEFAULT":"#a0a0a0","VIP":"#51f451", "MVP":"#5afdfd", "MVP++":"#e7a501", "YOUTUBE":"#ed5454"}

def wscolor(ws):
    try:
        ws = int(ws)
        if ws < 10:
            return "#FFFFFF"
        if 10 <= ws < 20:
            return "#55FFFF"
        if 20 <= ws < 50:
            return "#55FF55"
        if 50 <= ws < 100:
            return "#FFFF55"
        if 100 <= ws < 1000:
            return "#FF5555"
        if 1000 <= ws:
            return "#AA0000"
    except:
        return "#FFFFFF"

def fkdrcolor(fkdr):
    try:
        fkdr = float(fkdr)
        if fkdr < 2:
            return "#FFFFFF"
        if 2 <= fkdr < 5:
            return "#55FFFF"
        if 5 <= fkdr < 10:
            return "#55FF55"
        if 10 <= fkdr < 50:
            return "#FFFF55"
        if 50 <= fkdr < 100:
            return "#FF5555"
        if 100 <= fkdr:
            return "#AA0000"
    except:
        return "#FFFFFF"

def wlrcolor(wlr):
    try:
        wlr = float(wlr)
        if wlr < 1:
            return "#FFFFFF"
        if 1 <= wlr < 2:
            return "#55FFFF"
        if 2 <= wlr < 5:
            return "#55FF55"
        if 5 <= wlr < 10:
            return "#FFFF55"
        if 10 <= wlr < 50:
            return "#FF5555"
        if 50 <= wlr:
            return "#AA0000"
    except:
        return "#FFFFFF"

def finalcolor(final):
    try:
        final = int(final)
        if final < 3000:
            return "#FFFFFF"
        if 3000 <= final < 5000:
            return "#55FFFF"
        if 5000 <= final < 10000:
            return "#55FF55"
        if 10000 <= final < 15000:
            return "#FFFF55"
        if 15000 <= final < 20000:
            return "#FF5555"
        if 20000 <= final:
            return "#AA0000"
    except:
        return "#FFFFFF"

def rankcolor(rank):
    if rank == "MVP++":
        return rankcolors["MVP++"]
    if rank.startswith("VIP"):
        return rankcolors["VIP"]
    if rank.startswith("MVP"):
        return rankcolors["MVP"]
    if rank == "DEFAULT":
        return rankcolors["DEFAULT"]
    if rank == "YOUTUBER":
        return rankcolors["YOUTUBE"]
    return "#FFFFFF"

def color(stars):
    data = int(int(stars) / 100)
    if data >= 30:
        temp = colorlist[30].split(" ")
        result = []
        result.append("[:{}".format(temp[0]))
        for n in range(len(str(stars))):
            result.append("{}:{}".format(str(stars)[n], temp[n+1]))
        result.append("{}:{}".format(temp[7],temp[5]))
        result.append("]:{}".format(temp[6]))
    else:
        temp = colorlist[data].split(" ")
        result = []
        result.append("[:{}".format(temp[0]))
        for n in range(len(str(stars))):
            result.append("{}:{}".format(str(stars)[n], temp[n+1]))
        result.append("{}:{}".format(temp[-1],temp[-3]))
        result.append("]:{}".format(temp[-2]))
    return result


def star_color(canvas, color, n, font):
    c = 10
    if len(color) == 4:
        a = color[0].split(":")
        canvas.create_text(6 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
        a = color[1].split(":")
        canvas.create_text(13 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
        a = color[2].split(":")
        canvas.create_text(22 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
        a = color[3].split(":")
        canvas.create_text(31 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
    if len(color) == 5:
        a = color[0].split(":")
        canvas.create_text(6 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
        a = color[1].split(":")
        canvas.create_text(13 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
        a = color[2].split(":")
        canvas.create_text(22 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
        a = color[3].split(":")
        canvas.create_text(30 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
        a = color[4].split(":")
        canvas.create_text(39 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
    if len(color) == 6:
        a = color[0].split(":")
        canvas.create_text(6 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
        a = color[1].split(":")
        canvas.create_text(13 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
        a = color[2].split(":")
        canvas.create_text(22 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
        a = color[3].split(":")
        canvas.create_text(31 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
        a = color[4].split(":")
        canvas.create_text(39 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
        a = color[5].split(":")
        canvas.create_text(48 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
    if len(color) == 7:
        a = color[0].split(":")
        canvas.create_text(6 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
        a = color[1].split(":")
        canvas.create_text(13 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
        a = color[2].split(":")
        canvas.create_text(22 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
        a = color[3].split(":")
        canvas.create_text(31 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
        a = color[4].split(":")
        canvas.create_text(40 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
        a = color[5].split(":")
        canvas.create_text(48 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
        a = color[6].split(":")
        canvas.create_text(57 + c, 55 + 15 * n, text=a[0], anchor="center", font=font, fill=a[1], tag="stats")
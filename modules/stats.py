from lib import data

def render(self, rewrite=False):
    if rewrite:
        self.canvas.create_line(0, 45, 1000, 45, fill='#FFFFFF', tag='mode')
        self.canvas.create_text(35, 35, text='Star', anchor='center', font=self.font, fill='#FFFFFF', tag='mode')
    players = []
    maximum = {'Player': 0, 'TAG': 0, 'WS': 0, 'FKDR': 0, 'WLR': 0, 'Finals': 0, 'Wins': 0}
    moves = {'Player': 0, 'TAG': 0, 'WS': 0, 'FKDR': 0, 'WLR': 0, 'Finals': 0, 'Wins': 0}
    places = {'Player': 135, 'TAG': 235, 'WS': 290, 'FKDR': 330, 'WLR': 375, 'Finals': 420, 'Wins': 465}
    self.status.add_width = 0
    for player in list(set(self.players)):
        try:
            players.append(self.player_data[player])
            playerdata = self.player_data[player]
            for mode in self.limits:
                if len(str(playerdata[mode])) > self.limits[mode]:
                    value = len(str(playerdata[mode])) - self.limits[mode]
                    if value > maximum[mode]:
                        maximum[mode] = value
        except:
            players.append({'TAG': 'Loading', 'star': '-', 'rank': '-', 'Player': player, 'FKDR': '-', 'WLR': '-', 'Finals': '-', 'Wins': '-', 'WS': '-', 'score': 10000})
    plus = 0
    for mode in self.limits:
        value = maximum[mode]
        if moves[mode] < plus + value:
            moves[mode] = plus + value * 4
        plus += value * 8
    for a in moves:
        places[a] += moves[a]
    self.status.add_width += places['Wins'] - 475
    self.canvas.create_text(places['Player'], 35, text='Player', anchor='center', font=self.font, fill='#FFFFFF', tag='rewrite')
    self.canvas.create_text(places['TAG'], 35, text='TAG', anchor='center', font=self.font, fill='#FFFFFF', tag='rewrite')
    self.canvas.create_text(places['WS'], 35, text='WS', anchor='center', font=self.font, fill='#FFFFFF', tag='rewrite')
    self.canvas.create_text(places['FKDR'], 35, text='FKDR', anchor='center', font=self.font, fill='#FFFFFF', tag='rewrite')
    self.canvas.create_text(places['WLR'], 35, text='WLR', anchor='center', font=self.font, fill='#FFFFFF', tag='rewrite')
    self.canvas.create_text(places['Finals'], 35, text='Finals', anchor='center', font=self.font, fill='#FFFFFF', tag='rewrite')
    self.canvas.create_text(places['Wins'], 35, text='Wins', anchor='center', font=self.font, fill='#FFFFFF', tag='rewrite')
    players = sorted(players, key=lambda x: float(x['score']))
    players.reverse()
    if len(players) > 19:
        del players[21:]
    if len(players) > 11:
        self.status.add_height = 15 * (len(players) - 11)
    try:
        for n in range(len(players)):
            try:
                color = data.color(players[n]['star'])
                #print(color)
                data.star_color(self.canvas, color, n, self.font)
            except:
                pass
            if players[n]['Player'].lower() == self.mcid.lower():
                self.canvas.create_text(places['Player'], 55 + 15 * n, text='You', anchor='center', font=self.font, fill=data.rankcolor(players[n]['rank']), tag='rewrite')
            else:
                self.canvas.create_text(places['Player'], 55 + 15 * n, text=players[n]['Player'], anchor='center', font=self.font, fill=data.rankcolor(players[n]['rank']), tag='rewrite')
            #self.canvas.create_text(places['Player'], 55 + 15 * n, text=players[n]['Player'], anchor='center', font=self.font, fill=data.rankcolor(players[n]['rank']), tag='rewrite')
            self.canvas.create_text(places['TAG'], 55 + 15 * n, text=players[n]['TAG'], anchor='center', font=self.font, fill='#FFFFFF', tag='rewrite')
            self.canvas.create_text(places['WS'], 55 + 15 * n, text=players[n]['WS'], anchor='center', font=self.font, fill=data.wscolor(players[n]['WS']), tag='rewrite')
            self.canvas.create_text(places['FKDR'], 55 + 15 * n, text=players[n]['FKDR'], anchor='center', font=self.font, fill=data.fkdrcolor(players[n]['FKDR']), tag='rewrite')
            self.canvas.create_text(places['WLR'], 55 + 15 * n, text=players[n]['WLR'], anchor='center', font=self.font, fill=data.wlrcolor(players[n]['WLR']), tag='rewrite')
            self.canvas.create_text(places['Finals'], 55 + 15 * n, text=players[n]['Finals'], anchor='center', font=self.font, fill=data.finalcolor(players[n]['Finals']), tag='rewrite')
            self.canvas.create_text(places['Wins'], 55 + 15 * n, text=players[n]['Wins'], anchor='center', font=self.font, fill=data.wincolor(players[n]['Wins']), tag='rewrite')
    except Exception as e:
        print(e)

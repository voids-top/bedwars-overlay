import webbrowser

def render(self, rewrite=False):
    if rewrite:
        self.canvas.create_text(250, 45, text='About', anchor='center', font=('Minecraftia', 15), fill='#FFFFFF', tag='mode')
        self.canvas.create_text(250, 80, text='Creator / Developer', anchor='center', font=('Minecraftia', 10), fill='#FFFFFF', tag='mode')
        self.canvas.create_text(250, 100, text='VoidPro (@voidpro_dev)', anchor='center', font=('Minecraftia', 11), fill='#FFFFFF', tag='mode')
        self.canvas.create_text(250, 130, text='Tester / Ideas', anchor='center', font=('Minecraftia', 10), fill='#FFFFFF', tag='mode')
        self.canvas.create_text(250, 150, text='Shimamal (@MaybeShim)', anchor='center', font=('Minecraftia', 10), fill='#FFFFFF', tag='mode')
        self.canvas.create_text(250, 180, text='Special Thanks', anchor='center', font=('Minecraftia', 10), fill='#FFFFFF', tag='mode')
        self.canvas.create_text(250, 200, text='Antisniper (antisniper.net)', anchor='center', font=('Minecraftia', 9), fill='#FFFFFF', tag='mode')
        self.canvas.create_text(250, 220, text='Minecraftia', anchor='center', font=('Minecraftia', 9), fill='#FFFFFF', tag='mode')
        self.canvas.create_text(250, 260, text='Support : https://discord.gg/DZKqfdQGem', anchor='center', font=('Minecraftia', 9), fill='#FFFFFF', tag='mode')

def event(self, mode, event):
    if mode == 1:
        y = 260
        if y - 8 <= event.y <= y + 8 and 166 <= event.x <= 400:
            webbrowser.open('https://discord.gg/DZKqfdQGem', new=2, autoraise=True)
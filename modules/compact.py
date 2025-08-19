
from lib import utils

def render(self):
    if self.player.minialbumart:
        more = 24
        self.canvas.create_image(13, 37, image=self.player.minialbumart, tag='rewrite')
    else:
        more = 0
    if len(self.player.info) >= 50:
        self.canvas.create_text(8 + more, 35, text='{:.50}...'.format(self.player.info), anchor='w', font=self.font, fill='#FFFFFF', tag='rewrite')
    else:
        self.canvas.create_text(8 + more, 35, text='{}'.format(self.player.info), anchor='w', font=self.font, fill='#FFFFFF', tag='rewrite')
    self.canvas.create_text(492, 35, text='{}/{} {}/{}'.format(self.player.position + 1, len(self.player.queue), utils.calc(int(self.player.get_pos())), utils.calc(int(self.player.duration))), anchor='e', font=self.font, fill='#FFFFFF', tag='rewrite')
    try:
        percent = int(float(self.player.get_pos() / self.player.duration * (500 - more))) + more
    except:
        percent = 0
    self.canvas.create_line(more, 47, percent, 47, fill='#FFFFFF', tag='rewrite')
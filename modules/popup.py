from tkinter import Tk, Button, Entry, Label, messagebox, filedialog
import webbrowser

class AntiSniperKeyInput:
    def __init__(self):
        self.tk = Tk()
        self.tk.attributes('-topmost', 1)
        self.label = Label(self.tk, text=f'Enter a antisniper API Key')
        self.label.pack()
        link = Label(self.tk, text="You don't know about antisniper?\nClick here to Discord Server and Get API Key\n or leave blank to without antisniper api", fg='blue')
        link.pack()
        link.bind('<Button-1>', lambda e: webbrowser.open('https://antisniper.net', new=2, autoraise=True))
        self.entry = Entry(self.tk, width=50)
        self.entry.focus_set()
        self.entry.pack()
        self.button = Button(self.tk, text='OK', command=self.entried)
        self.button.pack()
        self.tk.lift()
        self.result = None
        self.tk.protocol('WM_DELETE_WINDOW', self.on_closing)
        self.tk.mainloop()
        if self.result:
            return

    def on_closing(self):
        self.tk.quit()
        self.tk.destroy()

    def entried(self):
        self.result = self.entry.get()
        self.tk.quit()
        self.tk.destroy()

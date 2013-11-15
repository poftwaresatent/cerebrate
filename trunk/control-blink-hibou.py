#!/usr/bin/python

from os import *
from signal import *
from time import *
from Tkinter import *
from subprocess import *
from select import *


class Application(Frame):

    def __init__(self, master=None):
        Frame.__init__(self, master)
        self.blinkstate = StringVar()
        self.blinkstate.set("stopped")
        self.blinkcommand = StringVar()
        self.blinkcommand.set("blinkd -r -c blink/iocactus-test.blink -l /dev/stdout -p /dev/null")
        self.blinklog = StringVar()
        self.blinklog.set("(no message)")
        self.blinkerr = StringVar()
        self.blinkerr.set("(no error)")
        self.blinkmsg = StringVar()
        self.blinkmsg.set("(no message)")
        self.blinkd = None
        self.poll = poll()
        self.after(100, self.idle)
        self.createWidgets()
        self.pack()


    def createWidgets(self):
        FRAME = Frame(self)
        LABEL = Label(FRAME)
        LABEL["width"] = 10
        LABEL["text"] = "Blink:"
        LABEL.pack({"side": "left"})
        LABEL = Label(FRAME)
        LABEL["width"] = 10
        LABEL["text"] = "state:"
        LABEL.pack({"side": "left"})
        LABEL = Label(FRAME)
        LABEL["width"] = 10
        LABEL["bg"] = "white"
        LABEL["textvariable"] = self.blinkstate
        LABEL["relief"] = "ridge"
        LABEL.pack({"side": "left"})
        LABEL = Label(FRAME)
        LABEL["width"] = 10
        LABEL["text"] = "message:"
        LABEL.pack({"side": "left"})
        LABEL = Label(FRAME)
        LABEL["width"] = 20
        LABEL["bg"] = "white"
        LABEL["textvariable"] = self.blinkmsg
        LABEL["relief"] = "ridge"
        LABEL.pack({"side": "left"})
        self.BLINKBTN = Button(FRAME)
        self.BLINKBTN["text"] = "Start"
        self.BLINKBTN["width"] = 20
        self.BLINKBTN["bg"] = "#00ff00"
        self.BLINKBTN["activebackground"] = "#00cc00"
        self.BLINKBTN["command"] = self.blink
        self.BLINKBTN.pack({"side": "right"})
        FRAME.pack({"side": "top"})
        
        FRAME = Frame(self)
        LABEL = Label(FRAME)
        LABEL["text"] = "command:"
        LABEL["width"] = 10
        LABEL.pack({"side": "left"})
        ENTRY = Entry(FRAME)
        ENTRY["width"] = 80
        ENTRY["textvariable"] = self.blinkcommand
        ENTRY.pack({"side": "left"})
        FRAME.pack({"side": "top"})
        
        FRAME = Frame(self)
        LABEL = Label(FRAME)
        LABEL["text"] = "log:"
        LABEL["width"] = 10
        LABEL.pack({"side": "left"})
        LABEL = Label(FRAME)
        LABEL["width"] = 80
        LABEL["bg"] = "white"
        LABEL["textvariable"] = self.blinklog
        LABEL["relief"] = "ridge"
        LABEL.pack({"side": "left"})
        FRAME.pack({"side": "top"})
        
        FRAME = Frame(self)
        LABEL = Label(FRAME)
        LABEL["text"] = "error:"
        LABEL["width"] = 10
        LABEL.pack({"side": "left"})
        LABEL = Label(FRAME)
        LABEL["width"] = 80
        LABEL["bg"] = "white"
        LABEL["textvariable"] = self.blinkerr
        LABEL["relief"] = "ridge"
        LABEL.pack({"side": "left"})
        FRAME.pack({"side": "top"})
        
        BUTTON = Button(self)
        BUTTON["width"] = 40
        BUTTON["text"] = "QUIT"
        BUTTON["command"] = self.do_quit
        BUTTON["bg"] = "#ff0000"
        BUTTON["activebackground"] = "#cc0000"
        BUTTON.pack({"side": "bottom"})

        
    def blink(self):

        if self.blinkstate.get() == "stopped":
            self.blinkmsg.set("starting...")
            if self.blinkd != None:
                self.poll.unregister(self.blinkd.stdout)
                self.poll.unregister(self.blinkd.stderr)
            self.blinklog.set("(no message)")
            self.blinkerr.set("(no error)")
            self.blinkd = Popen(self.blinkcommand.get(), shell=True, stdout=PIPE, stderr=PIPE)
            self.poll.register(self.blinkd.stdout, POLLIN)
            self.poll.register(self.blinkd.stderr, POLLIN)
            self.BLINKBTN["text"] = "Stop"
            self.BLINKBTN["bg"] = "#ffff00"
            self.BLINKBTN["activebackground"] = "#ffcc00"
            self.blinkstate.set("running")
            self.blinkmsg.set("started")

        elif self.blinkstate.get() == "running":
            self.blinkmsg.set("kill(%d, %d)" % (self.blinkd.pid, SIGTERM))
            kill(self.blinkd.pid, SIGTERM)
            self.blinkmsg.set("waiting...")
            status = self.blinkd.wait()
            self.blinkmsg.set("exit status %d" % status)
            self.BLINKBTN["text"] = "Start"
            self.BLINKBTN["bg"] = "#00ff00"
            self.BLINKBTN["activebackground"] = "#00cc00"
            self.blinkstate.set("stopped")

        else:
            self.blinkmsg.set("error")
            self.BLINKBTN["text"] = "Kill"
            self.BLINKBTN["bg"] = "#ff0000"
            self.BLINKBTN["activebackground"] = "#cc0000"
            self.blinkstate.set("error")
            self.BLINKBTN["command"] = self.do_quit


    def idle(self):
        if self.blinkd != None:
            for status in self.poll.poll(50):
                fd, event = status
                if event & POLLIN:
                    if fd == self.blinkd.stdout.fileno():
                        log = self.blinkd.stdout.readline()
                        self.blinklog.set(log[:-1])
                    else:
                        err = self.blinkd.stderr.readline()
                        self.blinkerr.set(err[:-1])
        self.after(100, self.idle)
        
        
    def do_quit(self):
        if self.blinkstate.get() == "running":
            self.blinkmsg.set("kill(%d, %d)" % (self.blinkd.pid, SIGTERM))
            kill(self.blinkd.pid, SIGTERM)
            self.blinkmsg.set("waiting...")
            status = self.blinkd.wait()
            self.blinkmsg.set("exit status %d" % status)
            self.blinkstate.set("stopped")
        self.quit()


app = Application()
app.mainloop()

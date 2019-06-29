# -*- coding: iso-8859-1 -*-
from __future__ import division
from pygame import *
import pygame
import pygame.midi as midi
import random
import sys
import os
import serial
import serial.tools.list_ports
import platform

VERSION = 0.2

DEBUG = True

VIEWPORT_SIZE = (480, 270)
VIEWPORT_SIZE_RATIO = 2
INITIAL_VIEWPORT_SIZE = tuple([sizes * VIEWPORT_SIZE_RATIO for sizes in VIEWPORT_SIZE])

RECT_INIT = (0, 0)
RECT_CENTER = (VIEWPORT_SIZE[0] / 2, VIEWPORT_SIZE[1] / 2)

LEFT_MOUSE_BUTTON = 1
LOOP_SOUND = -1

NOALIAS = 1
WINDOWED = 0
BIT_DEPTH = 32

NO_TRANSPARENCY = 255
FULL_TRANSPARENCY = 0

cd = os.path.join
MAIN_PATH = os.path.dirname(__file__)
RES_PATH = cd(MAIN_PATH, "res")


# SDL CONFIGS
# the windib driver works best on Windows,
# as it is more fault-tolerant
if sys.platform == "win32":
    os.environ["SDL_VIDEODRIVER"] = "windib"
os.environ['SDL_VIDEO_CENTERED'] = '1'

class Screen():
    window_rect = rect.Rect(RECT_INIT, INITIAL_VIEWPORT_SIZE)
    rect = rect.Rect(RECT_INIT, VIEWPORT_SIZE)

class Colors():
    REPLACE_COLOR = (255, 255, 255)
    WHITE = (255, 255, 255)
    GRAY = (60, 60, 60)
    BLACK = (0, 0, 0)
    LIGHT_GRAY = (165, 165, 165)
    TRANSPARENT = (0, 0, 0, 0)
    SKIN = (248, 218, 163)
    RED = (255, 0, 0)
    DARK_RED = (180, 0, 0)

class Game():
    def main(self):
        # serial init
        port_number = sys.argv[1]
        if platform.system() == "Linux":
            port_name = "/dev/tty%s"
        elif platform.system() == "Windows":
            port_name = "COM%s"
        self.bps = 2000000
        self.port = port_name % port_number
        #self.ser = serial.Serial(self.port, self.bps, timeout=1)
        
        # midi init
        midi.init()
        self.midi = pygame.midi.Output(int(port_number), latency=0, buffer_size=8)
        #self.midi = midi.MidiConnector(self.port, timeout=0.016)

        # for use with notes
        self.octave = 4
        
        # pygame init
        display.init()
        font.init()

        self.fullscreen = False
        if self.fullscreen:
            self.real_window = display.set_mode(INITIAL_VIEWPORT_SIZE, FULLSCREEN, BIT_DEPTH)
        else:
            self.real_window = display.set_mode(INITIAL_VIEWPORT_SIZE, WINDOWED+RESIZABLE, BIT_DEPTH)
        self.window = surface.Surface(VIEWPORT_SIZE)
        
        # clock
        clock = time.Clock()
        
        # fonts        
        self.osd_font = font.SysFont("Courier", 16)
        self.title_font = font.SysFont("Courier", 24) # @800x600, max 56 chars
        self.big_font = font.SysFont("Courier", 60, bold=True)

        # pseudo message box
        self.show_special_message = False
        self.show_text_message = False
        self.special_message = Message("Error")
        self.text_message = self.big_font.render("", NOALIAS, Colors.WHITE)
        self.current_char = ""
        
        # background
        bg = surface.Surface(VIEWPORT_SIZE)
        bg.fill(Colors.BLACK)
        self.window.blit(bg, RECT_INIT)
        
        # main cycle
        while True:
            # time
            clock.tick(60)

            self.handle_game()

            # debug fps
            if DEBUG:
                text_ticks = self.osd_font.render("delta time: %04d ms" %
                        (clock.get_rawtime()), NOALIAS, Colors.WHITE)
                self.window.blit(text_ticks, (5, 5))
                if clock.get_fps() > 0:
                    text_fps = self.osd_font.render("fps: %d (~%.2f ms)" % 
                            (clock.get_fps(), clock.get_time()), NOALIAS, Colors.WHITE)
                    self.window.blit(text_fps, (5, 20))

            # update window
            transform.smoothscale(self.window, Screen.window_rect.size, self.real_window)
            display.update()

    def handle_game(self):
        # blits
        # background
        bg = surface.Surface(VIEWPORT_SIZE)
        bg.fill(Colors.GRAY)
        self.window.blit(bg, RECT_INIT)
        
        # inputs
        for e in event.get():
            self.handle_events_quit(e)
            self.handle_events_resize(e)
            self.handle_events_fullscreen(e)
            char = ""
            if e.type == KEYDOWN:
                if (e.key < 256):
                    char = chr(e.key)
                    self.current_char = char
                    self.text_message = self.big_font.render(char.upper(), NOALIAS, Colors.WHITE)
                    self.show_text_message = True
                    #self.ser.write(">" + char)
                    note = self.charkey_to_note(char)
                    self.midi.note_off(note, velocity=127, channel=1)
                    
                    #msg = self.midi.read()
                    #print(msg)
        
                # special functions
                if e.key == K_F1:
                    self.ser.write("!f")
                    self.special_message = Message("MOTOR FORWARD")
                    self.show_special_message = True
                elif e.key == K_F2:
                    self.ser.write("!b")
                    self.special_message = Message("MOTOR BACKWARD")
                    self.show_special_message = True
                elif e.key == K_F3:
                    self.ser.write("!m2")
                    self.special_message = Message("MAXIMUM VOLUME")
                    self.show_special_message = True
                elif e.key == K_F4:
                    self.ser.write("!m160")
                    self.special_message = Message("MAXIMUM MOVEMENT")
                    self.show_special_message = True
                elif e.key == K_F5:
                    self.ser.write("!p")
                    self.special_message = Message("PLAY / PAUSE")
                    self.show_special_message = True
                elif e.key == K_F6:
                    self.ser.write("!s")
                    self.special_message = Message("STOP")
                    self.show_special_message = True
                elif e.key == K_F7:
                    self.ser.write("!+")
                    self.special_message = Message("FASTER")
                    self.show_special_message = True
                elif e.key == K_F8:
                    self.ser.write("!-")
                    self.special_message = Message("SLOWER")
                    self.show_special_message = True
                elif e.key == K_F9:
                    self.ser.write("!r")
                    self.special_message = Message("RESET")
                    self.show_special_message = True
                elif e.key == K_F12:
                    self.special_message = Message("**RESTART**")
                    self.show_special_message = True
                    self.ser.close()
                    self.ser = serial.Serial(self.port, self.bps, timeout=1)
            elif e.type == KEYUP:
                if (e.key < 256):
                    char = chr(e.key)
                    note = self.charkey_to_note(char)
                    self.midi.note_off(note, velocity=127, channel=1)
                    #self.ser.write("<" + char)
                self.show_special_message = False
                if (self.current_char == char):
                    self.show_text_message = False
                        
        if self.show_special_message:
            self.special_message.pos.center = (
                VIEWPORT_SIZE[0] / 2 ,
                VIEWPORT_SIZE[1] - self.special_message.pos.height - 10)
            self.window.blit(self.special_message.surf, self.special_message.pos)
        if self.show_text_message:
            self.window.blit(self.text_message, 
                (VIEWPORT_SIZE[0] / 2 - self.text_message.get_rect().centerx, 
                VIEWPORT_SIZE[1] / 2 - self.text_message.get_rect().centery))

    def handle_events_quit(self, event):
        if event.type == QUIT or (event.type == KEYDOWN and event.key == K_ESCAPE):
            quit()
            sys.exit()

    def handle_events_fullscreen(self, event):
        if event.type == KEYDOWN and event.mod == KMOD_LALT and event.key == K_RETURN:
            self.fullscreen = not self.fullscreen
            if self.fullscreen:
                self.real_window = display.set_mode(Screen.window_rect.size, FULLSCREEN)
            else:
                self.real_window = display.set_mode(Screen.window_rect.size, WINDOWED+RESIZABLE)

    def handle_events_resize(self, event):
        if event.type == VIDEORESIZE:
            # force 16:9 window size
            Screen.window_rect.size = (event.dict['size'][0], event.dict['size'][0] * 9 / 16)
            # must reset the display when resizing the window, or else you
            # can't draw on the new parts of the window.
            if self.fullscreen:
                self.real_window = display.set_mode(Screen.window_rect.size, FULLSCREEN, BIT_DEPTH)
            else:
                self.real_window = display.set_mode(Screen.window_rect.size, WINDOWED+RESIZABLE, BIT_DEPTH)

    def charkey_to_note(self, char):
        if char == "z": return self.octave * 12 + 0
        if char == "s": return self.octave * 12 + 1
        if char == "x": return self.octave * 12 + 2
        if char == "d": return self.octave * 12 + 3
        if char == "c": return self.octave * 12 + 4
        if char == "v": return self.octave * 12 + 5
        if char == "g": return self.octave * 12 + 6
        if char == "b": return self.octave * 12 + 7
        if char == "h": return self.octave * 12 + 8
        if char == "n": return self.octave * 12 + 9
        if char == "j": return self.octave * 12 + 10
        if char == "m": return self.octave * 12 + 11
        if char == ",": return self.octave * 12 + 12
        if char == "l": return self.octave * 12 + 13
        if char == ".": return self.octave * 12 + 14
        if char == "ñ": return self.octave * 12 + 15
        if char == "-": return self.octave * 12 + 16
        
        if char == "q": return self.octave * 12 + 12
        if char == "2": return self.octave * 12 + 13
        if char == "w": return self.octave * 12 + 14
        if char == "3": return self.octave * 12 + 15
        if char == "e": return self.octave * 12 + 16
        if char == "r": return self.octave * 12 + 17
        if char == "5": return self.octave * 12 + 18
        if char == "t": return self.octave * 12 + 19
        if char == "6": return self.octave * 12 + 20
        if char == "y": return self.octave * 12 + 21
        if char == "7": return self.octave * 12 + 22
        if char == "u": return self.octave * 12 + 23
        if char == "i": return self.octave * 12 + 24
        if char == "9": return self.octave * 12 + 25
        if char == "o": return self.octave * 12 + 26
        if char == "0": return self.octave * 12 + 27
        if char == "p": return self.octave * 12 + 28
        
class Message():
    def __init__(self, text=" ", text_color=Colors.WHITE, bg_color=Colors.BLACK):
        message_font = font.SysFont("Courier", 24)
        text_message = message_font.render(text, NOALIAS, text_color)
        self.surf = surface.Surface(
            (text_message.get_rect().width + 20, 
            text_message.get_rect().height + 20))
        self.surf.fill(bg_color)
        draw.polygon(self.surf, Colors.WHITE, (
            (0, 0), 
            (self.surf.get_rect().width - 1, 0), 
            (self.surf.get_rect().width - 1, self.surf.get_rect().height - 1), 
            (0, self.surf.get_rect().height - 1)
            ), 1)
        
        self.surf.blit(text_message, (10, text_message.get_rect().centery - 2))
        self.pos = rect.Rect(
                0, 0, self.surf.get_rect().width, self.surf.get_rect().height)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        # print serial ports
        print("Port is missing (tty USB o COM).")
        print("\nAvailable serial ports:")
        available_ports = list(serial.tools.list_ports.comports())
        print("\n".join([str(x) for x in available_ports]))
        
        # print midi ports
        midi.init()
        midi_devices = midi.get_count()
        print("\nAvailable MIDI Devices:")
        for i in range(midi_devices):
            # (interf, name, input, output, opened)
            device = midi.get_device_info(i)
            #print(", ".join([repr(d) for d in midi.get_device_info(i)]))
            print("%s: %s (%s)" % (i, str(device[1], "utf-8"), ("input" if device[2] else "") + ("output" if device[3] else "")))
        
        sys.exit()
    
    game = Game()
    game.main()
    

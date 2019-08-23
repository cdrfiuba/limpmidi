# -*- coding: iso-8859-1 -*-

import os
import contextlib
with contextlib.redirect_stdout(None):
    import pygame

from pygame import *
import pygame.midi as midi
import sys
import serial
import serial.tools.list_ports
import argparse

VERSION = 0.4

DEBUG = True

VIEWPORT_SIZE = (480, 270)
VIEWPORT_SIZE_RATIO = 2
INITIAL_VIEWPORT_SIZE = tuple([sizes * VIEWPORT_SIZE_RATIO for sizes in VIEWPORT_SIZE])
INITIAL_FULLSCREEN_MODE = False

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

INVALID_NOTE = -1
MIN_NOTE = 0
MAX_NOTE = 127 # 0b1111

# SDL CONFIGS
# the windib driver works best on Windows,
# as it is more fault-tolerant
if sys.platform == "win32":
    os.environ["SDL_VIDEODRIVER"] = "windib"
os.environ['SDL_VIDEO_CENTERED'] = '1'

class Screen():
    monitor_rect = rect.Rect(RECT_INIT, INITIAL_VIEWPORT_SIZE)
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
    TEAL = (0, 180, 180)

class Game():
    MODE_MIDI, MODE_SERIAL = list(range(2))

    def list_ports(self):
        # print serial ports
        print("Available serial ports:")
        available_ports = list(serial.tools.list_ports.comports())
        print(("\n".join([str(x) for x in available_ports])))
        
        # print midi devices
        midi.init()
        midi_devices = midi.get_count()
        print("\nAvailable MIDI devices:")
        for i in range(midi_devices):
            # (interf, name, input, output, opened)
            device = midi.get_device_info(i)
            print("%s: %s (%s)" % (i, str(device[1], "utf-8"), ("input" if device[2] else "") + ("output" if device[3] else "")))
    
    def main(self):

        # initiate the parser
        parser = argparse.ArgumentParser()
        parser.add_argument("-si", "--serial-input", help="set serial input port (eg: /dev/tty)")
        parser.add_argument("-so", "--serial-output", help="set serial output port (eg: /dev/tty)")
        parser.add_argument("-m", "--midi", help="set midi output device (eg: 0)", type=int)
        parser.add_argument("-l", "--list-ports", help="show serial ports and MIDI devices available and exit", action="store_true")

        # read arguments from the command line
        args = parser.parse_args()

        if args.list_ports:
            self.list_ports()
            parser.exit()

        if not args.serial_output and args.midi == None:
            print("Error: Serial port or MIDI device is missing.\n")
            self.list_ports()
            parser.exit()
        
        if args.serial_output and args.midi != None:
            print("Error: Must only specify Serial or MIDI.")
            parser.exit()
            
        self.serial_input_available = False
        if args.serial_input:
            self.serial_input_available = True
        
        if args.serial_output or args.serial_input:
            self.mode = Game.MODE_SERIAL
            # serial init
            self.bps = 2000000
            self.port = args.serial_output or args.serial_input
            self.ser = serial.Serial(self.port, self.bps, timeout=1)
        
        if args.midi != None:
            self.mode = Game.MODE_MIDI
            # midi init
            midi.init()
            self.port = int(args.midi)
            self.midi = midi.Output(self.port, latency=0, buffer_size=0)

        # for use with notes
        self.octave = 4
        self.channel = 1
        self.notes = []
        self.pressed_notes = []
        self.pressed_notes_text = []
        self.released_notes = []
        self.pixels_to_move_keyboard = 0
        
        # pygame init
        display.init()
        font.init()
        display_info = display.Info()
        Screen.monitor_rect.size = (display_info.current_w, display_info.current_h)

        self.fullscreen = INITIAL_FULLSCREEN_MODE
        if self.fullscreen:
            self.real_window = display.set_mode(Screen.monitor_rect.size, FULLSCREEN, BIT_DEPTH)
        else:
            self.real_window = display.set_mode(Screen.window_rect.size, WINDOWED+RESIZABLE, BIT_DEPTH)
        self.window = surface.Surface(VIEWPORT_SIZE)
        
        # clock
        clock = time.Clock()
        
        # fonts        
        self.key_font = font.SysFont("monospace", 8, bold=True)
        self.osd_font = font.SysFont("monospace", 14)
        self.title_font = font.SysFont("monospace", 24) # @800x600, max 56 chars
        self.big_font = font.SysFont("monospace", 50, bold=True)

        # pseudo message box
        self.special_message = None
        self.text_message = None
        self.current_char = ""
        
        # keyboard drawing
        previous_white_note = KeyboardNote(KeyboardNote.WHITE_KEY)
        previous_white_note.pos.x = (-previous_white_note.pos.width - 1) * 24
        previous_white_note.pos.bottom = self.window.get_rect().bottom - 2
        black_keys = []
        white_keys = []
        for note in range(MIN_NOTE, MAX_NOTE + 1):
            n = note % 12
            if n == 1 or n == 3 or n == 6 or n == 8 or n == 10:
                new_note = KeyboardNote(KeyboardNote.BLACK_KEY, note)
                x_movement = -new_note.pos.width / 2 + 1
            else:
                new_note = KeyboardNote(KeyboardNote.WHITE_KEY, note)
                x_movement = 1
            new_note.pos.topleft = previous_white_note.pos.topright
            new_note.pos.x += x_movement
                
            previous_note = new_note
            if n == 1 or n == 3 or n == 6 or n == 8 or n == 10:
                black_keys.append(new_note)
            else:
                previous_white_note = new_note
                white_keys.append(new_note)
                
        # union of notes with black keys at the end so they get draw on top
        self.notes = white_keys + black_keys
        
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
            if self.fullscreen:
                transform.scale(self.window, Screen.monitor_rect.size, self.real_window)
            else:
                transform.smoothscale(self.window, Screen.window_rect.size, self.real_window)

            display.update()

    def handle_game(self):
        # blits
        # background
        bg = surface.Surface(VIEWPORT_SIZE)
        bg.fill(Colors.GRAY)
        self.window.blit(bg, RECT_INIT)
        
        # read serial inputs
        if self.serial_input_available:
            incoming_serial_bytes = self.ser.inWaiting()
            if incoming_serial_bytes > 0:
                data_str = self.ser.read(incoming_serial_bytes).decode('ascii')
                print(data_str, end='')
        
        # inputs
        for e in event.get():
            self.handle_events_quit(e)
            self.handle_events_resize(e)
            self.handle_events_fullscreen(e)
            char = ""
            
            # Midi Mode
            if self.mode == Game.MODE_MIDI:
                if e.type == KEYDOWN:
                    note = self.charkey_to_note(key.name(e.key))
                    if note >= MIN_NOTE and note <= MAX_NOTE:
                        self.midi.note_on(note, velocity=127, channel=self.channel)
                        self.pressed_notes.append(note)
                        self.pressed_notes_text.append(note)
                    if e.key >= K_KP0 and e.key <= K_KP9:
                        self.channel = e.key - K_KP1
                        if e.key == K_KP0: self.channel = 9
                        self.special_message = Message("CHANNEL %s" % (self.channel + 1))
                    elif e.key == K_KP_PLUS and not self.pressed_notes_text:
                        self.octave += 1
                        if self.octave <= 10:
                            self.pixels_to_move_keyboard += 7 * (KeyboardNote.SIZE_WHITE_KEY[0] + 1)
                        else:
                            self.octave = 10
                        self.special_message = Message("OCTAVE %s" % self.octave)
                    elif e.key == K_KP_MINUS and not self.pressed_notes_text:
                        self.octave -= 1
                        if self.octave >= 0:
                            self.pixels_to_move_keyboard -= 7 * (KeyboardNote.SIZE_WHITE_KEY[0] + 1)
                        else:
                            self.octave = 0
                        self.special_message = Message("OCTAVE %s" % self.octave)

                elif e.type == KEYUP:
                    note = self.charkey_to_note(key.name(e.key))
                    if note >= MIN_NOTE and note <= MAX_NOTE:
                        self.midi.note_off(note, velocity=127, channel=self.channel)
                        self.released_notes.append(note)
                        self.pressed_notes_text.remove(note) # TODO: crashes when changing octaves
                    self.special_message = None
                        
            # Serial Mode
            if self.mode == Game.MODE_SERIAL:
                if e.type == KEYDOWN:
                    if e.key < 256:
                        char = chr(e.key)
                        self.current_char = char
                        self.text_message = self.big_font.render(char.upper(), NOALIAS, Colors.WHITE)
                        self.serial_write(">" + char)
            
                    # special functions
                    if e.key == K_F1:
                        self.serial_write("!f")
                        self.special_message = Message("MOTOR FORWARD")
                    elif e.key == K_F2:
                        self.serial_write("!b")
                        self.special_message = Message("MOTOR BACKWARD")
                    elif e.key == K_F3:
                        self.serial_write("!m2")
                        self.special_message = Message("MAXIMUM VOLUME")
                    elif e.key == K_F4:
                        self.serial_write("!m160")
                        self.special_message = Message("MAXIMUM MOVEMENT")
                    elif e.key == K_F5:
                        self.serial_write("!p")
                        self.special_message = Message("PLAY / PAUSE")
                    elif e.key == K_F6:
                        self.serial_write("!s")
                        self.special_message = Message("STOP")
                    elif e.key == K_F7:
                        self.serial_write("!+")
                        self.special_message = Message("FASTER")
                    elif e.key == K_F8:
                        self.serial_write("!-")
                        self.special_message = Message("SLOWER")
                    elif e.key == K_F9:
                        self.serial_write("!r")
                        self.special_message = Message("RESET")
                    elif e.key == K_F12:
                        self.special_message = Message("**RESTART**")
                        self.ser.close()
                        self.ser = serial.Serial(self.port, self.bps, timeout=1)
                elif e.type == KEYUP:
                    if e.key < 256:
                        char = chr(e.key)
                        self.serial_write("<" + char)
                    self.special_message = None
                    if self.current_char == char:
                        self.text_message = None
                        
        if self.special_message:
            self.special_message.pos.center = (
                VIEWPORT_SIZE[0] / 2,
                self.special_message.pos.height - 10)
            self.window.blit(self.special_message.surf, self.special_message.pos)
        
        if self.mode == Game.MODE_MIDI:
            text_message = ""
            for note in self.pressed_notes_text:
                text_message += self.note_to_text(note) + " "
            if text_message:
                text_message = text_message[:-len(" ")]
                self.text_message = self.big_font.render(text_message, NOALIAS, Colors.WHITE)
            else:
                self.text_message = None
        
        if self.text_message:
            self.window.blit(self.text_message, 
                (0, 
                VIEWPORT_SIZE[1] / 2 - self.text_message.get_rect().centery))
                
        # drawing keyboard
        for note in self.notes:
            if note.note_number in self.pressed_notes:
                note.change_state()
                note.repaint()
                self.pressed_notes.remove(note.note_number)
            if note.note_number in self.released_notes:
                note.change_state()
                note.repaint()
                self.released_notes.remove(note.note_number)
            if self.pixels_to_move_keyboard > 0:
                note.pos.x -= 7
            elif self.pixels_to_move_keyboard < 0:
                note.pos.x += 7
            self.window.blit(note.surf, note.pos)
        if self.pixels_to_move_keyboard > 0:
            self.pixels_to_move_keyboard -= 7
        elif self.pixels_to_move_keyboard < 0:
            self.pixels_to_move_keyboard += 7
            
        # debug drawing of note numbers on top of notes
        if DEBUG:
            for note in self.notes:
                note_number = self.key_font.render(str(note.note_number), NOALIAS, Colors.RED)
                note_pos = (note.pos.centerx - note_number.get_rect().width / 2 + 1,
                            note.pos.bottom - note_number.get_rect().height)
                self.window.blit(note_number, note_pos)

    def handle_events_quit(self, event):
        if event.type == QUIT or (event.type == KEYDOWN and event.key == K_ESCAPE):
            self.midi.close()
            sys.exit()

    def handle_events_fullscreen(self, event):
        if event.type == KEYDOWN and event.mod == KMOD_LALT and event.key == K_RETURN:
            self.fullscreen = not self.fullscreen
            if self.fullscreen:
                self.real_window = display.set_mode(Screen.monitor_rect.size, FULLSCREEN)
            else:
                self.real_window = display.set_mode(Screen.window_rect.size, WINDOWED+RESIZABLE)

    def handle_events_resize(self, event):
        if event.type == VIDEORESIZE:
            # must reset the display when resizing the window, or else you
            # can't draw on the new parts of the window.
            if self.fullscreen:
                self.real_window = display.set_mode(Screen.monitor_rect.size, FULLSCREEN, BIT_DEPTH)
            else:
                # force 16:9 window size
                Screen.window_rect.size = (event.dict['size'][0], event.dict['size'][0] * 9 / 16)
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
        if char == ";": return self.octave * 12 + 15
        if char == "/": return self.octave * 12 + 16
        
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
        if char == "[": return self.octave * 12 + 29
        if char == "=": return self.octave * 12 + 30
        if char == "]": return self.octave * 12 + 31
        
        return INVALID_NOTE

    def note_to_text(self, note):
        basenote = note % 12
        relative_octave = str(note // 12)
        if basenote == 0:  name = "C"
        if basenote == 1:  name = "C#"
        if basenote == 2:  name = "D"
        if basenote == 3:  name = "D#"
        if basenote == 4:  name = "E"
        if basenote == 5:  name = "F"
        if basenote == 6:  name = "F#"
        if basenote == 7:  name = "G"
        if basenote == 8:  name = "G#"
        if basenote == 9:  name = "A"
        if basenote == 10: name = "A#"
        if basenote == 11: name = "B"
        return name + relative_octave

    def serial_write(self, chars):
        self.ser.write(bytes(chars, "utf-8"))
        
class Message():
    def __init__(self, text=" ", text_color=Colors.WHITE, bg_color=Colors.BLACK):
        message_font = font.SysFont("monospace", 24)
        text_message = message_font.render(text, NOALIAS, text_color)
        self.text = text
        self.surf = surface.Surface(
            (text_message.get_rect().width + 20, 
            text_message.get_rect().height + 20))
        self.surf.fill(bg_color)
        self_rect = self.surf.get_rect()
        draw.polygon(self.surf, Colors.WHITE, (
            (0, 0), 
            (self_rect.width - 1, 0), 
            (self_rect.width - 1, self_rect.height - 1), 
            (0, self_rect.height - 1)
            ), 1)
        
        self.surf.blit(text_message, (10, text_message.get_rect().centery - 2))
        self.pos = rect.Rect(0, 0, self_rect.width, self_rect.height)
        
class KeyboardNote():
    BLACK_KEY, WHITE_KEY = list(range(2))

    COLOR_WHITE_FG = (255, 255, 230)
    COLOR_WHITE_BORDER = (255, 255, 255)
    SIZE_WHITE_KEY = (16, 70)
    COLOR_BLACK_FG = (30, 30, 30)
    COLOR_BLACK_BORDER = (100, 100, 100)
    SIZE_BLACK_KEY = (11, 40)
    
    def __init__(self, black_or_white=None, note_number=0):
        self.black_or_white = black_or_white
        self.note_number = note_number
        self.pressed_state = False
        self.repaint()
        if self.black_or_white == KeyboardNote.WHITE_KEY:
            size = KeyboardNote.SIZE_WHITE_KEY
        if self.black_or_white == KeyboardNote.BLACK_KEY:
            size = KeyboardNote.SIZE_BLACK_KEY
        self.pos = rect.Rect(0, 0, size[0], size[1])
        
    def change_state(self):
        self.pressed_state =  not self.pressed_state
        
    def repaint(self):
        if self.black_or_white == KeyboardNote.WHITE_KEY:
            key_color = KeyboardNote.COLOR_WHITE_FG
            border_color = KeyboardNote.COLOR_WHITE_BORDER
            highlight_color = Colors.TEAL
            size = KeyboardNote.SIZE_WHITE_KEY
        if self.black_or_white == KeyboardNote.BLACK_KEY:
            key_color = KeyboardNote.COLOR_BLACK_FG
            border_color = KeyboardNote.COLOR_BLACK_BORDER
            size = KeyboardNote.SIZE_BLACK_KEY
            highlight_color = Colors.TEAL
        self.surf = surface.Surface(size)
        if self.pressed_state:
            key_color = highlight_color
        self.surf.fill(key_color)
        self_rect = self.surf.get_rect()
        draw.polygon(self.surf, border_color, (
            (0, 0), 
            (self_rect.width - 1, 0), 
            (self_rect.width - 1, self_rect.height - 1), 
            (0, self_rect.height - 1)
            ), 1)
        

if __name__ == "__main__":
    game = Game()
    game.main()

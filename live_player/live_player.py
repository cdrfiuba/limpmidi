# -*- coding: iso-8859-1 -*-
from pygame import *
import random
import time as system_time
import os
import sys
import serial
import serial.tools.list_ports
import sys
import platform

VERSION = 0.1

SCREEN_SIZE = (800, 600)

WHITE = (255, 255, 255)
GRAY = (60, 60, 60)
BLACK = (0, 0, 0)
DARK_BLUE = (0, 10, 80)
DARK_RED = (80, 10, 0)
RED = (220, 0, 0)
NO_TRANSPARENCY = 255

RECT_INIT = (0, 0)

LEFT_MOUSE_BUTTON = 1

NOALIAS = 1
WINDOWED = 0
BIT_DEPTH = 32

DEBUG = True

MAIN_PATH = os.path.dirname(__file__)

class Game():
    def main(self):
        # inits
        display.init()
        font.init()

        # serial
        bps = 2000000
        port_number = sys.argv[1]
        if platform.system() == "Linux":
            port_name = "/dev/tty%s"
        elif platform.system() == "Windows":
            port_name = "COM%s"
        port = port_name % port_number
        self.ser = serial.Serial(port, bps, timeout=1)

        self.window = display.set_mode(SCREEN_SIZE, WINDOWED, BIT_DEPTH)
        display.set_caption("limpmidi")

        clock = time.Clock()
        self.osd_font = font.SysFont("Courier", 16)
        self.title_font = font.SysFont("Courier", 24) # @800x600, max 56 chars
        self.end_font_large = font.SysFont("Courier", 48)
        self.end_font_small = font.SysFont("Courier", 40)
        ticks = 0

        old_time = system_time.time()
        fps_counter = 0
        fps = 0

        self.show_error_message = False
        self.error_message = {}
        self.error_message["error"] = Message("Error")
        
        self.message_time = system_time.time()

        self.intro_title = surface.Surface(SCREEN_SIZE)
        self.intro_title.fill(BLACK)
        
        # main cycle
        while True:
            # time
            clock.tick(60)
            ticks = time.get_ticks()

            # background
            bg = surface.Surface(SCREEN_SIZE)
            bg.fill(GRAY)
            self.window.blit(bg, RECT_INIT)
            
            self.handle_game()

            # debug fps
            if DEBUG:
                text_ticks = self.osd_font.render("delta time: %04d ms" %
                        (time.get_ticks() - ticks), NOALIAS, WHITE)
                self.window.blit(text_ticks, (5, 5))

                fps_counter += 1
                if fps > 0:
                    text_fps = self.osd_font.render("fps: %d (~%.2f ms)" % (fps, 1000.0 / fps), NOALIAS, WHITE)
                    self.window.blit(text_fps, (5, 20))
                if system_time.time() - old_time >= 1:
                    fps = fps_counter
                    fps_counter = 0
                    old_time = system_time.time()

            # update window
            display.update()

    def handle_game(self):
        # blits

        # inputs
        for e in event.get():
            char = ""
            if e.type == QUIT or (e.type == KEYDOWN and e.key == K_ESCAPE):
                quit()
                sys.exit()
            elif e.type == KEYDOWN:
                if (e.key < 256): char = chr(e.key)
                self.show_error_message = True
                self.error_message["error"] = Message(char)
                # especiales
                if e.key == K_F1:
                    self.ser.write("!f")
                    self.error_message["error"] = Message("MOTOR FORWARD")
                elif e.key == K_F2:
                    self.ser.write("!b")
                    self.error_message["error"] = Message("MOTOR BACKWARD")
                elif e.key == K_F3:
                    self.ser.write("!m2")
                    self.error_message["error"] = Message("MAXIMUM VOLUME")
                elif e.key == K_F4:
                    self.ser.write("!m160")
                    self.error_message["error"] = Message("MAXIMUM MOVEMENT")
                else:
                    self.ser.write(">" + char)
            elif e.type == KEYUP:
                if (e.key < 256): char = chr(e.key)
                self.ser.write("<" + char)
                self.show_error_message = False
                        
        # auto-hiding error message
        if self.show_error_message:
            self.error_message["error"].pos.center = (
                SCREEN_SIZE[0] / 2 ,
                SCREEN_SIZE[1] - self.error_message["error"].pos.height - 10)
            self.window.blit(self.error_message["error"].surf, self.error_message["error"].pos)
            #if system_time.time() - self.message_time > 5:
            #    self.show_error_message = False
    
class Message():
    def __init__(self, text=" ", text_color=WHITE, bg_color=DARK_RED):
        message_font = font.SysFont("Courier", 24)
        text_message = message_font.render(text, NOALIAS, text_color)
        self.surf = surface.Surface(
            (text_message.get_rect().width + 20, 
            text_message.get_rect().height + 20))
        self.surf.fill(bg_color)
        draw.polygon(self.surf, WHITE, (
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
        print("Falta el puerto (tty USB o COM).")
        print("Puertos disponibles:")
        available_ports = list(serial.tools.list_ports.comports())
        print "\n".join([str(x) for x in available_ports])
        sys.exit()
    
    game = Game()
    game.main()


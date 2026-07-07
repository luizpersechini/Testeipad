#!/usr/bin/env python3
"""
Command & Conquer: Tiberian Dawn - Terminal Demo
================================================
Interactive demo that runs in any terminal. No dependencies beyond Python 3.

Controls:
  Arrow keys / WASD  - Scroll map
  1-3               - Select unit group
  SPACE             - Pause/unpause
  T                 - Toggle tiberium growth
  B                 - Build animation
  Q / ESC           - Quit

Run: python3 terminal_demo.py
"""

import sys
import os
import time
import random
import math
import select
import termios
import tty

# ============================================================
# ANSI Terminal Helpers
# ============================================================

class Term:
    """ANSI terminal control."""
    CLEAR = "\033[2J"
    HOME = "\033[H"
    HIDE_CURSOR = "\033[?25l"
    SHOW_CURSOR = "\033[?25h"
    RESET = "\033[0m"
    BOLD = "\033[1m"
    DIM = "\033[2m"

    @staticmethod
    def goto(x, y):
        return f"\033[{y+1};{x+1}H"

    @staticmethod
    def fg(r, g, b):
        return f"\033[38;2;{r};{g};{b}m"

    @staticmethod
    def bg(r, g, b):
        return f"\033[48;2;{r};{g};{b}m"

    @staticmethod
    def get_size():
        try:
            cols, rows = os.get_terminal_size()
            return cols, rows
        except:
            return 80, 24


def getch_nonblocking():
    """Non-blocking keyboard read."""
    if select.select([sys.stdin], [], [], 0.0)[0]:
        return sys.stdin.read(1)
    return None


# ============================================================
# Game Colors
# ============================================================

class Colors:
    # Terrain
    CLEAR = (100, 80, 50)
    SAND = (160, 140, 80)
    ROCK = (85, 80, 70)
    WATER = (25, 50, 130)
    ROAD = (90, 85, 75)
    ROUGH = (75, 65, 45)
    CLIFF = (60, 55, 45)

    # Tiberium
    TIB_GREEN = (0, 200, 0)
    TIB_DARK = (0, 140, 0)
    TIB_LIGHT = (40, 255, 40)

    # Factions
    GDI_GOLD = (218, 165, 32)
    GDI_DARK = (140, 105, 20)
    NOD_RED = (180, 0, 0)
    NOD_DARK = (120, 0, 0)

    # UI
    SIDEBAR_BG = (30, 30, 35)
    SIDEBAR_BORDER = (80, 80, 80)
    CREDITS_GREEN = (0, 200, 0)
    POWER_GREEN = (0, 180, 0)
    POWER_RED = (255, 50, 50)
    TEXT_WHITE = (220, 220, 220)
    TEXT_DIM = (120, 120, 120)
    WARNING_RED = (255, 80, 80)
    READY_GREEN = (0, 255, 0)
    PROGRESS_CYAN = (0, 200, 220)

    # Effects
    EXPLOSION_1 = (255, 200, 50)
    EXPLOSION_2 = (255, 120, 20)
    EXPLOSION_3 = (200, 60, 0)
    MUZZLE_FLASH = (255, 255, 150)
    BULLET_TRAIL = (255, 255, 100)


# ============================================================
# Map Generation
# ============================================================

MAP_W = 48
MAP_H = 32

TERRAIN_CHARS = {
    'clear': ('░', Colors.CLEAR),
    'sand': ('░', Colors.SAND),
    'rock': ('▓', Colors.ROCK),
    'water': ('≈', Colors.WATER),
    'road': ('─', Colors.ROAD),
    'rough': ('▒', Colors.ROUGH),
    'cliff': ('█', Colors.CLIFF),
    'tiberium': ('♦', Colors.TIB_GREEN),
}

def generate_map():
    """Generate a terrain map with tiberium fields."""
    terrain = [['clear'] * MAP_W for _ in range(MAP_H)]

    # River
    rx = MAP_W // 2
    for y in range(MAP_H):
        rx += random.randint(-1, 1)
        rx = max(2, min(MAP_W - 3, rx))
        for dx in range(-1, 2):
            if 0 <= rx + dx < MAP_W:
                terrain[y][rx + dx] = 'water'

    # Roads
    road_y = MAP_H // 3
    for x in range(MAP_W):
        if terrain[road_y][x] != 'water':
            terrain[road_y][x] = 'road'

    # Rocks and cliffs
    for _ in range(20):
        rx, ry = random.randint(0, MAP_W-1), random.randint(0, MAP_H-1)
        if terrain[ry][rx] == 'clear':
            terrain[ry][rx] = 'rock'
            for dx, dy in [(-1,0),(1,0),(0,-1),(0,1)]:
                nx, ny = rx+dx, ry+dy
                if 0<=nx<MAP_W and 0<=ny<MAP_H and terrain[ny][nx]=='clear' and random.random()<0.4:
                    terrain[ny][nx] = 'rock'

    # Sand patches
    for _ in range(8):
        cx, cy = random.randint(0, MAP_W-1), random.randint(0, MAP_H-1)
        for dx in range(-2, 3):
            for dy in range(-2, 3):
                nx, ny = cx+dx, cy+dy
                if 0<=nx<MAP_W and 0<=ny<MAP_H and terrain[ny][nx]=='clear' and random.random()<0.6:
                    terrain[ny][nx] = 'sand'

    # Tiberium fields
    tib_fields = [(MAP_W//2 - 5, MAP_H//2), (MAP_W//2 + 5, MAP_H//2 - 3),
                  (MAP_W//4, MAP_H//4*3)]
    for fx, fy in tib_fields:
        for dx in range(-3, 4):
            for dy in range(-2, 3):
                nx, ny = fx+dx, fy+dy
                if 0<=nx<MAP_W and 0<=ny<MAP_H and terrain[ny][nx] not in ('water','cliff','rock'):
                    dist = abs(dx) + abs(dy)
                    if dist <= 3 and random.random() < 0.7:
                        terrain[ny][nx] = 'tiberium'

    return terrain


# ============================================================
# Game Entities
# ============================================================

class Unit:
    def __init__(self, name, char, x, y, faction, hp, speed, weapon_range=3):
        self.name = name
        self.char = char
        self.x = float(x)
        self.y = float(y)
        self.faction = faction  # 'gdi' or 'nod'
        self.hp = hp
        self.max_hp = hp
        self.speed = speed
        self.weapon_range = weapon_range
        self.target_x = x
        self.target_y = y
        self.selected = False
        self.alive = True
        self.fire_cooldown = 0
        self.mission = 'guard'  # guard, move, attack, harvest
        self.attack_target = None
        self.harvesting = False
        self.tib_load = 0
        self.group = 0

    @property
    def ix(self): return int(self.x)
    @property
    def iy(self): return int(self.y)

    def color(self):
        if self.faction == 'gdi':
            return Colors.GDI_GOLD
        return Colors.NOD_RED

    def update(self, dt, terrain, units, buildings):
        if not self.alive:
            return

        self.fire_cooldown = max(0, self.fire_cooldown - dt)

        if self.mission == 'move':
            dx = self.target_x - self.x
            dy = self.target_y - self.y
            dist = math.sqrt(dx*dx + dy*dy)
            if dist > 0.3:
                self.x += (dx/dist) * self.speed * dt
                self.y += (dy/dist) * self.speed * dt
            else:
                self.x = self.target_x
                self.y = self.target_y
                self.mission = 'guard'

        elif self.mission == 'attack' and self.attack_target:
            t = self.attack_target
            if not t.alive:
                self.attack_target = None
                self.mission = 'guard'
                return
            dx = t.x - self.x
            dy = t.y - self.y
            dist = math.sqrt(dx*dx + dy*dy)
            if dist <= self.weapon_range:
                if self.fire_cooldown <= 0:
                    t.hp -= random.randint(5, 15)
                    if t.hp <= 0:
                        t.alive = False
                    self.fire_cooldown = 0.8
                    return  # Signal: fired
            else:
                self.target_x = t.x
                self.target_y = t.y
                self.x += (dx/dist) * self.speed * dt
                self.y += (dy/dist) * self.speed * dt

        elif self.mission == 'guard':
            # Auto-attack nearby enemies
            for u in units:
                if u.faction != self.faction and u.alive:
                    dx = u.x - self.x
                    dy = u.y - self.y
                    dist = math.sqrt(dx*dx + dy*dy)
                    if dist <= self.weapon_range + 1:
                        self.attack_target = u
                        self.mission = 'attack'
                        break

        elif self.mission == 'harvest':
            tx, ty = int(self.target_x), int(self.target_y)
            if 0<=tx<MAP_W and 0<=ty<MAP_H and terrain[ty][tx] == 'tiberium':
                dx = self.target_x - self.x
                dy = self.target_y - self.y
                dist = math.sqrt(dx*dx + dy*dy)
                if dist > 0.3:
                    self.x += (dx/dist) * self.speed * dt
                    self.y += (dy/dist) * self.speed * dt
                else:
                    self.tib_load += dt * 200
                    if self.tib_load >= 700:
                        self.tib_load = 700
                        self.mission = 'guard'  # Full, return


class Building:
    def __init__(self, name, char, x, y, w, h, faction, hp, power=0):
        self.name = name
        self.char = char
        self.x = x
        self.y = y
        self.w = w
        self.h = h
        self.faction = faction
        self.hp = hp
        self.max_hp = hp
        self.power = power  # positive = output, negative = drain
        self.alive = True
        self.under_construction = False
        self.construction_progress = 1.0

    def color(self):
        if self.faction == 'gdi':
            return Colors.GDI_DARK
        return Colors.NOD_DARK

    def border_color(self):
        if self.faction == 'gdi':
            return Colors.GDI_GOLD
        return Colors.NOD_RED


class Explosion:
    def __init__(self, x, y):
        self.x = x
        self.y = y
        self.age = 0
        self.lifetime = 0.6

    @property
    def alive(self):
        return self.age < self.lifetime


class Projectile:
    def __init__(self, x1, y1, x2, y2):
        self.x = float(x1)
        self.y = float(y1)
        self.tx = float(x2)
        self.ty = float(y2)
        self.age = 0
        self.lifetime = 0.15
        self.alive = True


# ============================================================
# Game State
# ============================================================

class GameState:
    def __init__(self):
        self.terrain = generate_map()
        self.units = []
        self.buildings = []
        self.explosions = []
        self.projectiles = []
        self.cam_x = 0
        self.cam_y = 0
        self.credits_gdi = 5000
        self.credits_nod = 5000
        self.power_output = 200
        self.power_drain = 120
        self.paused = False
        self.game_time = 0
        self.selected_group = 0
        self.messages = []
        self.build_queue = []
        self.build_progress = 0
        self.building_item = None
        self.tib_growth = True

        self._setup_bases()

    def _setup_bases(self):
        # GDI base (top-left area)
        bx, by = 3, 3
        self.buildings.append(Building("CY", "⌂", bx, by, 2, 2, 'gdi', 600, 0))
        self.buildings.append(Building("PP", "⚡", bx+3, by, 2, 2, 'gdi', 400, 100))
        self.buildings.append(Building("REF", "⛽", bx, by+3, 2, 2, 'gdi', 500, -30))
        self.buildings.append(Building("BAR", "♜", bx+3, by+3, 2, 2, 'gdi', 400, -20))
        self.buildings.append(Building("WF", "⚙", bx+6, by, 2, 2, 'gdi', 500, -30))

        # GDI units
        u1 = Unit("Med Tank", "▣", bx+8, by+2, 'gdi', 120, 2.5, 4)
        u1.group = 1
        u2 = Unit("Med Tank", "▣", bx+9, by+3, 'gdi', 120, 2.5, 4)
        u2.group = 1
        u3 = Unit("Mammoth", "▣", bx+8, by+4, 'gdi', 200, 1.8, 5)
        u3.group = 1
        u4 = Unit("Humvee", "▢", bx+7, by+6, 'gdi', 80, 4.0, 3)
        u4.group = 2
        u5 = Unit("Harvester", "▤", bx+2, by+5, 'gdi', 100, 1.5, 0)
        u5.group = 3
        u5.mission = 'harvest'
        u5.target_x = MAP_W//4
        u5.target_y = MAP_H//4*3
        self.units.extend([u1, u2, u3, u4, u5])

        # Infantry
        for i in range(3):
            inf = Unit("Rifle", "•", bx+4+i*0.5, by+5, 'gdi', 50, 2.0, 2)
            inf.group = 2
            self.units.append(inf)

        # NOD base (bottom-right area)
        nx, ny = MAP_W - 10, MAP_H - 8
        self.buildings.append(Building("CY", "⌂", nx, ny, 2, 2, 'nod', 600, 0))
        self.buildings.append(Building("PP", "⚡", nx+3, ny, 2, 2, 'nod', 400, 100))
        self.buildings.append(Building("HoN", "♞", nx, ny+3, 2, 2, 'nod', 400, -20))
        self.buildings.append(Building("OBL", "☼", nx+3, ny+3, 1, 1, 'nod', 300, -60))
        self.buildings.append(Building("AF", "✈", nx+5, ny, 2, 2, 'nod', 400, -30))

        # NOD units
        n1 = Unit("Lt Tank", "▣", nx-3, ny+1, 'nod', 90, 3.0, 3)
        n2 = Unit("Lt Tank", "▣", nx-4, ny+2, 'nod', 90, 3.0, 3)
        n3 = Unit("Buggy", "▢", nx-2, ny+4, 'nod', 60, 4.5, 2)
        n4 = Unit("Bike", "◇", nx-3, ny+5, 'nod', 40, 5.0, 3)
        n5 = Unit("Flame Tk", "▣", nx-5, ny+3, 'nod', 100, 2.5, 2)
        self.units.extend([n1, n2, n3, n4, n5])

        for i in range(4):
            inf = Unit("Militia", "•", nx+1+i*0.5, ny+5, 'nod', 40, 2.0, 2)
            self.units.append(inf)

        # Set NOD units to patrol toward center
        for u in self.units:
            if u.faction == 'nod' and u.name != 'Militia':
                u.mission = 'move'
                u.target_x = MAP_W//2 + random.randint(-5, 5)
                u.target_y = MAP_H//2 + random.randint(-3, 3)

        # Build queue
        self.build_queue = [
            ("Ref", 2000, 0.75),
            ("GT", 500, 0.0),
        ]
        self.building_item = "Refinery"
        self.build_progress = 0.75

        self.add_message("Mission: Destroy the Nod base", Colors.GDI_GOLD)
        self.add_message("Construction in progress...", Colors.CREDITS_GREEN)

    def add_message(self, text, color=Colors.TEXT_WHITE):
        self.messages.append((text, color, time.time()))
        if len(self.messages) > 5:
            self.messages.pop(0)

    def update(self, dt):
        if self.paused:
            return

        self.game_time += dt

        # Update units
        for u in self.units:
            old_cd = u.fire_cooldown
            u.update(dt, self.terrain, self.units, self.buildings)
            # Check if unit just fired
            if old_cd <= 0 and u.fire_cooldown > 0 and u.attack_target and u.attack_target.alive:
                self.projectiles.append(Projectile(u.x, u.y,
                                                     u.attack_target.x, u.attack_target.y))

        # Check for dead units -> explosions
        for u in list(self.units):
            if not u.alive:
                self.explosions.append(Explosion(u.x, u.y))
                faction = "GDI" if u.faction == 'gdi' else "Nod"
                self.add_message(f"{faction} {u.name} destroyed!", Colors.WARNING_RED)

        self.units = [u for u in self.units if u.alive]

        # Update explosions
        for e in self.explosions:
            e.age += dt
        self.explosions = [e for e in self.explosions if e.alive]

        # Update projectiles
        for p in self.projectiles:
            p.age += dt
            if p.age >= p.lifetime:
                p.alive = False
        self.projectiles = [p for p in self.projectiles if p.alive]

        # Build progress
        if self.build_progress < 1.0 and self.building_item:
            self.build_progress += dt * 0.08
            if self.build_progress >= 1.0:
                self.build_progress = 1.0
                self.add_message("Construction Complete", Colors.READY_GREEN)

        # Credits ticking
        alive_harvesters = [u for u in self.units if u.faction=='gdi' and u.name=='Harvester']
        for h in alive_harvesters:
            if h.tib_load > 0 and h.mission == 'guard':
                self.credits_gdi += int(h.tib_load)
                h.tib_load = 0
                self.add_message(f"+${int(h.tib_load)} credits", Colors.CREDITS_GREEN)

        # Tiberium growth
        if self.tib_growth and random.random() < dt * 0.3:
            for y in range(MAP_H):
                for x in range(MAP_W):
                    if self.terrain[y][x] == 'tiberium':
                        for dx, dy in [(-1,0),(1,0),(0,-1),(0,1)]:
                            nx, ny = x+dx, y+dy
                            if (0<=nx<MAP_W and 0<=ny<MAP_H and
                                self.terrain[ny][nx] in ('clear','sand') and
                                random.random() < 0.02):
                                self.terrain[ny][nx] = 'tiberium'


# ============================================================
# Renderer
# ============================================================

class TerminalRenderer:
    def __init__(self):
        self.cols, self.rows = Term.get_size()
        self.sidebar_w = 22
        self.game_w = self.cols - self.sidebar_w
        self.game_h = self.rows - 3  # Reserve bottom for messages
        self.buf = []

    def refresh_size(self):
        self.cols, self.rows = Term.get_size()
        self.game_w = self.cols - self.sidebar_w
        self.game_h = self.rows - 3

    def render(self, state):
        self.buf = []
        self.buf.append(Term.HIDE_CURSOR)
        self.buf.append(Term.HOME)

        self._render_terrain(state)
        self._render_buildings(state)
        self._render_units(state)
        self._render_effects(state)
        self._render_sidebar(state)
        self._render_messages(state)

        self.buf.append(Term.RESET)
        sys.stdout.write(''.join(self.buf))
        sys.stdout.flush()

    def _put(self, x, y, char, fg, bg=None):
        if x < 0 or x >= self.cols or y < 0 or y >= self.rows:
            return
        s = Term.goto(x, y)
        s += Term.fg(*fg)
        if bg:
            s += Term.bg(*bg)
        s += char
        s += Term.RESET
        self.buf.append(s)

    def _put_str(self, x, y, text, fg, bg=None):
        for i, c in enumerate(text):
            if x + i >= self.cols:
                break
            self._put(x + i, y, c, fg, bg)

    def _render_terrain(self, state):
        for sy in range(self.game_h):
            my = state.cam_y + sy
            row_buf = Term.goto(0, sy)
            for sx in range(self.game_w):
                mx = state.cam_x + sx
                if 0 <= mx < MAP_W and 0 <= my < MAP_H:
                    t = state.terrain[my][mx]
                    char, color = TERRAIN_CHARS.get(t, ('?', (255,0,255)))
                    # Add variation
                    r, g, b = color
                    v = ((mx * 7 + my * 13) % 20) - 10
                    r = max(0, min(255, r + v))
                    g = max(0, min(255, g + v))
                    b = max(0, min(255, b + v))

                    if t == 'tiberium':
                        # Animate tiberium shimmer
                        phase = (state.game_time * 2 + mx * 0.5 + my * 0.3) % 1.0
                        if phase < 0.3:
                            char = '♦'
                            g = min(255, g + 40)
                        elif phase < 0.6:
                            char = '◆'
                        else:
                            char = '♦'

                    # Darken based on distance from GDI base (fog)
                    dist = math.sqrt((mx - 8)**2 + (my - 6)**2)
                    if dist > 20:
                        r = r // 3; g = g // 3; b = b // 3
                        char = ' '
                    elif dist > 15:
                        r = r * 2 // 3; g = g * 2 // 3; b = b * 2 // 3

                    row_buf += Term.fg(r, g, b) + char
                else:
                    row_buf += Term.fg(20, 20, 20) + '·'
            row_buf += Term.RESET
            self.buf.append(row_buf)

    def _render_buildings(self, state):
        for b in state.buildings:
            if not b.alive:
                continue
            for dy in range(b.h):
                for dx in range(b.w):
                    sx = b.x + dx - state.cam_x
                    sy = b.y + dy - state.cam_y
                    if 0 <= sx < self.game_w and 0 <= sy < self.game_h:
                        if dx == 0 and dy == 0:
                            self._put(sx, sy, b.char, b.border_color(), b.color())
                        else:
                            self._put(sx, sy, '▪', b.border_color(), b.color())

            # Building name above
            sx = b.x - state.cam_x
            sy = b.y - 1 - state.cam_y
            if 0 <= sx < self.game_w - 3 and 0 <= sy < self.game_h:
                self._put_str(sx, sy, b.name[:3], b.border_color())

            # HP bar
            if b.hp < b.max_hp:
                sx = b.x - state.cam_x
                sy = b.y + b.h - state.cam_y
                if 0 <= sy < self.game_h:
                    ratio = b.hp / b.max_hp
                    bar_w = b.w * 2
                    filled = int(bar_w * ratio)
                    for i in range(bar_w):
                        if sx + i < self.game_w:
                            c = Colors.POWER_GREEN if i < filled else (80, 0, 0)
                            self._put(sx + i, sy, '▬', c)

    def _render_units(self, state):
        for u in state.units:
            sx = int(u.x) - state.cam_x
            sy = int(u.y) - state.cam_y
            if 0 <= sx < self.game_w and 0 <= sy < self.game_h:
                color = u.color()
                # Brighten if selected
                if u.selected:
                    color = (min(255, color[0]+60), min(255, color[1]+60), min(255, color[2]+60))

                self._put(sx, sy, u.char, color)

                # Selection indicator
                if u.selected and sx > 0:
                    self._put(sx - 1, sy, '›', Colors.READY_GREEN)

                # HP bar for damaged units
                if u.hp < u.max_hp:
                    if sy > 0:
                        ratio = u.hp / u.max_hp
                        c = Colors.POWER_GREEN if ratio > 0.5 else Colors.WARNING_RED
                        bar_char = '▮' if ratio > 0.5 else '▯'
                        self._put(sx, sy - 1, bar_char, c)

                # Harvester tiberium indicator
                if u.name == 'Harvester' and u.tib_load > 0:
                    load_pct = u.tib_load / 700
                    self._put(sx + 1, sy, f'{int(load_pct*100)}%'[0], Colors.TIB_GREEN)

    def _render_effects(self, state):
        # Projectiles
        for p in state.projectiles:
            t = p.age / p.lifetime
            px = p.x + (p.tx - p.x) * t
            py = p.y + (p.ty - p.y) * t
            sx = int(px) - state.cam_x
            sy = int(py) - state.cam_y
            if 0 <= sx < self.game_w and 0 <= sy < self.game_h:
                self._put(sx, sy, '*', Colors.MUZZLE_FLASH)

        # Explosions
        for e in state.explosions:
            sx = int(e.x) - state.cam_x
            sy = int(e.y) - state.cam_y
            if 0 <= sx < self.game_w and 0 <= sy < self.game_h:
                phase = e.age / e.lifetime
                if phase < 0.3:
                    self._put(sx, sy, '✸', Colors.EXPLOSION_1)
                elif phase < 0.6:
                    self._put(sx, sy, '✦', Colors.EXPLOSION_2)
                    for dx, dy in [(-1,0),(1,0),(0,-1),(0,1)]:
                        self._put(sx+dx, sy+dy, '·', Colors.EXPLOSION_3)
                else:
                    self._put(sx, sy, '·', Colors.EXPLOSION_3)

    def _render_sidebar(self, state):
        sx = self.game_w
        bg = Colors.SIDEBAR_BG

        # Fill sidebar background
        for y in range(self.rows):
            self._put_str(sx, y, ' ' * self.sidebar_w, Colors.TEXT_WHITE, bg)

        # Border
        for y in range(self.rows):
            self._put(sx, y, '│', Colors.SIDEBAR_BORDER)

        y = 0
        # Title
        self._put_str(sx+1, y, "C&C TIBERIAN DAWN", Colors.GDI_GOLD, bg)
        y += 1
        self._put_str(sx+1, y, "─" * (self.sidebar_w-2), Colors.SIDEBAR_BORDER, bg)
        y += 1

        # Credits
        self._put_str(sx+1, y, f"Credits: ${state.credits_gdi}", Colors.CREDITS_GREEN, bg)
        y += 1

        # Power
        ratio = state.power_output / max(1, state.power_drain)
        power_color = Colors.POWER_GREEN if ratio >= 1.0 else Colors.WARNING_RED
        power_str = f"Power: {state.power_output}/{state.power_drain}"
        self._put_str(sx+1, y, power_str, power_color, bg)
        y += 1

        # Power bar
        bar_w = self.sidebar_w - 3
        filled = min(bar_w, int(bar_w * min(1.0, ratio)))
        bar = '█' * filled + '░' * (bar_w - filled)
        self._put_str(sx+1, y, bar, power_color, bg)
        y += 1

        self._put_str(sx+1, y, "─" * (self.sidebar_w-2), Colors.SIDEBAR_BORDER, bg)
        y += 1

        # Minimap
        self._put_str(sx+1, y, "  TACTICAL MAP", Colors.TEXT_DIM, bg)
        y += 1
        mini_w = self.sidebar_w - 3
        mini_h = 8
        for my in range(mini_h):
            for mx in range(mini_w):
                # Map to world coords
                wx = int(mx * MAP_W / mini_w)
                wy = int(my * MAP_H / mini_h)
                t = state.terrain[wy][wx] if 0<=wy<MAP_H and 0<=wx<MAP_W else 'clear'

                if t == 'water':
                    c = (30, 50, 120)
                elif t == 'tiberium':
                    c = (0, 130, 0)
                elif t == 'rock':
                    c = (70, 65, 55)
                else:
                    c = (70, 55, 35)

                # Buildings on minimap
                for b in state.buildings:
                    if b.alive and abs(wx - b.x) <= 1 and abs(wy - b.y) <= 1:
                        c = Colors.GDI_GOLD if b.faction == 'gdi' else Colors.NOD_RED
                        break

                # Units on minimap
                for u in state.units:
                    if abs(wx - int(u.x)) <= 1 and abs(wy - int(u.y)) <= 1:
                        c = Colors.GDI_GOLD if u.faction == 'gdi' else Colors.NOD_RED
                        break

                self._put(sx + 1 + mx, y + my, '▪', c, bg)

        # Viewport box
        vx1 = int(state.cam_x * mini_w / MAP_W)
        vy1 = int(state.cam_y * mini_h / MAP_H)
        vx2 = int((state.cam_x + self.game_w) * mini_w / MAP_W)
        vy2 = int((state.cam_y + self.game_h) * mini_h / MAP_H)
        for vx in range(max(0,vx1), min(mini_w, vx2+1)):
            self._put(sx+1+vx, y+vy1, '─', Colors.TEXT_WHITE, bg)
            self._put(sx+1+vx, y+min(mini_h-1,vy2), '─', Colors.TEXT_WHITE, bg)

        y += mini_h
        self._put_str(sx+1, y, "─" * (self.sidebar_w-2), Colors.SIDEBAR_BORDER, bg)
        y += 1

        # Build queue
        self._put_str(sx+1, y, "BUILD QUEUE", Colors.TEXT_DIM, bg)
        y += 1

        if state.building_item:
            self._put_str(sx+1, y, state.building_item[:18], Colors.TEXT_WHITE, bg)
            y += 1

            # Progress bar
            prog_w = self.sidebar_w - 3
            filled = int(prog_w * state.build_progress)
            if state.build_progress >= 1.0:
                self._put_str(sx+1, y, "█" * prog_w, Colors.READY_GREEN, bg)
                y += 1
                self._put_str(sx+1, y, "  ▶ READY ◀", Colors.READY_GREEN, bg)
            else:
                bar = '█' * filled + '░' * (prog_w - filled)
                self._put_str(sx+1, y, bar, Colors.PROGRESS_CYAN, bg)
                y += 1
                pct = f"{int(state.build_progress*100)}%"
                self._put_str(sx+1, y, pct, Colors.PROGRESS_CYAN, bg)
            y += 1

        self._put_str(sx+1, y, "─" * (self.sidebar_w-2), Colors.SIDEBAR_BORDER, bg)
        y += 1

        # Selected units
        selected = [u for u in state.units if u.selected]
        if selected:
            self._put_str(sx+1, y, f"Selected: {len(selected)}", Colors.TEXT_WHITE, bg)
            y += 1
            for u in selected[:4]:
                hp_bar = f"{u.name[:8]:8s} HP:{u.hp}"
                c = Colors.POWER_GREEN if u.hp > u.max_hp//2 else Colors.WARNING_RED
                self._put_str(sx+1, y, hp_bar[:self.sidebar_w-2], c, bg)
                y += 1
        else:
            self._put_str(sx+1, y, "No selection", Colors.TEXT_DIM, bg)
            y += 1

        y += 1
        self._put_str(sx+1, y, "─" * (self.sidebar_w-2), Colors.SIDEBAR_BORDER, bg)
        y += 1

        # Game time
        mins = int(state.game_time) // 60
        secs = int(state.game_time) % 60
        self._put_str(sx+1, y, f"Time: {mins:02d}:{secs:02d}", Colors.TEXT_DIM, bg)
        y += 1

        # Status
        if state.paused:
            self._put_str(sx+1, y, "  ⏸ PAUSED", Colors.WARNING_RED, bg)
        else:
            gdi_count = sum(1 for u in state.units if u.faction=='gdi')
            nod_count = sum(1 for u in state.units if u.faction=='nod')
            self._put_str(sx+1, y, f"GDI:{gdi_count} NOD:{nod_count}", Colors.TEXT_DIM, bg)

    def _render_messages(self, state):
        y = self.rows - 3
        now = time.time()

        # Controls bar
        self._put_str(0, self.rows - 1,
                      " WASD:Scroll  1-3:Group  SPACE:Pause  B:Build  Q:Quit ",
                      Colors.TEXT_DIM, (20, 20, 25))

        for msg, color, t in state.messages[-3:]:
            age = now - t
            if age < 8:
                # Fade
                fade = max(0.3, 1.0 - (age / 8))
                c = (int(color[0]*fade), int(color[1]*fade), int(color[2]*fade))
                self._put_str(1, y, msg[:self.game_w-2], c)
                y += 1


# ============================================================
# Main Loop
# ============================================================

def main():
    # Setup terminal
    old_settings = termios.tcgetattr(sys.stdin)
    try:
        tty.setcbreak(sys.stdin.fileno())

        state = GameState()
        renderer = TerminalRenderer()

        sys.stdout.write(Term.CLEAR + Term.HIDE_CURSOR)
        sys.stdout.flush()

        last_time = time.time()
        running = True

        while running:
            now = time.time()
            dt = min(0.1, now - last_time)
            last_time = now

            # Input
            key = getch_nonblocking()
            if key:
                if key == 'q' or key == '\x1b':
                    # Check for escape sequence
                    if key == '\x1b':
                        k2 = getch_nonblocking()
                        if k2 == '[':
                            k3 = getch_nonblocking()
                            if k3 == 'A': key = 'UP'
                            elif k3 == 'B': key = 'DOWN'
                            elif k3 == 'C': key = 'RIGHT'
                            elif k3 == 'D': key = 'LEFT'
                        elif k2 is None:
                            running = False
                            continue

                if key in ('w', 'W', 'UP'):
                    state.cam_y = max(0, state.cam_y - 2)
                elif key in ('s', 'S', 'DOWN'):
                    state.cam_y = min(MAP_H - renderer.game_h, state.cam_y + 2)
                elif key in ('a', 'A', 'LEFT'):
                    state.cam_x = max(0, state.cam_x - 3)
                elif key in ('d', 'D', 'RIGHT'):
                    state.cam_x = min(MAP_W - renderer.game_w, state.cam_x + 3)
                elif key == ' ':
                    state.paused = not state.paused
                    if state.paused:
                        state.add_message("Game Paused", Colors.TEXT_WHITE)
                    else:
                        state.add_message("Game Resumed", Colors.TEXT_WHITE)
                elif key == 't':
                    state.tib_growth = not state.tib_growth
                    state.add_message(f"Tiberium growth: {'ON' if state.tib_growth else 'OFF'}",
                                     Colors.TIB_GREEN)
                elif key == 'b':
                    state.build_progress = 0
                    state.building_item = random.choice(["Guard Tower", "Refinery",
                                                           "Barracks", "Power Plant"])
                    state.add_message(f"Building {state.building_item}...",
                                     Colors.PROGRESS_CYAN)
                elif key in ('1', '2', '3'):
                    g = int(key)
                    for u in state.units:
                        u.selected = (u.faction == 'gdi' and u.group == g)
                    count = sum(1 for u in state.units if u.selected)
                    state.add_message(f"Group {g}: {count} units selected", Colors.GDI_GOLD)

            # Update
            state.update(dt)

            # Render
            renderer.refresh_size()
            renderer.render(state)

            # Target ~20 FPS
            elapsed = time.time() - now
            sleep_time = max(0, 0.05 - elapsed)
            time.sleep(sleep_time)

    finally:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
        sys.stdout.write(Term.SHOW_CURSOR + Term.RESET + Term.CLEAR + Term.HOME)
        sys.stdout.flush()
        print("C&C Terminal Demo ended.")


if __name__ == "__main__":
    main()

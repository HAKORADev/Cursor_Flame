import sys
import os
import random
import math
from PyQt5.QtWidgets import QApplication, QWidget
from PyQt5.QtCore import Qt, QTimer, QPointF, QRectF, pyqtSignal, QObject
from PyQt5.QtGui import QPainter, QColor, QRadialGradient, QCursor, QPainterPath, QPen, QBrush, QIcon
from pynput import mouse


def get_icon_path():
    if hasattr(sys, '_MEIPASS'):
        return os.path.join(sys._MEIPASS, 'cf_ico.png')
    return 'cf_ico.png'


class SignalBridge(QObject):
    move_signal = pyqtSignal(float, float)
    click_signal = pyqtSignal(float, float, bool, int)
    scroll_signal = pyqtSignal(float, float, int)


class PynputListener:
    def __init__(self):
        self.bridge = SignalBridge()
        self.move_callback = None
        self.click_callback = None
        self.scroll_callback = None

    def _on_move(self, x, y):
        if self.move_callback:
            self.move_callback(x, y)
        self.bridge.move_signal.emit(float(x), float(y))

    def _on_click(self, x, y, button, pressed):
        if self.click_callback:
            self.click_callback(x, y, pressed, button)
        self.bridge.click_signal.emit(float(x), float(y), pressed, button)

    def _on_scroll(self, x, y, dx, dy):
        if self.scroll_callback:
            self.scroll_callback(x, y, dx, dy)
        self.bridge.scroll_signal.emit(float(x), float(y), dy)

    def start(self):
        self.listener = mouse.Listener(
            on_move=self._on_move,
            on_click=self._on_click,
            on_scroll=self._on_scroll
        )
        self.listener.start()


class M2:
    @staticmethod
    def create(x=0, y=0):
        return [[x, y]]

    @staticmethod
    def add(a, b):
        return [[a[0][0] + b[0][0], a[0][1] + b[0][1]]]

    @staticmethod
    def sub(a, b):
        return [[a[0][0] - b[0][0], a[0][1] - b[0][1]]]

    @staticmethod
    def mul(m, s):
        return [[m[0][0] * s, m[0][1] * s]]

    @staticmethod
    def len(m):
        return math.sqrt(m[0][0] ** 2 + m[0][1] ** 2)

    @staticmethod
    def norm(m):
        l = M2.len(m)
        if l == 0:
            return [[0, 0]]
        return [[m[0][0] / l, m[0][1] / l]]

    @staticmethod
    def lerp(a, b, t):
        return [[a[0][0] + (b[0][0] - a[0][0]) * t,
                 a[0][1] + (b[0][1] - a[0][1]) * t]]


class M4:
    @staticmethod
    def create(r=255, g=255, b=255, a=255):
        return [[r, g, b, a]]

    @staticmethod
    def mul(m, s):
        return [[m[0][0] * s, m[0][1] * s, m[0][2] * s, m[0][3] * s]]

    @staticmethod
    def add(a, b):
        return [[a[0][0] + b[0][0], a[0][1] + b[0][1],
                 a[0][2] + b[0][2], a[0][3] + b[0][3]]]

    @staticmethod
    def clamp(m):
        return [[min(255, max(0, m[0][0])),
                 min(255, max(0, m[0][1])),
                 min(255, max(0, m[0][2])),
                 min(255, max(0, m[0][3]))]]

    @staticmethod
    def lerp(a, b, t):
        return [[a[0][0] + (b[0][0] - a[0][0]) * t,
                 a[0][1] + (b[0][1] - a[0][1]) * t,
                 a[0][2] + (b[0][2] - a[0][2]) * t,
                 a[0][3] + (b[0][3] - a[0][3]) * t]]

    @staticmethod
    def to_qcolor(m):
        return QColor(int(m[0][0]), int(m[0][1]), int(m[0][2]), int(m[0][3]))


class CMatrix:
    KEYFRAMES = [
        [0, [255, 80, 0]],
        [2, [255, 120, 0]],
        [4, [255, 180, 0]],
        [6, [100, 150, 255]],
        [30, [150, 50, 200]],
        [60, [10, 10, 15]]
    ]

    @staticmethod
    def get(elapsed):
        kf = CMatrix.KEYFRAMES
        if elapsed >= kf[-1][0]:
            return M4.create(kf[-1][1][0], kf[-1][1][1], kf[-1][1][2], 220)

        for i in range(len(kf) - 1):
            t0, c0 = kf[i]
            t1, c1 = kf[i + 1]
            if elapsed < t1:
                factor = (elapsed - t0) / (t1 - t0)
                factor = max(0, min(1, factor))
                r = c0[0] + (c1[0] - c0[0]) * factor
                g = c0[1] + (c1[1] - c0[1]) * factor
                b = c0[2] + (c1[2] - c0[2]) * factor
                return M4.create(r, g, b, 220)

        return M4.create(kf[-1][1][0], kf[-1][1][1], kf[-1][1][2], 220)


class PMatrix:
    POOL_SIZE = 200

    def __init__(self):
        self.x = [0] * PMatrix.POOL_SIZE
        self.y = [0] * PMatrix.POOL_SIZE
        self.vx = [0] * PMatrix.POOL_SIZE
        self.vy = [0] * PMatrix.POOL_SIZE
        self.life = [0] * PMatrix.POOL_SIZE
        self.max_life = [0] * PMatrix.POOL_SIZE
        self.size = [0] * PMatrix.POOL_SIZE
        self.active = [0] * PMatrix.POOL_SIZE
        self.is_scroll = [0] * PMatrix.POOL_SIZE

    def spawn(self, idx, x, y, vx, vy, life, size, is_scroll=False):
        self.x[idx] = x
        self.y[idx] = y
        self.vx[idx] = vx
        self.vy[idx] = vy
        self.life[idx] = life
        self.max_life[idx] = life
        self.size[idx] = size
        self.active[idx] = 1
        self.is_scroll[idx] = 1 if is_scroll else 0

    def kill(self, idx):
        self.active[idx] = 0

    def is_active(self, idx):
        return self.active[idx] == 1

    def count(self):
        return sum(self.active)


class Config:
    WINDOW_SIZE = 280
    HALF_SIZE = 140
    EDGE_MARGIN = 60
    FADE_POWER = 2
    EDGE_SOFT_THRESHOLD = 0.05

    PHYS_DT = 0.016
    PHYS_DRAG = 0.98
    PHYS_GRAVITY = 0.02
    PHYS_WOBBLE = 0.3

    VELOCITY_THRESHOLD = 1.0
    VELOCITY_SPREAD = 0.5
    VELOCITY_INHERIT = 0.35

    SPAWN_BASE = 7
    SPAWN_OFFSET_X = 15
    SPAWN_OFFSET_Y = 10
    SPAWN_SPEED_MIN = 2
    SPAWN_SPEED_MAX = 5

    SPARK_THRESHOLD = 17
    SPARK_MAX = 50
    SPARK_MERGE_DIST = 25
    SPARK_DECAY = 0.11

    SMOKE_MAX = 30
    SMOKE_CHANCE = 0.02
    SMOKE_LIFE_THR = 0.3
    SMOKE_DECAY = 0.01

    INTERP_THRESH = 20
    INTERP_STEP = 8

    WIND_FORCE = 0.5
    WIND_DURATION = 45
    WIND_DECAY = 0.95
    WIND_MAX = 5

    BURNOUT_DELAY = 200
    BURNOUT_TOLERANCE = 2.5
    BURNOUT_SPEED_MIN = 1
    BURNOUT_SPEED_MAX = 3

    FIREBALL_THR = 15
    FIREBALL_INHERIT = 0.6
    FIREBALL_LIFE = 1.5
    FIREBALL_COUNT = 2

    COLOR_ALPHA = 220
    
    BURNOUT_PHASE_1_DURATION = 5000
    BURNOUT_PHASE_2_DURATION = 10000
    BURNOUT_PHASE_3_DURATION = 15000
    BURNOUT_PHASE_4_DURATION = 5000


class State:
    NORMAL = 0
    BURNOUT = 1
    FIREBALL = 2


class BurnoutPhase:
    NONE = 0
    PHASE_1 = 1
    PHASE_2 = 2
    PHASE_3 = 3
    PHASE_4 = 4


class SparkPool:
    MAX = 50

    def __init__(self):
        self.x = [0] * SparkPool.MAX
        self.y = [0] * SparkPool.MAX
        self.life = [0] * SparkPool.MAX
        self.active = [0] * SparkPool.MAX

    def spawn(self, idx, x, y):
        self.x[idx] = x + random.uniform(-30, 30)
        self.y[idx] = y + random.uniform(-25, 35)
        self.life[idx] = 1.0
        self.active[idx] = 1

    def kill(self, idx):
        self.active[idx] = 0

    def update(self, idx):
        if not self.active[idx]:
            return
        self.life[idx] -= Config.SPARK_DECAY
        self.x[idx] += random.uniform(-2, 2) + math.sin(self.life[idx] * 20) * 2.5
        self.y[idx] += random.uniform(-3, 1)
        if self.life[idx] <= 0:
            self.kill(idx)

    def count(self):
        return sum(self.active)


class SmokePool:
    MAX = 30

    def __init__(self):
        self.x = [0] * SmokePool.MAX
        self.y = [0] * SmokePool.MAX
        self.life = [0] * SmokePool.MAX
        self.size = [0] * SmokePool.MAX
        self.active = [0] * SmokePool.MAX

    def spawn(self, idx, x, y, size):
        self.x[idx] = x
        self.y[idx] = y
        self.life[idx] = 1.0
        self.size[idx] = size
        self.active[idx] = 1

    def kill(self, idx):
        self.active[idx] = 0

    def update(self, idx):
        if not self.active[idx]:
            return
        self.life[idx] -= Config.SMOKE_DECAY
        self.size[idx] += 0.02
        self.y[idx] -= random.uniform(0.5, 1.5)
        self.x[idx] += random.uniform(-0.5, 0.5)
        if self.life[idx] <= 0:
            self.kill(idx)

    def count(self):
        return sum(self.active)


class CursorFlame(QWidget):
    def __init__(self):
        super().__init__()
        self.p = PMatrix()
        self.sparks = SparkPool()
        self.smoke = SmokePool()

        self.cx = 0
        self.cy = 0
        self.px = 0
        self.py = 0
        self.vx = 0
        self.vy = 0
        self.speed = 0

        self.state = State.NORMAL
        self.mouse_down = False
        self.hold_start = 0
        self.last_move = 0
        
        self.hold_duration = 0
        self.max_hold_duration = 10000

        self.wind_x = 0
        self.wind_y = 0
        self.wind_timer = 0

        self.burnout_active = False
        self.burnout_start = None
        self.burnout_transition = 0
        self.last_click_time = 0
        
        self.burnout_phase = BurnoutPhase.NONE
        self.phase_start_time = 0
        self.phase_duration = 0
        self.burnout_cycle_count = 0

        self.lightning_active = False
        self.lightning_frame = 0
        self.lightning_max_frames = 8
        self.lightning_start_time = 0
        self.lightning_color = None
        self.lightning_radius = 0

        self.color_override = None
        self.color_override_t = 0
        self.color_decay = 0

        self.duration_start = None
        self.frame_count = 0

        self.pidx = 0
        self.sidx = 0
        self.smidx = 0

        self._setup()
        self._timers()
        self._setup_pynput()

    def _setup(self):
        self.setWindowTitle("Cursor_Flame")
        icon_path = get_icon_path()
        if os.path.exists(icon_path):
            self.setWindowIcon(QIcon(icon_path))
        self.setAttribute(Qt.WA_TranslucentBackground)
        self.setAttribute(Qt.WA_NoSystemBackground)
        self.setAttribute(Qt.WA_TransparentForMouseEvents)
        self.setAttribute(Qt.WA_X11NetWmWindowTypeDesktop)
        self.setWindowFlags(
            Qt.FramelessWindowHint | 
            Qt.WindowStaysOnTopHint | 
            Qt.Tool | 
            Qt.WindowDoesNotAcceptFocus
        )
        self.setFocusPolicy(Qt.NoFocus)

        screen = QApplication.desktop().screenGeometry()
        self.screen_width = screen.width()
        self.screen_height = screen.height()
        self.resize(self.screen_width, self.screen_height)
        self.move(0, 0)

        self._move_to_system_cursor()

    def _setup_pynput(self):
        self.pynput = PynputListener()
        self.pynput.bridge.move_signal.connect(self._on_system_move)
        self.pynput.bridge.click_signal.connect(self._on_system_click)
        self.pynput.bridge.scroll_signal.connect(self._on_system_scroll)
        self.pynput.start()

    def _move_to_system_cursor(self):
        p = QCursor.pos()
        self.cx = p.x()
        self.cy = p.y()
        self.px = self.cx
        self.py = self.cy

    def _on_system_move(self, x, y):
        self.cx = x
        self.cy = y

    def _on_system_click(self, x, y, pressed, button):
        self.mouse_down = pressed
        if pressed:
            self.hold_start = self.ut.remainingTime()
            self.last_move = self.ut.remainingTime()
            self._start_lightning_effect()
            self._color_flash(M4.create(255, 255, 200, 255), 0.15)
            
            self.burnout_cycle_count = 0
            self.burnout_phase = BurnoutPhase.NONE
            self.phase_start_time = 0
        else:
            self.burnout_active = False
            self.burnout_start = None
            self.burnout_transition = 0
            self.state = State.NORMAL
            self.burnout_phase = BurnoutPhase.NONE
            self.phase_start_time = 0

    def _on_system_scroll(self, x, y, dy):
        if dy != 0:
            scroll_vy = -dy * 2.0
            
            for _ in range(8):
                spawn_x = self.cx + random.uniform(-30, 30)
                spawn_y = self.cy + random.uniform(-30, 30)
                self._spawn_scroll_particle(spawn_x, spawn_y, scroll_vy)

    def _timers(self):
        self.ut = QTimer()
        self.ut.timeout.connect(self._update)
        self.ut.start(16)

        self.ct = QTimer()
        self.ct.timeout.connect(self._cursor)
        self.ct.start(8)

        self.lt = QTimer()
        self.lt.timeout.connect(self._update_lightning)
        self.lt.start(16)

    def _cursor(self):
        self.vx = self.cx - self.px
        self.vy = self.cy - self.py
        self.speed = math.sqrt(self.vx ** 2 + self.vy ** 2)

        self.px = self.cx
        self.py = self.cy

        self._update_state()

    def _update_state(self):
        now = self.ct.remainingTime()

        if self.mouse_down:
            self.hold_duration = min(self.max_hold_duration, now - self.hold_start)
            
            is_moving = abs(self.vx) > Config.BURNOUT_TOLERANCE or abs(self.vy) > Config.BURNOUT_TOLERANCE
            
            if self.speed > Config.FIREBALL_THR:
                self.state = State.FIREBALL
                self.burnout_phase = BurnoutPhase.NONE
            elif is_moving:
                self.state = State.NORMAL
                self.burnout_phase = BurnoutPhase.NONE
            else:
                self.state = State.BURNOUT
                self._update_burnout_phase()
        else:
            self.hold_duration = 0
            self.state = State.NORMAL
            self.burnout_phase = BurnoutPhase.NONE

        if abs(self.vx) > Config.BURNOUT_TOLERANCE or abs(self.vy) > Config.BURNOUT_TOLERANCE:
            self.last_move = now

    def _update_burnout_phase(self):
        if self.phase_start_time == 0:
            self.phase_start_time = self.hold_duration
        
        phase_duration = self.hold_duration - self.phase_start_time
        
        if self.burnout_cycle_count < 5:
            if phase_duration >= Config.BURNOUT_PHASE_1_DURATION:
                self.burnout_phase = BurnoutPhase.PHASE_1
                self.burnout_cycle_count += 1
                self.phase_start_time = self.hold_duration
            else:
                self.burnout_phase = BurnoutPhase.NONE
        else:
            if self.burnout_phase == BurnoutPhase.NONE:
                self.burnout_phase = BurnoutPhase.PHASE_1
                self.phase_start_time = self.hold_duration
            elif self.burnout_phase == BurnoutPhase.PHASE_1:
                if phase_duration >= Config.BURNOUT_PHASE_2_DURATION:
                    self.burnout_phase = BurnoutPhase.PHASE_2
                    self.phase_start_time = self.hold_duration
            elif self.burnout_phase == BurnoutPhase.PHASE_2:
                if phase_duration >= Config.BURNOUT_PHASE_3_DURATION:
                    self.burnout_phase = BurnoutPhase.PHASE_3
                    self.phase_start_time = self.hold_duration
            elif self.burnout_phase == BurnoutPhase.PHASE_3:
                if phase_duration >= Config.BURNOUT_PHASE_4_DURATION:
                    self.burnout_phase = BurnoutPhase.PHASE_4
                    self.phase_start_time = self.hold_duration
                    self.burnout_cycle_count = 0

    def _color_flash(self, c, d):
        self.color_override = c
        self.color_override_t = 1.0
        self.color_decay = d

    def _start_lightning_effect(self):
        self.lightning_active = True
        self.lightning_frame = 0
        self.lightning_start_time = self.frame_count
        self.lightning_radius = 0
        
        for _ in range(20):
            a = random.uniform(0, 6.283)
            s = random.uniform(8, 12)
            self._spawn_particle(self.cx, self.cy, math.cos(a) * s, math.sin(a) * s, 1.5, 2.0)

        for _ in range(15):
            a = random.uniform(0, 6.283)
            s = random.uniform(12, 18)
            self._spawn_spark(self.cx, self.cy)

        for _ in range(8):
            a = random.uniform(0, 6.283)
            s = random.uniform(4, 6)
            self._spawn_smoke(self.cx, self.cy)

    def _update_lightning(self):
        if not self.lightning_active:
            return
            
        self.lightning_frame += 1
        self.lightning_radius = self.lightning_frame * 25
        
        if self.lightning_frame < 6:
            ring_count = 15 - self.lightning_frame * 2
            for _ in range(ring_count):
                angle = random.uniform(0, 6.283)
                radius = self.lightning_radius * random.uniform(0.8, 1.2)
                px = self.cx + math.cos(angle) * radius
                py = self.cy + math.sin(angle) * radius
                self._spawn_particle(px, py, 0, 0, 1.2, 1.0)
        
        if self.lightning_frame >= self.lightning_max_frames:
            self.lightning_active = False
            self.lightning_frame = 0
            self.lightning_radius = 0

    def _edge_fade(self, x, y):
        l = x
        r = self.screen_width - x
        t = y
        b = self.screen_height - y
        d = min(l, r, t, b)
        if d >= Config.EDGE_MARGIN:
            return 1.0
        return (d / Config.EDGE_MARGIN) ** Config.FADE_POWER

    def _interp_spawn(self):
        dist = self.speed
        if dist < Config.INTERP_THRESH:
            return

        steps = min(int(dist / Config.INTERP_STEP), 50)

        for i in range(steps):
            t = (i + 1) / (steps + 1)
            sx = self.cx - self.vx * t
            sy = self.cy - self.vy * t

            if 0 <= sx <= self.screen_width and 0 <= sy <= self.screen_height:
                self._spawn_particle(sx, sy, self.vx, self.vy, self._intensity(), 1.0)

    def _intensity(self):
        return 0.6 + min(0.9, self.speed / 25)

    def _get_spawn_angle(self, vx, vy):
        speed = math.sqrt(vx * vx + vy * vy)
        if speed < Config.VELOCITY_THRESHOLD:
            return random.uniform(-2.199, -0.942)
        base_angle = math.atan2(-vy, -vx)
        spread = random.uniform(-Config.VELOCITY_SPREAD, Config.VELOCITY_SPREAD)
        return base_angle + spread

    def _spawn_particle(self, x, y, ivx, ivy, intensity, life_mult, is_scroll=False):
        idx = self.pidx
        self.pidx = (self.pidx + 1) % PMatrix.POOL_SIZE

        angle = self._get_spawn_angle(ivx, ivy)
        speed = random.uniform(Config.SPAWN_SPEED_MIN, Config.SPAWN_SPEED_MAX) * intensity
        vx = math.cos(angle) * speed * random.uniform(0.3, 0.8)
        vy = math.sin(angle) * speed

        size = random.uniform(8, 20) * intensity
        life = random.uniform(0.015, 0.035) * life_mult

        self.p.spawn(idx, x, y, vx, vy, 1.0, size, is_scroll)
        self.p.max_life[idx] = life

    def _spawn_scroll_particle(self, x, y, scroll_vy):
        idx = self.pidx
        self.pidx = (self.pidx + 1) % PMatrix.POOL_SIZE

        vx = random.uniform(-1, 1) * abs(scroll_vy)
        vy = scroll_vy + random.uniform(-0.5, 0.5)

        size = random.uniform(10, 18)
        life = random.uniform(0.02, 0.04)

        self.p.spawn(idx, x, y, vx, vy, 1.0, size, is_scroll=True)
        self.p.max_life[idx] = life

    def _spawn_spark(self, x, y, vx=0, vy=0):
        idx = self.sidx
        self.sidx = (self.sidx + 1) % SparkPool.MAX

        self.sparks.spawn(idx, x, y)
        if vx != 0 or vy != 0:
            self.sparks.x[idx] = x + random.uniform(-30, 30)
            self.sparks.y[idx] = y + random.uniform(-25, 35)

    def _spawn_smoke(self, x, y, vx=0, vy=0):
        idx = self.smidx
        self.smidx = (self.smidx + 1) % SmokePool.MAX

        self.smoke.spawn(idx, x, y, random.uniform(10, 15))
        if vx != 0 or vy != 0:
            self.smoke.x[idx] = x
            self.smoke.y[idx] = y

    def _spawn_mode(self):
        if self.state == State.NORMAL:
            self._spawn_normal()
        elif self.state == State.BURNOUT:
            self._spawn_burnout()
        elif self.state == State.FIREBALL:
            self._spawn_fireball()

    def _spawn_normal(self):
        count = int(Config.SPAWN_BASE * min(2.5, self.speed / 12) * self._intensity())

        for _ in range(count):
            if self.p.count() >= PMatrix.POOL_SIZE:
                break
            x = self.cx + random.uniform(-Config.SPAWN_OFFSET_X, Config.SPAWN_OFFSET_X)
            y = self.cy + random.uniform(-Config.SPAWN_OFFSET_Y, Config.SPAWN_OFFSET_Y)
            self._spawn_particle(x, y, self.vx, self.vy, self._intensity(), 1.0)

    def _spawn_burnout(self):
        if self.burnout_phase == BurnoutPhase.NONE:
            hold_progress = self.hold_duration / self.max_hold_duration
            
            speed_multiplier = 1.0 + hold_progress * 2.0
            rate_multiplier = 0.5 + hold_progress * 1.5
            
            count = int(Config.SPAWN_BASE * 2 * rate_multiplier)

            for _ in range(count):
                if self.p.count() >= PMatrix.POOL_SIZE:
                    break
                a = random.uniform(0, 6.283)
                s = random.uniform(Config.BURNOUT_SPEED_MIN, Config.BURNOUT_SPEED_MAX) * speed_multiplier
                vx = math.cos(a) * s
                vy = math.sin(a) * s
                x = self.cx + random.uniform(-5, 5)
                y = self.cy + random.uniform(-5, 5)
                self._spawn_particle(x, y, vx, vy, self._intensity() * 1.2, 0.5)
                
        elif self.burnout_phase == BurnoutPhase.PHASE_1:
            phase_progress = (self.hold_duration - self.phase_start_time) / Config.BURNOUT_PHASE_2_DURATION
            speed_acceleration = 1.0 + phase_progress * 1.0
            smoke_multiplier = 1.0 + phase_progress * 0.4
            
            count = int(Config.SPAWN_BASE * 3)
            
            for _ in range(count):
                if self.p.count() >= PMatrix.POOL_SIZE:
                    break
                a = random.uniform(0, 6.283)
                s = random.uniform(Config.BURNOUT_SPEED_MIN * 2, Config.BURNOUT_SPEED_MAX * 2) * speed_acceleration
                vx = math.cos(a) * s
                vy = math.sin(a) * s
                x = self.cx + random.uniform(-8, 8)
                y = self.cy + random.uniform(-8, 8)
                self._spawn_particle(x, y, vx, vy, self._intensity() * 1.5, 0.3)
                
                if random.random() < Config.SMOKE_CHANCE * smoke_multiplier:
                    self._spawn_smoke(x, y)
                    
        elif self.burnout_phase == BurnoutPhase.PHASE_2:
            phase_progress = (self.hold_duration - self.phase_start_time) / Config.BURNOUT_PHASE_3_DURATION
            speed_multiplier = 2.0
            smoke_multiplier = 1.4
            
            count = int(Config.SPAWN_BASE * 4)
            
            for _ in range(count):
                if self.p.count() >= PMatrix.POOL_SIZE:
                    break
                a = random.uniform(0, 6.283)
                s = random.uniform(Config.BURNOUT_SPEED_MIN * 3, Config.BURNOUT_SPEED_MAX * 3) * speed_multiplier
                vx = math.cos(a) * s
                vy = math.sin(a) * s
                x = self.cx + random.uniform(-12, 12)
                y = self.cy + random.uniform(-12, 12)
                self._spawn_particle(x, y, vx, vy, self._intensity() * 2.0, 0.2)
                
                if random.random() < Config.SMOKE_CHANCE * smoke_multiplier:
                    self._spawn_smoke(x, y)
                    
        elif self.burnout_phase == BurnoutPhase.PHASE_3:
            phase_progress = (self.hold_duration - self.phase_start_time) / Config.BURNOUT_PHASE_4_DURATION
            smoke_multiplier = 2.0
            
            for _ in range(3):
                self._spawn_smoke(
                    self.cx + random.uniform(-20, 20),
                    self.cy + random.uniform(-20, 20)
                )
                
        elif self.burnout_phase == BurnoutPhase.PHASE_4:
            self.burnout_phase = BurnoutPhase.NONE
            self.phase_start_time = self.hold_duration

    def _spawn_fireball(self):
        count = Config.SPAWN_BASE * Config.FIREBALL_COUNT

        if abs(self.vx) > 0.1 or abs(self.vy) > 0.1:
            ma = math.atan2(self.vy, self.vx)
        else:
            ma = -1.571

        for _ in range(count):
            if self.p.count() >= PMatrix.POOL_SIZE:
                break
            a = ma + 3.141 + random.uniform(-0.5, 0.5)
            s = random.uniform(2, 5) * self._intensity()
            x = self.cx + random.uniform(-Config.FIREBALL_THR, Config.FIREBALL_THR)
            y = self.cy + random.uniform(-Config.FIREBALL_THR, Config.FIREBALL_THR)
            self._spawn_particle(x, y, math.cos(a) * s + self.vx * Config.FIREBALL_INHERIT,
                                 math.sin(a) * s + self.vy * Config.FIREBALL_INHERIT,
                                 self._intensity() * 1.3, 1.0 / Config.FIREBALL_LIFE)

    def _update(self):
        self.frame_count += 1

        if self.color_override_t > 0:
            self.color_override_t -= self.color_decay
            if self.color_override_t <= 0:
                self.color_override_t = 0
                self.color_override = None

        if self.wind_timer > 0:
            self.wind_timer -= 1
            self.wind_x *= Config.WIND_DECAY
            self.wind_y *= Config.WIND_DECAY
        else:
            self.wind_x = 0
            self.wind_y = 0

        if self.p.count() > 0:
            if self.duration_start is None:
                self.duration_start = self.frame_count
        else:
            self.duration_start = None

        self._interp_spawn()
        self._spawn_mode()

        for i in range(PMatrix.POOL_SIZE):
            if not self.p.is_active(i):
                continue

            self.p.life[i] -= self.p.max_life[i]
            self.p.x[i] += self.p.vx[i] + self.wind_x
            self.p.y[i] += self.p.vy[i] + self.wind_y

            wobble = math.sin(self.frame_count * 0.1 + i) * Config.PHYS_WOBBLE
            self.p.x[i] += wobble * 0.3

            self.p.vx[i] *= Config.PHYS_DRAG
            self.p.vy[i] *= Config.PHYS_DRAG
            self.p.vy[i] -= Config.PHYS_GRAVITY

            if self.p.life[i] <= 0:
                self.p.kill(i)

        for i in range(SparkPool.MAX):
            self.sparks.update(i)

        for i in range(SmokePool.MAX):
            self.smoke.update(i)

        self._merge_sparks()
        self._emit_smoke()

        self.update()

    def _merge_sparks(self):
        for si in range(SparkPool.MAX):
            if not self.sparks.active[si]:
                continue
            sx, sy = self.sparks.x[si], self.sparks.y[si]

            for pi in range(PMatrix.POOL_SIZE):
                if not self.p.is_active(pi):
                    continue
                dx = sx - self.p.x[pi]
                dy = sy - self.p.y[pi]
                if dx * dx + dy * dy < Config.SPARK_MERGE_DIST ** 2:
                    self.p.life[pi] = min(1.0, self.p.life[pi] + 0.15)
                    self.p.size[pi] = min(self.p.max_life[pi] * 1.5, self.p.size[pi] * 1.2)
                    self.sparks.kill(si)
                    break

    def _emit_smoke(self):
        if self.smoke.count() >= SmokePool.MAX:
            return

        for i in range(PMatrix.POOL_SIZE):
            if not self.p.is_active(i):
                continue
            if self.p.life[i] < Config.SMOKE_LIFE_THR and random.random() < Config.SMOKE_CHANCE:
                smoke_chance = Config.SMOKE_CHANCE
                if self.state == State.BURNOUT and self.burnout_phase == BurnoutPhase.PHASE_1:
                    smoke_chance *= 1.4
                elif self.state == State.BURNOUT and self.burnout_phase == BurnoutPhase.PHASE_2:
                    smoke_chance *= 1.4
                
                if random.random() < smoke_chance:
                    self._spawn_smoke(self.p.x[i], self.p.y[i])

    def _color_duration(self):
        if self.duration_start is None:
            return M4.create(255, 80, 0, Config.COLOR_ALPHA)

        elapsed = (self.frame_count - self.duration_start) / 60.0
        return CMatrix.get(elapsed)

    def _color(self, life, intensity, edge_fade, is_scroll=False):
        if is_scroll:
            scroll_color = M4.create(100, 200, 150, int(life * Config.COLOR_ALPHA * edge_fade))
            return scroll_color
        
        if self.lightning_active:
            lightning_progress = self.lightning_frame / self.lightning_max_frames
            
            if lightning_progress < 0.2:
                t = lightning_progress / 0.2
                l_r = int(255 * (1 - t) + 200 * t)
                l_g = int(255 * (1 - t) + 255 * t)
                l_b = int(255 * (1 - t) + 255 * t)
            elif lightning_progress < 0.5:
                t = (lightning_progress - 0.2) / 0.3
                l_r = int(200 * (1 - t) + 100 * t)
                l_g = int(255 * (1 - t) + 150 * t)
                l_b = 255
            else:
                t = (lightning_progress - 0.5) / 0.5
                l_r = int(100 * (1 - t) + 180 * t)
                l_g = int(150 * (1 - t) + 100 * t)
                l_b = int(255 * (1 - t) + 200 * t)
            
            lightning_color = M4.create(l_r, l_g, l_b, int(life * Config.COLOR_ALPHA * edge_fade))
            return lightning_color
        
        base = self._color_duration()

        if life > 0.8:
            r = 255
            g = int(220 + 35 * (life - 0.8) * 4.17)
            b = int(150 + 105 * (life - 0.8) * 4.17)
        elif life > 0.6:
            r = 255
            g = int(150 + 70 * (life - 0.6) * 3.33)
            b = int(50 + 50 * (life - 0.6) * 1.67)
        elif life > 0.4:
            r = int(230 + 25 * (life - 0.4) * 1.25)
            g = int(80 + 70 * (life - 0.4) * 1.25)
            b = int(25 + 25 * (life - 0.4))
        elif life > 0.2:
            r = int(180 + 50 * (life - 0.2) * 2.5)
            g = int(40 + 40 * (life - 0.2) * 2)
            b = int(0 + 25 * (life - 0.2))
        else:
            r = int(80 + 100 * life)
            g = int(20 * life)
            b = 0

        intensity_factor = 0.8 + intensity * 0.2
        r = min(255, int(r * intensity_factor * base[0][0] / 255))
        g = min(255, int(g * intensity_factor * base[0][1] / 255))
        b = min(255, int(b * intensity_factor * base[0][2] / 255))

        base_m = M4.create(r, g, b, int(life * Config.COLOR_ALPHA * edge_fade))

        if self.color_override and self.color_override_t > 0:
            co = M4.mul(self.color_override, self.color_override_t)
            base_m = M4.lerp(base_m, co, self.color_override_t)

        return M4.clamp(base_m)

    def _draw_teardrop(self, painter, x, y, w, h, color, distortion, vx=0, vy=0):
        cx = x
        cy = y + h * 0.15
        s = 1.8 * distortion
        hw = w / 2
        hh = h / 2

        if abs(vx) > 0.1 or abs(vy) > 0.1:
            angle = math.atan2(-vy, -vx)
            offset_x = math.cos(angle) * hw * 0.3
            offset_y = math.sin(angle) * hh * 0.3
            cx += offset_x
            cy += offset_y

        ty = cy - hh * s
        by = cy + hh * 0.5
        tr = hw * 0.15

        grad = QRadialGradient(QPointF(cx, cy), hw * 1.2)
        c = M4.to_qcolor(color)

        c1 = QColor(c)
        c1.setAlpha(min(255, int(c.alpha() * 1.2)))
        grad.setColorAt(0, c1)
        grad.setColorAt(0.4, c)
        c2 = QColor(c)
        c2.setAlpha(int(c.alpha() * 0.3))
        grad.setColorAt(0.7, c2)
        grad.setColorAt(1, QColor(0, 0, 0, 0))

        painter.setPen(Qt.NoPen)
        painter.setBrush(grad)

        pts = [
            (cx, ty),
            (cx - hw + tr, cy - hh * 0.3 * s),
            (cx - hw, by),
            (cx, by + hh * 0.2),
            (cx + hw, by),
            (cx + hw - tr, cy - hh * 0.3 * s),
            (cx, ty)
        ]

        path = QPainterPath()
        path.moveTo(QPointF(pts[0][0], pts[0][1]))
        for px, py in pts[1:]:
            path.lineTo(QPointF(px, py))
        path.closeSubpath()

        painter.fillPath(path, grad)

    def _draw_spark(self, painter, x, y, life, intensity):
        c = min(255, int(255 * intensity))
        g = min(255, int(220 + 35 * life))
        b = int(180 + 75 * life)
        a = int(life * 255)
        color = QColor(c, g, b, a)

        size = max(1, life * 8)
        grad = QRadialGradient(QPointF(x, y), size * 2)
        c1 = QColor(color)
        c1.setAlpha(min(255, int(a * 1.3)))
        grad.setColorAt(0, c1)
        grad.setColorAt(0.5, color)
        c2 = QColor(color)
        c2.setAlpha(int(a * 0.2))
        grad.setColorAt(0.8, c2)
        grad.setColorAt(1, QColor(0, 0, 0, 0))

        painter.fillRect(QRectF(x - size * 2, y - size * 2, size * 4, size * 4), grad)

    def paintEvent(self, e):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        painter.setCompositionMode(QPainter.CompositionMode_Plus)

        for i in range(SmokePool.MAX):
            if not self.smoke.active[i]:
                continue
            life = self.smoke.life[i]
            a = int(life * 60)
            color = QColor(80, 80, 80, a)
            size = self.smoke.size[i]
            grad = QRadialGradient(QPointF(self.smoke.x[i], self.smoke.y[i]), size)
            grad.setColorAt(0, color)
            grad.setColorAt(1, QColor(0, 0, 0, 0))
            painter.fillRect(QRectF(self.smoke.x[i] - size, self.smoke.y[i] - size, size * 2, size * 2), grad)

        for i in range(PMatrix.POOL_SIZE):
            if not self.p.is_active(i):
                continue
            life = self.p.life[i]
            edge = self._edge_fade(self.p.x[i], self.p.y[i])
            if life <= 0:
                continue
            if edge < Config.EDGE_SOFT_THRESHOLD:
                continue
            color = self._color(life, 1.0, edge, self.p.is_scroll[i])
            size = max(4, self.p.size[i])
            w = size * 1.4
            h = size * 2.2
            wob = math.sin(self.p.life[i] * 10 + i) * 0.1
            dist = 0.8 + life * 0.4
            self._draw_teardrop(painter, self.p.x[i], self.p.y[i], w * (1 + wob * 0.3), h, color, dist, self.p.vx[i], self.p.vy[i])

        for i in range(SparkPool.MAX):
            if not self.sparks.active[i]:
                continue
            self._draw_spark(painter, self.sparks.x[i], self.sparks.y[i], self.sparks.life[i], 1.0)


def main():
    QApplication.setAttribute(Qt.AA_EnableHighDpiScaling)
    QApplication.setAttribute(Qt.AA_UseHighDpiPixmaps)
    app = QApplication(sys.argv)
    icon_path = get_icon_path()
    if os.path.exists(icon_path):
        app.setWindowIcon(QIcon(icon_path))
    app.setApplicationName("Cursor_Flame")
    flame = CursorFlame()
    flame.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
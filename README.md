# Cursor_Flame

A stunning flame particle effect that follows your mouse cursor.

![Python](https://img.shields.io/badge/Python-3.7+-blue.svg)
![C++](https://img.shields.io/badge/C++-17-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

## Preview

### Flame Trail
![Flame Trail](assets/Flame_Tail.gif)

### Click Effect
![Click Burst](assets/click_Bomb.gif)

### Hold Click - Burnout Mode
![Burnout Effect](assets/Burnout_effect.gif)

### Scroll Effect
![Scroll Effect](assets/scrolling_Up-Down.gif)

## Features

- Realistic flame particles that follow your cursor
- Dynamic color transitions (orange to yellow to blue to purple)
- Click effects with lightning burst animation
- Scroll wheel creates directional flame particles
- Hold-click "burnout" mode with intensifying flames
- Smooth particle physics with wobble, drag, and gravity
- Sparks and smoke effects
- Transparent overlay - works on top of any application

## Repository Structure

```
Cursor_Flame/
├── assets/                          # GIFs, icons, preview images
│   ├── Flame_Tail.gif
│   ├── click_Bomb.gif
│   ├── Burnout_effect.gif
│   ├── scrolling_Up-Down.gif
│   ├── cf_ico.ico
│   └── cf_ico.png
├── src/
│   ├── windows/
│   │   ├── cpp/
│   │   │   ├── kursorflame_v1.cpp   # Windows C++ v1 (basic)
│   │   │   └── kursorflame_v2.5.cpp # Windows C++ v2.5 native (NVIDIA+D3D11+AVX2)
│   │   └── python/
│   │       └── kursorflame_v1.py    # Windows Python v1 (PyQt5)
│   └── linux/
│       └── cpp/
│           ├── kursorflame_v1.cpp   # Linux C++ v1 (basic)
│           ├── kursorflame_v2.cpp   # Linux C++ v2 (GPU/OpenGL)
│           └── kursorflame_v2.5.cpp # Linux C++ v2.5 (dual-core CPU)
├── run.bat                          # Windows launcher scripts
├── kill.bat
├── startup_ON.bat
├── startup_OFF.bat
├── .github/workflows/
│   └── windows-build.yml            # CI: builds Windows native exe
├── LICENSE
└── README.md
```

## Downloads

### C++ Native Version (Recommended)

Lightweight, high-performance native binaries:

| Platform | Version | Download | Size | Features |
|----------|---------|----------|------|----------|
| **Windows** | **v2.5** | `KursorFlame_windows_v2.5.zip` | ~200 KB | Native NVIDIA GPU + DirectX 11 + multi-core CPU + AVX2 |
| **Linux** | **v2.5** | `KursorFlame_linux_cpp_2.5.zip` | ~310 KB | Full customization, themes, gradients, dual-core CPU |
| Linux | v2 | `KursorFlame_linux_cpp_2.zip` | ~300 KB | GPU accel, themes, config |
| Linux | v1 | `KursorFlame_linux_cpp.zip` | ~65 KB | Basic flame effect |
| Windows | v1 | `Cursor_Flame_cpp.zip` | ~300 KB | Basic flame effect |

### Python Version

| Platform | Download | Size |
|----------|----------|------|
| Windows | `Cursor_Flame.zip` | ~28 MB |

### Version Comparison

| Feature | v1 | v2 | v2.5 Linux | v2.5 Windows |
|---------|----|----|------------|--------------|
| Basic flame effect | ✅ | ✅ | ✅ | ✅ |
| Toggle/Close hotkeys | ✅ | ✅ | ✅ | ✅ |
| GPU acceleration | ❌ | ✅ | ❌ (CPU opt) | ✅ (D3D11 NVIDIA) |
| Themes (Fire/Snow/Water) | ❌ | ✅ | ✅ | ✅ (enhanced physics) |
| Custom colors | ❌ | ✅ | ✅ | ✅ |
| Config file | ❌ | ✅ | ✅ | ✅ |
| Quality presets | ❌ | ✅ (4) | ✅ (5 incl. custom) | ✅ (5 incl. custom) |
| Physics customization | ❌ | ✅ | ✅ | ✅ |
| Custom quality profile | ❌ | ❌ | ✅ | ✅ |
| 10-level color gradient | ❌ | ❌ | ✅ | ✅ |
| Sub-effect colors | ❌ | ❌ | ✅ | ✅ |
| Wave/trail dynamics | ❌ | ❌ | ✅ | ✅ |
| Render styles | ❌ | ❌ | ✅ (3 styles) | ✅ (3 styles) |
| Multi-core CPU rendering | ❌ | ❌ | ✅ (dual-core) | ✅ (all cores) |
| Single-instance lock | ❌ | ❌ | ❌ | ✅ |
| Particle capacity | 200 | 5,000 | 30,000 | **30,000** |

## Hotkeys

| Hotkey | Action |
|--------|--------|
| `Ctrl+Alt+E` | Toggle effect ON/OFF |
| `Ctrl+Alt+Q` | Close KursorFlame |

## Installation

### Windows

**Option 1: Download Release (Recommended)**
1. Download `KursorFlame_windows_v2.5.zip` from [Releases](../../releases)
2. Extract and run `run.bat` or `KursorFlame.exe`

**Option 2: Build from Source**
```bash
git clone https://github.com/HAKORADev/Cursor_Flame.git
cd Cursor_Flame

# Requires Visual Studio 2019+ or Build Tools (MSVC + Windows SDK)
# Run from "x64 Native Tools Command Prompt for VS":
cl /O2 /EHsc /std:c++17 /arch:AVX2 /fp:fast /Oi /Ot /Oy /Qpar /GL ^
   /DUNICODE /D_UNICODE /Fe:KursorFlame.exe ^
   src\windows\cpp\kursorflame_v2.5.cpp ^
   /link /SUBSYSTEM:WINDOWS /MACHINE:X64 /STACK:8388608 /LTCG ^
   d3d11.lib dxgi.lib dxguid.lib d3dcompiler.lib ^
   user32.lib gdi32.lib dwmapi.lib dcomp.lib ole32.lib shell32.lib shlwapi.lib
```

**Option 3: Run Python Version from Source**
```bash
pip install PyQt5 pynput
python src/windows/python/kursorflame_v1.py
```

### Linux

**Option 1: Download Release (Recommended)**
1. Download `KursorFlame_linux_cpp_2.5.zip` (v2.5 - recommended) from [Releases](../../releases)
2. Extract and run:
```bash
cd KursorFlame
chmod +x KursorFlame *.sh
./run.sh
# or directly: ./KursorFlame
```

**Option 2: Build from Source**
```bash
# Requires X11 development libraries
# Ubuntu/Debian: sudo apt install libx11-dev libxrandr-dev libxi-dev

# v1
g++ -O2 -o KursorFlame src/linux/cpp/kursorflame_v1.cpp -lX11 -lXrandr -lXi -lpthread

# v2 (with GPU support - requires OpenGL)
g++ -O2 -o KursorFlame src/linux/cpp/kursorflame_v2.cpp -lX11 -lXrandr -lXi -lGL -lpthread

# v2.5 (CPU dual-core optimized)
g++ -O2 -o KursorFlame src/linux/cpp/kursorflame_v2.5.cpp -lX11 -lXrandr -lXi -lpthread
./KursorFlame
```

## Requirements

### Windows (v2.5 Native)
- Windows 10/11 64-bit
- CPU with AVX2 support (Intel Haswell 2013+ / AMD Ryzen 2017+)
- NVIDIA GPU recommended (falls back to default DXGI adapter if absent)
- DirectX 11 runtime (pre-installed on Windows 10/11)

### Windows (Python v1)
- Windows 10/11
- Python 3.7+
- PyQt5, pynput

### Linux
- X11 display server (Xorg)
- libX11, libXrandr, libXi

## Usage

Simply run the application and move your mouse around. The flame effect will appear automatically.

| Action | Effect |
|--------|--------|
| Move mouse | Flame trail follows cursor |
| Click | Lightning burst effect |
| Hold click | Intensifying burnout flames |
| Scroll | Directional flame burst |
| Fast movement | Fireball mode with trailing flames |

## Configuration (v2.5)

Edit `kursor.conf` to customize all effects (auto-generated on first run):

```ini
[General]
tail = 1               # Enable trailing particles
on_click = 1           # Enable click effects
on_hold = 1            # Enable burnout mode
on_scroll = 1          # Enable scroll effects
strike = 1             # Enable fast-movement flash
interactive_edges = 1  # Particles bounce off screen

[Visuals]
quality = 4            # 0=Low, 1=Med, 2=High, 3=Ultra, 4=Custom
theme = 0              # 0=Fire, 1=Snow, 2=Water, 4=Custom

[Physics]
gravity_mult = 1.0     # Vertical pull (negative = up)
flicker_mult = 1.0     # Particle jitter
wind_x = 0.0           # Horizontal push
wind_y = 0.0           # Vertical push

[Quality]              # Used when quality = 4
q_spawn_base = 30      # Particles per tick (0-5000)
q_smoke_max = 100      # Max smoke particles (0-5000)
particle_life_sec = 1.0 # Particle lifespan (0-600)
q_particle_size = 1.0  # Size multiplier (0-10)
q_softness = 2.5       # Blur radius (0-15)
q_jitter_int = 1.0     # Flicker intensity (0-10)
q_smoke_blend = 1      # 0=Additive, 1=Alpha
q_render_style = 1     # 0=Teardrop, 1=Soft, 2=Shaded

[SubEffects]
overall_opacity = 220  # Master alpha (0-255)
# Strike effect (fast click while moving)
strike_r = 200
strike_g = 255
strike_b = 255
strike_a = 255
# Click effect
click_r = 255
click_g = 255
click_b = 200
click_a = 255
# Scroll effect
scroll_r = 255
scroll_g = 50
scroll_b = 20
scroll_a = 220
# Tail dynamics
tail_len_mult = 1.0    # Trail length (0.1-10)
tail_thick_mult = 1.0  # Trail thickness (0.1-10)
wave_amp = 0.0         # Wave amplitude (0-100)
wave_freq = 0.1        # Wave frequency (0-5)

[CustomTheme]          # Used when theme = 4
gradient_speed = 60.0
gradient_reverse_speed = 60.0
l0_r = 255  l0_g = 100  l0_b = 50
l1_r = 235  l1_g = 110  l1_b = 60
# ... l2 through l9 for 10-level gradient
```

## Included Scripts

### Windows (.bat)

| Script | Description |
|--------|-------------|
| `run.bat` | Launch KursorFlame |
| `kill.bat` | Stop KursorFlame |
| `startup_ON.bat` | Enable auto-start with Windows |
| `startup_OFF.bat` | Disable auto-start |

### Linux (.sh)

| Script | Description |
|--------|-------------|
| `run.sh` | Launch KursorFlame |
| `kill.sh` | Stop KursorFlame |
| `startup_ON.sh` | Enable auto-start (XDG autostart) |
| `startup_OFF.sh` | Disable auto-start |

## Building the Executable

### Python Version (Windows)
```bash
pip install pyinstaller
pyinstaller --onefile --windowed --icon=assets/cf_ico.ico src/windows/python/kursorflame_v1.py
```

### C++ Version (Windows v2.5 Native)
```bash
# MSVC from "x64 Native Tools Command Prompt for VS"
cl /O2 /EHsc /std:c++17 /arch:AVX2 /fp:fast /Oi /Ot /Oy /Qpar /GL ^
   /DUNICODE /D_UNICODE /Fe:KursorFlame.exe ^
   src\windows\cpp\kursorflame_v2.5.cpp ^
   /link /SUBSYSTEM:WINDOWS /MACHINE:X64 /STACK:8388608 /LTCG ^
   d3d11.lib dxgi.lib dxguid.lib d3dcompiler.lib ^
   user32.lib gdi32.lib dwmapi.lib dcomp.lib ole32.lib shell32.lib shlwapi.lib
```

### C++ Version (Linux)
```bash
# v1
g++ -O2 -o KursorFlame src/linux/cpp/kursorflame_v1.cpp -lX11 -lXrandr -lXi -lpthread

# v2 (with OpenGL/GPU support)
g++ -O2 -o KursorFlame src/linux/cpp/kursorflame_v2.cpp -lX11 -lXrandr -lXi -lGL -lpthread

# v2.5 (CPU dual-core optimized)
g++ -O2 -o KursorFlame src/linux/cpp/kursorflame_v2.5.cpp -lX11 -lXrandr -lXi -lpthread
```

## License

MIT License - feel free to use and modify!

## Contributing

Pull requests are welcome! Feel free to open issues for bugs or feature requests.

# Cursor_Flame

A stunning flame particle effect that follows your mouse cursor.

![Python](https://img.shields.io/badge/Python-3.7+-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

## Preview

### Flame Trail
![Flame Trail](Flame_Tail.gif)

### Click Effect
![Click Burst](click_Bomb.gif)

### Hold Click - Burnout Mode
![Burnout Effect](Burnout_effect.gif)

### Scroll Effect
![Scroll Effect](scrolling_Up-Down.gif)

## Features

- Realistic flame particles that follow your cursor
- Dynamic color transitions (orange to yellow to blue to purple)
- Click effects with lightning burst animation
- Scroll wheel creates directional flame particles
- Hold-click "burnout" mode with intensifying flames
- Smooth particle physics with wobble, drag, and gravity
- Sparks and smoke effects
- Transparent overlay - works on top of any application

## Downloads

### C++ Native Version (Recommended)

Lightweight, high-performance native binaries:

| Platform | Download | Size |
|----------|----------|------|
| Windows | `Cursor_Flame_cpp.zip` | ~300 KB |
| Linux | `KursorFlame_linux_cpp.zip` | ~65 KB |

### Python Version

| Platform | Download | Size |
|----------|----------|------|
| Windows | `Cursor_Flame.zip` | ~28 MB |

## Hotkeys (C++ Version)

| Hotkey | Action |
|--------|--------|
| `Ctrl+Alt+E` | Toggle effect ON/OFF |
| `Ctrl+Alt+Q` | Close KursorFlame |

## Installation

### Windows

**Option 1: Download Release (Recommended)**
1. Download `Cursor_Flame_cpp.zip` (C++ version) or `Cursor_Flame.zip` (Python version) from [Releases](../../releases)
2. Extract and run `run.bat` or `Cursor_Flame.exe`

**Option 2: Run from Source**
```bash
git clone https://github.com/HAKORADev/Cursor_Flame.git
cd Cursor_Flame
pip install PyQt5 pynput
python cf.py
```

### Linux

**Option 1: Download Release (Recommended)**
1. Download `KursorFlame_linux_cpp.zip` from [Releases](../../releases)
2. Extract and run:
```bash
cd KursorFlame
./run.sh
# or directly: ./KursorFlame
```

**Option 2: Build from Source**
```bash
# Requires X11 development libraries
# Ubuntu/Debian: sudo apt install libx11-dev libxrandr-dev libxi-dev

g++ -O2 -o KursorFlame kf.cpp -lX11 -lXrandr -lXi -lpthread
./KursorFlame
```

## Requirements

### Windows
- Windows 10/11
- Python 3.7+ (if running from source)
- PyQt5, pynput (if running from source)

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

## Included Scripts

### Windows (.bat)

| Script | Description |
|--------|-------------|
| `run.bat` | Launch Cursor Flame |
| `kill.bat` | Stop Cursor Flame |
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
pyinstaller --onefile --windowed --icon=cf_ico.ico cf.py
```

### C++ Version (Windows)
```bash
# MSVC
cl /O2 /EHsc /Fe:Cursor_Flame.exe cf.cpp user32.lib gdi32.lib winmm.lib
```

### C++ Version (Linux)
```bash
g++ -O2 -o KursorFlame kf.cpp -lX11 -lXrandr -lXi -lpthread
```

## License

MIT License - feel free to use and modify!

## Contributing

Pull requests are welcome! Feel free to open issues for bugs or feature requests.

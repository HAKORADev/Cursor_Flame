# Cursor_Flame

A stunning flame particle effect that follows your mouse cursor on Windows.

![Python](https://img.shields.io/badge/Python-3.7+-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)
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

## Installation

### Option 1: Download Release (Recommended)
1. Download the latest release `.zip` from [Releases](../../releases)
2. Extract and run `Cursor_Flame.exe`

### Option 2: Run from Source
```bash
# Clone the repository
git clone https://github.com/YOUR_USERNAME/Cursor_Flame.git
cd Cursor_Flame

# Install dependencies
pip install PyQt5 pynput

# Run
python cf.py
```

## Requirements

- Windows 10/11
- Python 3.7+ (if running from source)
- PyQt5
- pynput

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

| Script | Description |
|--------|-------------|
| `run.bat` | Launch Cursor Flame |
| `kill.bat` | Stop Cursor Flame |
| `startup_ON.bat` | Enable auto-start with Windows |
| `startup_OFF.bat` | Disable auto-start |

## Building the Executable

```bash
pip install pyinstaller
pyinstaller --onefile --windowed --icon=cf_ico.ico cf.py
```

## License

MIT License - feel free to use and modify!

## Contributing

Pull requests are welcome! Feel free to open issues for bugs or feature requests.

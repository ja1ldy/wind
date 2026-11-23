# wind

**wind** (WindWM) is a lightweight, minimalist floating window manager for the X Window System, written in C.

Built with simplicity and efficiency in mind, wind provides a traditional desktop experience without requiring a compositor, desktop environment, or heavyweight graphical toolkit.

It uses Xlib for window management, Xft and Fontconfig for text rendering, and Lua 5.4 for configuration.

wind is designed for Linux systems running X11, from older and resource-constrained computers to modern desktops.

> **Status:** Experimental — version 0.3.2

## Features

- **Lightweight:** Written in C with minimal dependencies.
- **Floating windows:** Simple window management without mandatory tiling.
- **Focus follows mouse:** Windows receive keyboard focus when the pointer enters them.
- **Keyboard-oriented:** Familiar dwm-style shortcuts and modifier-based mouse controls.
- **Lua configuration:** Customize appearance and behavior without recompiling.
- **Built-in application menu:** Launch installed applications directly from the desktop.
- **Minimal decorations:** Thin borders, no titlebar buttons, and an amber highlight for focused windows.
- **Custom themes:** Includes sunset, noir, nord, forest, and amber presets.
- **Wallpaper support:** Includes a selection of CC0 photographs.
- **Independent About application:** `wind-about` runs as a separate, lightweight X11 client.
- **No compositor required:** Runs directly on a standard Xorg session.

## Requirements

wind requires an X11 environment and the following libraries:

| Dependency | Purpose |
|---|---|
| Xlib | X11 window management |
| Xft | Font rendering |
| Fontconfig | Font selection |
| Lua 5.4 | Configuration |
| pkg-config | Build configuration |
| C compiler and Make | Compilation |

Optional dependencies include:

- **Xephyr** for running wind inside another graphical session.
- **Xvfb and XTest** for automated testing.
- **feh** for wallpaper functionality.
- **Go Mono Nerd Font** for the intended interface typography.

The font is not bundled with wind and can be obtained from the [Nerd Fonts project](https://www.nerdfonts.com/).

## Installation

### 1. Install dependencies

On Debian, Ubuntu, and other Debian-based distributions:

```sh
sudo apt install \
    build-essential \
    pkg-config \
    libx11-dev \
    libxft-dev \
    liblua5.4-dev \
    xorg \
    xinit \
    feh
```

For other distributions, install the equivalent development packages using your distribution's package manager.

### 2. Download the source

Clone the repository:

```sh
git clone https://github.com/ja1ldy/wind.git
cd wind
```

### 3. Build

Compile wind from source:

```sh
make
```

This builds two executables:

- `wind` — the X11 window manager.
- `wind-about` — the standalone About application.

### 4. Install

```sh
sudo make install
```

By default, the executables are installed to `/usr/local/bin`.

Verify the installation:

```sh
wind --version
wind-about --version
```

Both should report version `0.3.2`.

## Starting wind

### Using startx

Create or edit `~/.xinitrc`:

```sh
exec /usr/local/bin/wind
```

Then, from a Linux text console, start the X11 session:

```sh
startx
```

If you already have an `.xinitrc`, back it up before changing it.

### Using a display manager

The installation includes an X11 session entry, `wind.desktop`.

On compatible display managers, select **wind** from the session menu before logging in.

### Running in a nested X11 session

wind can also run inside Xephyr, allowing it to be tested without replacing the currently running desktop.

Install Xephyr and run:

```sh
./scripts/run-nested.sh
```

This is useful for development, testing, and debugging.

## Keyboard and mouse controls

wind follows familiar dwm-style conventions, using the Super key as its primary keyboard modifier.

| Shortcut | Action |
|---|---|
| `Super + Shift + Enter` | Open terminal |
| `Super + P` | Open application menu |
| `Super + J` | Focus next window |
| `Super + K` | Focus previous window |
| `Super + Shift + C` | Close focused window |
| `Super + Shift + R` | Reload configuration |
| `Super + Shift + Q` | Exit wind |
| `Alt + Left Drag` | Move window |
| `Alt + Right Drag` | Resize window |
| Desktop left-click | Open application menu |
| Hover over window | Focus window |

Windows cannot be moved by ordinary titlebar dragging.

## Configuration

wind uses **Lua 5.4** for configuration.

The default configuration is provided in `config.lua.example`.

To create a personal configuration:

```sh
mkdir -p ~/.config/wind
cp config.lua.example ~/.config/wind/config.lua
```

Edit the configuration using your preferred text editor.

Configuration options include themes, wallpaper selection, terminal commands, application menu entries, and focus behavior.

Changes can be reloaded using `Super + Shift + R`.

### Themes

Five themes are included:

| Theme | Description |
|---|---|
| sunset | Warm, muted colors with amber accents |
| noir | Dark monochromatic appearance |
| nord | Cool colors inspired by Nordic palettes |
| forest | Natural green tones |
| amber | Dark background with warm highlights |

Theme definitions are stored in the `themes/` directory.

### Wallpapers

wind supports several wallpapers:

- `sunflowers`
- `ridge`
- `pine`
- `dusk`

The selected photographs are released under CC0 and sourced from Wikimedia Commons.

Wallpapers can be fetched with:

```sh
make wallpapers
```

Their original source pages and licensing information are documented in `assets/WALLPAPERS.md`.

No AI-generated wallpapers are included.

## About wind

`wind-about` is an independent X11 application written in C.

Unlike an internal window-manager panel, it maintains its own X11 connection, event loop, and drawing resources.

It displays information about wind and the running system.

The application supports:

- X11 `Expose` and `ConfigureNotify` handling.
- Double-buffered rendering using an off-screen pixmap.
- Redrawing after overlapping windows are removed.
- Dynamic repainting when resized.
- The standard `WM_DELETE_WINDOW` protocol.
- Normal WindWM focus, movement, and resizing behavior.

It can be launched through the application menu or directly:

```sh
wind-about
```

## Source structure

```text
wind/
├── apps/
│   └── wind-about.c
├── assets/
│   ├── wind-logo.svg
│   └── WALLPAPERS.md
├── data/
│   ├── wind.desktop
│   └── wind-about.desktop
├── scripts/
│   ├── run-nested.sh
│   └── fetch-wallpapers.sh
├── src/
│   ├── wind.c
│   ├── wind.h
│   ├── about.c
│   ├── clients.c
│   ├── config.c
│   ├── events.c
│   ├── graphics.c
│   ├── input.c
│   ├── menu.c
│   └── wallpaper.c
├── tests/
│   ├── run-xvfb.sh
│   └── x11-smoke.c
├── themes/
│   ├── amber.lua
│   ├── forest.lua
│   ├── noir.lua
│   ├── nord.lua
│   └── sunset.lua
├── config.lua.example
├── Makefile
├── LICENSE
└── README.md
```

The source is divided into small modules to keep the window manager understandable and maintainable.

The project intentionally avoids unnecessary abstraction layers and external frameworks.

## Testing

wind includes an automated X11 regression test suite.

On Debian-based distributions, install the test dependencies:

```sh
sudo apt install xvfb libxtst-dev x11-utils
```

Build and run the tests:

```sh
make
./tests/run-xvfb.sh
```

The tests cover window creation, keyboard focus, pointer-based movement and resizing, About-window repainting, and window closing.

The current regression suite has been exercised using Xvfb on x86_64 Linux. Compatibility across other architectures and X11 configurations is an ongoing development goal.

## Current limitations

wind is still under active development.

Version 0.3.2 provides a functional, single-screen floating window manager but does not yet implement every ICCCM or EWMH feature.

Features not yet fully supported include:

- Multiple virtual workspaces.
- Comprehensive multi-monitor management.
- System tray integration.
- Complete fullscreen and EWMH compatibility.
- Advanced window layouts.

The goal is to improve compatibility and stability while preserving a small codebase, low resource usage, and minimal dependencies.

## Contributing

Contributions, bug reports, and suggestions are welcome.

If you encounter a problem, please open a GitHub issue with your Linux distribution, Xorg version, steps to reproduce, and any relevant logs.

Pull requests should aim to preserve the project's minimal design and avoid introducing unnecessary dependencies.

## License

wind is released under the **MIT License**. See [LICENSE](LICENSE) for details.

The external wallpapers are CC0 photographs. Their sources and licensing details can be found in [assets/WALLPAPERS.md](assets/WALLPAPERS.md).

Third-party fonts are not bundled with the project.

---

**wind — a small window manager that stays out of your way.**

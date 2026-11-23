# windwm / wind 0.3.2

A small floating X11 window manager in C, with Xlib, Xft/Fontconfig and Lua
5.4. No compositor or heavyweight toolkit. `wind-about` is now a **separate
X11 application** written in C rather than a client drawn by the window
manager itself.

## Fixed in 0.3.2: the about window disappears under overlapping windows

Previously, the window manager created and painted the About client itself.
That unusual ownership model caused unreliable redrawing when other clients
covered or uncovered the window.

Now:

- `wind` starts a separate program, **`wind-about`**, using the existing X11
  `DISPLAY`. Each program owns its own X11 connection and event loop.
- `wind-about` handles `Expose` and `ConfigureNotify` and redraws the *whole*
  window using a cached off-screen X11 pixmap. Text/logo remain visible after
  cover/uncover and after resize.
- It sets `WM_CLASS=wind-about`, so choosing **about wind** again focuses the
  existing client instead of opening another copy.
- It supports the standard `WM_DELETE_WINDOW` protocol. Use **Super+Shift+C**
  to close it; there are no titlebar buttons.
- Dragging/resizing still uses **Alt+left** / **Alt+right**, exactly as for
  other managed windows. No plain-mouse dragging.
- The separate program inherits the current WindWM theme colors and Go Mono
  Nerd Font setting at launch. If you switch themes while it is open, close
  and reopen About to apply the new theme.
- The wind SVG logo remains `assets/wind-logo.svg`, parsed with a tiny,
  purpose-built line/circle renderer; no SVG library or toolkit is required.

## Install on Raspberry Pi OS / Debian 13

Your existing build dependencies remain sufficient:

```sh
sudo apt install build-essential pkg-config libx11-dev libxft-dev liblua5.4-dev
```

Back up the old source and extract the new archive (adjust the ZIP location if
it isn't in `~/Downloads`):

```sh
cd ~
cp -a WindWM WindWM.backup-0.3.1
unzip -o ~/Downloads/WindWM-0.3.2-source.zip
cd ~/WindWM
make clean
make
sudo make install
wind --version
wind-about --version
```

Both commands should report **0.3.2**. `sudo make install` installs two
executables into `/usr/local/bin`: `wind` (WM) and `wind-about` (X11 client),
plus an `about wind` desktop entry for application launchers.
**Do not skip installing `wind-about`.** You can also run it directly from a
terminal in an X11 session to test independently:

```sh
wind-about
```

Restart the WM without rebooting: use **Super+Shift+Q** to exit, then `startx`
from your Raspberry Pi's local text console. Alternatively run
`./scripts/run-nested.sh` inside your existing graphical session for Xephyr
mode. Do not call `startx` inside an already running desktop.

## Basic shortcuts

| Action | Input |
|---|---|
| Focus | Hover pointer over window |
| Move | **Alt+left-drag** |
| Resize | **Alt+right-drag** |
| Close focused client (including About) | **Super+Shift+C** |
| Terminal | **Super+Shift+Enter** |
| Application menu | Desktop left-click or **Super+P** |
| Cycle focus | **Super+J / Super+K** |
| Reload Lua settings | **Super+Shift+R** |
| Exit wind | **Super+Shift+Q** |

## Files

```
WindWM/
├── src/
│   ├── wind.c            startup, event loop
│   ├── wind.h            WM types and interfaces
│   ├── about.c           launcher for external X11 app
│   ├── clients.c         frames, focus, move, resize
│   ├── config.c          Lua config and menu actions
│   ├── events.c          X11 WM event handling
│   ├── graphics.c        WM panel, Xft text and SVG logo
│   ├── input.c           keyboard commands
│   ├── menu.c            double-buffered application menu
│   └── wallpaper.c       wallpaper and spawn helpers
├── apps/
│   └── wind-about.c      NEW: independent X11 About application
├── assets/               SVG logo and CC0 wallpaper references
├── themes/               Lua theme presets
├── scripts/              nested Xephyr and wallpaper download scripts
├── tests/                Xvfb + XTest regression tests
├── Makefile
└── config.lua.example
```

The 0.3.1 wallpaper and Lua configuration functionality is unchanged. To
fetch the previously selected **CC0 photos** from Wikimedia Commons, run
`make wallpapers`; their sources and licenses are documented in
`assets/WALLPAPERS.md`. No generated wallpapers are included.

## Tests and limitations

This revision was compiled and tested on an x86_64 X11/Xvfb server. Tests
create actual X11 windows and check About-opening, focus, Alt-drag, resizing,
cover/uncover repaint, window class and keyboard closing. It has **not** yet
been validated on Raspberry Pi hardware, although it uses the same Xlib and
Xft APIs available in Debian 13 arm64.

`wind` is still an experimental single-screen floating WM, not a complete
ICCCM/EWMH implementation. Full multiple-monitor workspaces, trays and
fullscreen compatibility are not yet implemented.

To run the regression suite on Debian:

```sh
sudo apt install xvfb libxtst-dev x11-utils
make
./tests/run-xvfb.sh
```

## License

WindWM C source is MIT licensed. The external wallpapers are CC0 photos,
with source pages linked in `assets/WALLPAPERS.md`. Fonts are not bundled.

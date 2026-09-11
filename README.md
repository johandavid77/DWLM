# DWLM

**dwm for Wayland with a Niri-style scroll mode.**

DWLM is a fork of [dwl](https://codeberg.org/dwl/dwl) that adds a horizontally
scrolling "scroll" layout inspired by [Niri](https://github.com/YaLTeR/niri):
windows live in columns on an infinite strip, the viewport scrolls to follow
the focused column, and each column can hold a stack of windows.

It stays true to the suckless philosophy: a small, hackable C codebase built
on wlroots. Configuration is done by editing `src/config.def.h` and
recompiling (in the future, a runtime TOML config will be added).

## Building

Dependencies:

- wlroots (matching `WLRROOTS` in `config.mk`) and `scenefx`
- wayland-protocols, wayland-scanner
- xkbcommon, libinput
- (optional) XWayland support: `libxcb-icccm` (xcb-util-wm)

On Debian Trixie (wlroots 0.18), with the manual scenefx 0.2 build:

```sh
sudo apt install libwlroots-0.18-dev libwayland-dev wayland-protocols \
	libxkbcommon-dev libinput-dev libxcb-icccm4-dev
./packaging/build-scenefx.sh
```

On Arch / Void (wlroots 0.20), set `WLRROOTS = scenefx wlroots` in
`config.mk` and install scenefx from the AUR / Void repos.

Build and install:

```sh
make
sudo make install
# or a proper Debian package:
dpkg-buildpackage -us -uc -b   # needs the build deps from debian/control
```

## Usage

Add a session entry:

```
[Desktop Entry]
Name=dwlm
Comment=dwm for Wayland with Niri-style scroll mode
Exec=dwlm
Type=Application
```

Then pick *dwlm* from your display manager, or start it directly:

```sh
dwlm -s "foot"
```

## Keybindings (default)

The default layout is tiling, mirroring dwl. Scroll mode is one *layout*
(`-->`), selected with `Mod+Shift+S`. `Mod+T`, `Mod+F`, `Mod+M` select tile,
floating and monocle respectively.

Scroll/navigation (work in both tile and scroll layouts):

| Keys                | Action                              |
|---------------------|-------------------------------------|
| `Mod+H` / `Mod+L`   | focus column left / right           |
| `Mod+Shift+H/L`     | move column left / right            |
| `Mod+Home` / `Mod+End` | focus first / last column        |
| `Mod+Ctrl+Home/End` | move column to first / last         |
| `Mod+J` / `Mod+K`   | focus window below / above in column|
| `Mod+R`             | cycle column width preset (33/50/67%)|
| `Mod+-` / `Mod+=`   | shrink / grow column width          |
| `Mod+C`             | center focused column               |
| `Mod+[`             | consume window into column to left  |
| `Mod+]`             | expel window into its own column    |

App/window management:

| Keys              | Action                          |
|-------------------|---------------------------------|
| `Mod+Q`           | close window                    |
| `Mod+A`           | toggle floating                 |
| `Mod+E`           | toggle fullscreen               |
| `Mod+T/F/M`       | tile / floating / monocle layout|
| `Mod+Shift+S`     | scroll layout                   |
| `Mod+Space`       | toggle previous layout          |
| `Mod+Shift+Return`| spawn terminal (`foot`)         |
| `Mod+P`           | run menu (`wmenu-run`)          |
| `Mod+Shift+E`     | quit dwlm                       |

Tags/workspaces: `Mod+1..9` view, `Mod+Shift+1..9` tag, `Mod+Ctrl+1..9`
toggle view, etc. Desktop navigation: `Mod+,`/`Mod+.` focus monitor,
`Mod+Shift+<`/`>` send to monitor.

All bindings are defined in `src/config.def.h`; see the `keys[]` array and the
layout list.

## Layouts

- `[]=` master-and-stack (dwl's `tile`)
- `><>` floating
- `[M]` monocle
- `-->` Niri-style scroll

## Structure

- `src/dwlm.c` — the compositor (fork of dwl 0.7)
- `src/scroll.c` / `src/scroll.h` — the scroll layout (compiled into the same
  translation unit as `dwlm.c`, in the spirit of upstream dwl)
- `src/config.def.h` — compile-time configuration
- `src/config_runtime.c` — TOML runtime config (scroll params + window rules)
- `packaging/` — Debian (root `debian/`), Arch Linux and Void Linux packaging
- `.github/workflows/build.yml` — CI (Debian Trixie, zero-warnings + deb)
- `ROADMAP.md` — development roadmap

## Status bar

The status bar is a separate project in C++ (in the style of Noctalia5),
**outside** this repository. dwlm exposes `wlr-foreign-toplevel-management`
so that bar can track windows (title, app_id, state, placement, close).

## Credits

Forked from [dwl](https://codeberg.org/dwl/dwl) (MIT). Scroll layout design
inspired by Niri. See `LICENSE*` for details.
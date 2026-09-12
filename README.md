# DWLM

<p align="center">
  <b>English</b> · <a href="README.es.md">Español</a>
</p>

<p align="center">
  <img src="dwlm-banner.png" alt="dwlm" width="640">
</p>

**dwm for Wayland with a Niri-style scroll mode.**

Tested across five distros (see [`packaging/`](packaging/) for each recipe):

| Distro | Stack (wlroots / scenefx) | Package           | Status  |
|--------|---------------------------|-------------------|---------|
| Debian 13 (Trixie) | 0.18 / 0.2 | `.deb` (+ GitHub Release, CI) | tested |
| Arch Linux (2026) | 0.18 / 0.2 (AUR) | PKGBUILD | tested |
| Alpine 3.24 | 0.19 / 0.4 | `.apk` | tested |
| Void Linux | 0.19 / 0.4 | `.xbps` (xbps-src) | tested |
| Fedora 44 | 0.20 / 0.5 (snap-built) | `.rpm` | tested |

## Install

Expand the tab for your distro (the full tested recipe of each lives in
`packaging/<distro>/` or, for Debian, in the `debian/` folder at the root):

<details>
<summary><b>Debian 13 (Trixie)</b> — ready-made <code>.deb</code> <kbd>tested</kbd></summary>

A ready-made `.deb` is attached to
[GitHub Releases](https://github.com/johandavid77/DWLM/releases/latest)
(each CI run also uploads the fresh `.deb` as an artifact of the `build`
workflow). On a fresh system:

```sh
# 1. build tools + libraries (scenefx 0.2 needs them to compile)
sudo apt install git debhelper meson ninja-build pkg-config \
	libwlroots-0.18-dev libwayland-dev wayland-protocols \
	libxkbcommon-dev libinput-dev libxcb-icccm4-dev \
	libpixman-1-dev libgbm-dev libdrm-dev xwayland

# 2. the only missing dependency: scenefx 0.2 (not in Debian's repos)
git clone https://github.com/johandavid77/DWLM && cd DWLM
./packaging/build-scenefx.sh

# 3. dwlm itself
wget -qO dwlm.deb https://github.com/johandavid77/DWLM/releases/latest/download/dwlm_0.1.0-1_amd64.deb
sudo apt install ./dwlm.deb
```

</details>

<details>
<summary><b>Arch Linux</b> — PKGBUILD <kbd>tested</kbd> <kbd>2026</kbd></summary>

`packaging/arch` has a PKGBUILD. dwlm targets wlroots 0.18, so on Arch you
first build `wlroots0.18` and `scenefx-0.2` from the AUR (they install the
versioned pkg-config files `wlroots-0.18.pc` / `scenefx-0.2.pc`, which the
PKGBUILD uses). `wlroots0.18` needs a couple of compatibility fixes to
build with current Arch packages — the full tested recipe is in
[`packaging/arch/README.md`](packaging/arch/README.md).

```sh
cd packaging/arch
makepkg -f --noconfirm --skippgpcheck
sudo pacman -U dwlm-*.pkg.tar.zst
```

</details>

<details>
<summary><b>Alpine Linux 3.24</b> — <code>.apk</code> <kbd>tested</kbd></summary>

Build the `.apk` with `packaging/alpine/build-apk.sh` (or follow the native
recipe in [`packaging/alpine/README.md`](packaging/alpine/README.md)). On a
fresh system:

```sh
apk add wlroots0.19 scenefx wayland libxkbcommon libinput xcb-util-wm xwayland
apk add --allow-untrusted ./dwlm-0.1.0-r0.apk
```

</details>

<details>
<summary><b>Void Linux</b> — <code>.xbps</code> <kbd>tested</kbd> <kbd>xbps-src</kbd></summary>

`packaging/void` has a template + `build-xbps.sh`, validated against the
official `xbps-src` buildkit. Void ships `wlroots0.19` and `scenefx` (0.4)
in the repos — the template's runtime deps are collected by the repo build
(incl. `xorg-server-xwayland`). Recipe:
[`packaging/void/README.md`](packaging/void/README.md).

```sh
sudo xbps-install ./dwlm-0.1.0_1.x86_64.xbps
```

</details>

<details>
<summary><b>Fedora 44</b> — three <code>.rpm</code> <kbd>tested</kbd></summary>

Fedora ships `wlroots` 0.20 but no scenefx, so `packaging/fedora`
snap-builds scenefx 0.5 and dwlm as RPMs — `scenefx`, `scenefx-devel`
and `dwlm`. Recipe:
[`packaging/fedora/README.md`](packaging/fedora/README.md).

```sh
sudo dnf install ./scenefx-0.5-1.fc44.x86_64.rpm \
                 ./scenefx-devel-0.5-1.fc44.x86_64.rpm \
                 ./dwlm-0.1.0-1.fc44.x86_64.rpm
```

The wlroots 0.20 API changes (xwayland cursor buffer, corner radius,
xdg-shell constants) are handled in `src/dwlm.c` behind
`#if WLR_VERSION_0_20`.

</details>

## The philosophy

DWLM is built on three ideas that, combined, shape everything else:

**Suckless.** A small, readable C codebase built on
[wlroots](https://gitlab.freedesktop.org/wlroots/wlroots). There is no giant
config file to tame and no settings UI to dig through: the whole system fits
in a phone screen of source code, and if you want it different you edit
`src/config.def.h` and recompile. What the upstream dwm/dwl authors call "the
right thing as a bare-bones, non-bloated program".

**dwm's tiling.** Tiling done the dwm way: no decorations to drag, no
overlapping windows to babysit. The keyboard is the input device — every
action is a key away, with no hidden menus to discover. Tags are the
workspaces, and `Mod+1..9` moves you around them while `Mod+Shift+E` is
the only "settings menu" you will ever need: the exit.

**The infinite scroll.** From [Niri](https://github.com/YaLTeR/niri) DWLM
borrows its most liberating idea: instead of a fixed grid of workspaces, the
desktop is an *infinite horizontal strip*. Every window you open becomes a
column, the viewport follows your focus with a smooth pan, and your session
scales with you — you never run out of workspaces, you never have to decide
in advance "how many" you need. Each column can still hold a vertical stack
of windows, and the whole thing fades into classic dwm layouts (`tile`,
`floating`, `monocle`) whenever you want them.

The result keeps the suckless principle — hack all of it, understand all of
it — while borrowing the scroll philosophy: *the desktop should grow with
you, not stay still.*

DWLM is a fork of [dwl](https://codeberg.org/dwl/dwl) that takes this a step
further: the scroll mode is not a patch on top, it is a layout the compositor
understands natively, with animated viewport, per-monitor strips, floating
windows anchored to the visible area and whole-column moves between monitors.

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

On Arch use the PKGBUILD under `packaging/arch` (tested; needs `wlroots0.18`
and `scenefx-0.2` from the AUR — full recipe in `packaging/arch/README.md`).
On Alpine 3.24 use the APKBUILD under `packaging/alpine` (tested; builds against
`wlroots0.19` and `scenefx-0.4` from the official repos — recipe and one-shot
build script in `packaging/alpine/README.md`).
On Void use the template under `packaging/void` (tested via `xbps-src`; recipe
in `packaging/void/README.md`).
On Fedora 44 there's no scenefx in the repos: `packaging/fedora` builds scenefx
0.5 and dwlm from source as RPMs (`build-dwlm.sh`, tested).
Alternatively build against a wlroots 0.20 system directly by setting
`WLRROOTS = scenefx wlroots` (or `scenefx-0.5 wlroots-0.20`) in `config.mk`
and installing a wlroots 0.20-compatible scenefx from the AUR / Void repos.

Build and install:

```sh
make
sudo make install
# or a proper Debian package:
dpkg-buildpackage -us -uc -b   # needs the build deps from debian/control
```

## Usage

`make install` already drops a `dwlm.desktop` session entry into
`share/wayland-sessions`, which display managers (ly, greetd, GDM, SDDM…)
pick up automatically. To add it by hand, save this as `dwlm.desktop` in
`/usr/share/wayland-sessions` (system-wide) or
`~/.local/share/wayland-sessions` (per user):

```
[Desktop Entry]
Name=dwlm
Comment=dwm for Wayland with Niri-style scroll mode
Exec=dwlm
Type=Application
```

CLI options:

- `dwlm` — normal start
- `dwlm -s "foot"` — also start a program (e.g. a terminal) on launch
- `dwlm -d` — full wlroots debug logging
- `dwlm -v` — print the logo and version, then exit

## Runtime configuration

Besides the compile-time config (`src/config.def.h`), dwlm reads a TOML
overlay from `$XDG_CONFIG_HOME/dwlm/config.toml` (defaults to
`~/.config/dwlm/config.toml`) and re-applies it **live on every `SIGHUP`** —
colors, gaps, width presets and scroll behavior all change without
restarting:

```sh
kill -HUP "$(pgrep dwlm)"
```

Complete reference (keys are optional; omitted ones keep the compiled
default):

```toml
[general]
borderpx         = 3               # focus ring thickness (px)
corner_radius    = 10              # rounded corners (scenefx)
gap              = 12.0            # gap between columns/windows (px)
outer_gap        = 12.0            # margin around the tiling area (px)
width_min        = 0.20            # grow/shrink clamps (fraction of strip)
width_max        = 0.80
presets          = [0.33, 0.50, 0.67]
mouse_scroll     = 1               # 0/1: wheel/touchpad pan the strip in scroll mode
pixels_per_notch = 80.0            # strip px scrolled per wheel notch
continuous_speed = 1.5             # touchpad / continuous delta sensitivity
anim_ms          = 250             # viewport animation duration (0 = snap)
anim_ease        = 1               # 0: linear, 1: cubic ease-in-out

[colors]                           # #rrggbb or #rrggbbaa
root    = "#222222"
border  = "#222222"                # focus ring rest state (blends with root)
focus   = "#66c2ff"
urgent  = "#ff0000"

[rules.1]                          # runtime window rules (up to 24, 1-based)
app_id     = "foot"                # substring match on the window app id
title      = "Calculator"          # optional substring match on the title
isfloating = true                  # start this app floating
tags       = 1                     # optional tag mask
```

Rules under `[rules.N]` are matched **after** the compiled rules, so the
runtime values win. The parser is dependency-free and the file is drained in
the main loop (no async work inside the signal handler).

## Troubleshooting

- **`Mod+Return`/`Mod+P` do nothing.** The default `termcmd`/`menucmd` are
  `foot` and `wmenu-run`; the compositor ships no applications. Install them
  (Debian: `sudo apt install foot wmenu`, Arch: `sudo pacman -S foot wmenu`,
  Alpine: `apk add foot wmenu`, Void: `sudo xbps-install foot wmenu`,
  Fedora: `sudo dnf install foot wmenu`).
- **Cursor renders inverted/garbled on a VM (VirtIO, VMware).** Disable the
  KMS hardware cursor since wlroots would otherwise draw the cursor sprite on
  a virtual GPU: `Exec=/usr/bin/env WLR_NO_HARDWARE_CURSORS=1 dwlm`.
- **ly aborts login with `FileNotFound`.** ly's `session_log` points at
  `~/.local/state/ly-session.log` and does not create the directory; run
  `mkdir -p ~/.local/state` first.
- **Arch: AUR `wlroots0.18` fails to build** (`-Werror=switch`,
  `-Werror=discarded-qualifiers`) against current Arch packages. See
  `packaging/arch/README.md` for the tested recipe.

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
| `Mod+Shift+V`       | cycle column width preset (33/50/67%)|
| `Mod+R` / `Mod+Shift+R` | toggle column resize mode      |
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
| `Mod+M`           | toggle maximized                |
| `Mod+T/F`         | tile / floating layout          |
| `Mod+Shift+M`     | monocle layout                  |
| `Mod+Shift+S`     | scroll layout                   |
| `Mod+Space`       | toggle previous layout          |
| `Mod+Shift+Return`| spawn terminal (`foot`)         |
| `Mod+P`           | run menu (`wmenu-run`)          |
| `Mod+Shift+E`     | quit dwlm                       |

Tags/workspaces: `Mod+1..9` view, `Mod+Shift+1..9` tag, `Mod+Ctrl+1..9`
toggle view, etc. `Mod+Alt+1..9` focuses the Nth scroll column (workspace N
while tiling). Desktop navigation: `Mod+,`/`Mod+.` focus monitor,
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
- `src/config_runtime.c` — TOML runtime config (scroll params + window rules;
  live-reloaded on `SIGHUP`, see [Runtime configuration](#runtime-configuration))
- `packaging/` — Debian (root `debian/`), Arch Linux (PKGBUILD + recipe
  README, tested), Alpine Linux (APKBUILD + recipe README + build script,
  tested), Void Linux (template + build script, tested) and Fedora (RPM
  specs + build script, tested) packaging
- `.github/workflows/build.yml` — CI (Debian Trixie, zero-warnings + deb)
- `dwlm.svg` — el logo de dwlm, basado en el logo original de DWM
- `dwlm-banner.png` — banner del README
- `dwlm-social.png` — social preview image (GitHub repo settings)
- `ROADMAP.md` — development roadmap
- `CHANGELOG.md` — changelog
- `src/dwlm.1` — man page (see also the `SIGHUP` reload note)

## Status bar

The status bar is a separate project in C++ (in the style of Noctalia5),
**outside** this repository. dwlm exposes `wlr-foreign-toplevel-management`
so that bar can track windows (title, app_id, state, placement, close).

## Credits

Forked from [dwl](https://codeberg.org/dwl/dwl). Scroll layout design
inspired by Niri. See `LICENSE*` for details.

## License

GPL-3.0-or-later — see [LICENSE](LICENSE). dwl's attribution and license note
live in [LICENSE.dwl](LICENSE.dwl); upstream MIT licenses are in
`LICENSE.tinywl`, `LICENSE.dwm` and `LICENSE.sway`.

<a href="LICENSE"><img alt="License: GPLv3" src="https://img.shields.io/badge/license-GPLv3-blue.svg"></a>
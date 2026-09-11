# Building dwlm on Arch Linux (tested)

dwlm targets wlroots 0.18. Current Arch Linux has dropped the unversioned
`wlroots` / `scenefx` packages and only ships `wlroots0.20`, so the 0.18-era
stack has to come from the AUR as `wlroots0.18` and `scenefx-0.2`, which
install the versioned pkg-config files `wlroots-0.18.pc` and `scenefx-0.2.pc`.

This recipe was tested end-to-end on a fresh minimal Arch system (2026):
AUR builds → dwlm PKGBUILD → install → `dwlm -v` → compositor running.

## 0. Base packages

```sh
sudo pacman -S --needed base-devel git \
	libxkbcommon xcb-util-wm wayland wayland-protocols \
	meson ninja pkgconf xorg-xwayland libinput glslang \
	xcb-util-errors xcb-util-renderutil \
	lcms2 pixman seatd vulkan-icd-loader mesa libdrm \
	libdisplay-info libliftoff libxcb vulkan-headers
```

Note: several traditional package names moved on Arch 2026 — the providers of
`libseat.so`, `libpixman-1`, `liblcms2` and `libvulkan` are now `seatd`,
`pixman`, `lcms2` and `vulkan-icd-loader` respectively.

## 1. wlroots 0.18 (AUR)

The AUR `wlroots0.18` no longer builds out of the box against current Arch
packages (`-Werror` trips over newer libinput/libxcb). Apply these two fixes:

```sh
git clone https://aur.archlinux.org/wlroots0.18.git
cd wlroots0.18

# extract + prepare only (the stock build would fail here otherwise)
makepkg -o -A --noconfirm --skippgpcheck

# fix 1: libinput 2026 added LIBINPUT_SWITCH_KEYPAD_SLIDE, which wlroots 0.18
# doesn't handle (-Werror=switch)
sed -i 's#\tcase LIBINPUT_SWITCH_TABLET_MODE:#\tcase LIBINPUT_SWITCH_TABLET_MODE:\n\tcase LIBINPUT_SWITCH_KEYPAD_SLIDE:#' \
	src/wlroots0.18/backend/libinput/switch.c

# fix 2: newer libxcb discards const -> -Werror=discarded-qualifiers
sed -i 's/arch-meson "${pkgname}" build/arch-meson "${pkgname}" build -Dwerror=false/' PKGBUILD

# build using the existing $srcdir and install
makepkg -e -A --noconfirm --skippgpcheck
sudo pacman -U wlroots0.18-*.pkg.tar.zst
```

## 2. scenefx 0.2 (AUR)

```sh
git clone https://aur.archlinux.org/scenefx-0.2.git
cd scenefx-0.2
makepkg -A --noconfirm --skippgpcheck
sudo pacman -U scenefx-0.2-*.pkg.tar.zst
```

## 3. dwlm

The PKGBUILD in this directory is already wired for the 0.18 stack:

```sh
git clone https://github.com/johandavid77/DWLM
cd DWLM/packaging/arch
makepkg -f --noconfirm --skippgpcheck
sudo pacman -U dwlm-*.pkg.tar.zst
```

## 4. Verify / run

```sh
dwlm -v                # prints logo + version
ldd "$(which dwlm)"    # libwlroots-0.18 + libscenefx-0.2, no "not found"
```

`make install` drops `dwlm.desktop` into `/usr/share/wayland-sessions`, which
display managers pick up. With ly, auto-login needs `auto_login_user` /
`auto_login_session` in `/etc/ly/config.lua` and an existing `~/.local/state`
directory (ly's `session_log` target — if missing, ly aborts login with
`FileNotFound`).

## 5. Session apps

The default keybindings spawn `foot` (`Mod+Return`) and `wmenu-run`
(`Mod+P`); the compositor ships no applications:

```sh
sudo pacman -S --needed foot wmenu
```
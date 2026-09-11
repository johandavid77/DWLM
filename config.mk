_VERSION = 0.1.0
VERSION  = `git describe --tags --dirty 2>/dev/null || echo $(_VERSION)`

PKG_CONFIG = pkg-config

# paths
PREFIX = /usr/local
MANDIR = $(PREFIX)/share/man
DATADIR = $(PREFIX)/share

# wlroots package to link against. Debian Trixie ships wlroots 0.18
# (libwlroots-0.18-dev); Alpine 3.24 ships wlroots 0.19 (wlroots0.19);
# Arch Linux and Void Linux ship wlroots 0.20.
# Leave empty to use an unversioned wlroots.
# scenefx (wlrfx/scenefx v0.2) provides the FX renderer + rounded corners; keep
# the base wlroots linked after it so FX scene symbols interpose, while base
# backend/output symbols resolve from libwlroots.
WLRROOTS = scenefx wlroots-0.18

XWAYLAND = -DXWAYLAND
XLIBS = xcb xcb-icccm
# Uncomment to build XWayland support
#XWAYLAND = -DXWAYLAND
#XLIBS = xcb xcb-icccm

CC = gcc
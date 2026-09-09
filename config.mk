_VERSION = 0.1.0
VERSION  = `git describe --tags --dirty 2>/dev/null || echo $(_VERSION)`

PKG_CONFIG = pkg-config

# paths
PREFIX = /usr/local
MANDIR = $(PREFIX)/share/man
DATADIR = $(PREFIX)/share

# wlroots package to link against. Debian Trixie ships wlroots 0.18
# (libwlroots-0.18-dev); Arch Linux and Void Linux ship wlroots 0.20.
# Leave empty to use an unversioned wlroots.
WLRROOTS = wlroots-0.18

XWAYLAND = -DXWAYLAND
XLIBS = xcb xcb-icccm
# Uncomment to build XWayland support
#XWAYLAND = -DXWAYLAND
#XLIBS = xcb xcb-icccm

CC = gcc
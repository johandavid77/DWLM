#!/bin/sh
# Build and install SceneFX 0.2 (the FX renderer, wlrfx/scenefx) into the
# system, as required by dwlm (WLRROOTS = scenefx wlroots-0.18).
#
# Run as root (Debian Trixie, with libwlroots-0.18-dev installed):
#   ./packaging/build-scenefx.sh [ref]   (ref defaults to 0.2)
set -eu

REF="${1:-0.2}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "==> fetching scenefx ${REF}"
git clone -q --depth 1 --branch "${REF}" \
	https://github.com/wlrfx/scenefx.git "$TMP/scenefx"

echo "==> meson/ninja"
meson setup "$TMP/build" "$TMP/scenefx" --buildtype=release
ninja -C "$TMP/build"

echo "==> installing to system"
meson install -C "$TMP/build"

echo "==> done: $(pkg-config --modversion scenefx)"
#!/bin/sh
# Build SceneFX 0.2 as a Debian package (libscenefx-0.2 / -dev) and install
# it, as required by dwlm (WLRROOTS = scenefx wlroots-0.18). The package
# metadata lives in packaging/scenefx/debian and is copied into a shallow
# clone of wlrfx/scenefx so dpkg sees a proper ./debian at its root.
#
# Needs: debhelper, git, meson, ninja-build and the build deps from
# packaging/scenefx/debian/control. Run as root on Debian Trixie.
#   ./packaging/build-scenefx.sh [ref]   (ref defaults to 0.2)
set -eu

REF="${1:-0.2}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "==> fetching scenefx ${REF}"
git clone -q --depth 1 --branch "${REF}" \
	https://github.com/wlrfx/scenefx.git "$TMP/scenefx"
cp -r "$ROOT/packaging/scenefx/debian" "$TMP/scenefx/debian"

echo "==> dpkg-buildpackage"
cd "$TMP/scenefx"
dpkg-buildpackage -us -uc -b -d

echo "==> installing packages"
[ "$(id -u)" = 0 ] || SUDO=sudo
DEBIAN_FRONTEND=noninteractive $SUDO apt-get install -y \
	"$TMP"/libscenefx-0.2_*.deb "$TMP"/libscenefx-0.2-dev_*.deb

echo "==> done: $(pkg-config --modversion scenefx)"
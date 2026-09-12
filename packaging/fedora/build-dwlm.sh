#!/bin/sh
set -eu

# Build dwlm + scenefx RPMs for Fedora inside a throwaway podman container.
#
# Fedora ships wlroots 0.20 but NO scenefx, so this builds scenefx 0.5
# (the wlroots-0.20 flavor) from upstream first, then dwlm against it.
#
# Usage: sh packaging/fedora/build-dwlm.sh
# Result: packaging/fedora/out/:
#   scenefx-0.5-1.*.x86_64.rpm  scenefx-devel-0.5-1.*.x86_64.rpm
#   dwlm-0.1.0-1.*.x86_64.rpm

REPO_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
OUT="$REPO_ROOT/packaging/fedora/out"
IMG="fedora:latest"
CT="dwlm-fedora-rpm"
SCENEFX_URL="https://github.com/wlrfx/scenefx/archive/refs/tags/0.5.tar.gz"

mkdir -p "$OUT"
podman rm -f "$CT" >/dev/null 2>&1 || true
trap 'podman rm -f "$CT" >/dev/null 2>&1 || true' EXIT INT TERM

podman run -d --name "$CT" "$IMG" sleep infinity >/dev/null

# build tooling + deps for scenefx and dwlm
podman exec "$CT" dnf -y install \
	rpm-build rpmdevtools meson ninja-build gcc make pkgconfig \
	wlroots-devel wayland-devel wayland-protocols-devel \
	libinput-devel libxkbcommon-devel xcb-util-wm-devel \
	libdrm-devel pixman-devel mesa-libEGL-devel mesa-libGLES-devel \
	lcms2-devel >/dev/null

podman exec "$CT" sh -c '
	rpmdev-setuptree
	curl -sL "'"$SCENEFX_URL"'" -o /root/rpmbuild/SOURCES/scenefx-0.5.tar.gz
'

# dwlm source tarball, snapshot of the current working tree (no release tag yet;
# works with uncommitted changes). Ignored artifacts are excluded.
tar -C "$REPO_ROOT" --exclude=.git --exclude='packaging/*/out' \
	--exclude='*.xbps' --exclude='*.apk' --exclude='*.tar.gz' \
	--transform 's,^,dwlm-0.1.0/,' -czf /tmp/dwlm-0.1.0.tar.gz .
podman cp /tmp/dwlm-0.1.0.tar.gz "$CT:/root/rpmbuild/SOURCES/dwlm-0.1.0.tar.gz"
podman cp "$REPO_ROOT/packaging/fedora/scenefx.spec" "$CT:/root/rpmbuild/SPECS/scenefx.spec"
podman cp "$REPO_ROOT/packaging/fedora/dwlm.spec" "$CT:/root/rpmbuild/SPECS/dwlm.spec"

podman exec "$CT" sh -c '
	rpmbuild -bb /root/rpmbuild/SPECS/scenefx.spec >/tmp/build-scenefx.log 2>&1 \
		|| { echo "scenefx build failed:"; tail -30 /tmp/build-scenefx.log; exit 1; }
	dnf -y install /root/rpmbuild/RPMS/x86_64/scenefx-*.rpm >/dev/null
	rpmbuild -bb /root/rpmbuild/SPECS/dwlm.spec >/tmp/build-dwlm.log 2>&1 \
		|| { echo "dwlm build failed:"; tail -30 /tmp/build-dwlm.log; exit 1; }
	mkdir -p /rpms
	cp /root/rpmbuild/RPMS/x86_64/scenefx*.rpm /root/rpmbuild/RPMS/x86_64/dwlm*.rpm /rpms/
	cp /root/rpmbuild/RPMS/noarch/*.rpm /rpms/ 2>/dev/null || true
'

rm -rf /tmp/fedora-rpms && mkdir -p /tmp/fedora-rpms
podman cp "$CT:/rpms/." /tmp/fedora-rpms/
cp /tmp/fedora-rpms/*.rpm "$OUT/"
echo "OK:"; ls -1 "$OUT"
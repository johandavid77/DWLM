#!/bin/sh
# Build the dwlm .apk for Alpine Linux (tested against Alpine 3.24) using
# podman, so it can be produced from any Linux host without dropping into
# Alpine's toolchain directly. The result lands in packaging/alpine/out/.
#
# The friend target installs it with:
#   apk add wlroots0.19 scenefx wayland libxkbcommon libinput xcb-util-wm xwayland
#   apk add --allow-untrusted ./dwlm-0.1.0-r0.apk
set -eu

IMG=docker.io/library/alpine:3.24
CT=dwlm-abuild
HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(dirname "$(dirname "$HERE")")
PV=0.1.0
OUT="$HERE/out"

mkdir -p "$OUT"

# source tarball from the current git HEAD (no v0.1.0 tag exists yet)
git -C "$REPO" archive --format=tar.gz --prefix="dwlm-$PV/" -o "$HERE/dwlm-$PV.tar.gz" HEAD

podman pull "$IMG" >/dev/null
podman rm -f "$CT" >/dev/null 2>&1 || true
podman run -d --name "$CT" "$IMG" sleep infinity >/dev/null

podman exec "$CT" sh -c 'apk add --quiet --no-cache \
	build-base git make pkgconf abuild openssl \
	wlroots0.19-dev scenefx-dev wayland-dev wayland-protocols \
	libinput-dev libxkbcommon-dev xcb-util-wm-dev xwayland'

podman exec "$CT" sh -c '
adduser -D -h /home/builder -u 10000 -s /bin/sh builder
mkdir -p /home/builder/.abuild /home/builder/aport/dwlm
printf "PACKAGER=\"builder <johandavid77@users.noreply.github.com>\"\n" \
	> /home/builder/.abuild/abr.conf
chown -R builder:builder /home/builder'

# APKBUILD must end with a newline (abuild appends the refreshed sha512sums)
if [ -n "$(tail -c1 "$HERE/APKBUILD" 2>/dev/null)" ]; then
	printf '\n' >> "$HERE/APKBUILD"
fi

podman cp "$HERE/APKBUILD" "$CT:/home/builder/aport/dwlm/APKBUILD"
podman cp "$HERE/dwlm-$PV.tar.gz" "$CT:/home/builder/aport/dwlm/"
podman exec "$CT" sh -c 'su builder -c "abuild-keygen -a -n"' >/dev/null 2>&1 || true
podman exec "$CT" sh -c 'su builder -c "cd /home/builder/aport/dwlm && abuild checksum"'
podman exec "$CT" sh -c 'su builder -c "cd /home/builder/aport/dwlm && abuild -F -d -P /home/builder/repodest"' || true

# the repo-index step at the end fails on the untrusted self-signed key; the
# .apk itself is already produced, so grab it wherever abuild put it.
podman exec "$CT" sh -c "find /home/builder/repodest -name 'dwlm-$PV-r0.apk' -exec cp {} /tmp/dwlm.apk ';'"
podman exec --user root "$CT" sh -c "test -s /tmp/dwlm.apk" || {
	echo "ERROR: abuild produced no dwlm-$PV-r0.apk" >&2
	exit 1
}
podman cp "$CT:/tmp/dwlm.apk" "$OUT/dwlm-$PV-r0.apk"
podman rm -f "$CT" >/dev/null 2>&1 || true

echo "OK: $OUT/dwlm-$PV-r0.apk"
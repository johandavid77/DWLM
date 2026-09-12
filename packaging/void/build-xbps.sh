#!/bin/sh
set -eu

# Build dwlm-0.1.0_1 for Void Linux using xbps-src (the official Void
# buildkit) inside a throwaway podman container.
#
# Usage: sh packaging/void/build-xbps.sh
# Result: packaging/void/out/dwlm-0.1.0_1.x86_64.xbps

REPO_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
OUT="$REPO_ROOT/packaging/void/out"
IMG="void-glibc"   # = ghcr.io/void-linux/void-glibc
CT="dwlm-void-xbps"
PKG="dwlm-0.1.0_1.x86_64.xbps"

mkdir -p "$OUT"
podman rm -f "$CT" >/dev/null 2>&1 || true
trap 'podman rm -f "$CT" >/dev/null 2>&1 || true' EXIT INT TERM

podman run -d --name "$CT" "$IMG" sleep infinity >/dev/null
podman exec "$CT" sh -c '
	xbps-install -Syu >/dev/null 2>&1 || true
	xbps-install -y bash git util-linux ca-certificates >/dev/null
	grep -q "^builder:" /etc/passwd || printf "builder:x:1000:1000::/home/builder:/bin/sh\n" >> /etc/passwd
	grep -q "^builder:" /etc/group || printf "builder::1000:\n" >> /etc/group
	mkdir -p /home/builder && chown 1000:1000 /home/builder
	rm -rf /tmp/void-packages
	git clone --quiet --depth=1 https://github.com/void-linux/void-packages /tmp/void-packages
	mkdir -p /tmp/void-packages/srcpkgs/dwlm
	chown -R builder /tmp/void-packages /home/builder
'

podman cp "$REPO_ROOT/packaging/void/template" "$CT:/tmp/void-packages/srcpkgs/dwlm/template"
podman exec "$CT" chown builder /tmp/void-packages/srcpkgs/dwlm/template

run_as_builder() {
	podman exec "$CT" setpriv --reuid=1000 --regid=1000 --clear-groups \
		/bin/sh -c "cd /tmp/void-packages && $1"
}

run_as_builder './xbps-src binary-bootstrap'
run_as_builder './xbps-src pkg dwlm'

podman cp "$CT:/tmp/void-packages/hostdir/binpkgs/$PKG" "$OUT/"
echo "OK: $OUT/$PKG"
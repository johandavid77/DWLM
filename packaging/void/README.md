# Building for Void Linux

dwlm builds and installs cleanly on **Void Linux (glibc, x86_64)** using the
official **xbps-src** buildkit. Void ships `wlroots0.19` (0.19.3) and
`scenefx` (0.4.1) — the same wlroots 0.19 / scenefx 0.4 stack as
Alpine, which dwlm auto-detects via the `WLR_VERSION_0_19` guard.

Native pkg-config modules on Void:

```
pkg-config --modversion wlroots-0.19   # 0.19.3  (package: wlroots0.19-devel)
pkg-config --modversion scenefx-0.4    # 0.4.1   (package: scenefx-devel)
```

Note: Void has **no `xwayland` virtual package** — the runtime dependency is
the real package `xorg-server-xwayland` (like Arch). Installing from the
system repo, Void needs the `xorg` repository enabled (a standard desktop
install has it).

## Binary package (what a user installs)

```sh
# from this repo:
sh packaging/void/build-xbps.sh
# -> packaging/void/out/dwlm-0.1.0_1.x86_64.xbps

# on a Void desktop (deps come from the normal Void repos):
sudo mkdir -p /usr/local/dwlmlocal
sudo cp dwlm-0.1.0_1.x86_64.xbps /usr/local/dwlmlocal/
cd /usr/local/dwlmlocal
sudo xbps-rindex -a dwlm-0.1.0_1.x86_64.xbps
sudo xbps-install dwlm
# (enable the "xorg" repository in /etc/xbps.d if `xorg-server-xwayland`
#  is not found)
```

Then pick it as your session: `dwlm` registers a `dwlm.desktop` in
`/usr/share/wayland-sessions/` and ships a man page (`man 1 dwlm`).

Install the packages dwlm expects on the desktop:

```sh
sudo xbps-install wlroots0.19 scenefx foot wmenu xorg-server-xwayland
```

## Building with xbps-src (the Void way)

`build-xbps.sh` runs the real pipeline in a throwaway `podman` container:

1. clones `void-linux/void-packages`,
2. `./xbps-src binary-bootstrap`,
3. drops this repo's `template` at `srcpkgs/dwlm/template`,
4. `./xbps-src pkg dwlm`,
5. copies `hostdir/binpkgs/dwlm-0.1.0_1.x86_64.xbps` into `out/`.

It was validated end-to-end this way against the current shell (Sep 2026):
the chroot build compiles with `-DWLR_VERSION_0_19`, pkglint checks the
package, and the collected runtime deps come out as:

```
xorg-server-xwayland scenefx glibc libinput wayland wlroots0.19
xcb-util-wm libxcb libxkbcommon ...
```

Manually (on a real Void box):

```sh
sudo xbps-install git make bash util-linux
git clone https://github.com/void-linux/void-packages
cd void-packages
./xbps-src binary-bootstrap      # as a non-root user
cp <repo>/packaging/void/template srcpkgs/dwlm/template
./xbps-src pkg dwlm
# -> hostdir/binpkgs/dwlm-0.1.0_1.x86_64.xbps
```

## Template quirks worth knowing

- `template` uses the Void template format (`version`/`revision`,
  `short_desc`, `wlroots0.19-devel`/`scenefx-devel` in `makedepends`,
  `depends="xorg-server-xwayland scenefx"`).
- `do_build` guards `makejobs`: inside some containers/nproc reports lead to
  `makejobs=0`, which makes GNU make abort with
  `the '-j' option requires a positive integer` (seen in the podman chroot).
- the `distfiles`/`checksum` point at the `93bc40b` snapshot tarball; bump
  both if the released source moves.
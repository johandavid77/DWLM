# Building dwlm on Alpine Linux (tested)

dwlm targets wlroots 0.18 (dwl 0.7 base). Alpine 3.22 ships wlroots 0.18 but
has no `scenefx`; **Alpine 3.24 ships wlroots 0.19 (`wlroots0.19`) and
scenefx 0.4 (`scenefx`)**, so the build uses the 0.19 stack with the versioned
pkg-config files `wlroots-0.19.pc` and `scenefx-0.4.pc`.

wlroots 0.19 removed a handful of 0.18 helpers, so the Makefile auto-detects
the base wlroots version and sets `-DWLR_VERSION_0_19`, which switches the
code over to the 0.19 API (the `geometry` field of `wlr_xdg_surface`, the
renamed xwayland helpers `wlr_xwayland_surface_override_redirect_wants_focus`
/ `wlr_xwayland_surface_icccm_input_model`, and
`wlr_presentation_create(dpy, backend, 1)`). 0.18 builds (Debian Trixie, Arch
`wlroots0.18`) are unaffected.

This recipe was tested end-to-end in a clean `alpine:3.24` container: abuild
→ `.apk` → fresh-install with `apk add --allow-untrusted` → `dwlm -v` +
`ldd` resolving `libwlroots-0.19.so`/`libscenefx-0.4.so`.

## 1. Build the .apk (any Linux host, needs podman)

```sh
sh packaging/alpine/build-apk.sh
# outputs packaging/alpine/out/dwlm-0.1.0-r0.apk
```

## 2. Or build natively on Alpine

```sh
apk add abuild build-base git make pkgconf openssl \
	wlroots0.19-dev scenefx-dev wayland-dev wayland-protocols \
	libinput-dev libxkbcommon-dev xcb-util-wm-dev xwayland

adduser -D builder
su builder -c "mkdir -p /home/builder/.abuild /home/builder/aport/dwlm \
	&& abuild-keygen -a -n"

# source tarball from a checkout of this repo
git archive --format=tar.gz --prefix=dwlm-0.1.0/ -o /tmp/dwlm-0.1.0.tar.gz HEAD
cp packaging/alpine/APKBUILD /tmp/dwlm-0.1.0.tar.gz /home/builder/aport/dwlm/

su builder -c "cd /home/builder/aport/dwlm && abuild checksum \
	&& abuild -d -P /tmp/repodest"
# built: find /tmp/repodest -name 'dwlm-0.1.0-r0.apk'
```

## 3. Install on the target (your friend's Alpine 3.24)

```sh
apk add wlroots0.19 scenefx wayland libxkbcommon libinput xcb-util-wm xwayland
apk add --allow-untrusted ./dwlm-0.1.0-r0.apk
```

## 4. Verify / run

```sh
dwlm -v                # prints logo + version
ldd "$(which dwlm)"    # libwlroots-0.19 + libscenefx-0.4, no "not found"
```

`make install` drops `dwlm.desktop` into `/usr/share/wayland-sessions`, which
display managers pick up automatically.

## 5. Session apps

The default keybindings spawn `foot` (`Mod+Return`) and `wmenu-run`
(`Mod+P`); the compositor ships no applications, so install them too:

```sh
apk add foot wmenu
```
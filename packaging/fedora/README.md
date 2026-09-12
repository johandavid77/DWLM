# Fedora packaging (wlroots 0.20 + scenefx 0.5)

Fedora ships **wlroots 0.20** but does **not** package
[scenefx](https://github.com/wlrfx/scenefx). dwlm needs scenefx, so this
folder builds both from source as RPMs:

| File | What it is |
| --- | --- |
| `scenefx.spec` | spec that snap-builds scenefx **0.5** (the wlroots-0.20 flavor) from upstream; also produces `scenefx-devel` |
| `dwlm.spec` | spec that builds dwlm against `scenefx-0.5 wlroots-0.20` with XWayland support |
| `build-dwlm.sh` | one-shot script: builds both specs inside a throwaway `fedora:latest` podman container and drops the RPMs in `out/` |
| `out/` | generated RPMs (git-ignored) |

## Why scenefx is snap-built

scenefx is not in the Fedora repos (only wlroots 0.15/0.17 exist as
versioned packages, and plain `wlroots` is 0.20). The `scenefx.spec` here
pulls the upstream `0.5` tag and builds it locally; `dwlm.spec` then
`Requires: scenefx` and `BuildRequires: scenefx-devel` like any packaged
dependency. A COPR with the real rpm could replace this later.

## Build

```sh
sh packaging/fedora/build-dwlm.sh
```

Requires: podman (or docker) on the host. The script starts a throwaway
`fedora:latest` container, installs the build deps, builds `scenefx` +
`scenefx-devel`, installs them, builds `dwlm`, and copies every RPM to
`packaging/fedora/out/`.

Resulting RPMs:

```
scenefx-0.5-1.fc44.x86_64.rpm        # runtime library
scenefx-devel-0.5-1.fc44.x86_64.rpm  # headers + pkg-config
dwlm-0.1.0-1.fc44.x86_64.rpm         # the compositor
```

## Install on Fedora

```sh
sudo dnf install ./scenefx-0.5-1.fc44.x86_64.rpm \
                 ./scenefx-devel-0.5-1.fc44.x86_64.rpm \
                 ./dwlm-0.1.0-1.fc44.x86_64.rpm
```

`wlroots`, `xwayland` and the rest are pulled in from the Fedora repos
automatically. Then log in via the `dwlm.desktop` wayland session (`Install
JS` bar style), or launch with:

```sh
dbus-run-session -- dwlm
```

A note on ordering: `scenefx-devel` must be installed before building
`dwlm` from the spec, but end users only need the two runtime packages
(`scenefx` + `dwlm`). The script installs all three because it also builds
`dwlm` afterwards.

## Compatibility wiring (always tested against)

- wlroots 0.20 split: `wlr_xwayland_set_cursor()` takes a `struct wlr_buffer *`
  (the 0.18/0.19 signature took a raw pixel buffer), corner helpers dropped
  their `corner_location` argument, and xdg-shell protocol constants are no
  longer re-exported. `src/dwlm.c` has `#if WLR_VERSION_0_20` branches and
  wraps the XCursor pixels in a tiny read-only `wlr_buffer`.
- scenefx 0.5 is required for wlroots 0.20 (0.2/0.4 target older wlroots).
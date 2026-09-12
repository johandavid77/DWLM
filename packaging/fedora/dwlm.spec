# dwlm — Wlroots Wayland compositor with a Niri-style scroll workspace
Name:           dwlm
Version:        0.1.0
Release:        1%{?dist}
Summary:        Wlroots Wayland compositor with a Niri-style scroll workspace

License:        GPL-3.0-or-later
URL:            https://github.com/johandavid77/DWLM
Source0:        dwlm-0.1.0.tar.gz
BuildRequires:  gcc make pkgconfig wlroots-devel wayland-devel
BuildRequires:  wayland-protocols-devel libinput-devel libxkbcommon-devel
BuildRequires:  xcb-util-wm-devel scenefx-devel
Requires:       wlroots scenefx xorg-x11-server-Xwayland

%description
dwlm is a fork of dwl that adds a horizontal scroll layout: windows stack in
columns on an infinite strip, panned with the wheel or a two-finger touchpad
swipe, with an animated viewport and per-column workspaces, alongside the
classic dwm tiling layouts. Uses scenefx for rounded corners.

%prep
%setup -q

%build
make \
	WLRROOTS="scenefx-0.5 wlroots-0.20" \
	XWAYLAND=-DXWAYLAND XLIBS="xcb xcb-icccm" \
	PREFIX=%{_prefix} MANDIR=%{_mandir} DATADIR=%{_datadir}

%install
make install DESTDIR=%{buildroot} \
	PREFIX=%{_prefix} MANDIR=%{_mandir} DATADIR=%{_datadir}

%files
%{_bindir}/dwlm
%{_mandir}/man1/dwlm.1*
%{_datadir}/wayland-sessions/dwlm.desktop

%changelog
* Fri Sep 11 2026 Johan David <johandavid77@users.noreply.github.com> - 0.1.0-1
- Initial dwlm packaging for Fedora (wlroots 0.20 + scenefx 0.5).
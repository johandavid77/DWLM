# scenefx 0.5 (wlroots 0.20) — not in Fedora, snap-built from upstream
Name:           scenefx
Version:        0.5
Release:        1%{?dist}
Summary:        Wlroots effects library (scene API with eye-candy renderer)

License:        GPL-3.0-or-later
URL:            https://github.com/wlrfx/scenefx
Source0:        scenefx-0.5.tar.gz
BuildRequires:  gcc meson ninja-build pkgconfig wlroots-devel wayland-devel
BuildRequires:  wayland-protocols-devel libdrm-devel pixman-devel
BuildRequires:  mesa-libEGL-devel mesa-libGLES-devel lcms2-devel

%description
A drop-in replacement for the wlroots scene API that allows Wayland
compositors to render surfaces with eye-candy effects (blur, shadows,
rounded corners). Snap build of the 0.5 release, which requires wlroots
0.20 - the version shipped by Fedora.

%package devel
Summary:        Development files for %{name}
Requires:       %{name}%{?_isa} = %{version}-%{release} wlroots-devel

%description devel
Development headers and pkg-config file for %{name}.

%prep
%setup -q

%build
%meson
%meson_build

%install
%meson_install

%files
%{_libdir}/libscenefx-0.5.so*

%files devel
%{_includedir}/scenefx-0.5/*
%{_libdir}/pkgconfig/scenefx-0.5.pc

%changelog
* Fri Sep 11 2026 Johan David <johandavid77@users.noreply.github.com> - 0.5-1
- Snap build of scenefx 0.5 (wlroots 0.20) for dwlm on Fedora.
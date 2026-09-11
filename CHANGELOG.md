# Changelog

Todas las notas de cambio de DWLM se registran aquí.

## [0.1.0] - En desarrollo

### Añadido

- Config híbrida TOML en runtime (Fase 2 del roadmap), completa.
  - `~/.config/dwlm/config.toml`: secciones `[general]` (borderpx,
    corner_radius, gap, outer_gap, width_min/max, presets) y `[colors]`
    (root/border/focus/urgent, hex #rrggbb[aa]).
  - Valores por defecto en `config.def.h` ahora mutables; overrides en
    runtime sin dependencias.
  - Hot-reload con `kill -HUP <pid>`: parser diminuto (sin deps) que muta
    las variables de config y re-aplica color/anillos/layout/corners en vivo.
  - El reload se drena en el loop principal (`wl_event_loop_dispatch` con
    timeout) para no hacer trabajo no async-safe dentro del handler.
- SceneFX 0.2: esquinas redondeadas (corner_radius), renderer fx, anillo de
  foco como card redondeada trasera. Enlace `WLRROOTS = scenefx wlroots-0.18`.
- Scroll de viewport con rueda/touchpad (Fase 3.9 del roadmap).
  - Rueda/touchpad panean la franja solo si desborda; si cabe, el axis se
    reenvía al cliente (scroll interno de apps intacto).
  - Discreto: `delta_discrete * scroll_pixels_per_notch / 120`; continuo:
    `delta * scroll_continuous_speed`; fallback a `delta` cuando
    `delta_discrete == 0` (ruedas de dispositivos virtuales).
  - Swipe de 2 dedos: panea por `-dx * scroll_continuous_speed`
    (eventos swipe_begin/update/end, atados solo al primer pointer).
  - Config: `scroll_mouse_scroll`, `scroll_pixels_per_notch`,
    `scroll_continuous_speed` en `config.def.h` + claves TOML runtime
    (`mouse_scroll`, `pixels_per_notch`, `continuous_speed`).
  - Verificado en VM con virtual pointer (30 notches): `d=180 vp=2928`
    (clamped al límite del strip), sin crash; HUP con las 3 keys nuevas sin crash.
- Animación del viewport (Fase 3.10).
  - Deslizamiento suave con easing linear o cubic ease-in-out
    (`scroll_anim_ms` / `scroll_anim_ease`, 0 = snap).
  - El target geométrico se recalcula solo cuando cambia la columna del
    foco; flag "pinned" persistente para pan/center/resize que la
    navegación por teclado libera (el strip no vuelve al foco solo).
  - Pump de frames vía `wl_event_loop_add_timer` (tick ~16 ms).
- Workspaces por columna (Fase 4.4): Super+Alt+1..9 enfoca la columna N en
  scroll; en tile es cambiar de tag (view).
- Floating híbrido (Fase 5).
  - Reglas runtime `[rules.N]` en config.toml (app_id / title /
    isfloating / tags); se aplican después de las reglas compiladas.
  - Las ventanas flotantes nuevas se centran sobre el área visible del
    viewport en modo scroll (scroll_place_float).
- Núcleo del modo scroll estilo Niri sobre dwl 0.7 (`src/scroll.c`, `src/scroll.h`).
  - Columnas apilables en una franja horizontal infinita (consume/expel).
  - Viewport que sigue al foco automáticamente, con clamps de límites.
  - Presets de ancho de columna (33/50/67%) y grow/shrink.
  - Centrado de columna activa.
- Keybindings estilo Niri en `src/config.def.h` con fallback de layout dual
  (las mismas teclas funcionan en tile y en scroll).
- Movimiento de columnas entre monitores (Fase 6.2).
  - Super+Shift+< / >: toda la columna enfocada (pila incluida) salta al
    monitor adyacente. En scroll el destino inserta la columna junto a la
    activa y enfoca su cima; en tile se degrada a tagmon.
- XWayland (Fase 7.2): `config.mk` compila con `-DXWAYLAND` y `xcb xcb-icccm`;
  verificado en VM lanzando xterm en la sesión Wayland (se mapea y renderiza
  como cliente X11 vía Xwayland).
- wlr-foreign-toplevel-management (Fase 7.3).
  - Un handle por ventana mapeada: título y app_id al crear/map/title change,
    estado activated (focus), maximized/fullscreen, output_enter/leave en
    setmon, y requests activate/close/maximize/fullscreen del cliente.
  - Verificado con un cliente FTM en la sesión: listó correctamente un xterm
    (XWayland, `title='johan@dwlm: ~'` `app_id='XTerm'`) y un foot (`foot`).
- Construcción con wlroots configurable (0.18 para Debian Trixie, 0.20 para
  Arch/Void) vía `WLRROOTS` en `config.mk`.
- Soporte probado en Arch Linux (2026): PKGBUILD adaptado a `wlroots0.18` y
  `scenefx-0.2` (AUR) y a los pkg-config versionados `wlroots-0.18` /
  `scenefx-0.2`. Receta completa (con los fixes de compatibilidad que
  necesita `wlroots0.18` contra las libs de Arch 2026) en
  `packaging/arch/README.md`. Verificado end-to-end: build AUR → makepkg →
  instalación → `dwlm -v` + compositor corriendo en virtio-gpu vía ly
  (auto-login con `WLR_NO_HARDWARE_CURSORS=1` para el cursor software).
- Soporte probado en Alpine Linux 3.24: build contra
  `WLRROOTS="scenefx-0.4 wlroots-0.19"` y `.apk` 0.1.0-r0 generado con abuild
  (wlroots0.19 + scenefx de los repos oficiales de Alpine).
- Compatibilidad con wlroots 0.19+ vía guardas de versión en el Makefile
  (`-DWLR_VERSION_0_19`): renames `wlr_xdg_surface_get_geometry` (→ campo
  `geometry` de `wlr_xdg_surface`), `wlr_xwayland_surface_override_redirect_
  wants_focus` / `wlr_xwayland_surface_icccm_input_model`, y
  `wlr_presentation_create(dpy, backend, 1)`. Sin impacto en 0.18 (Debian/Arch).
- Renombrado de dwl → dwlm (binario, .desktop, man page).

### Heredado de dwl 0.7
- Tiling master-and-stack, monocle y layout flotante.
- Layer-shell, xdg-decoration, output management, session lock, etc.
- Reglas por app_id/título, tags, monitores.
- Idle-notify passthrough (wlr_idle_notify_v1).

### Pendiente
- Ext-optimized-sync / ext-foreign-toplevel-list: sin soporte en
  wlroots-0.18 (ext-optimized-sync) y diferido para el bar shell (la FTM
  clásica ya cubre la integración).
- Packaging Debian (Fase 8): cerrado (dwlm_0.1.0-1_amd64.deb + CI GitHub
  Actions en verde; probado en VM y en el runner). PKGBUILD Arch y Alpine
  (apk 0.1.0-r0) probados; template Void queda pendiente de prueba.
- Barra de estado: **proyecto nuevo en C++** (tipo Noctalia5), fuera de
  dwlm; dwlm expone wlr-foreign-toplevel-management para su integración.
- Pulido final y man page (Fase 9).
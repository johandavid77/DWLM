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
- Núcleo del modo scroll estilo Niri sobre dwl 0.7 (`src/scroll.c`, `src/scroll.h`).
  - Columnas apilables en una franja horizontal infinita (consume/expel).
  - Viewport que sigue al foco automáticamente, con clamps de límites.
  - Presets de ancho de columna (33/50/67%) y grow/shrink.
  - Centrado de columna activa.
- Keybindings estilo Niri en `src/config.def.h` con fallback de layout dual
  (las mismas teclas funcionan en tile y en scroll).
- Construcción con wlroots configurable (0.18 para Debian Trixie, 0.20 para
  Arch/Void) vía `WLRROOTS` en `config.mk`.
- Renombrado de dwl → dwlm (binario, .desktop, man page).

### Heredado de dwl 0.7
- Tiling master-and-stack, monocle y layout flotante.
- Layer-shell, xdg-decoration, output management, session lock, etc.
- Reglas por app_id/título, tags, monitores.

### Pendiente
- Scroll de viewport con touchpad/rueda (Fase 3.9).
- XWayland habilitado por defecto (actualmente comentado en config.mk).
- Packaging Debian/Arch/Void (Fase 8).
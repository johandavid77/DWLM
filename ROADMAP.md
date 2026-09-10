# DWLM Roadmap

- [x] **Fase 1 — Setup y fork**
  - [x] 1.1 Clonar dwl y crear la estructura del proyecto (src/, packaging/, config/)
  - [x] 1.2 Elegir versión base: dwl v0.7 (wlroots 0.18, Debian Trixie)
  - [x] 1.3 Copiar fuentes a src/ (dwlm.c, client.h, util, protocols, config.def.h)
  - [x] 1.4 Renombrar binario y archivos a dwlm (Makefile, config.mk, dwlm.desktop, dwlm.1)
  - [x] 1.5 Adecuar Makefile para wlroots configurable (0.18/0.20)
  - [x] 1.6 Compilar y verificar que la base funciona (libwlroots-0.18-dev, zero warnings)
  - [x] 1.7 git init + README inicial
- [x] **Fase 2 — Configuración híbrida**
  - [x] 2.1 Parser TOML (~300 SLOC, sin dependencias) para ~/.config/dwlm/config.toml
  - [x] 2.2 Unión con config.def.h (valores por defecto + overrides runtime)
  - [x] 2.3 Mapeado de los parámetros actuales (borderpx, corner_radius, gaps,
         clamps de ancho, presets, colores)
  - [x] 2.4 Hot-reload (SIGHUP → flag → drenado en el loop principal) sin reiniciar
- [ ] **Fase 3 — Modo scroll (núcleo)**
  - [x] 3.1 scroll.h: tipos ScrollCol/ScrollState y API pública
  - [x] 3.2 Inclusión en dwlm.c (struct Monitor, Client.scol, hooks createmon/cleanupmon)
  - [x] 3.3 Arrange: asignar clientes a columnas (nuevos después de la columna activa)
  - [x] 3.4 Física de columnas: x(col) = Σ w[j] + gap·i; screen = col_x − viewport_x
  - [x] 3.5 Ajuste automático del viewport al foco (+ clamps)
  - [x] 3.6 Consume/expel (apilar/desapilar ventanas)
  - [x] 3.7 Ciclo de anchos (presets 33/50/67%) y grow/shrink
  - [x] 3.8 Centrado de columna
  - [x] 3.9 Scroll del viewport con gestos de touchpad y scroll vertical (normalizer)
    - Rueda/touchpad panean el strip solo si desborda; si cabe, el axis se
      reenvía al cliente (scroll interno de apps intacto).
    - Discreto: delta_discrete * scroll_pixels_per_notch / 120; continuo:
      delta * scroll_continuous_speed; fallback a delta si delta_discrete=0.
    - Swipe 2 dedos: pan por -dx * scroll_continuous_speed (eventos
      swipe_begin/update/end, atados solo al primer pointer).
    - Config: scroll_mouse_scroll, scroll_pixels_per_notch,
      scroll_continuous_speed (config.def.h) + claves TOML runtime.
  - [x] 3.10 Refinar animación/eases de viewport
    - Viewport animado (scroll_anim_ms, 0 = snap) con easing lineal o cubic
      ease-in-out (scroll_anim_ease); el target geométrico se recalcula solo
      al cambiar de columna → la animación nunca pelea consigo misma.
    - Pump de frames con wl_event_loop_add_timer (tick 16ms) + flag
      "pinned" persistente que el pan/center/resize mantienen y la
      navegación libera (el scroll no "vuelve" al foco mientras arrancas).
    - Claves TOML runtime: anim_ms, anim_ease.
- [ ] **Fase 4 — Navegación y manipulación estilo Niri**
  - [x] 4.1 Keybindings Niri-style (config.def.h) + fallos de layout dual (tile/scroll)
  - [x] 4.2 focus first/last, movecol first/last
  - [x] 4.3 Navegación vertical dentro de la columna (focusstack)
  - [x] 4.4 Zonas de trabajo / workspaces por columna del scroll
    - Super+Alt+1..9: focus a la columna N del strip (clamps a la última);
      en layouts tile es "go to workspace N" (view). Misma tecla, dos
      sentidos.
- [ ] **Fase 5 — Floating + modo híbrido**
  - [x] 5.1 Toggle floating por ventana/regla (Super+a y botón central del ratón)
  - [x] 5.2 Reglas por app_id/título en config.toml
    - `[rules.N]` (1..24) con app_id, title, isfloating, tags; aplicadas
      después de las reglas compiladas (ganan runtime).
  - [x] 5.3 Ventanas flotantes centradas junto a la columna activa
    - Al mapear una ventana flotante en modo scroll se centra en el área
      visible del viewport (anclada a pantalla).
- [ ] **Fase 6 — Multi-monitor + workspaces**
  - [x] 6.1 ScrollState por monitor (cada Monitor guarda su ScrollState y strip)
  - [x] 6.2 Movimiento de columnas entre monitores
    - Super+Shift+< / >: la columna enfocada completa (con su pila de ventanas)
      salta al monitor adyacente; vista scroll inserta la columna tras la
      columna activa del destino y enfoca su cima; layout tile cae en tagmon.
  - [ ] 6.3 Barra de estado (dwl blocks) adaptada a scroll
    - **Pospuesto**: la barra será un proyecto nuevo en C++ (tipo Noctalia5);
      dwlm expone wlr-foreign-toplevel-management para facilitarlo.
- [ ] **Fase 7 — Protocolos Wayland**
  - [x] 7.1 xdg-shell completo, layer-shell, XDG decoration (heredados)
  - [x] 7.2 XWayland en Debian (config.mk ya activo: -DXWAYLAND + xcb/xcb-icccm;
       probado con xterm en la sesión Wayland)
  - [ ] 7.3 optimized sync, ext-foreign-toplevel, idle-notify passthrough
    - [x] wlr-foreign-toplevel-management_v1 (manager + handles por ventana:
          título, app_id, activated/maximized/fullscreen, output enter/leave,
          requests activate/close/maximize/fullscreen; verificado con un
          cliente FTM en la sesión)
    - [x] idle-notify passthrough (wlr_idle_notify_v1, heredado)
    - [ ] ext-optimized-sync y ext-foreign-toplevel-list: sin soporte en
          wlroots-0.18 (ext-optimized-sync) y para un futuro bar shell
          (ext list) — la FTM clásica cubre ya la integración del bar
- [ ] **Fase 8 — Packaging**
  - [ ] 8.1 Debian Trixie: packaging/debian/ (libwlroots-0.18-dev)
  - [ ] 8.2 Arch: packaging/arch/PKGBUILD (wlroots0.20)
  - [ ] 8.3 Void: packaging/void/template (wlroots0.20-devel)
  - [ ] 8.4 CI fixture
- [ ] **Fase 9 — Integración Noctalia + pulido**
  - [ ] 9.1 Probar con Noctalia5
  - [ ] 9.2 Estado del sistema (status bar, indicators, screen recording)
  - [ ] 9.3 Documentación final y man page

Total estimado: 24–34 h de desarrollo.
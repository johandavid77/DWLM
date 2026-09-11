# DWLM

<p align="center">
  <b><a href="README.md">English</a></b> · Español
</p>

<p align="center">
  <img src="dwlm-banner.png" alt="dwlm" width="640">
</p>

**dwm para Wayland con modo de scroll estilo Niri.**

## Instalación

**Debian 13 (Trixie)** — hay un `.deb` ya preparado adjunto a
[GitHub Releases](https://github.com/johandavid77/DWLM/releases/latest). En un
sistema limpio:

```sh
# 1. herramientas de build + librerías (scenefx 0.2 las necesita para compilar)
sudo apt install git debhelper meson ninja-build pkg-config \
	libwlroots-0.18-dev libwayland-dev wayland-protocols \
	libxkbcommon-dev libinput-dev libxcb-icccm4-dev \
	libpixman-1-dev libgbm-dev libdrm-dev xwayland

# 2. la única dependencia que falta: scenefx 0.2 (no está en los repos de Debian)
git clone https://github.com/johandavid77/DWLM && cd DWLM
./packaging/build-scenefx.sh

# 3. el propio dwlm
wget -qO dwlm.deb https://github.com/johandavid77/DWLM/releases/latest/download/dwlm_0.1.0-1_amd64.deb
sudo apt install ./dwlm.deb
```

Cada ejecución del CI también sube el `.deb` más reciente como *artefacto* del
workflow `build`.

**Arch Linux** — empaquetado en `packaging/arch` (PKGBUILD), **probado** en un
sistema Arch actual. dwlm apunta a wlroots 0.18, así que en Arch primero hay que
construir `wlroots0.18` y `scenefx-0.2` desde el AUR (instalan los archivos
pkg-config versionados `wlroots-0.18.pc` / `scenefx-0.2.pc`, que el PKGBUILD
usa). `wlroots0.18` necesita un par de arreglos de compatibilidad para compilar
con los paquetes de Arch actuales — la receta probada completa está en
`packaging/arch/README.md`.

**Alpine Linux 3.24** — se puede generar un `.apk` con
`packaging/alpine/build-apk.sh` (probado). Receta completa en
`packaging/alpine/README.md`. En un sistema limpio:

```sh
apk add wlroots0.19 scenefx wayland libxkbcommon libinput xcb-util-wm xwayland
apk add --allow-untrusted ./dwlm-0.1.0-r0.apk
```

**Void Linux** — empaquetado en `packaging/void` (template), pero **todavía sin
probar** (no hay sistema Void disponible); trátalo como trabajo en curso.

## La filosofía

DWLM se apoya en tres ideas que, combinadas, definen todo lo demás:

**Suckless.** Una base de código en C pequeña y legible construida sobre
[wlroots](https://gitlab.freedesktop.org/wlroots/wlroots). No hay un archivo de
configuración gigante que domar ni una interfaz de ajustes que excavar: todo el
sistema cabe en una pantalla de teléfono de código fuente, y si lo quieres
distinto, editas `src/config.def.h` y recompilas. Lo que los autores de dwm/dwl
llaman "lo correcto como programa pelado, sin inflarse".

**El tiling de dwm.** Tiling a la manera de dwm: sin decoraciones que arrastrar,
sin ventanas superpuestas que cuidar. El teclado es el dispositivo de entrada —
cada acción está a una tecla de distancia, sin menús ocultos que descubrir. Los
tags son los workspaces, y `Mod+1..9` te mueve por ellos mientras `Mod+Shift+E`
es el único "menú de ajustes" que necesitarás jamás: la salida.

**El scroll infinito.** De [Niri](https://github.com/YaLTeR/niri) DWLM toma su
idea más liberadora: en lugar de una cuadrícula fija de workspaces, el escritorio
es una *tira horizontal infinita*. Cada ventana que abres se convierte en una
columna, la vista sigue tu foco con un paneo suave, y tu sesión crece contigo —
nunca te quedas sin workspaces, nunca tienes que decidir de antemano "cuántos"
vas a necesitar. Cada columna puede seguir conteniendo una pila vertical de
ventanas, y todo se desvanece hacia los layouts clásicos de dwm (`tile`,
`floating`, `monocle`) cuando quieras.

El resultado conserva el principio sucless — hackéalo todo, entiéndelo todo —
mientras adopta la filosofía del scroll: *el escritorio debe crecer contigo, no
quedarse quieto.*

DWLM es un fork de [dwl](https://codeberg.org/dwl/dwl) que va un paso más allá:
el modo scroll no es un parche encima, es un layout que el compositor entiende
de forma nativa, con vista animada, tiras por monitor, ventanas flotantes
ancladas al área visible y movimientos de columnas completas entre monitores.

## Compilación

Dependencias:

- wlroots (coincidiendo con `WLRROOTS` en `config.mk`) y `scenefx`
- wayland-protocols, wayland-scanner
- xkbcommon, libinput
- (opcional) soporte XWayland: `libxcb-icccm` (xcb-util-wm)

En Debian Trixie (wlroots 0.18), con la compilación manual de scenefx 0.2:

```sh
sudo apt install libwlroots-0.18-dev libwayland-dev wayland-protocols \
	libxkbcommon-dev libinput-dev libxcb-icccm4-dev
./packaging/build-scenefx.sh
```

En Arch usa el PKGBUILD de `packaging/arch` (probado; requiere `wlroots0.18`
y `scenefx-0.2` del AUR — receta completa en `packaging/arch/README.md`).
En Alpine 3.24 usa el APKBUILD de `packaging/alpine` (probado; compila contra
`wlroots0.19` y `scenefx-0.4` de los repos oficiales — receta y script de
build en `packaging/alpine/README.md`).
En Void usa `packaging/void` (sin probar).
Alternativamente, compila contra un wlroots 0.20 del sistema poniendo
`WLRROOTS = scenefx wlroots` en `config.mk` e instalando un scenefx
compatible con wlroots 0.20 desde el AUR / los repos de Void.

Compilar e instalar:

```sh
make
sudo make install
# o un paquete Debian de verdad:
dpkg-buildpackage -us -uc -b   # necesita las dependencias de build de debian/control
```

## Uso

`make install` ya deja una entrada `dwlm.desktop` en
`share/wayland-sessions`, que los display managers (ly, greetd, GDM, SDDM…)
detectan solos. Para añadirla a mano, guárdala como `dwlm.desktop` en
`/usr/share/wayland-sessions` (sistema) o `~/.local/share/wayland-sessions`
(por usuario):

```
[Desktop Entry]
Name=dwlm
Comment=dwm para Wayland con modo de scroll estilo Niri
Exec=dwlm
Type=Application
```

Opciones de línea de comandos:

- `dwlm` — arranque normal
- `dwlm -s "foot"` — arrancar también un programa (p. ej. una terminal)
- `dwlm -d` — logging completo de wlroots con debug
- `dwlm -v` — imprime el logo y la versión, y sale

## Solución de problemas

- **`Mod+Return`/`Mod+P` no hacen nada.** El `termcmd`/`menucmd` por defecto
  son `foot` y `wmenu-run`; el compositor no trae aplicaciones. Instálalos
  (Debian: `sudo apt install foot wmenu`, Arch: `sudo pacman -S foot wmenu`,
  Alpine: `apk add foot wmenu`).
- **Cursor invertido/distorcionado en una VM (VirtIO, VMware).** Desactiva el
  cursor de hardware de KMS (wlroots dibuja el sprite sobre una GPU virtual):
  `Exec=/usr/bin/env WLR_NO_HARDWARE_CURSORS=1 dwlm`.
- **ly aborta el login con `FileNotFound`.** El `session_log` de ly apunta a
  `~/.local/state/ly-session.log` y ly no crea ese directorio; ejecuta
  `mkdir -p ~/.local/state` primero.
- **Arch: el `wlroots0.18` de AUR no compila** (`-Werror=switch`,
  `-Werror=discarded-qualifiers`) con los paquetes actuales. La receta probada
  está en `packaging/arch/README.md`.

## Atajos de teclado (por defecto)

El layout por defecto es tiling, igual que dwl. El modo scroll es un *layout*
(`-->`) que se selecciona con `Mod+Shift+S`. `Mod+T`, `Mod+F`, `Mod+M`
seleccionan tile, floating y monocle respectivamente.

Scroll/navegación (funcionan tanto en tile como en scroll):

| Teclas             | Acción                              |
|--------------------|-------------------------------------|
| `Mod+H` / `Mod+L`  | enfocar columna izquierda / derecha |
| `Mod+Shift+H/L`    | mover columna izquierda / derecha   |
| `Mod+Home` / `Mod+End` | enfocar primera / última columna|
| `Mod+Ctrl+Home/End`| mover columna al inicio / final     |
| `Mod+J` / `Mod+K`  | enfocar ventana de abajo / arriba   |
| `Mod+Shift+V`      | ciclar ancho de columna (33/50/67%) |
| `Mod+R` / `Mod+Shift+R` | alternar modo redimensionado  |
| `Mod+-` / `Mod+=`  | encoger / agrandar ancho de columna |
| `Mod+C`            | centrar la columna enfocada         |
| `Mod+[`            | consumir ventana en la columna de la izquierda |
| `Mod+]`            | expulsar ventana a su propia columna|

Gestión de apps/ventanas:

| Teclas             | Acción                          |
|--------------------|---------------------------------|
| `Mod+Q`            | cerrar ventana                  |
| `Mod+A`            | alternar flotante               |
| `Mod+E`           | alternar pantalla completa      |
| `Mod+M`           | alternar maximizado             |
| `Mod+T/F`         | layout tile / floating          |
| `Mod+Shift+M`     | layout monocle                  |
| `Mod+Shift+S`     | layout scroll                   |
| `Mod+Space`        | alternar al layout anterior     |
| `Mod+Shift+Return` | abrir terminal (`foot`)         |
| `Mod+P`            | abrir menú (`wmenu-run`)        |
| `Mod+Shift+E`      | salir de dwlm                   |

Tags/workspaces: `Mod+1..9` ver, `Mod+Shift+1..9` asignar, `Mod+Ctrl+1..9`
alternar vista, etc. `Mod+Alt+1..9` enfoca la columna N del scroll (workspace
N en tiling). Navegación de escritorios: `Mod+,`/`Mod+.` enfocar
monitor, `Mod+Shift+<`/`>` enviar a monitor.

Todos los atajos se definen en `src/config.def.h`; mira el array `keys[]` y la
lista de layouts.

## Layouts

- `[]=` master-and-stack (el `tile` de dwl)
- `><>` floating
- `[M]` monocle
- `-->` scroll estilo Niri

## Estructura

- `src/dwlm.c` — el compositor (fork de dwl 0.7)
- `src/scroll.c` / `src/scroll.h` — el layout scroll (compilado en la misma
  unidad de traducción que `dwlm.c`, como hace dwl)
- `src/config.def.h` — configuración en tiempo de compilación
- `src/config_runtime.c` — config TOML en tiempo de ejecución (parámetros del
  scroll + reglas de ventana)
- `packaging/` — Debian (`debian/` en la raíz), Arch Linux (PKGBUILD + receta
  README, probado), Alpine Linux (APKBUILD + receta README + script de build,
  probado) y Void (template, sin probar)
- `.github/workflows/build.yml` — CI (Debian Trixie, zero-warnings + deb)
- `dwlm.svg` — el logo de dwlm, basado en el logo original de DWM
- `dwlm-banner.png` — banner del README
- `README.es.md` — este documento en español
- `ROADMAP.md` — hoja de ruta de desarrollo

## Barra de estado

La barra de estado es un proyecto aparte en C++ (al estilo de Noctalia5),
**fuera** de este repositorio. dwlm expone `wlr-foreign-toplevel-management`
para que esa barra pueda seguir las ventanas (título, app_id, estado, posición,
cerrar).

## Créditos

Fork de [dwl](https://codeberg.org/dwl/dwl). El diseño del layout scroll
está inspirado en Niri. Ver `LICENSE*` para los detalles.

## Licencia

GPL-3.0-or-later — ver [LICENSE](LICENSE). La atribución y nota de licencia de
dwl viven en [LICENSE.dwl](LICENSE.dwl); las licencias MIT de los upstream
están en `LICENSE.tinywl`, `LICENSE.dwm` y `LICENSE.sway`.

<a href="LICENSE"><img alt="Licencia: GPLv3" src="https://img.shields.io/badge/Licencia-GPLv3-blue.svg"></a>
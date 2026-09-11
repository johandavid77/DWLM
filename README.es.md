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

**Arch Linux / Void Linux** — empaquetado incluido en `packaging/arch`
(PKGBUILD) y `packaging/void` (template), pero **todavía sin probar** (no hay
sistema Arch/Void disponible). Trátalo como trabajo en curso; la compilación
estilo dwl (`config.mk`, `WLRROOTS = scenefx wlroots`) debería funcionar con
wlroots 0.20.

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

En Arch / Void (wlroots 0.20) usa el empaquetado de `packaging/` (de momento
sin probar), o pon `WLRROOTS = scenefx wlroots` en `config.mk` e instala
scenefx desde el AUR / los repos de Void.

Compilar e instalar:

```sh
make
sudo make install
# o un paquete Debian de verdad:
dpkg-buildpackage -us -uc -b   # necesita las dependencias de build de debian/control
```

## Uso

Añade una entrada de sesión:

```
[Desktop Entry]
Name=dwlm
Comment=dwm para Wayland con modo de scroll estilo Niri
Exec=dwlm
Type=Application
```

Después elige *dwlm* desde tu gestor de sesión, o arráncalo directamente:

```sh
dwlm -s "foot"
```

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
| `Mod+R`            | ciclar ancho de columna (33/50/67%) |
| `Mod+-` / `Mod+=`  | encoger / agrandar ancho de columna |
| `Mod+C`            | centrar la columna enfocada         |
| `Mod+[`            | consumir ventana en la columna de la izquierda |
| `Mod+]`            | expulsar ventana a su propia columna|

Gestión de apps/ventanas:

| Teclas             | Acción                          |
|--------------------|---------------------------------|
| `Mod+Q`            | cerrar ventana                  |
| `Mod+A`            | alternar flotante               |
| `Mod+E`            | alternar pantalla completa      |
| `Mod+T/F/M`        | layout tile / floating / monocle|
| `Mod+Shift+S`      | layout scroll                   |
| `Mod+Space`        | alternar al layout anterior     |
| `Mod+Shift+Return` | abrir terminal (`foot`)         |
| `Mod+P`            | abrir menú (`wmenu-run`)        |
| `Mod+Shift+E`      | salir de dwlm                   |

Tags/workspaces: `Mod+1..9` ver, `Mod+Shift+1..9` asignar, `Mod+Ctrl+1..9`
alternar vista, etc. Navegación de escritorios: `Mod+,`/`Mod+.` enfocar
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
- `packaging/` — Debian (`debian/` en la raíz), Arch Linux (PKGBUILD) y Void
  (template) — Arch/Void sin probar
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
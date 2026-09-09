/*
 * DWLM scroll mode: Niri-style horizontal scrolling layout, adapted to dwl.
 * See LICENSE file for copyright and license details.
 *
 * The scroll mode is compiled as part of the single translation unit dwlm.c
 * (scroll.c is #included at the end of dwlm.c), in the spirit of upstream dwl.
 * This header declares the types and the public API used by dwlm.c and
 * config.def.h. It is meant to be included into dwlm.c, after the Arg union
 * (line ~95) so that `const Arg *` is already known.
 */
#ifndef DWLM_SCROLL_H
#define DWLM_SCROLL_H

#include <wayland-server-core.h>

typedef struct Monitor Monitor;
typedef struct Client Client;

/* A column of stacked windows in the scroll layout. `link` orders columns
 * left to right on the strip; `clients` holds the per-column stack. */
typedef struct ScrollCol ScrollCol;
struct ScrollCol {
	struct wl_list link;
	struct wl_list clients;
	double width; /* width as a fraction of the window area */
};

/* Per-monitor state of the scroll layout. */
typedef struct ScrollState ScrollState;
struct ScrollState {
	double viewport_x;  /* strip-coordinate of the viewport's left edge */
	struct wl_list cols; /* ordered list of ScrollCol */
	int width_idx;      /* index of the active width preset */
	int keep_viewport;  /* arrange() must not auto-scroll to the focus */
};

/* Layout arrange function. Assigns clients to columns, lays them out and
 * moves the viewport so the focused column is visible. */
void scroll(Monitor *m);

/* Free all columns of a monitor's scroll state. */
void scroll_release(Monitor *m);

/* Remove a client from its column (used on unmap/destroy), freeing the
 * column if it becomes empty. */
void scroll_detach(Client *c);

/* Move focus to the column at the given offset (-1: left, +1: right).
 * In non-scroll layouts falls back to focusstack(). */
void scroll_focus(const Arg *arg);

/* Move the whole focused column one position (swap with the neighbour).
 * In non-scroll layouts falls back to zoom(). */
void scroll_movecol(const Arg *arg);

/* Focus the first/last column. */
void scroll_first(const Arg *arg);
void scroll_last(const Arg *arg);

/* Move the focused column to the first/last position. */
void scroll_movecol_first(const Arg *arg);
void scroll_movecol_last(const Arg *arg);

/* Cycle the focused column through the configured width presets. */
void scroll_cycle_width(const Arg *arg);

/* Grow/shrink the focused column width. In non-scroll layouts falls back
 * to setmfact(), so the same keys control the master factor while tiling. */
void scroll_width(const Arg *arg);

/* Bring the focused column to the center of the viewport. */
void scroll_center(const Arg *arg);

/* Window management (Niri consume/expel):
 * - consume: stack the focused window into the column to its left.
 * - expel:   pop the focused window out of its column into a new one. */
void scroll_consume(const Arg *arg);
void scroll_expel(const Arg *arg);

/* Focus the window above/below within the focused column. arg->i: -1/+1. */
void scroll_focus_up_down(const Arg *arg);

#endif
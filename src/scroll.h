/*
 * DWLM scroll mode: Niri-style horizontal scrolling layout, adapted to dwl.
 * SPDX-License-Identifier: GPL-3.0-or-later
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

#include <stdint.h>
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
	double vp_from;     /* viewport_x when the current animation started */
	double vp_to;       /* animation target */
	uint64_t vp_begin;  /* animation start time (ms, CLOCK_MONOTONIC) */
	uint64_t vp_end;    /* animation end time (ms) */
	int vp_animating;   /* an animation is currently running */
	struct wl_event_source *anim_timer; /* NULL when idle */
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

/* True when the strip overflows the window area (i.e. a wheel/touchpad pan
 * would actually move the viewport). */
int scroll_can_pan(Monitor *m);

/* Pan the viewport by `delta` strip pixels, clamped to its bounds. The
 * viewport is pinned on the next arrange() (the focus will not pull it
 * back). Used by the mouse wheel / touchpad gesture handlers. */
void scroll_pan(Monitor *m, double delta);

/* Window management (Niri consume/expel):
 * - consume: stack the focused window into the column to its left.
 * - expel:   pop the focused window out of its column into a new one. */
void scroll_consume(const Arg *arg);
void scroll_expel(const Arg *arg);

/* Focus the window above/below within the focused column. arg->i: -1/+1. */
void scroll_focus_up_down(const Arg *arg);

/* Move the whole focused column (with its window stack) to the adjacent
 * monitor in the direction arg->i. In non-scroll layouts falls back to
 * tagmon(), so the same keys send the focused window to the next monitor. */
void scroll_movecolmon(const Arg *arg);

/* Focus the column at index arg->i (0-based); clamps to the last column.
 * In non-scroll layouts falls back to view() (workspace switch), so the
 * same keys behave as "go to workspace N" while tiling. */
void scroll_focus_number(const Arg *arg);

/* Center a just-mapped floating window over the visible viewport area
 * (scroll mode only; a no-op otherwise). */
void scroll_place_float(Client *c);

/* Niri-style interactive resize: in scroll mode the pointer moves the right
 * edge of the focused column; on floating windows it resizes in place. */
void scroll_resize_drag(Monitor *m, double cx, double cy);

#endif
/*
 * DWLM scroll mode: Niri-style horizontally scrolling layout.
 * See LICENSE file for copyright and license details.
 *
 * This file is #included at the end of dwlm.c to form a single translation
 * unit, so it can use dwlm.c's internal (static) symbols freely. It also
 * #includes scroll.h (include-guarded) so the two files stay in sync.
 *
 * Model (adapted from Niri):
 *   - An infinite horizontal strip of columns, each holding a stack of
 *     windows [consume/expel to nest/unnest].
 *   - Column layout: x(col_i) = sum(widths[0..i-1]) + gap * i
 *   - screen_position = strip_position - viewport_x (viewport_x is the
 *     scroll offset). Columns create new windows to the right of / after
 *     the focused column.
 */
#include <stdlib.h>
#include <time.h>

#include "scroll.h"

/* Helpers ---------------------------------------------------------------- */

/* wayland < 1.23 lacks wl_list_swap(); this is the canonical upstream
 * implementation, used to swap two elements of the same list. */
static void
scroll_swap_list(struct wl_list *b1, struct wl_list *b2)
{
	struct wl_list tmp = *b1;
	*b1 = *b2;
	*b2 = tmp;
	wl_list_remove(b1);
	if (b1->next != b1)
		wl_list_insert(b2, b1);
	wl_list_remove(b2);
	if (b2->next != b2)
		wl_list_insert(tmp.prev, b2);
}

/* Tiling area: the monitor work area inset by the outer gap */
static void
scroll_area(Monitor *m, struct wlr_box *a)
{
	int g = (int)scroll_outer_gap;
	a->x = m->w.x + g;
	a->y = m->w.y + g;
	a->width = m->w.width - 2 * g;
	a->height = m->w.height - 2 * g;
	if (a->width < 1)
		a->width = 1;
	if (a->height < 1)
		a->height = 1;
}

static double
scroll_col_width(Monitor *m, ScrollCol *col)
{
	struct wlr_box a;
	double w, max;

	scroll_area(m, &a);
	w = col->width * a.width;
	max = a.width - 2 * scroll_gap;
	if (max < 1)
		max = 1;
	return w > max ? max : w;
}

/* Strip coordinate of a column's left edge */
static double
scroll_col_x(Monitor *m, ScrollCol *col)
{
	struct wlr_box a;
	ScrollCol *c;
	double x;

	scroll_area(m, &a);
	x = a.x + scroll_gap;
	wl_list_for_each(c, &m->scroll.cols, link) {
		if (c == col)
			return x;
		x += scroll_col_width(m, c) + scroll_gap;
	}
	return x;
}

/* Valid viewport range [lo, hi]; lo == hi when the whole strip fits */
static void
scroll_viewport_bounds(Monitor *m, double *lo, double *hi)
{
	struct wlr_box a;
	ScrollCol *c;
	double s0, s1;

	scroll_area(m, &a);
	s0 = a.x + scroll_gap;
	s1 = s0;
	wl_list_for_each(c, &m->scroll.cols, link)
		s1 += scroll_col_width(m, c) + scroll_gap;

	if (s1 - scroll_gap <= a.x + a.width) {
		/* The whole strip fits on screen: no scrolling */
		*lo = *hi = a.x;
		return;
	}
	*lo = a.x;                /* scroll back to the first column */
	*hi = s1 - a.width;       /* scroll forward to the last column */
}

/* Animation --------------------------------------------------------------- */

static uint64_t
scroll_now_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)(ts.tv_sec) * 1000ULL + (uint64_t)(ts.tv_nsec) / 1000000UL;
}

/* 0..1 progress -> eased progress (scroll_anim_ease: 0 linear, 1 cubic io) */
static double
scroll_ease(double p)
{
	if (scroll_anim_ease != 1)
		return p; /* linear */
	if (p < 0.5)
		return 4 * p * p * p;
	return 1 - (-2 * p + 2) * (-2 * p + 2) * (-2 * p + 2) / 2;
}

/* Set the animation target. If a different target was already in flight,
 * restart from the current (animated) position so the viewport chases it. */
static void
scroll_set_target(Monitor *m, double to)
{
	if (m->scroll.vp_animating && m->scroll.vp_to != to) {
		m->scroll.vp_from = m->scroll.viewport_x;
		m->scroll.vp_begin = scroll_now_ms();
	}
	m->scroll.vp_to = to;
}

static int scroll_anim_tick(void *data);

/* Advance the running animation (called by arrange(); also from the timer
 * pump). Snaps to the target when animations are disabled or finished. */
static void
scroll_vp_tick(Monitor *m)
{
	uint64_t now;
	double p, e;

	if (scroll_anim_ms <= 0) {
		m->scroll.viewport_x = m->scroll.vp_to;
		return;
	}
	if (!m->scroll.vp_animating) {
		if (m->scroll.viewport_x == m->scroll.vp_to)
			return;
		m->scroll.vp_animating = 1;
		m->scroll.vp_from = m->scroll.viewport_x;
		m->scroll.vp_begin = scroll_now_ms();
	}
	m->scroll.vp_end = m->scroll.vp_begin + (uint64_t)scroll_anim_ms;

	now = scroll_now_ms();
	if (now >= m->scroll.vp_end) {
		m->scroll.viewport_x = m->scroll.vp_to;
		m->scroll.vp_animating = 0;
		if (m->scroll.anim_timer) {
			wl_event_source_remove(m->scroll.anim_timer);
			m->scroll.anim_timer = NULL;
		}
		return;
	}
	p = (double)(now - m->scroll.vp_begin)
		/ (double)(m->scroll.vp_end - m->scroll.vp_begin);
	if (p < 0)
		p = 0;
	e = scroll_ease(p);
	m->scroll.viewport_x = m->scroll.vp_from + (m->scroll.vp_to - m->scroll.vp_from) * e;

	/* keep pumping frames while the animation runs */
	if (!m->scroll.anim_timer && event_loop) {
		m->scroll.anim_timer = wl_event_loop_add_timer(event_loop,
				scroll_anim_tick, m);
		if (m->scroll.anim_timer)
			wl_event_source_timer_update(m->scroll.anim_timer, 16);
	}
}

static int
scroll_anim_tick(void *data)
{
	Monitor *m = data;
	arrange(m); /* advances the animation via scroll_vp_tick() */
	if (!m->scroll.vp_animating) {
		if (m->scroll.anim_timer) {
			wl_event_source_remove(m->scroll.anim_timer);
			m->scroll.anim_timer = NULL;
		}
		return 0;
	}
	if (m->scroll.anim_timer)
		wl_event_source_timer_update(m->scroll.anim_timer, 16);
	return 1;
}

/* Scroll the viewport so `active` is visible, or clamp if keep_viewport */
static void
scroll_ensure_viewport(Monitor *m, ScrollCol *active)
{
	struct wlr_box a;
	double lo, hi, ref, to;

	scroll_area(m, &a);
	scroll_viewport_bounds(m, &lo, &hi);
	if (m->scroll.keep_viewport) {
		/* user-pinned (wheel pan / center / resize): keep current target.
		 * Released by the keyboard navigation functions. */
		scroll_set_target(m, m->scroll.vp_to);
		return;
	}
	if (!active) {
		ref = m->scroll.vp_to;
		scroll_set_target(m, ref < lo ? lo : (ref > hi ? hi : ref));
		return;
	}

	/* Geometric target: bring the focused column fully into view, starting
	 * from the current target so in-flight animations don't fight. */
	{
		double pad = scroll_gap;
		double L = scroll_col_x(m, active);
		double W = scroll_col_width(m, active);
		double tlo = L + W - (a.x + a.width - pad);
		double thi = L - (a.x + pad);
		to = ref = m->scroll.vp_to;
		if (to < tlo)
			to = tlo;
		else if (to > thi)
			to = thi;
		if (to < lo)
			to = lo;
		else if (to > hi)
			to = hi;
		scroll_set_target(m, to);
	}
}

/* Focus the top client of a column */
static void
scroll_focus_col(Monitor *m, ScrollCol *col)
{
	Client *c;
	if (!wl_list_empty(&col->clients)) {
		c = wl_container_of(col->clients.next, c, scol);
		focusclient(c, 1);
	}
}

/* Layout ---------------------------------------------------------------- */

void
scroll(Monitor *m)
{
	ScrollCol *col, *tmp;
	Client *c;

	if (!m->wlr_output->enabled)
		return;

	/* 1. Detach clients that are no longer tiled here; drop empty columns */
	wl_list_for_each_safe(col, tmp, &m->scroll.cols, link) {
		Client *cc, *ctmp;
		wl_list_for_each_safe(cc, ctmp, &col->clients, scol) {
			if (!VISIBLEON(cc, m) || cc->isfloating || cc->isfullscreen) {
				wl_list_remove(&cc->scol);
				cc->scol_col = NULL;
			}
		}
		if (wl_list_empty(&col->clients)) {
			wl_list_remove(&col->link);
			free(col);
		}
	}

	/* 2. New tiled clients get their own column, inserted after the focused
	 * column (Niri behavior). */
	{
		Client *sel = focustop(m);
		ScrollCol *active = sel ? sel->scol_col : NULL;
		struct wl_list *after = active ? &active->link : m->scroll.cols.prev;
		wl_list_for_each(c, &clients, link) {
			if (!VISIBLEON(c, m) || c->isfloating || c->isfullscreen)
				continue;
			if (c->scol_col)
				continue;
			col = ecalloc(1, sizeof(*col));
			wl_list_init(&col->clients);
			col->width = scroll_column_width;
			wl_list_insert(&col->clients, &c->scol);
			c->scol_col = col;
			wl_list_insert(after, &col->link);
			after = &col->link;
		}
	}

	/* 3. Move the viewport so the focused column is visible */
	{
		Client *sel = focustop(m);
		scroll_ensure_viewport(m, sel ? sel->scol_col : NULL);
		scroll_vp_tick(m);
	}

	/* 4. Lay out clients, stacked vertically inside their columns */
	{
		struct wlr_box a;
		double vpx;
		int yy;
		scroll_area(m, &a);
		vpx = m->scroll.viewport_x;
		yy = a.y;
		wl_list_for_each(col, &m->scroll.cols, link) {
			int n = wl_list_length(&col->clients);
			double colw = scroll_col_width(m, col);
			int x = (int)(scroll_col_x(m, col) - vpx);
			int ch = n ? (int)((a.height - scroll_gap * (n - 1)) / n) : 0;
			Client *cc;
			if (ch < 1)
				ch = 1;
			yy = a.y;
			wl_list_for_each(cc, &col->clients, scol) {
				resize(cc, (struct wlr_box){.x = x, .y = yy,
					.width = (int)colw, .height = ch}, 0);
				yy += ch + (int)scroll_gap;
			}
		}
	}
}

void
scroll_release(Monitor *m)
{
	ScrollCol *col, *tmp;

	if (m->scroll.anim_timer) {
		wl_event_source_remove(m->scroll.anim_timer);
		m->scroll.anim_timer = NULL;
	}

	wl_list_for_each_safe(col, tmp, &m->scroll.cols, link) {
		Client *c, *ctmp;
		wl_list_for_each_safe(c, ctmp, &col->clients, scol) {
			wl_list_remove(&c->scol);
			c->scol_col = NULL;
		}
		wl_list_remove(&col->link);
		free(col);
	}
}

void
scroll_detach(Client *c)
{
	ScrollCol *col = c->scol_col;

	if (!col)
		return;
	wl_list_remove(&c->scol);
	c->scol_col = NULL;
	if (wl_list_empty(&col->clients)) {
		wl_list_remove(&col->link);
		free(col);
	}
}

/* Navigation ------------------------------------------------------------- */

void
scroll_resize_drag(Monitor *m, double cx, double cy)
{
	Client *sel;
	ScrollCol *col;
	double left, wf;

	if (!m)
		return;
	sel = focustop(m);
	if (!sel)
		return;

	if (sel->isfloating && !sel->isfullscreen) {
		/* Floating windows: drag the bottom-right corner in place */
		resize(sel, (struct wlr_box){.x = sel->geom.x, .y = sel->geom.y,
			.width = (int)round(cx - sel->geom.x),
			.height = (int)round(cy - sel->geom.y)}, 1);
		return;
	}

	col = sel->scol_col;
	if (!col)
		return;

	/* Tiled: follow the pointer as the column's right edge */
	{
		struct wlr_box a;
		double wf2;
		scroll_area(m, &a);
		left = scroll_col_x(m, col) - m->scroll.viewport_x;
		wf2 = (cx - left) / (double)a.width;
		wf = MAX(scroll_width_min, MIN(scroll_width_max, wf2));
	}
	col->width = wf;
	arrange(m);
}

void
scroll_focus(const Arg *arg)
{
	Client *sel;
	ScrollCol *col, *target;
	struct wl_list *next;

	if (!selmon)
		return;
	if (selmon->lt[selmon->sellt]->arrange != scroll) {
		focusstack(arg);
		return;
	}
	selmon->scroll.keep_viewport = 0;
sel = focustop(selmon);
	if (!sel || !(col = sel->scol_col))
		return;
	next = (arg->i > 0) ? col->link.next : col->link.prev;
	if (next == &selmon->scroll.cols)
		return; /* at the edge; no wrap */
	target = wl_container_of(next, target, link);
	scroll_focus_col(selmon, target);
	arrange(selmon);
	printstatus();
}

void
scroll_movecol(const Arg *arg)
{
	Client *sel;
	ScrollCol *col, *other;
	struct wl_list *next;

	if (!selmon)
		return;
	if (selmon->lt[selmon->sellt]->arrange != scroll) {
		zoom(arg);
		return;
	}
	selmon->scroll.keep_viewport = 0;
	sel = focustop(selmon);
	if (!sel || !(col = sel->scol_col))
		return;
	next = (arg->i > 0) ? col->link.next : col->link.prev;
	if (next == &selmon->scroll.cols)
		return;
	other = wl_container_of(next, other, link);
	scroll_swap_list(&col->link, &other->link);
	arrange(selmon);
	printstatus();
}

void
scroll_first(const Arg *arg)
{
	ScrollCol *col;

	if (!selmon || selmon->lt[selmon->sellt]->arrange != scroll)
		return;
	selmon->scroll.keep_viewport = 0;
	if (wl_list_empty(&selmon->scroll.cols))
		return;
	col = wl_container_of(selmon->scroll.cols.next, col, link);
	scroll_focus_col(selmon, col);
	arrange(selmon);
	printstatus();
}

void
scroll_last(const Arg *arg)
{
	ScrollCol *col;

	if (!selmon || selmon->lt[selmon->sellt]->arrange != scroll)
		return;
	selmon->scroll.keep_viewport = 0;
	if (wl_list_empty(&selmon->scroll.cols))
		return;
	col = wl_container_of(selmon->scroll.cols.prev, col, link);
	scroll_focus_col(selmon, col);
	arrange(selmon);
	printstatus();
}

void
scroll_movecol_first(const Arg *arg)
{
	Client *sel;
	ScrollCol *col;

	if (!selmon || selmon->lt[selmon->sellt]->arrange != scroll)
		return;
	selmon->scroll.keep_viewport = 0;
	sel = focustop(selmon);
	if (!sel || !(col = sel->scol_col) || col->link.prev == &selmon->scroll.cols)
		return;
	wl_list_remove(&col->link);
	wl_list_insert(selmon->scroll.cols.next, &col->link);
	arrange(selmon);
	printstatus();
}

void
scroll_movecol_last(const Arg *arg)
{
	Client *sel;
	ScrollCol *col;

	if (!selmon || selmon->lt[selmon->sellt]->arrange != scroll)
		return;
	selmon->scroll.keep_viewport = 0;
	sel = focustop(selmon);
	if (!sel || !(col = sel->scol_col) || col->link.next == &selmon->scroll.cols)
		return;
	wl_list_remove(&col->link);
	wl_list_insert(&selmon->scroll.cols, &col->link);
	arrange(selmon);
	printstatus();
}

void
scroll_focus_up_down(const Arg *arg)
{
	Client *sel, *cc;
	ScrollCol *col;
	struct wl_list *next;

	if (!selmon || selmon->lt[selmon->sellt]->arrange != scroll)
		return;
	selmon->scroll.keep_viewport = 0;
	sel = focustop(selmon);
	if (!sel || !(col = sel->scol_col))
		return;
	next = (arg->i > 0) ? sel->scol.next : sel->scol.prev;
	if (next == &col->clients)
		return; /* at the top/bottom of the stack */
	cc = wl_container_of(next, cc, scol);
	focusclient(cc, 1);
	arrange(selmon);
	printstatus();
}

/* Column sizing ---------------------------------------------------------- */

void
scroll_cycle_width(const Arg *arg)
{
	int idx;
	Client *sel;
	ScrollCol *col;

	if (!selmon || selmon->lt[selmon->sellt]->arrange != scroll)
		return;
	selmon->scroll.keep_viewport = 0;
	sel = focustop(selmon);
	if (!sel || !(col = sel->scol_col))
		return;

	idx = selmon->scroll.width_idx + (arg && arg->i ? arg->i : 1);
	idx %= (int)scroll_preset_count;
	if (idx < 0)
		idx += (int)scroll_preset_count;
	col->width = scroll_preset_widths[idx];
	selmon->scroll.width_idx = idx;
	arrange(selmon);
	printstatus();
}

void
scroll_width(const Arg *arg)
{
	Client *sel;
	ScrollCol *col;
	double w;

	if (!selmon)
		return;
	if (selmon->lt[selmon->sellt]->arrange != scroll) {
		setmfact(arg);
		return;
	}
	selmon->scroll.keep_viewport = 0;
	sel = focustop(selmon);
	if (!sel || !(col = sel->scol_col))
		return;
	w = col->width + arg->f;
	if (w < scroll_width_min)
		w = scroll_width_min;
	else if (w > scroll_width_max)
		w = scroll_width_max;
	col->width = w;
	arrange(selmon);
	printstatus();
}

void
scroll_center(const Arg *arg)
{
	Client *sel;
	ScrollCol *col;
	double lo, hi, vp;

	if (!selmon || selmon->lt[selmon->sellt]->arrange != scroll)
		return;
	sel = focustop(selmon);
	if (!sel || !(col = sel->scol_col))
		return;

	vp = scroll_col_x(selmon, col) + scroll_col_width(selmon, col) / 2
		- (selmon->w.x + selmon->w.width / 2);
	scroll_viewport_bounds(selmon, &lo, &hi);
	selmon->scroll.vp_to = vp < lo ? lo : (vp > hi ? hi : vp);
	selmon->scroll.keep_viewport = 1;
	arrange(selmon);
	printstatus();
}

/* Mouse wheel / touchpad pan -------------------------------------------- */

int
scroll_can_pan(Monitor *m)
{
	double lo, hi;

	if (!m || !m->wlr_output->enabled)
		return 0;
	scroll_viewport_bounds(m, &lo, &hi);
	return lo != hi;
}

void
scroll_pan(Monitor *m, double delta)
{
	double lo, hi, to;

	if (!m || !m->wlr_output->enabled)
		return;
	scroll_viewport_bounds(m, &lo, &hi);
	to = m->scroll.vp_to + delta;
	if (to < lo)
		to = lo;
	else if (to > hi)
		to = hi;
	m->scroll.vp_to = to;
	m->scroll.keep_viewport = 1;
	arrange(m);
}

/* Window management ------------------------------------------------------ */

void
scroll_consume(const Arg *arg)
{
	Client *sel;
	ScrollCol *col, *leftcol;
	struct wl_list *lnk;

	if (!selmon || selmon->lt[selmon->sellt]->arrange != scroll)
		return;
	selmon->scroll.keep_viewport = 0;
	sel = focustop(selmon);
	if (!sel || !(col = sel->scol_col))
		return;
	lnk = col->link.prev;
	if (lnk == &selmon->scroll.cols)
		return; /* nothing to the left */
	leftcol = wl_container_of(lnk, leftcol, link);

	/* stack sel at the bottom of the column to its left */
	wl_list_remove(&sel->scol);
	wl_list_insert(&leftcol->clients, &sel->scol);
	sel->scol_col = leftcol;

	if (wl_list_empty(&col->clients)) {
		wl_list_remove(&col->link);
		free(col);
	}
	arrange(selmon);
	printstatus();
}

void
scroll_expel(const Arg *arg)
{
	Client *sel;
	ScrollCol *col, *newcol;

	if (!selmon || selmon->lt[selmon->sellt]->arrange != scroll)
		return;
	selmon->scroll.keep_viewport = 0;
	sel = focustop(selmon);
	if (!sel || !(col = sel->scol_col))
		return;
	if (wl_list_length(&col->clients) == 1)
		return; /* already on its own */

	newcol = ecalloc(1, sizeof(*newcol));
	wl_list_init(&newcol->clients);
	newcol->width = col->width;
	wl_list_insert(&col->link, &newcol->link); /* new column after col */

	wl_list_remove(&sel->scol);
	wl_list_insert(&newcol->clients, &sel->scol);
	sel->scol_col = newcol;
	arrange(selmon);
	printstatus();
}

void
scroll_focus_number(const Arg *arg)
{
	ScrollCol *col;
	int i = 0;

	if (!selmon)
		return;
	if (selmon->lt[selmon->sellt]->arrange != scroll) {
		/* workspace switch while tiling: same key, other meaning */
		view(arg);
		return;
	}
	selmon->scroll.keep_viewport = 0;
	if (wl_list_empty(&selmon->scroll.cols))
		return;
	wl_list_for_each(col, &selmon->scroll.cols, link) {
		if (i++ == arg->i) {
			scroll_focus_col(selmon, col);
			arrange(selmon);
			printstatus();
			return;
		}
	}
	/* index out of range: focus the last column */
	col = wl_container_of(selmon->scroll.cols.prev, col, link);
	scroll_focus_col(selmon, col);
	arrange(selmon);
	printstatus();
}

void
scroll_movecolmon(const Arg *arg)
{
	Client *sel, *c, *ctmp, *top;
	ScrollCol *col, *act;
	Monitor *target;
	struct wl_list *after;

	if (!selmon)
		return;
	if (selmon->lt[selmon->sellt]->arrange != scroll) {
		tagmon(arg);
		return;
	}
	sel = focustop(selmon);
	if (!sel || !(col = sel->scol_col))
		return;
	if (wl_list_length(&mons) < 2)
		return;
	target = dirtomon(arg->i);
	if (!target || target == selmon || !target->wlr_output->enabled)
		return;

	/* Unlink the whole column from this monitor's strip first, so the
	 * arrange() inside setmon() can't detach its clients and free it. */
	wl_list_remove(&col->link);
	wl_list_for_each_safe(c, ctmp, &col->clients, scol)
		setmon(c, target, c->tags);

	if (target->lt[target->sellt]->arrange == scroll) {
		/* keep the column as one unit next to the target's focused column */
		Client *tsel = focustop(target);
		act = tsel ? tsel->scol_col : NULL;
		after = act ? &act->link : target->scroll.cols.prev;
		wl_list_insert(after, &col->link);
		if (!wl_list_empty(&col->clients)) {
			top = wl_container_of(col->clients.next, top, scol);
			focusclient(top, 1);
		}
	} else {
		/* non-scroll target: detach every client from the column, which
		 * frees the column once it becomes empty */
		wl_list_for_each_safe(c, ctmp, &col->clients, scol)
			scroll_detach(c);
	}
	arrange(target);
	printstatus();
}

void
scroll_place_float(Client *c)
{
	Monitor *m = c->mon;
	struct wlr_box a;
	int w, h, x, y;

	if (!m || !m->wlr_output->enabled || m->lt[m->sellt]->arrange != scroll)
		return;
	scroll_area(m, &a);
	w = c->geom.width;
	h = c->geom.height;
	if (w < 1)
		w = 1;
	if (h < 1)
		h = 1;
	x = (int)m->scroll.vp_to + (a.width - w) / 2;
	y = a.y + (a.height - h) / 2;
	if (x < a.x)
		x = a.x;
	if (y < a.y)
		y = a.y;
	if (x + w > a.x + a.width)
		x = a.x + a.width - w;
	if (y + h > a.y + a.height)
		y = a.y + a.height - h;
	c->geom.x = x;
	c->geom.y = y;
	resize(c, (struct wlr_box){.x = x, .y = y, .width = w, .height = h}, 0);
}
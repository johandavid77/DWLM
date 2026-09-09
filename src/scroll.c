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

static double
scroll_col_width(Monitor *m, ScrollCol *col)
{
	double w = col->width * m->w.width;
	double max = m->w.width - 2 * scroll_gap;
	if (max < 1)
		max = 1;
	return w > max ? max : w;
}

/* Strip coordinate of a column's left edge */
static double
scroll_col_x(Monitor *m, ScrollCol *col)
{
	ScrollCol *c;
	double x = m->w.x + scroll_gap;
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
	ScrollCol *c;
	double s0 = m->w.x + scroll_gap;
	double s1 = s0;
	wl_list_for_each(c, &m->scroll.cols, link)
		s1 += scroll_col_width(m, c) + scroll_gap;
	*lo = MAX(m->w.x, s1 - m->w.width);
	*hi = MAX(*lo, s0);
}

/* Scroll the viewport so `active` is visible, or clamp if keep_viewport */
static void
scroll_ensure_viewport(Monitor *m, ScrollCol *active)
{
	double lo, hi, vp;

	scroll_viewport_bounds(m, &lo, &hi);
	if (m->scroll.keep_viewport) {
		m->scroll.keep_viewport = 0;
		if (m->scroll.viewport_x < lo)
			m->scroll.viewport_x = lo;
		else if (m->scroll.viewport_x > hi)
			m->scroll.viewport_x = hi;
		return;
	}
	if (!active) {
		if (m->scroll.viewport_x < lo)
			m->scroll.viewport_x = lo;
		else if (m->scroll.viewport_x > hi)
			m->scroll.viewport_x = hi;
		return;
	}

	vp = m->scroll.viewport_x;
	if (scroll_col_x(m, active) - vp < m->w.x + scroll_gap)
		vp = scroll_col_x(m, active) - (m->w.x + scroll_gap);
	if (scroll_col_x(m, active) + scroll_col_width(m, active) - vp
			> m->w.x + m->w.width - scroll_gap)
		vp = scroll_col_x(m, active) + scroll_col_width(m, active)
			- (m->w.x + m->w.width - scroll_gap);
	if (vp < lo)
		vp = lo;
	else if (vp > hi)
		vp = hi;
	m->scroll.viewport_x = vp;
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
	}

	/* 4. Lay out clients, stacked vertically inside their columns */
	{
		double vpx = m->scroll.viewport_x;
		int yy = m->w.y;
		wl_list_for_each(col, &m->scroll.cols, link) {
			int n = wl_list_length(&col->clients);
			double colw = scroll_col_width(m, col);
			int x = (int)(scroll_col_x(m, col) - vpx);
			int ch = n ? (int)((m->w.height - scroll_gap * (n - 1)) / n) : 0;
			Client *cc;
			if (ch < 1)
				ch = 1;
			yy = m->w.y;
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
	sel = focustop(selmon);
	if (!sel || !(col = sel->scol_col))
		return;

	idx = selmon->scroll.width_idx + (arg && arg->i ? arg->i : 1);
	idx %= LENGTH(scroll_preset_widths);
	if (idx < 0)
		idx += LENGTH(scroll_preset_widths);
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
	selmon->scroll.viewport_x = vp < lo ? lo : (vp > hi ? hi : vp);
	selmon->scroll.keep_viewport = 1;
	arrange(selmon);
	printstatus();
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
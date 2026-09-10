/* config_runtime.c -- tiny TOML-style runtime configuration overlay.
 *
 * Included directly into dwlm.c (after config.h/client.h) so it can mutate
 * the config.def.h default variables at runtime without losing the suckless
 * "edit + rebuild" workflow when no config file exists.
 *
 * Supported file: $XDG_CONFIG_HOME/dwlm/config.toml (or ~/.config/...)
 *
 * [general]
 * borderpx     = 3            # focus ring thickness (px)
 * corner_radius = 10          # rounded window corners (scenefx)
 * gap          = 12.0         # gap between columns/windows (px)
 * outer_gap    = 12.0         # margin around the tiling area (px)
 * width_min    = 0.20         # grow/shrink clamps (fraction)
 * width_max    = 0.80
 * presets      = [0.33, 0.50, 0.67]
 *
 * [colors]
 * root    = "#222222"
 * border  = "#222222"
 * focus   = "#66c2ff"
 * urgent  = "#ff0000"
 *
 * Hot-reload: send SIGHUP to the compositor (kill -HUP <pid>). No restart.
 */

#include <ctype.h>

static char *
config_runtime_trim(char *s)
{
	char *e;
	while (isspace((unsigned char)*s))
		s++;
	e = s + strlen(s);
	while (e > s && isspace((unsigned char)e[-1]))
		*--e = '\0';
	return s;
}

static int
config_runtime_parse_int(const char *v, int *out)
{
	char *end;
	long n;
	errno = 0;
	n = strtol(v, &end, 10);
	if (errno || end == v || *config_runtime_trim(end))
		return -1;
	*out = (int)n;
	return 0;
}

static int
config_runtime_parse_double(const char *v, double *out)
{
	char *end;
	double d;
	errno = 0;
	d = strtod(v, &end);
	if (errno || end == v || *config_runtime_trim(end))
		return -1;
	*out = d;
	return 0;
}

/* hex color "#rrggbb" | "#rrggbbaa" into float[4] */
static int
config_runtime_parse_color(const char *v, float out[4])
{
	int i;
	unsigned long rgb;
	char hex[9] = {0};
	if (*v != '#')
		return -1;
	if (strlen(v) != 7 && strlen(v) != 9)
		return -1;
	for (i = 0; i < (int)strlen(v) - 1; i++) {
		if (!isxdigit((unsigned char)v[i + 1]))
			return -1;
		hex[i] = v[i + 1];
	}
	rgb = strtoul(hex, NULL, 16);
	out[0] = ((rgb >> 24) & 0xFF) / 255.0f;
	out[1] = ((rgb >> 16) & 0xFF) / 255.0f;
	out[2] = ((rgb >> 8) & 0xFF) / 255.0f;
	out[3] = strlen(v) == 9 ? (rgb & 0xFF) / 255.0f : 1.0f;
	return 0;
}

/* parse "[a, b, c]" into dst (max maxn); returns count, -1 on error */
static int
config_runtime_parse_doubles(const char *v, double *dst, size_t maxn)
{
	size_t n = 0;
	const char *p = v;
	while (*p && isspace((unsigned char)*p))
		p++;
	if (*p != '[')
		return -1;
	p++;
	for (;;) {
		char *tok;
		while (*p && (isspace((unsigned char)*p) || *p == ',' || *p == ']')) {
			if (*p == ']')
				return (int)n;
			p++;
		}
		/* token start */
		tok = (char *)p;
		while (*p && *p != ',' && *p != ']' && *p != '\0')
			p++;
		if (*p == '\0')
			return -1;
		if (n >= maxn)
			return -1;
		if (config_runtime_parse_double(tok, &dst[n]))
			return -1;
		n++;
		/* consume delimiter or end */
		while (*p && (isspace((unsigned char)*p)))
			p++;
		if (*p == ',')
			p++;
		else if (*p == ']')
			return (int)n;
	}
}

/* Apply a single key=value under the current section. */
static void
config_runtime_apply_key(const char *section, const char *key, const char *value)
{
	double d;
	int i;
	if (!strcmp(section, "general")) {
		if (!strcmp(key, "borderpx") && !config_runtime_parse_int(value, &i)
				&& i >= 0)
			borderpx = (unsigned int)i;
		else if (!strcmp(key, "corner_radius")
				&& !config_runtime_parse_int(value, &i) && i >= 0)
			corner_radius = i;
		else if (!strcmp(key, "gap")
				&& !config_runtime_parse_double(value, &d))
			scroll_gap = d;
		else if (!strcmp(key, "outer_gap")
				&& !config_runtime_parse_double(value, &d))
			scroll_outer_gap = d;
		else if (!strcmp(key, "width_min")
				&& !config_runtime_parse_double(value, &d))
			scroll_width_min = d;
		else if (!strcmp(key, "width_max")
				&& !config_runtime_parse_double(value, &d))
			scroll_width_max = d;
		else if (!strcmp(key, "presets")) {
			size_t n = (size_t)config_runtime_parse_doubles(value,
					scroll_preset_widths, 8);
			if (n > 0) {
				scroll_preset_count = n;
				if (selmon && selmon->scroll.width_idx >= (int)n)
					selmon->scroll.width_idx = 0;
			}
		}
	} else if (!strcmp(section, "colors")) {
		if (!strcmp(key, "root"))
			config_runtime_parse_color(value, rootcolor);
		else if (!strcmp(key, "border"))
			config_runtime_parse_color(value, bordercolor);
		else if (!strcmp(key, "focus"))
			config_runtime_parse_color(value, focuscolor);
		else if (!strcmp(key, "urgent"))
			config_runtime_parse_color(value, urgentcolor);
	}
}

/* Push the current config values into the live scene graph. */
static void
config_runtime_apply(void)
{
	Client *c;
	Client *focused;
	Monitor *m;

	/* re-colour the root background */
	if (root_bg)
		wlr_scene_rect_set_color(root_bg, rootcolor);

	/* refresh client rings: thickness + colour */
	focused = selmon ? focustop(selmon) : NULL;
	wl_list_for_each(c, &clients, link) {
		if (client_is_unmanaged(c))
			continue;
		c->bw = c->isfullscreen ? 0 : borderpx;
		if (c == focused || exclusive_focus == c)
			client_set_border_color(c, focuscolor);
		else
			client_set_border_color(c, bordercolor);
	}

	/* re-layout everything: geometry + rounded corners */
	wl_list_for_each(m, &mons, link) {
		if (m == selmon)
			continue;
		arrange(m);
	}
	if (selmon)
		arrange(selmon);
}

static void
config_runtime_reload(void)
{
	char path[4096];
	const char *home, *xdg;
	const char *cfg;
	char line[1024];
	char section[64] = "";
	FILE *f;
	home = getenv("HOME");
	xdg = getenv("XDG_CONFIG_HOME");
	if (xdg && *xdg)
		snprintf(path, sizeof(path), "%s/dwlm/config.toml", xdg);
	else if (home && *home)
		snprintf(path, sizeof(path), "%s/.config/dwlm/config.toml", home);
	else
		return;

	f = fopen(path, "r");
	if (!f) {
		write(2, "[cfg:nofile]\n", 13);
		return; /* no config file: keep compiled defaults */
	}
	write(2, "[cfg:opened]\n", 14);

	while ((cfg = fgets(line, sizeof(line), f))) {
		char *s = line;
		char *eol = s + strlen(s);
		/* strip trailing newline/CR */
		while (eol > s && (eol[-1] == '\n' || eol[-1] == '\r'))
			*--eol = '\0';
		s = config_runtime_trim(s);
		if (!*s || *s == '#')
			continue;
		if (*s == '[') {
			char *close = strchr(s, ']');
			if (!close)
				continue;
			*close = '\0';
			snprintf(section, sizeof(section), "%s",
					config_runtime_trim(s + 1));
			continue;
		}
		{
			char *eq = strchr(s, '=');
			char *key, *value;
			if (!eq)
				continue;
			*eq = '\0';
			key = config_runtime_trim(s);
			value = config_runtime_trim(eq + 1);
			/* strip trailing comment */
			{
				int inq = 0;
				char *c2 = value;
				for (; *c2; c2++) {
					if (*c2 == '"')
						inq = !inq;
					else if (*c2 == '#' && !inq) {
						*c2 = '\0';
						break;
					}
				}
			}
			value = config_runtime_trim(value);
			if (*key && *value)
				config_runtime_apply_key(section, key, value);
		}
	}
	fclose(f);

	config_runtime_apply();
	write(2, "[cfg:applied]\n", 15);

	wlr_log(WLR_INFO, "[config] reloaded: borderpx=%u radius=%d gap=%.1f "
			"outer=%.1f min=%.2f max=%.2f presets=%zu "
			"root=%02X%02X%02X border=%02X%02X%02X focus=%02X%02X%02X",
			borderpx, corner_radius, scroll_gap, scroll_outer_gap,
			scroll_width_min, scroll_width_max, scroll_preset_count,
			(unsigned)(rootcolor[0] * 255), (unsigned)(rootcolor[1] * 255),
			(unsigned)(rootcolor[2] * 255),
			(unsigned)(bordercolor[0] * 255), (unsigned)(bordercolor[1] * 255),
			(unsigned)(bordercolor[2] * 255),
			(unsigned)(focuscolor[0] * 255), (unsigned)(focuscolor[1] * 255),
			(unsigned)(focuscolor[2] * 255));
}

/* Drained by the main-loop poll in run() (see dwlm.c). SIGHUP only flags;
 * the work happens here in the main thread (async-signal-safe). */
static void
config_runtime_drain(void)
{
	static int ticks;
	if (ticks < 5)
		ticks++, write(2, "[drainwrite]\n", 13);
	if (config_reload_pending) {
		config_reload_pending = 0;
		config_runtime_reload();
	}
}

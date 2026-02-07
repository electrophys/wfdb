/* file: scope.c	G. Moody	31 July 1991
			Last revised:	2026
Scope window functions for WAVE

-------------------------------------------------------------------------------
WAVE: Waveform analyzer, viewer, and editor
Copyright (C) 1991-2005 George B. Moody

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, see <http://www.gnu.org/licenses/>.

You may contact the author by e-mail (wfdb@physionet.org) or postal mail
(MIT Room E25-505A, Cambridge, MA 02139 USA).  For updates to this software,
please visit PhysioNet (http://www.physionet.org/).
_______________________________________________________________________________

*/

#include "wave.h"
#include "gtkwave.h"
#include <wfdb/ecgmap.h>

#define SQRTMAXSPEED	(30)
#define MAXSPEED  (SQRTMAXSPEED*SQRTMAXSPEED)

/* Number of ring-buffer surfaces for the fade effect. */
#define SCOPE_NPLANES	4

/* Alpha values for compositing the ring surfaces (oldest to newest). */
static const double scope_alpha[SCOPE_NPLANES] = { 0.25, 0.50, 0.75, 1.0 };

/* Scope foreground / background colors. */
static double scope_fg_r, scope_fg_g, scope_fg_b;
static double scope_bg_r, scope_bg_g, scope_bg_b;

static void scan(int speed);
static void scope_proc(const char *action);
static void set_dt(int x);

void save_scope_params(int a, int b, int c)
{
    /* In the GTK version overlay planes, color mode, and grey mode are
       irrelevant -- Cairo gives us full RGBA compositing.  We store
       the parameters so that the signature stays compatible with the
       rest of the codebase but do nothing with them. */
    (void)a; (void)b; (void)c;
}

/* ---- Ring buffer of Cairo image surfaces for the fade effect ---- */

static cairo_surface_t *ring[SCOPE_NPLANES];
static int ring_head;		/* index of the *newest* surface */

static unsigned int scope_height, scope_width;
static WavePoint *sbuf;		/* polyline vertex buffer */
static int v0n, v0f, v0v, xt, yt;
static WFDB_Time scope_dt;

/* GTK widgets making up the scope window. */
static GtkWidget *scope_window;
static GtkWidget *scope_canvas;		/* GtkDrawingArea */
static GtkWidget *speed_scale;		/* GtkScale (speed slider) */
static GtkWidget *dt_entry;		/* GtkEntry (dt text field) */

static guint scan_timer_id;		/* g_timeout source id, 0 = inactive */

/* ---- Ring buffer helpers ---- */

/* Ensure all ring surfaces match the current scope dimensions. */
static void ensure_ring_surfaces(void)
{
    int i;
    for (i = 0; i < SCOPE_NPLANES; i++) {
	if (ring[i] &&
	    (int)cairo_image_surface_get_width(ring[i]) == (int)scope_width &&
	    (int)cairo_image_surface_get_height(ring[i]) == (int)scope_height)
	    continue;
	if (ring[i])
	    cairo_surface_destroy(ring[i]);
	ring[i] = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
					     scope_width, scope_height);
	/* Clear to fully transparent. */
	cairo_t *cr = cairo_create(ring[i]);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cr);
	cairo_destroy(cr);
    }
}

/* Clear the oldest surface and advance the ring so it becomes newest. */
static void ring_advance(void)
{
    ring_head = (ring_head + 1) % SCOPE_NPLANES;
    /* Clear the surface that is now "newest" (it was the oldest). */
    cairo_t *cr = cairo_create(ring[ring_head]);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_destroy(cr);
}

/* Composite all ring surfaces (oldest first) onto cr. */
static void ring_composite(cairo_t *cr)
{
    int i, idx;
    /* Fill background first. */
    cairo_set_source_rgb(cr, scope_bg_r, scope_bg_g, scope_bg_b);
    cairo_paint(cr);
    for (i = 0; i < SCOPE_NPLANES; i++) {
	/* Walk from oldest to newest. */
	idx = (ring_head + 1 + i) % SCOPE_NPLANES;
	if (!ring[idx]) continue;
	cairo_set_source_surface(cr, ring[idx], 0, 0);
	cairo_paint_with_alpha(cr, scope_alpha[i]);
    }
}

/* ---- Drawing helpers on the current (newest) ring surface ---- */

static void scope_draw_lines(WavePoint *pts, int npts)
{
    int i;
    cairo_t *cr;
    if (npts < 2 || !ring[ring_head]) return;
    cr = cairo_create(ring[ring_head]);
    cairo_set_source_rgb(cr, scope_fg_r, scope_fg_g, scope_fg_b);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, pts[0].x + 0.5, pts[0].y + 0.5);
    for (i = 1; i < npts; i++)
	cairo_line_to(cr, pts[i].x + 0.5, pts[i].y + 0.5);
    cairo_stroke(cr);
    cairo_destroy(cr);
}

static void scope_draw_string(int x, int y, const char *str)
{
    cairo_t *cr;
    PangoLayout *layout;
    if (!str || !*str || !ring[ring_head]) return;
    cr = cairo_create(ring[ring_head]);
    cairo_set_source_rgb(cr, scope_fg_r, scope_fg_g, scope_fg_b);
    layout = pango_cairo_create_layout(cr);
    if (ann_font)
	pango_layout_set_font_description(layout, ann_font);
    pango_layout_set_text(layout, str, -1);
    /* y is a baseline coordinate; adjust upward. */
    PangoLayoutIter *iter = pango_layout_get_iter(layout);
    int baseline = pango_layout_iter_get_baseline(iter);
    pango_layout_iter_free(iter);
    double y_top = y - (double)baseline / PANGO_SCALE;
    cairo_move_to(cr, x, y_top);
    pango_cairo_show_layout(cr, layout);
    g_object_unref(layout);
    cairo_destroy(cr);
}

/* ---- Resize handling ---- */

static void scope_do_resize(int w, int h)
{
    int i;

    xt = mmx(2);
    if (w > (int)scope_width) {
	WavePoint *sbt;
	if (sbuf == NULL)
	    sbt = (WavePoint *)malloc(w * sizeof(WavePoint));
	else
	    sbt = (WavePoint *)realloc(sbuf, w * sizeof(WavePoint));
	if (sbt == NULL) {
	    wave_notice_prompt("Error in allocating memory for scope");
	    return;
	}
	sbuf = sbt;
    }
    if (w != (int)scope_width - 1) {
	if (scope_width == 0)
	    scope_dt = strtim("0.5");
	scope_width = w - 1;
	if (tscale <= 1.0)
	    for (i = 0; i <= (int)scope_width; i++)
		sbuf[i].x = i;
	else
	    for (i = 0; i <= (int)scope_width; i++)
		sbuf[i].x = i * tscale;
    }
    if (h != (int)scope_height) {
	scope_height = h;
	yt = scope_height - mmy(2);
	v0n = scope_height / 3;
	v0f = scope_height / 2;
	v0v = 2 * scope_height / 3;
    }

    ensure_ring_surfaces();
}

/* ---- GTK drawing callback ---- */

static gboolean scope_on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    (void)widget; (void)data;
    ring_composite(cr);
    return FALSE;
}

/* ---- GTK size-allocate callback ---- */

static void scope_on_size_allocate(GtkWidget *widget, GdkRectangle *alloc,
				   gpointer data)
{
    (void)widget; (void)data;
    scope_do_resize(alloc->width, alloc->height);
}

/* ---- Frame display logic ---- */

static int show_this_frame(void)
{
    static int first_frame = 1;
    int i, i0, tt, tt0 = 0, v0;
    WFDB_Time t;

    if (first_frame) {
	first_frame = 0;
	if (scope_canvas) {
	    GtkAllocation alloc;
	    gtk_widget_get_allocation(scope_canvas, &alloc);
	    scope_do_resize(alloc.width, alloc.height);
	}
    }

    if (scope_annp == NULL) return 0;
    if ((t = scope_annp->this.time - scope_dt) < 0L) {
	tt0 = (int)(-t);
	t = 0L;
    }
    if (isigsettime(t) < 0 || getvec(scope_v) < 0) return 0;
    switch (map2(scope_annp->this.anntyp)) {
      case NORMAL:
      case LEARN:
	v0 = scope_v[signal_choice] * vscale[signal_choice] - v0n;
	break;
      case FUSION:
	v0 = scope_v[signal_choice] * vscale[signal_choice] - v0f;
	break;
      case PVC:
	v0 = scope_v[signal_choice] * vscale[signal_choice] - v0v;
	break;
      default:
	v0 = scope_v[signal_choice] * vscale[signal_choice] - v0n;
	break;
    }
    if (tscale >= 1.0) {
	for (i = i0 = tt0; i < (int)scope_width; i++) {
	    if (getvec(scope_v) <= 0) break;
	    sbuf[i].y = scope_v[signal_choice] * vscale[signal_choice] - v0;
	}
	i--;
    }
    else {
	int vmax, vmin, vv, x;

	(void)getvec(scope_v);
	vmax = vmin = scope_v[signal_choice];
	i = i0 = tt0 * tscale;
	if (i < (int)scope_width)
	    sbuf[i].y = scope_v[signal_choice] * vscale[signal_choice] - v0;
	for (tt = tt0 + 1; i < (int)scope_width && getvec(scope_v) > 0;
	     tt++) {
	    if (scope_v[signal_choice] > vmax) vmax = scope_v[signal_choice];
	    else if (scope_v[signal_choice] < vmin)
		vmin = scope_v[signal_choice];
	    if ((x = tt * tscale) > i) {
		i = x;
		if (vmax - vv > vv - vmin)
		    vv = vmin = vmax;
		else
		    vv = vmax = vmin;
		sbuf[i].y = vv * vscale[signal_choice] - v0;
	    }
	}
    }

    /* Advance the ring: clear the oldest surface, make it newest. */
    ring_advance();

    /* Every full cycle (4 advances), draw the timestamp. */
    {
	static int plane_counter;
	if (++plane_counter > 3) {
	    char *tp;
	    plane_counter = 0;
	    tp = wtimstr(t);
	    scope_draw_string(xt, yt, tp);
	}
    }

    if (i > i0)
	scope_draw_lines(sbuf + i0, i - i0);
    return 1;
}

static void refresh_time(void)
{
    char *tp;
    WFDB_Time t;

    if (scope_annp) {
	/* Advance ring so a fresh surface receives the time text. */
	ring_advance();
	t = scope_annp->this.time - scope_width / 2;
	tp = wtimstr(t);
	scope_draw_string(xt, yt, tp);
    }
    if (scope_canvas)
	gtk_widget_queue_draw(scope_canvas);
}

static int show_next_frame(void)
{
    if (scope_annp == NULL) {
	scan(0);
	return 0;
    }
    while (scope_annp->this.time < begin_analysis_time) {
	if (scope_annp->next == NULL)
	    break;
	scope_annp = scope_annp->next;
    }
    do {
	if (((scope_annp = scope_annp->next) == NULL) ||
	    (scope_annp->this.anntyp == INDEX_MARK) ||
	    (end_analysis_time > 0L &&
	     scope_annp->this.time > end_analysis_time)) {
	    if (scope_annp == NULL) scope_annp = ap_end;
	    else if (scope_annp->this.anntyp == INDEX_MARK &&
		     scope_annp->next != NULL)
		scope_annp = scope_annp->next;
	    else scope_annp = scope_annp->previous;
	    scan(0);
	    return 0;
	}
    } while (!isqrs(scope_annp->this.anntyp) ||
	    (ann_mode == 1 && scope_annp->this.chan != signal_choice));
    return show_this_frame();
}

static int show_prev_frame(void)
{
    if (scope_annp == NULL) {
	scan(0);
	return 0;
    }
    while (end_analysis_time > 0L &&
	   scope_annp->this.time > end_analysis_time) {
	if (scope_annp->previous == NULL)
	    break;
	scope_annp = scope_annp->previous;
    }
    do {
	if (((scope_annp = scope_annp->previous) == NULL) ||
	    (scope_annp->this.anntyp == INDEX_MARK) ||
	    (scope_annp->this.time < begin_analysis_time)) {
	    if (scope_annp == NULL) scope_annp = ap_start;
	    else if (scope_annp->this.anntyp == INDEX_MARK &&
		     scope_annp->previous != NULL)
		scope_annp = scope_annp->previous;
	    else scope_annp = scope_annp->next;
	    scan(0);
	    return 0;
	}
    } while (!isqrs(scope_annp->this.anntyp) ||
	    (ann_mode == 1 && scope_annp->this.chan != signal_choice));
    return show_this_frame();
}

static int speed = MAXSPEED;

static gboolean show_next_n_frames(gpointer data)
{
    int i;
    (void)data;
    for (i = speed / 10 + 1; i > 0; i--)
	if (show_next_frame() == 0) break;
    refresh_time();
    return G_SOURCE_CONTINUE;
}

static gboolean show_prev_n_frames(gpointer data)
{
    int i;
    (void)data;
    for (i = speed / 10 + 1; i > 0; i--)
	if (show_prev_frame() == 0) break;
    refresh_time();
    return G_SOURCE_CONTINUE;
}

static void scan(int s)
{
    /* Remove any existing timer. */
    if (scan_timer_id) {
	g_source_remove(scan_timer_id);
	scan_timer_id = 0;
    }
    if (s > 0) {
	if (s > MAXSPEED) s = MAXSPEED;
	guint interval_ms = 1000 / s;
	if (interval_ms == 0) interval_ms = 1;
	scan_timer_id = g_timeout_add(interval_ms, show_next_n_frames, NULL);
	scan_active = 1;
    }
    else if (s == 0) {
	scan_active = 0;
    }
    else {
	if (s < -MAXSPEED) s = -MAXSPEED;
	guint interval_ms = 1000 / (-s);
	if (interval_ms == 0) interval_ms = 1;
	scan_timer_id = g_timeout_add(interval_ms, show_prev_n_frames, NULL);
	scan_active = -1;
    }
}

static void scope_proc(const char *action)
{
    WFDB_Time t0;
    int ch;

    if (!action || !*action) return;
    ch = action[0];

    if (ap_start == NULL) {
	wave_notice_prompt("Scope functions cannot be used while the "
			   "annotation list is empty.");
	return;
    }
    if (attached &&
	(begin_analysis_time <= attached->this.time &&
	 (attached->this.time <= end_analysis_time ||
	  end_analysis_time < 0L))) {
	scope_annp = attached;
	attached = NULL;
    }
    else if (scope_annp == NULL) {
	(void)locate_annotation(display_start_time, -128);
	scope_annp = annp;
    }

    switch (ch) {
      case '[':		/* scan backwards */
	box(0, 0, 0);
	scan(-speed);
	break;
      case '<':		/* single-step backwards */
	show_prev_frame();
	refresh_time();
	if (display_start_time < scope_annp->this.time &&
	    scope_annp->this.time < display_start_time + nsamp)
	    box((int)((scope_annp->this.time - display_start_time) * tscale),
		(ann_mode == 1 && (unsigned)scope_annp->this.chan < (unsigned)nsig) ?
		(int)(base[(unsigned)scope_annp->this.chan] + mmy(2)) : abase,
		1);
	else
	    box(0, 0, 0);
	break;
      case '*':		/* interrupt scan / pause */
	scan(0);
	refresh_time();
	if ((t0 = scope_annp->this.time - nsamp / 2) < 0L) t0 = 0L;
	find_display_list(t0);
	set_start_time(wtimstr(t0));
	set_end_time(wtimstr(t0 + nsamp));
	disp_proc(".");
	box((int)((scope_annp->this.time - display_start_time) * tscale),
	    (ann_mode == 1 && (unsigned)scope_annp->this.chan < (unsigned)nsig) ?
	    (int)(base[(unsigned)scope_annp->this.chan] + mmy(2)) : abase,
	    1);
	break;
      case '>':		/* single-step forwards */
	show_next_frame();
	refresh_time();
	if (display_start_time < scope_annp->this.time &&
	    scope_annp->this.time < display_start_time + nsamp)
	    box((int)((scope_annp->this.time - display_start_time) * tscale),
		(ann_mode == 1 && (unsigned)scope_annp->this.chan < (unsigned)nsig) ?
		(int)(base[(unsigned)scope_annp->this.chan] + mmy(2)) : abase,
		1);
	else
	    box(0, 0, 0);
	break;
      case ']':		/* scan forwards */
      default:
	box(0, 0, 0);
	scan(speed);
	break;
    }
}

static void adjust_speed(GtkRange *range, gpointer data)
{
    int value;
    (void)data;
    value = (int)gtk_range_get_value(range);
    speed = value * value;
    if (scan_active) scan(scan_active * speed);
}

static char *lmstimstr(WFDB_Time t)
{
    char *p, *p0;

    if (t == 0L)
	return "0";
    else if (t > 0L)
	p = wmstimstr(t);
    else
	p = p0 = wmstimstr(-t);
    while (*p == ' ' || *p == '0' || *p == ':')
	p++;
    if (*p == '.') p--;
    if (t < 0L && p > p0)
	*(--p) = '-';
    return p;
}

static void adjust_dt(GtkEntry *entry, gpointer data)
{
    const char *dt_string;
    (void)data;
    dt_string = gtk_entry_get_text(entry);
    while (*dt_string == ' ' || *dt_string == '\t')
	dt_string++;
    if (*dt_string != '-')
	scope_dt = strtim(dt_string);
    else
	scope_dt = -strtim(dt_string + 1);
    gtk_entry_set_text(GTK_ENTRY(dt_entry), lmstimstr(scope_dt));
}

static void set_dt(int x)
{
    scope_dt = x;
    scope_dt /= tscale;
    gtk_entry_set_text(GTK_ENTRY(dt_entry), lmstimstr(scope_dt));
}

/* ---- Button click callbacks ---- */

static void on_scan_back(GtkButton *btn, gpointer data)
{
    (void)btn; (void)data;
    scope_proc("[");
}

static void on_step_back(GtkButton *btn, gpointer data)
{
    (void)btn; (void)data;
    scope_proc("<");
}

static void on_pause(GtkButton *btn, gpointer data)
{
    (void)btn; (void)data;
    scope_proc("*");
}

static void on_step_fwd(GtkButton *btn, gpointer data)
{
    (void)btn; (void)data;
    scope_proc(">");
}

static void on_scan_fwd(GtkButton *btn, gpointer data)
{
    (void)btn; (void)data;
    scope_proc("]");
}

/* ---- Scope canvas event handler (keyboard / mouse on drawing area) ---- */

static gboolean scope_canvas_event(GtkWidget *widget, GdkEvent *event,
				   gpointer data)
{
    (void)widget; (void)data;
    if (event->type == GDK_KEY_PRESS) {
	guint kv = event->key.keyval;
	gboolean ctrl = (event->key.state & GDK_CONTROL_MASK) != 0;
	if (kv == GDK_KEY_Left) {
	    if (ctrl) scope_proc("[");
	    else      scope_proc("<");
	    return TRUE;
	}
	if (kv == GDK_KEY_Right) {
	    if (ctrl) scope_proc("]");
	    else      scope_proc(">");
	    return TRUE;
	}
	if (kv == GDK_KEY_Up || kv == GDK_KEY_KP_Begin) {
	    if (ctrl) {
		/* Ctrl+middle-click equivalent: set dt from cursor x. */
	    } else {
		scope_proc("*");
	    }
	    return TRUE;
	}
    }
    else if (event->type == GDK_BUTTON_PRESS) {
	guint button = event->button.button;
	gboolean ctrl = (event->button.state & GDK_CONTROL_MASK) != 0;
	if (button == 1) {
	    if (ctrl) scope_proc("[");
	    else      scope_proc("<");
	    return TRUE;
	}
	if (button == 2) {
	    if (ctrl) set_dt((int)event->button.x);
	    else      scope_proc("*");
	    return TRUE;
	}
	if (button == 3) {
	    if (ctrl) scope_proc("]");
	    else      scope_proc(">");
	    return TRUE;
	}
    }
    return FALSE;
}

/* ---- Build the scope window ---- */

static void create_scope_popup(void)
{
    GtkWidget *vbox, *hbox, *btn, *label;
    const char *env;

    /* Determine foreground and background colors. */
    env = getenv("WAVE_SCOPE_BG");
    if (!env) env = "white";
    {
	GdkRGBA rgba;
	if (gdk_rgba_parse(&rgba, env)) {
	    scope_bg_r = rgba.red;
	    scope_bg_g = rgba.green;
	    scope_bg_b = rgba.blue;
	} else {
	    scope_bg_r = scope_bg_g = scope_bg_b = 1.0;
	}
    }
    env = getenv("WAVE_SCOPE_FG");
    if (!env) env = "blue";
    {
	GdkRGBA rgba;
	if (gdk_rgba_parse(&rgba, env)) {
	    scope_fg_r = rgba.red;
	    scope_fg_g = rgba.green;
	    scope_fg_b = rgba.blue;
	} else {
	    scope_fg_r = scope_fg_g = 0.0;
	    scope_fg_b = 1.0;
	}
    }

    /* Top-level window. */
    scope_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(scope_window), "Scope");
    gtk_window_set_default_size(GTK_WINDOW(scope_window),
				mmx(25) + 4, mmy(150));
    if (main_window)
	gtk_window_set_transient_for(GTK_WINDOW(scope_window),
				     GTK_WINDOW(main_window));
    /* Do not destroy -- just hide on close. */
    g_signal_connect(scope_window, "delete-event",
		     G_CALLBACK(gtk_widget_hide_on_delete), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_add(GTK_CONTAINER(scope_window), vbox);

    /* ---- Control panel (toolbar row) ---- */
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

    /* Speed slider (vertical in XView, horizontal here for compactness). */
    label = gtk_label_new("Speed");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
    speed_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
					   1, SQRTMAXSPEED, 1);
    gtk_range_set_value(GTK_RANGE(speed_scale), SQRTMAXSPEED);
    gtk_scale_set_draw_value(GTK_SCALE(speed_scale), FALSE);
    gtk_widget_set_size_request(speed_scale, 80, -1);
    g_signal_connect(speed_scale, "value-changed",
		     G_CALLBACK(adjust_speed), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), speed_scale, FALSE, FALSE, 2);

    /* dt text entry. */
    label = gtk_label_new("dt:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
    dt_entry = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(dt_entry), 6);
    gtk_entry_set_text(GTK_ENTRY(dt_entry), "0.500");
    g_signal_connect(dt_entry, "activate",
		     G_CALLBACK(adjust_dt), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), dt_entry, FALSE, FALSE, 2);

    /* Navigation buttons. */
    btn = gtk_button_new_with_label("<<");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_scan_back), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), btn, FALSE, FALSE, 0);

    btn = gtk_button_new_with_label("<");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_step_back), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), btn, FALSE, FALSE, 0);

    btn = gtk_button_new_with_label("Pause");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_pause), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), btn, FALSE, FALSE, 0);

    btn = gtk_button_new_with_label(">");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_step_fwd), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), btn, FALSE, FALSE, 0);

    btn = gtk_button_new_with_label(">>");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_scan_fwd), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), btn, FALSE, FALSE, 0);

    /* ---- Scope canvas (drawing area) ---- */
    scope_canvas = gtk_drawing_area_new();
    gtk_widget_set_size_request(scope_canvas, mmx(25), mmy(100));
    gtk_widget_set_can_focus(scope_canvas, TRUE);
    gtk_widget_add_events(scope_canvas,
			  GDK_BUTTON_PRESS_MASK |
			  GDK_KEY_PRESS_MASK |
			  GDK_STRUCTURE_MASK);
    g_signal_connect(scope_canvas, "draw",
		     G_CALLBACK(scope_on_draw), NULL);
    g_signal_connect(scope_canvas, "size-allocate",
		     G_CALLBACK(scope_on_size_allocate), NULL);
    g_signal_connect(scope_canvas, "event",
		     G_CALLBACK(scope_canvas_event), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), scope_canvas, TRUE, TRUE, 0);

    gtk_widget_show_all(scope_window);

    /* Trigger initial resize. */
    {
	GtkAllocation alloc;
	gtk_widget_get_allocation(scope_canvas, &alloc);
	scope_do_resize(alloc.width, alloc.height);
    }
}

static int scope_popup_active = -1;

void show_scope_window(void)
{
    if (scope_popup_active < 0)
	create_scope_popup();
    gtk_window_present(GTK_WINDOW(scope_window));
    scope_popup_active = 1;
}

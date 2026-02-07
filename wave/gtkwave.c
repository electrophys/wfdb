/* file: gtkwave.c	G. Moody	27 April 1990
			Last revised:	2026

GTK 3 support functions for WAVE

This file replaces the original xvwave.c (XView/X11 toolkit layer).

-------------------------------------------------------------------------------
WAVE: Waveform analyzer, viewer, and editor
Copyright (C) 1990-2005 George B. Moody

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

#define INIT
#include "wave.h"
#include "gtkwave.h"
#include "bitmaps.h"
#include <glib-unix.h>
#include <pwd.h>
#include <signal.h>
#include <unistd.h>

static char sentinel[30];
static int in_main_loop;

/* ---- Preferences (GKeyFile, replaces XView defaults database) ---- */

static GKeyFile *prefs;
static char prefs_path[PATH_MAX];

static void load_prefs(void)
{
    const char *home = getenv("HOME");
    if (!home) home = ".";
    snprintf(prefs_path, sizeof(prefs_path), "%s/.config/wave/waverc", home);
    prefs = g_key_file_new();
    g_key_file_load_from_file(prefs, prefs_path, G_KEY_FILE_NONE, NULL);
}

static const char *prefs_get_string(const char *key, const char *fallback)
{
    static char buf[256];
    char *val = g_key_file_get_string(prefs, "Wave", key, NULL);
    if (val) {
	strncpy(buf, val, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';
	g_free(val);
	return buf;
    }
    return fallback;
}

static int prefs_get_integer(const char *key, int fallback)
{
    GError *err = NULL;
    int val = g_key_file_get_integer(prefs, "Wave", key, &err);
    if (err) { g_error_free(err); return fallback; }
    return val;
}

static gboolean prefs_get_boolean(const char *key, gboolean fallback)
{
    GError *err = NULL;
    gboolean val = g_key_file_get_boolean(prefs, "Wave", key, &err);
    if (err) { g_error_free(err); return fallback; }
    return val;
}

/* ---- Color management ---- */

/* Parse a color name or hex string into a WaveColor. */
static void parse_color(const char *name, WaveColor *c)
{
    GdkRGBA rgba;
    if (gdk_rgba_parse(&rgba, name)) {
	c->r = rgba.red;
	c->g = rgba.green;
	c->b = rgba.blue;
	c->a = rgba.alpha;
    } else {
	/* fallback to black */
	c->r = c->g = c->b = 0.0;
	c->a = 1.0;
    }
}

static void init_colors(void)
{
    parse_color(prefs_get_string("Color.Background", "white"),
		&wave_colors[WAVE_COLOR_BACKGROUND]);
    parse_color(prefs_get_string("Color.Grid", "#E5E5E5"),
		&wave_colors[WAVE_COLOR_GRID]);
    parse_color(prefs_get_string("Color.GridCoarse", "#CCCCCC"),
		&wave_colors[WAVE_COLOR_GRID_COARSE]);
    parse_color(prefs_get_string("Color.Cursor", "OrangeRed"),
		&wave_colors[WAVE_COLOR_CURSOR]);
    parse_color(prefs_get_string("Color.Annotation", "YellowGreen"),
		&wave_colors[WAVE_COLOR_ANNOTATION]);
    parse_color(prefs_get_string("Color.Signal", "blue"),
		&wave_colors[WAVE_COLOR_SIGNAL]);
    parse_color(prefs_get_string("Color.Highlight", "OrangeRed"),
		&wave_colors[WAVE_COLOR_HIGHLIGHT]);
}

/* ---- Drawing helper API ---- */

void wave_set_color(cairo_t *cr, WaveColorIndex idx)
{
    WaveColor *c = &wave_colors[idx];
    cairo_set_source_rgba(cr, c->r, c->g, c->b, c->a);
}

void wave_draw_line(cairo_t *cr, WaveColorIndex color,
		    int x1, int y1, int x2, int y2)
{
    wave_set_color(cr, color);
    cairo_set_line_width(cr, (color == WAVE_COLOR_GRID_COARSE) ? 2.0 : 1.0);
    cairo_move_to(cr, x1 + 0.5, y1 + 0.5);
    cairo_line_to(cr, x2 + 0.5, y2 + 0.5);
    cairo_stroke(cr);
}

void wave_draw_lines(cairo_t *cr, WaveColorIndex color,
		     WavePoint *pts, int npts)
{
    int i;
    if (npts < 2) return;
    wave_set_color(cr, color);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, pts[0].x + 0.5, pts[0].y + 0.5);
    for (i = 1; i < npts; i++)
	cairo_line_to(cr, pts[i].x + 0.5, pts[i].y + 0.5);
    cairo_stroke(cr);
}

void wave_draw_string(cairo_t *cr, WaveColorIndex color,
		      int x, int y, const char *str)
{
    if (!str || !*str) return;
    wave_set_color(cr, color);
    pango_layout_set_text(ann_layout, str, -1);
    /* y is baseline position; pango draws from top-left, so adjust */
    PangoLayoutIter *iter = pango_layout_get_iter(ann_layout);
    int baseline_pango = pango_layout_iter_get_baseline(iter);
    pango_layout_iter_free(iter);
    double y_top = y - (double)baseline_pango / PANGO_SCALE;
    cairo_move_to(cr, x, y_top);
    pango_cairo_show_layout(cr, ann_layout);
}

void wave_draw_segments(cairo_t *cr, WaveColorIndex color,
			WaveSegment *segs, int n)
{
    int i;
    wave_set_color(cr, color);
    cairo_set_line_width(cr, 1.0);
    for (i = 0; i < n; i++) {
	cairo_move_to(cr, segs[i].x1 + 0.5, segs[i].y1 + 0.5);
	cairo_line_to(cr, segs[i].x2 + 0.5, segs[i].y2 + 0.5);
    }
    cairo_stroke(cr);
}

void wave_fill_rect(cairo_t *cr, WaveColorIndex color,
		    int x, int y, int w, int h)
{
    wave_set_color(cr, color);
    cairo_rectangle(cr, x, y, w, h);
    cairo_fill(cr);
}

cairo_t *wave_begin_paint(void)
{
    return cairo_create(osb);
}

void wave_end_paint(cairo_t *cr)
{
    cairo_destroy(cr);
}

int wave_text_width(const char *str)
{
    int w;
    if (!str || !*str) return 0;
    pango_layout_set_text(ann_layout, str, -1);
    pango_layout_get_pixel_size(ann_layout, &w, NULL);
    return w;
}

int wave_text_height(void)
{
    int h;
    pango_layout_set_text(ann_layout, "Xg", -1);
    pango_layout_get_pixel_size(ann_layout, NULL, &h);
    return h;
}

void wave_refresh(void)
{
    if (drawing_area)
	gtk_widget_queue_draw(drawing_area);
}

/* ---- Utility dialogs ---- */

int wave_notice_prompt(const char *message)
{
    GtkWidget *dialog;
    int result;

    dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
				    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				    GTK_MESSAGE_QUESTION,
				    GTK_BUTTONS_YES_NO,
				    "%s", message);
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return (result == GTK_RESPONSE_YES);
}

void wave_set_frame_title(const char *title)
{
    if (main_window)
	gtk_window_set_title(GTK_WINDOW(main_window), title);
}

void wave_set_left_footer(const char *text)
{
    if (status_bar) {
	guint ctx = gtk_statusbar_get_context_id(GTK_STATUSBAR(status_bar), "left");
	gtk_statusbar_pop(GTK_STATUSBAR(status_bar), ctx);
	if (text && *text)
	    gtk_statusbar_push(GTK_STATUSBAR(status_bar), ctx, text);
    }
}

void wave_set_right_footer(const char *text)
{
    /* GTK statusbar doesn't have left/right sections natively.
       We use a single statusbar; right footer is shown as a second push. */
    if (status_bar) {
	guint ctx = gtk_statusbar_get_context_id(GTK_STATUSBAR(status_bar), "right");
	gtk_statusbar_pop(GTK_STATUSBAR(status_bar), ctx);
	if (text && *text)
	    gtk_statusbar_push(GTK_STATUSBAR(status_bar), ctx, text);
    }
}

void wave_set_busy(int busy)
{
    if (main_window) {
	GdkWindow *gwin = gtk_widget_get_window(main_window);
	if (gwin) {
	    if (busy) {
		GdkCursor *cursor = gdk_cursor_new_for_display(
		    gdk_window_get_display(gwin), GDK_WATCH);
		gdk_window_set_cursor(gwin, cursor);
		g_object_unref(cursor);
	    } else {
		gdk_window_set_cursor(gwin, NULL);
	    }
	}
    }
}

/* ---- Signal window callbacks ---- */

static void set_crosshair_cursor(GtkWidget *widget, gpointer data)
{
    GdkDisplay *disp = gtk_widget_get_display(widget);
    GdkCursor *cursor = gdk_cursor_new_for_display(disp, GDK_CROSSHAIR);
    (void)data;
    gdk_window_set_cursor(gtk_widget_get_window(widget), cursor);
    g_object_unref(cursor);
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    (void)widget; (void)data;

    /* Blit the offscreen buffer */
    if (osb) {
	cairo_set_source_surface(cr, osb, 0, 0);
	cairo_paint(cr);
    }

    /* Draw cursor overlay (XOR-like effect via DIFFERENCE operator) */
    if (cursor_active || box_active) {
	cairo_set_operator(cr, CAIRO_OPERATOR_DIFFERENCE);
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_set_line_width(cr, 1.0);

	if (cursor_active) {
	    /* Vertical bar spanning the window */
	    cairo_move_to(cr, cursor_x + 0.5, 0);
	    cairo_line_to(cr, cursor_x + 0.5, canvas_height);
	    cairo_stroke(cr);
	}

	if (box_active) {
	    cairo_rectangle(cr,
			    box_x0 + 0.5, box_y0 + 0.5,
			    box_x1 - box_x0, box_y1 - box_y0);
	    cairo_stroke(cr);
	}
    }

    return FALSE;
}

static void do_resize(int width, int height)
{
    int canvas_width_mm, u;
    cairo_surface_t *old_osb;

    /* Adjust canvas width to a standard time interval */
    canvas_width_mm = width / dmmx(1);
    if (canvas_width_mm > 125) {
	canvas_width_sec = canvas_width_mm / 25;
	canvas_width_mm = 25 * (int)canvas_width_sec;
    } else {
	u = canvas_width_mm / 5;
	canvas_width_mm = u * 5;
	canvas_width_sec = u * 0.2;
    }
    canvas_width = mmx(canvas_width_mm);

    canvas_height = height;
    canvas_height_mv = canvas_height / dmmy(10);

    /* Create new offscreen buffer */
    old_osb = osb;
    osb = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
				     width, canvas_height);

    /* Clear the new surface */
    {
	cairo_t *cr = cairo_create(osb);
	wave_set_color(cr, WAVE_COLOR_BACKGROUND);
	cairo_paint(cr);
	cairo_destroy(cr);
    }

    if (old_osb)
	cairo_surface_destroy(old_osb);

    /* Recalibrate based on selected scales, clear the display list cache. */
    if (*record) {
	set_baselines();
	alloc_sigdata(nsig > 2 ? nsig : 2);
	dismiss_mode();

	vscale[0] = 0.;
	calibrate();
    }

    /* Repaint */
    restore_grid();
    do_disp();
    restore_cursor();
}

static gboolean on_configure(GtkWidget *widget, GdkEventConfigure *event,
			      gpointer data)
{
    (void)data;
    if (in_main_loop) {
	int w = gtk_widget_get_allocated_width(widget);
	int h = gtk_widget_get_allocated_height(widget);
	do_resize(w, h);
    }
    return FALSE;
}

/* ---- Remote control via SIGUSR1 + sentinel file ---- */

static gboolean handle_sigusr1(gpointer data)
{
    char buf[80], new_annotator[30], new_time[30], new_record[70],
	new_siglist[70];
    FILE *sfile;
    extern void wfdb_addtopath(const char *);

    (void)data;

    sfile = fopen(sentinel, "r");
    if (!sfile) return G_SOURCE_CONTINUE;

    new_annotator[0] = new_time[0] = new_record[0] = new_siglist[0] = '\0';
    while (fgets(buf, sizeof(buf), sfile)) {
	if (buf[0] != '-') continue;
	switch (buf[1]) {
	  case 'a':
	      new_annotator[sizeof(new_annotator)-1] = '\0';
	      strncpy(new_annotator, buf+3, sizeof(new_annotator)-1);
	      new_annotator[strlen(new_annotator)-1] = '\0';
	      break;
	  case 'f':
	      new_time[sizeof(new_time)-1] = '\0';
	      strncpy(new_time, buf+3, sizeof(new_time)-1);
	      new_time[strlen(new_time)-1] = '\0';
	      break;
	  case 'p':
	      wfdb_addtopath(buf+3);
	      break;
	  case 'r':
	      new_record[sizeof(new_record)-1] = '\0';
	      strncpy(new_record, buf+3, sizeof(new_record)-1);
	      new_record[strlen(new_record)-1] = '\0';
	      break;
	  case 's':
	      new_siglist[sizeof(new_siglist)-1] = '\0';
	      strncpy(new_siglist, buf+3, sizeof(new_siglist)-1);
	      new_siglist[strlen(new_siglist)-1] = '\0';
	      break;
	}
    }
    fclose(sfile);

    if (*new_record && strcmp(record, new_record)) {
	char *p = new_record, *q;
	int change_record = 1;

	while (*p && (q = strchr(p, '+'))) {
	    if (strncmp(p, record, q-p) == 0) {
		change_record = 0;
		break;
	    }
	    p = q+1;
	}
	if (change_record) set_record_item(p);
	if (p > new_record) p--;
	*p = '\0';
    }
    if (*new_annotator && strcmp(annotator, new_annotator))
	set_annot_item(new_annotator);
    if (*new_time)
	set_start_time(new_time);
    if (*new_siglist) {
	set_siglist_from_string(new_siglist);
	if (sig_mode == 0) {
	    sig_mode = 1;
	    mode_undo();
	    set_baselines();
	}
	freeze_siglist = 1;
    }

    if (wave_ppid) {
	if (*new_time == '\0')
	    strcpy(new_time, mstimstr(display_start_time));
	if (*new_record)
	    sprintf(buf, "wave-remote -pid %d -r %s -f '%s'",
		    wave_ppid, new_record, new_time);
	else
	    sprintf(buf, "wave-remote -pid %d -f '%s'", wave_ppid, new_time);
	if (*new_siglist) {
	    strcat(buf, " -s ");
	    strcat(buf, new_siglist);
	}
	strcat(buf, "\n");
	system(buf);
    }
    disp_proc(".");
    fclose(fopen(sentinel, "w"));
    return G_SOURCE_CONTINUE;
}

void sync_other_wave_processes(void)
{
    char buf[80];

    if (wave_ppid) {
	sprintf(buf, "wave-remote -pid %d -f '%s'\n", wave_ppid,
		mstimstr(-display_start_time));
	system(buf);
    }
}

/* ---- Window close handler ---- */

static gboolean on_delete_event(GtkWidget *widget, GdkEvent *event,
				gpointer data)
{
    (void)widget; (void)event; (void)data;

    if (!wave_notice_prompt("Are you sure you want to Quit?"))
	return TRUE;	/* prevent close */

    if (post_changes())
	finish_log();
    unlink(sentinel);
    return FALSE;	/* allow close */
}

/* ---- Save current settings ---- */

void save_defaults(void)
{
    char tstring[256], *dir;

    snprintf(tstring, sizeof(tstring), "%gx%g", 25.4 * dpmmx, 25.4 * dpmmy);
    g_key_file_set_string(prefs, "Wave", "Dpi", tstring);
    g_key_file_set_integer(prefs, "Wave", "SignalWindow.Height_mm",
			   (int)(canvas_height / (dpmmy * 10)) * 10 + 10);
    g_key_file_set_integer(prefs, "Wave", "SignalWindow.Width_mm",
			   (int)(canvas_width / (dpmmx * 25)) * 25 + 25);
    g_key_file_set_boolean(prefs, "Wave", "View.Subtype", show_subtype);
    g_key_file_set_boolean(prefs, "Wave", "View.Chan", show_chan);
    g_key_file_set_boolean(prefs, "Wave", "View.Num", show_num);
    g_key_file_set_boolean(prefs, "Wave", "View.Aux", show_aux);
    g_key_file_set_boolean(prefs, "Wave", "View.Markers", show_marker);
    g_key_file_set_boolean(prefs, "Wave", "View.SignalNames", show_signame);
    g_key_file_set_boolean(prefs, "Wave", "View.Baselines", show_baseline);
    g_key_file_set_boolean(prefs, "Wave", "View.Level", show_level);
    if (sampfreq(NULL) >= 10.0)
	g_key_file_set_integer(prefs, "Wave", "View.TimeScale", tsa_index);
    else
	g_key_file_set_integer(prefs, "Wave", "View.CoarseTimeScale",
			       tsa_index);
    g_key_file_set_integer(prefs, "Wave", "View.AmplitudeScale", vsa_index);
    g_key_file_set_integer(prefs, "Wave", "View.AnnotationMode", ann_mode);
    g_key_file_set_integer(prefs, "Wave", "View.AnnotationOverlap", overlap);
    g_key_file_set_integer(prefs, "Wave", "View.SignalMode", sig_mode);
    g_key_file_set_integer(prefs, "Wave", "View.TimeMode", time_mode);
    if (tsa_index > MAX_COARSE_TSA_INDEX)
	g_key_file_set_integer(prefs, "Wave", "View.GridMode", grid_mode);
    else
	g_key_file_set_integer(prefs, "Wave", "View.CoarseGridMode",
			       grid_mode);

    /* Ensure directory exists */
    dir = g_path_get_dirname(prefs_path);
    g_mkdir_with_parents(dir, 0755);
    g_free(dir);

    g_key_file_save_to_file(prefs, prefs_path, NULL);
}

/* ---- Graphics initialization ---- */

void strip_x_args(int *pargc, char *argv[])
{
    /* In the GTK version, gtk_init handles argument parsing. */
    gtk_init(pargc, &argv);

    /* Set HOME environment variable if necessary. */
    if (getenv("HOME") == NULL) {
	static char homedir[80];
	char *tmp;
	struct passwd *pw;

	if ((tmp = getenv("USER")) != NULL)
	    pw = getpwnam(tmp);
	else
	    pw = getpwuid(getuid());
	snprintf(homedir, sizeof(homedir), "HOME=%s",
		 pw ? pw->pw_dir : ".");
	putenv(homedir);
    }

    load_prefs();
}

int initialize_graphics(int mode)
{
    GdkDisplay *gdk_display;
    GdkMonitor *monitor;
    GdkRectangle geom;
    int height_mm, width_mm, height_px, width_px;
    int hmmpref = 120, wmmpref = 250;
    const char *annfontname;
    GtkWidget *vbox;

    (void)mode;	 /* graphics mode flags (mono/grey/overlay) are not needed
		    with Cairo's full RGBA support */

    /* Get display info for screen resolution */
    gdk_display = gdk_display_get_default();
    monitor = gdk_display_get_primary_monitor(gdk_display);
    if (!monitor)
	monitor = gdk_display_get_monitor(gdk_display, 0);
    gdk_monitor_get_geometry(monitor, &geom);

    height_px = geom.height;
    width_px = geom.width;
    height_mm = gdk_monitor_get_height_mm(monitor);
    width_mm = gdk_monitor_get_width_mm(monitor);

    /* Determine screen resolution */
    if (dpmmx == 0) {
	const char *dpi_str = prefs_get_string("Dpi", "0x0");
	sscanf(dpi_str, "%lfx%lf", &dpmmx, &dpmmy);
	dpmmx /= 25.4;
	dpmmy /= 25.4;
    }
    if (height_mm > 0) {
	if (dpmmy == 0.) dpmmy = ((double)height_px) / height_mm;
	else height_mm = height_px / dpmmy;
    } else {
	dpmmy = DPMMY;
	height_mm = height_px / dpmmy;
    }
    if (width_mm > 0) {
	if (dpmmx == 0.) dpmmx = ((double)width_px) / width_mm;
	else width_mm = width_px / dpmmx;
    } else {
	dpmmx = DPMMX;
	width_mm = width_px / dpmmx;
    }

    /* Check minimum display size */
    if (width_mm < 53 || height_mm < 75) {
	fprintf(stderr, "%s: display too small\n", pname);
	return 1;
    }

    /* Choose signal window dimensions */
    wmmpref = prefs_get_integer("SignalWindow.Width_mm", 250);
    if (wmmpref > width_mm - 3) wmmpref = width_mm - 3;
    else if (wmmpref < 50) wmmpref = 50;
    hmmpref = prefs_get_integer("SignalWindow.Height_mm", 120);
    if (hmmpref > height_mm - 25) hmmpref = height_mm - 25;
    else if (hmmpref < 50) hmmpref = 50;
    canvas_width = mmx(wmmpref);
    canvas_height = mmy(hmmpref);
    linesp = mmy(4);

    /* Initialize colors */
    init_colors();

    /* Create main window */
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), pname);
    gtk_window_set_default_size(GTK_WINDOW(main_window),
				canvas_width + 4,
				canvas_height + mmy(15));
    g_signal_connect(main_window, "delete-event",
		     G_CALLBACK(on_delete_event), NULL);
    g_signal_connect(main_window, "destroy",
		     G_CALLBACK(gtk_main_quit), NULL);

    /* Set window icon from XBM data */
    {
	GdkPixbuf *icon_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
						 TRUE, 8,
						 icon_width, icon_height);
	guchar *pixels = gdk_pixbuf_get_pixels(icon_pixbuf);
	int rowstride = gdk_pixbuf_get_rowstride(icon_pixbuf);
	int ix, iy;
	for (iy = 0; iy < icon_height; iy++) {
	    for (ix = 0; ix < icon_width; ix++) {
		int byte_idx = iy * ((icon_width + 7) / 8) + ix / 8;
		int bit = (icon_bits[byte_idx] >> (ix % 8)) & 1;
		guchar *p = pixels + iy * rowstride + ix * 4;
		p[0] = bit ? 0 : 255;	/* R */
		p[1] = bit ? 0 : 255;	/* G */
		p[2] = bit ? 0 : 255;	/* B */
		p[3] = 255;		/* A */
	    }
	}
	gtk_window_set_icon(GTK_WINDOW(main_window), icon_pixbuf);
	g_object_unref(icon_pixbuf);
    }

    /* Create sentinel file for remote control */
    sprintf(sentinel, "/tmp/.wave.%d.%d", (int)getuid(), (int)getpid());
    fclose(fopen(sentinel, "w"));

    /* Install SIGUSR1 handler for remote control */
    g_unix_signal_add(SIGUSR1, handle_sigusr1, NULL);

    /* Create View panel and read preferences */
    create_mode_popup();

    if (!show_subtype)
	show_subtype = prefs_get_boolean("View.Subtype", 0);
    if (!show_chan)
	show_chan = prefs_get_boolean("View.Chan", 0);
    if (!show_num)
	show_num = prefs_get_boolean("View.Num", 0);
    if (!show_aux)
	show_aux = prefs_get_boolean("View.Aux", 0);
    if (!show_marker)
	show_marker = prefs_get_boolean("View.Markers", 0);
    if (!show_signame)
	show_signame = prefs_get_boolean("View.SignalNames", 0);
    if (!show_baseline)
	show_baseline = prefs_get_boolean("View.Baselines", 0);
    if (!show_level)
	show_level = prefs_get_boolean("View.Level", 0);
    if (tsa_index < 0) {
	tsa_index = fine_tsa_index =
	    prefs_get_integer("View.TimeScale", DEF_TSA_INDEX);
	coarse_tsa_index =
	    prefs_get_integer("View.CoarseTimeScale", DEF_COARSE_TSA_INDEX);
    }
    if (vsa_index < 0)
	vsa_index = prefs_get_integer("View.AmplitudeScale", DEF_VSA_INDEX);
    if (ann_mode < 0)
	ann_mode = prefs_get_integer("View.AnnotationMode", 0);
    if (overlap < 0)
	overlap = prefs_get_integer("View.AnnotationOverlap", 0);
    if (sig_mode < 0)
	sig_mode = prefs_get_integer("View.SignalMode", 0);
    if (time_mode < 0)
	time_mode = prefs_get_integer("View.TimeMode", 0);
    if (grid_mode < 0) {
	grid_mode = fine_grid_mode =
	    prefs_get_integer("View.GridMode", 0);
	coarse_grid_mode = prefs_get_integer("View.CoarseGridMode", 0);
    }
    mode_undo();

    /* Layout: vbox = [main_panel_box | drawing_area | status_bar] */
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(main_window), vbox);

    /* Main panel (toolbar) — created by mainpan.c */
    main_panel_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(vbox), main_panel_box, FALSE, FALSE, 0);
    create_main_panel();

    /* Drawing area */
    drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(drawing_area, mmx(50), mmy(20));
    gtk_widget_set_hexpand(drawing_area, TRUE);
    gtk_widget_set_vexpand(drawing_area, TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), drawing_area, TRUE, TRUE, 0);

    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(drawing_area, "configure-event",
		     G_CALLBACK(on_configure), NULL);

    /* Enable events on the drawing area */
    gtk_widget_add_events(drawing_area,
			  GDK_BUTTON_PRESS_MASK |
			  GDK_BUTTON_RELEASE_MASK |
			  GDK_POINTER_MOTION_MASK |
			  GDK_KEY_PRESS_MASK |
			  GDK_KEY_RELEASE_MASK |
			  GDK_ENTER_NOTIFY_MASK |
			  GDK_STRUCTURE_MASK);
    gtk_widget_set_can_focus(drawing_area, TRUE);

    /* Connect the event handler from edit.c */
    g_signal_connect(drawing_area, "event",
		     G_CALLBACK(canvas_event_handler), NULL);

    /* Set crosshair cursor once the drawing area is realized */
    g_signal_connect_after(drawing_area, "realize",
	G_CALLBACK(set_crosshair_cursor), NULL);

    /* Status bar (replaces frame footer) */
    status_bar = gtk_statusbar_new();
    gtk_box_pack_end(GTK_BOX(vbox), status_bar, FALSE, FALSE, 0);

    /* Initialize annotation font */
    annfontname = prefs_get_string("SignalWindow.Font", DEFANNFONT);
    ann_font = pango_font_description_from_string(annfontname);
    ann_layout = pango_layout_new(
	gtk_widget_get_pango_context(drawing_area));
    pango_layout_set_font_description(ann_layout, ann_font);

    /* Create the initial offscreen buffer */
    osb = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
				     canvas_width + mmx(10), canvas_height);
    {
	cairo_t *cr = cairo_create(osb);
	wave_set_color(cr, WAVE_COLOR_BACKGROUND);
	cairo_paint(cr);
	cairo_destroy(cr);
    }

    /* Compute initial canvas dimensions via do_resize */
    canvas_height_mv = canvas_height / dmmy(10);
    canvas_width_sec = canvas_width / dmmx(25);

    /* Save scope parameters */
    save_scope_params(0, 1, 0);

    /* Show the window */
    gtk_widget_show_all(main_window);

    return 0;
}

/* Hide/unhide grid — with Cairo we just use a flag + repaint */

static int grid_hidden;

void hide_grid(void)
{
    grid_hidden = 1;
}

void unhide_grid(void)
{
    grid_hidden = 0;
}

int wave_grid_is_hidden(void)
{
    return grid_hidden;
}

void display_and_process_events(void)
{
    in_main_loop = 1;
    gtk_main();
}

void quit_proc(void)
{
    if (post_changes()) {
	finish_log();
	unlink(sentinel);
	gtk_main_quit();
    }
}

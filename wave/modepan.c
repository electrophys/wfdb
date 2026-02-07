/* file: modepan.c	G. Moody        30 April 1990
			Last revised:	2026
Mode panel functions for WAVE (GTK 3 version)

-------------------------------------------------------------------------------
WAVE: Waveform analyzer, viewer, and editor
Copyright (C) 1990-2009 George B. Moody

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

/* GTK widgets for the View popup */
static GtkWidget *mode_window;

/* "Show" check buttons (replaces PANEL_CHOICE with PANEL_CHOOSE_ONE=FALSE) */
static GtkWidget *show_subtype_btn, *show_chan_btn, *show_num_btn,
    *show_aux_btn, *show_marker_btn, *show_signame_btn,
    *show_baseline_btn, *show_level_btn;

/* Combo boxes (replace PANEL_CHOICE_STACK items) */
static GtkWidget *ts_combo, *vs_combo, *sig_combo, *grid_combo,
    *ann_combo, *ov_combo, *tim_combo;

int mode_popup_active = -1;
char *tchoice[] = {"0.25 mm/hour", "1 mm/hour", "5 mm/hour",
    "0.25 mm/min", "1 mm/min", "5 mm/min", "25 mm/min",
    "50 mm/min", "125 mm/min", "250 mm/min", "500 mm/min",
    "12.5 mm/sec", "25 mm/sec", "50 mm/sec", "125 mm/sec", "250 mm/sec",
    "500 mm/sec", "1000 mm/sec", "2000 mm/sec", "5000 mm/sec",
    "10 mm/ms", "20 mm/ms", "50 mm/ms", "100 mm/ms", "200 mm/ms", "500 mm/ms"};
char *vchoice[] = { "1 mm/mV", "2.5 mm/mV", "5 mm/mV", "10 mm/mV", "20 mm/mV",
   "40 mm/mV", "100 mm/mV" };

/* Undo any mode changes: reset widgets to match current global state. */
void mode_undo(void)
{
    if (mode_popup_active < 0) return;

    gtk_combo_box_set_active(GTK_COMBO_BOX(ts_combo), tsa_index);
    gtk_combo_box_set_active(GTK_COMBO_BOX(vs_combo), vsa_index);
    gtk_combo_box_set_active(GTK_COMBO_BOX(sig_combo), sig_mode);
    gtk_combo_box_set_active(GTK_COMBO_BOX(ann_combo), ann_mode);
    gtk_combo_box_set_active(GTK_COMBO_BOX(ov_combo), overlap);
    gtk_combo_box_set_active(GTK_COMBO_BOX(tim_combo), time_mode);
    gtk_combo_box_set_active(GTK_COMBO_BOX(grid_combo), grid_mode);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_subtype_btn),
				 show_subtype & 1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_chan_btn),
				 show_chan & 1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_num_btn),
				 show_num & 1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_aux_btn),
				 show_aux & 1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_marker_btn),
				 show_marker & 1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_signame_btn),
				 show_signame & 1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_baseline_btn),
				 show_baseline & 1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_level_btn),
				 show_level & 1);
}

/* Callback for "Undo changes" button. */
static void undo_clicked(GtkWidget *widget, gpointer data)
{
    mode_undo();
}

/* Callback for "Redraw" button: dismiss window and redraw. */
static void redraw_clicked(GtkWidget *widget, gpointer data)
{
    dismiss_mode();
    disp_proc(".");
}

/* Callback for "Save as new defaults" button. */
static void save_defaults_clicked(GtkWidget *widget, gpointer data)
{
    set_modes();
    save_defaults();
}

/* Prevent the window from being destroyed when closed; hide it instead. */
static gboolean on_delete_event(GtkWidget *widget, GdkEvent *event,
				gpointer data)
{
    dismiss_mode();
    return TRUE;	/* TRUE = do not destroy the window */
}

/* Helper: create a GtkComboBoxText and populate it from a NULL-terminated
   array of strings, setting initial_active as the active item. */
static GtkWidget *make_combo(const char **choices, int n, int initial_active)
{
    GtkWidget *combo = gtk_combo_box_text_new();
    int i;

    for (i = 0; i < n; i++)
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), choices[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), initial_active);
    return combo;
}

/* Set up popup window for adjusting display modes. */
void create_mode_popup(void)
{
    GtkWidget *grid, *label, *btn_box;
    GtkWidget *undo_btn, *redraw_btn, *save_btn;
    int row = 0;

    /* Create the top-level window. */
    mode_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(mode_window), "View");
    gtk_window_set_transient_for(GTK_WINDOW(mode_window),
				 GTK_WINDOW(main_window));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(mode_window), TRUE);
    g_signal_connect(mode_window, "delete-event",
		     G_CALLBACK(on_delete_event), NULL);
    gtk_container_set_border_width(GTK_CONTAINER(mode_window), 8);

    /* Use a GtkGrid for the layout. */
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_container_add(GTK_CONTAINER(mode_window), grid);

    /* --- "Show:" check buttons (replaces multi-select PANEL_CHOICE) --- */
    label = gtk_label_new("Show:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

    {
	/* Pack all check buttons into a vertical box in column 1. */
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

	show_subtype_btn  = gtk_check_button_new_with_label("subtype");
	show_chan_btn      = gtk_check_button_new_with_label("'chan' field");
	show_num_btn      = gtk_check_button_new_with_label("'num' field");
	show_aux_btn      = gtk_check_button_new_with_label("'aux' field");
	show_marker_btn   = gtk_check_button_new_with_label("markers");
	show_signame_btn  = gtk_check_button_new_with_label("signal names");
	show_baseline_btn = gtk_check_button_new_with_label("baselines");
	show_level_btn    = gtk_check_button_new_with_label("level");

	gtk_box_pack_start(GTK_BOX(vbox), show_subtype_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), show_chan_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), show_num_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), show_aux_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), show_marker_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), show_signame_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), show_baseline_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), show_level_btn, FALSE, FALSE, 0);

	gtk_grid_attach(GTK_GRID(grid), vbox, 1, row, 1, 1);
    }
    row++;

    /* --- Time scale --- */
    label = gtk_label_new("Time scale:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    ts_combo = make_combo((const char **)tchoice, 26, DEF_TSA_INDEX);
    gtk_grid_attach(GTK_GRID(grid), ts_combo, 1, row, 1, 1);
    row++;

    /* --- Amplitude scale --- */
    label = gtk_label_new("Amplitude scale:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    vs_combo = make_combo((const char **)vchoice, 7, DEF_VSA_INDEX);
    gtk_grid_attach(GTK_GRID(grid), vs_combo, 1, row, 1, 1);
    row++;

    /* --- Draw (signal display mode) --- */
    label = gtk_label_new("Draw:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    {
	static const char *sig_choices[] = {
	    "all signals", "listed signals only", "valid signals only"
	};
	sig_combo = make_combo(sig_choices, 3, 0);
    }
    gtk_grid_attach(GTK_GRID(grid), sig_combo, 1, row, 1, 1);
    row++;

    /* --- Grid --- */
    label = gtk_label_new("Grid:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    {
	static const char *grid_choices[] = {
	    "None", "0.2 s", "0.5 mV", "0.2 s x 0.5 mV",
	    "0.04 s x 0.1 mV", "1 m x 0.5 mV", "1 m x 0.1 mV"
	};
	grid_combo = make_combo(grid_choices, 7, 0);
    }
    gtk_grid_attach(GTK_GRID(grid), grid_combo, 1, row, 1, 1);
    row++;

    /* --- Show annotations --- */
    label = gtk_label_new("Show annotations:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    {
	static const char *ann_choices[] = {
	    "centered", "attached to signals", "as a signal"
	};
	ann_combo = make_combo(ann_choices, 3, 0);
    }
    gtk_grid_attach(GTK_GRID(grid), ann_combo, 1, row, 1, 1);
    row++;

    /* --- Overlap --- */
    label = gtk_label_new("Overlap:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    {
	static const char *ov_choices[] = {
	    "avoid overlap", "allow overlap"
	};
	ov_combo = make_combo(ov_choices, 2, 0);
    }
    gtk_grid_attach(GTK_GRID(grid), ov_combo, 1, row, 1, 1);
    row++;

    /* --- Time display --- */
    label = gtk_label_new("Time display:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    {
	static const char *tim_choices[] = {
	    "elapsed", "absolute", "in sample intervals"
	};
	tim_combo = make_combo(tim_choices, 3, 0);
    }
    gtk_grid_attach(GTK_GRID(grid), tim_combo, 1, row, 1, 1);
    row++;

    /* --- Action buttons --- */
    btn_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(btn_box), GTK_BUTTONBOX_START);
    gtk_box_set_spacing(GTK_BOX(btn_box), 8);

    undo_btn = gtk_button_new_with_label("Undo changes");
    g_signal_connect(undo_btn, "clicked", G_CALLBACK(undo_clicked), NULL);
    gtk_container_add(GTK_CONTAINER(btn_box), undo_btn);

    redraw_btn = gtk_button_new_with_label("Redraw");
    g_signal_connect(redraw_btn, "clicked", G_CALLBACK(redraw_clicked), NULL);
    gtk_container_add(GTK_CONTAINER(btn_box), redraw_btn);

    save_btn = gtk_button_new_with_label("Save as new defaults");
    g_signal_connect(save_btn, "clicked",
		     G_CALLBACK(save_defaults_clicked), NULL);
    gtk_container_add(GTK_CONTAINER(btn_box), save_btn);

    gtk_grid_attach(GTK_GRID(grid), btn_box, 0, row, 2, 1);

    gtk_widget_show_all(grid);
    mode_popup_active = 0;
}

/* Make the display mode popup window appear. */
void show_mode(void)
{
    if (mode_popup_active < 0) create_mode_popup();
    gtk_widget_show(mode_window);
    gtk_window_present(GTK_WINDOW(mode_window));
    mode_popup_active = 1;
}

void set_modes(void)
{
    int i, otsai = tsa_index, ovsai = vsa_index;
    double osh = canvas_height_mv, osw = canvas_width_sec;
    static double vsa[] = { 1.0, 2.5, 5.0, 10.0, 20.0, 40.0, 100.0 };

    /* If the panel has never been instantiated, there is nothing to do. */
    if (mode_popup_active < 0) return;

    /* Read the current panel settings, beginning with the grid option. */
    switch (grid_mode = gtk_combo_box_get_active(GTK_COMBO_BOX(grid_combo))) {
      case 0: ghflag = gvflag = visible = 0; break;
      case 1: ghflag = 0; gvflag = visible = 1; break;
      case 2: ghflag = visible = 1; gvflag = 0; break;
      case 3: ghflag = gvflag = visible = 1; break;
      case 4: ghflag = gvflag = visible = 2; break;
      case 5: ghflag = visible = 1; gvflag = 3; break;
      case 6: ghflag = visible = 2; gvflag = 3; break;
    }

    /* Next, check the annotation display options.  Each check button
       corresponds to one bit in the original opt_item bitmask. */
    show_subtype  = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(show_subtype_btn)) ? 1 : 0;
    show_chan     = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(show_chan_btn)) ? 1 : 0;
    show_num      = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(show_num_btn)) ? 1 : 0;
    show_aux      = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(show_aux_btn)) ? 1 : 0;
    show_marker   = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(show_marker_btn)) ? 1 : 0;
    show_signame  = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(show_signame_btn)) ? 1 : 0;
    show_baseline = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(show_baseline_btn)) ? 1 : 0;
    show_level    = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(show_level_btn)) ? 1 : 0;

    /* Check the signal display options next. */
    i = sig_mode;
    sig_mode = gtk_combo_box_get_active(GTK_COMBO_BOX(sig_combo));
    if (i != sig_mode || sig_mode == 2)
	set_baselines();

    /* Check the annotation display mode next. */
    ann_mode = gtk_combo_box_get_active(GTK_COMBO_BOX(ann_combo));
    overlap = gtk_combo_box_get_active(GTK_COMBO_BOX(ov_combo));

    /* Check the time display mode next. */
    i = time_mode;
    time_mode = gtk_combo_box_get_active(GTK_COMBO_BOX(tim_combo));
    /* The `nsig > 0' test is a bit of a kludge -- we don't want to reset the
       time_mode before the record has been opened, because we don't know if
       absolute times are available until then. */
    if (nsig > 0 && time_mode == 1)
	(void)wtimstr(0L);	/* check if absolute times are available --
				   if not, time_mode is reset to 0 */
    if (i != time_mode) {
	set_start_time(wtimstr(display_start_time));
	set_end_time(wtimstr(display_start_time + nsamp));
	reset_start();
	reset_stop();
    }

    /* The purpose of the complex method of computing canvas_width_sec is to
       obtain a "rational" value for it even if the frame has been resized.
       First, we determine the number of 5 mm time-grid units in the window
       (note that the resize procedure guarantees that this will be an integer;
       the odd calculation is intended to take care of roundoff error in
       pixel-to-millimeter conversion).  For each scale, the multiplier of u
       is simply the number of seconds that would be represented by 5 mm.
       Since u is usually a multiple of 5 (except on small displays, or if
       the frame has been resized to a small size), the calculated widths in
       seconds are usually integers, at worst expressible as an integral
       number of tenths of seconds. */
    if ((i = gtk_combo_box_get_active(GTK_COMBO_BOX(ts_combo))) >= 0) {
	int u = ((int)(canvas_width/dmmx(1) + 1)/5);	/* number of 5 mm
							   time-grid units */
	switch (tsa_index = i) {
	  case 0:	/* 0.25 mm/hour */
	    mmpersec = (0.25/3600.);
	    canvas_width_sec = 72000 * u; break;
	  case 1:	/* 1 mm/hour */
	    mmpersec = (1./3600.);
	    canvas_width_sec = 18000 * u; break;
	  case 2:	/* 5 mm/hour */
	    mmpersec = (5./3600.);
	    canvas_width_sec = 3600 * u; break;
	  case 3:	/* 0.25 mm/min */
	    mmpersec = (0.25/60.);
	    canvas_width_sec = 1200 * u; break;
	  case 4:	/* 1 mm/min */
	    mmpersec = (1./60.);
	    canvas_width_sec = 300 * u; break;
	  case 5:	/* 5 mm/min */
	    mmpersec = (5./60.);
	    canvas_width_sec = 60 * u; break;
	  case 6:	/* 25 mm/min */
	    mmpersec = (25./60.);
	    canvas_width_sec = 12 * u; break;
	  case 7:	/* 50 mm/min */
	    mmpersec = (50./60.);
	    canvas_width_sec = 6 * u; break;
	  case 8:	/* 125 mm/min */
	    mmpersec = (125./60.);
	    canvas_width_sec = (12 * u) / 5; break;
	  case 9:	/* 250 mm/min */
	    mmpersec = (250./60.);
	    canvas_width_sec = (6 * u) / 5; break;
	  case 10:	/* 500 mm/min */
	    mmpersec = (500./60.);
	    canvas_width_sec = (3 * u) / 5; break;
	  case 11:	/* 12.5 mm/sec */
	    mmpersec = 12.5;
	    canvas_width_sec = (2 * u) / 5; break;
	  case 12:	/* 25 mm/sec */
	    mmpersec = 25.;
	    canvas_width_sec = u / 5; break;
	  case 13:	/* 50 mm/sec */
	    mmpersec = 50.;
	    canvas_width_sec = u / 10; break;
	  case 14:	/* 125 mm/sec */
	    mmpersec = 125.;
	    canvas_width_sec = u / 25; break;
	  case 15:	/* 250 mm/sec */
	    mmpersec = 250.;
	    canvas_width_sec = u / 50.0; break;
	  case 16:	/* 500 mm/sec */
	    mmpersec = 500.;
	    canvas_width_sec = u / 100.0; break;
	  case 17:	/* 1000 mm/sec */
	    mmpersec = 1000.;
	    canvas_width_sec = u / 200.0; break;
	  case 18:	/* 2000 mm/sec */
	    mmpersec = 2000.;
	    canvas_width_sec = u / 400.0; break;
	  case 19:	/* 5000 mm/sec */
	    mmpersec = 5000.;
	    canvas_width_sec = u / 1000.0; break;
	  case 20:	/* 10 mm/ms */
	    mmpersec = 10000.;
	    canvas_width_sec = u / 2000.0; break;
	  case 21:	/* 20 mm/ms */
	    mmpersec = 20000.;
	    canvas_width_sec = u / 4000.0; break;
	  case 22:	/* 50 mm/ms */
	    mmpersec = 50000.;
	    canvas_width_sec = u / 10000.0; break;
	  case 23:	/* 100 mm/ms */
	    mmpersec = 100000.;
	    canvas_width_sec = u / 20000.0; break;
	  case 24:	/* 200 mm/ms */
	    mmpersec = 200000.;
	    canvas_width_sec = u / 40000.0; break;
	  case 25:	/* 500 mm/ms */
	    mmpersec = 500000.;
	    canvas_width_sec = u / 100000.0; break;
	}
    }

    /* Computation of canvas_height_mv could be as complex as above, but
       it doesn't seem so important to obtain a "rational" value here. */
    if ((i = gtk_combo_box_get_active(GTK_COMBO_BOX(vs_combo))) >= 0 && i < 7) {
	mmpermv = vsa[i];
	canvas_height_mv = canvas_height/dmmy(vsa[vsa_index = i]);
    }
    if (osh != canvas_height_mv || osw != canvas_width_sec ||
	otsai != tsa_index || ovsai != vsa_index) {
	vscale[0] = 0.0;
	calibrate();
    }
}

/* Effect any mode changes that were selected and make the popup disappear. */
void dismiss_mode(void)
{
    /* If the panel is currently visible, hide it. */
    if (mode_popup_active > 0) {
	gtk_widget_hide(mode_window);
	mode_popup_active = 0;
    }
    set_modes();
}

/* Time-to-string conversion functions.  These functions use those in the
   WFDB library, but ensure that (1) elapsed times are displayed if time_mode
   is 0, and (2) absolute times (if available) are displayed without brackets
   if time_mode is non-zero. */

WFDB_Time wstrtim(char *s)
{
    WFDB_Time t;

    while (*s == ' ' || *s == '\t') s++;
    if (time_mode == 1 && *s != '[' && *s != 's' && *s != 'c' && *s != 'e') {
	char buf[80];

	sprintf(buf, "[%s]", s);
	s = buf;
    }
    t = strtim(s);
    if (*s == '[') {	/* absolute time specified - strtim returns a negated
			   sample number if s is valid */
	if (t > 0L) t = 0L;	/* invalid s (before sample 0) -- reset t */
	else t = -t;	/* valid s -- convert t to a positive sample number */
    }
    return (t);
}

char *wtimstr(WFDB_Time t)
{
    switch (time_mode) {
      case 0:
      default:
	if (t == 0L) return ("0:00");
	else if (t < 0L) t = -t;
	return (timstr(t));
      case 1:
	{
	    char *p, *q;

	    if (t > 0L) t = -t;
	    p = timstr(t);
	    if (*p == '[') {
		p++;
		q = p + strlen(p) - 1;
		if (*q == ']') *q = '\0';
	    }
	    else {
		time_mode = 0;
		if (tim_combo)
		    gtk_combo_box_set_active(GTK_COMBO_BOX(tim_combo),
					    time_mode);
	    }
	    return (p);
        }
      case 2:
	{
	    static char buf[100];

	    if (t < 0L) t = -t;
	    sprintf(buf, "s%"WFDB_Pd_TIME, t);
	    return (buf);
	}
    }
}

char *wmstimstr(WFDB_Time t)
{
    switch (time_mode) {
      case 0:
      default:
	if (t == 0L) return ("0:00");
	else if (t < 0L) t = -t;
	return (mstimstr(t));
      case 1:
	{
	    char *p, *q;

	    if (t > 0L) t = -t;
	    p = mstimstr(t);
	    if (*p == '[') {
		p++;
		q = p + strlen(p) - 1;
		if (*q == ']') *q = '\0';
	    }
	    else {
		time_mode = 0;
		if (tim_combo)
		    gtk_combo_box_set_active(GTK_COMBO_BOX(tim_combo),
					    time_mode);
	    }
	    return (p);
	}
      case 2:
	{
	    static char buf[100];

	    if (t < 0L) t = -t;
	    sprintf(buf, "s%"WFDB_Pd_TIME, t);
	    return (buf);
	}
    }
}

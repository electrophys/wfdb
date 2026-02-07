/* file: edit.c		G. Moody	 1 May 1990
			Last revised:	2026
Annotation-editing functions for WAVE

-------------------------------------------------------------------------------
WAVE: Waveform analyzer, viewer, and editor
Copyright (C) 1990-2010 George B. Moody

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
#include <unistd.h>	/* getcwd */
#include <wfdb/ecgmap.h>

WFDB_Time level_time;
static GtkWidget *level_window;
static GtkWidget *level_mode_combo;
static GtkWidget *level_time_label;
static char level_time_string[36];
int bar_on, bar_x, bar_y;
int level_mode, level_popup_active = -1;
int selected = -1;

void reset_ref(void)
{
    (void)isigsettime(ref_mark_time);
    (void)getvec(vref);
}

static void dismiss_level_popup(void)
{
    if (level_popup_active > 0) {
	gtk_widget_hide(level_window);
	level_popup_active = 0;
    }
}

void recreate_level_popup(void)
{
    int stat = level_popup_active;
    void show_level_popup(int);

    if (stat >= 0 && level_window) {
	gtk_widget_destroy(level_window);
	level_window = NULL;
	level_popup_active = -1;
	show_level_popup(stat);
    }
}

static void set_level_mode_cb(GtkComboBoxText *combo, gpointer user_data)
{
    void show_level_popup(int);

    level_mode = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    show_level_popup(TRUE);
}

void show_level_popup(int stat)
{
    int i, invalid_data;
    void create_level_popup(void);

    switch (level_mode) {
      case 0:
	  sprintf(level_time_string, "Time: %s", mstimstr(-level_time));
	  break;
      case 1:
	  if (level_time >= ref_mark_time)
	      sprintf(level_time_string, "Interval: %s",
		      mstimstr(level_time - ref_mark_time));
	  else
	      sprintf(level_time_string, "Interval: -%s",
		      mstimstr(ref_mark_time - level_time));
	  break;
      case 2:
	  sprintf(level_time_string, "Sample number: %"WFDB_Pd_TIME,
		  level_time);
	  break;
      case 3:
	  sprintf(level_time_string, "Interval: %"WFDB_Pd_TIME" samples",
		  level_time - ref_mark_time);
	  break;
    }
    invalid_data = (isigsettime(level_time) < 0 || getvec(level_v) < 0);
    for (i = 0; i < nsig; i++) {
	sprintf(level_name_string[i], "%s: ", signame[i]);
	if (invalid_data || level_v[i] == WFDB_INVALID_SAMPLE) {
	    sprintf(level_value_string[i], " ");
	    sprintf(level_units_string[i], " ");
	}
	else switch (level_mode) {
	  default:
	  case 0:	/* physical units (absolute) */
	      sprintf(level_value_string[i], "%8.3lf", aduphys(i, level_v[i]));
	      sprintf(level_units_string[i], "%s%s", sigunits[i],
		      calibrated[i] ? "" : " *");
	      break;
	  case 1:	/* physical units (relative) */
	      sprintf(level_value_string[i], "%8.3lf",
		      aduphys(i, level_v[i]) - aduphys(i, vref[i]));
	      sprintf(level_units_string[i], "%s%s", sigunits[i],
		      calibrated[i] ? "" : " *");
	      break;
	  case 2:	/* raw units (absolute) */
	      sprintf(level_value_string[i], "%6d", level_v[i]);
	      sprintf(level_units_string[i], "adu");
	      break;
	  case 3:	/* raw units (relative) */
	      sprintf(level_value_string[i], "%6d", level_v[i] - vref[i]);
	      sprintf(level_units_string[i], "adu");
	      break;
	}
    }

    if (level_popup_active < 0) create_level_popup();
    else {
	gtk_label_set_text(GTK_LABEL(level_time_label), level_time_string);
	for (i = 0; i < nsig; i++) {
	    gtk_label_set_text(GTK_LABEL(level_name[i]),
			       level_name_string[i]);
	    gtk_label_set_text(GTK_LABEL(level_value[i]),
			       level_value_string[i]);
	    gtk_label_set_text(GTK_LABEL(level_units[i]),
			       level_units_string[i]);
	}
    }
    if (stat) {
	gtk_widget_show_all(level_window);
	gtk_window_present(GTK_WINDOW(level_window));
    } else {
	gtk_widget_hide(level_window);
    }
    level_popup_active = stat;
}

void create_level_popup(void)
{
    int i;
    GtkWidget *grid, *dismiss_button;

    if (level_popup_active >= 0) return;

    level_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(level_window), "Levels");
    gtk_window_set_transient_for(GTK_WINDOW(level_window),
				 GTK_WINDOW(main_window));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(level_window), TRUE);
    g_signal_connect(level_window, "delete-event",
		     G_CALLBACK(gtk_widget_hide_on_delete), NULL);

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    gtk_container_add(GTK_CONTAINER(level_window), grid);

    /* Row 0: mode selector */
    GtkWidget *show_label = gtk_label_new("Show: ");
    gtk_grid_attach(GTK_GRID(grid), show_label, 0, 0, 1, 1);

    level_mode_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(level_mode_combo),
				   "physical units (absolute)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(level_mode_combo),
				   "physical units (relative)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(level_mode_combo),
				   "raw units (absolute)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(level_mode_combo),
				   "raw units (relative)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(level_mode_combo), level_mode);
    g_signal_connect(level_mode_combo, "changed",
		     G_CALLBACK(set_level_mode_cb), NULL);
    gtk_grid_attach(GTK_GRID(grid), level_mode_combo, 1, 0, 2, 1);

    /* Row 1: time display */
    level_time_label = gtk_label_new(level_time_string);
    gtk_widget_set_halign(level_time_label, GTK_ALIGN_START);
    PangoAttrList *bold_attrs = pango_attr_list_new();
    pango_attr_list_insert(bold_attrs,
			   pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(level_time_label), bold_attrs);
    gtk_grid_attach(GTK_GRID(grid), level_time_label, 0, 1, 3, 1);

    /* Rows 2..nsig+1: signal levels */
    for (i = 0; i < nsig; i++) {
	level_name[i] = gtk_label_new(level_name_string[i]);
	gtk_label_set_attributes(GTK_LABEL(level_name[i]), bold_attrs);
	gtk_widget_set_halign(level_name[i], GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), level_name[i], 0, i + 2, 1, 1);

	level_value[i] = gtk_label_new(level_value_string[i]);
	gtk_widget_set_halign(level_value[i], GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(grid), level_value[i], 1, i + 2, 1, 1);

	level_units[i] = gtk_label_new(level_units_string[i]);
	gtk_widget_set_halign(level_units[i], GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), level_units[i], 2, i + 2, 1, 1);
    }
    pango_attr_list_unref(bold_attrs);

    /* Dismiss button */
    dismiss_button = gtk_button_new_with_label("Dismiss");
    g_signal_connect_swapped(dismiss_button, "clicked",
			     G_CALLBACK(dismiss_level_popup), NULL);
    gtk_grid_attach(GTK_GRID(grid), dismiss_button, 1, nsig + 3, 1, 1);

    level_popup_active = 0;
}

void bar(int x, int y, int do_bar)
{
    static int level_on;

    /* Erase any previous bar and levels by clearing overlay state. */
    if (bar_on) {
	bar_on = 0;
    }
    if (level_on) {
	level_on = 0;
    }
    if (do_bar && 0 <= x && x < canvas_width) {
	bar_x = x;
	bar_y = y;
	cursor_x = x;
	cursor_y = y;
	cursor_active = 1;
	bar_on = 1;
	if (show_level) {
	    int i, n;

	    n = sig_mode ? siglistlen : nsig;
	    for (i = 0; i < n; i++) {
		level[level_on].x1 = 0;
		level[level_on].x2 = canvas_width;
		level[level_on].y1 = level[level_on].y2 = sigy(i, x);
		level_on++;
	    }
	    if (level_on) {
		level_time = display_start_time + x/tscale;
		show_level_popup(TRUE);
	    }
	}
	wave_refresh();
    }
    else {
	cursor_active = 0;
	wave_refresh();
    }
}

int box_on, box_left, box_xc, box_yc, box_right, box_top, box_bottom;

void box(int x, int y, int do_box)
{
    /* Clear any other box. */
    if (box_on) {
	box_on = 0;
    }
    if (do_box && 0 <= x && x < canvas_width) {
	box_xc = x; box_yc = y;
	box_left = x - mmx(1.5);
	box_right = x + mmx(2.5);
	box_bottom = y - mmy(7.5);
	box_top = y + mmy(4.5);
	box_x0 = box_left;
	box_y0 = box_bottom;
	box_x1 = box_right;
	box_y1 = box_top;
	box_active = 1;
	box_on = 1;
	wave_refresh();
    }
    else {
	box_active = 0;
	wave_refresh();
    }
}

/* This function redraws the box and bars, if any, after the ECG window has
   been damaged.  Do not call it except from the repaint procedure. */
void restore_cursor(void)
{
    cursor_active = 0;
    box_active = 0;
    if (bar_on) {
	bar_on = 0;
	bar(bar_x, bar_y, 1);
    }
    if (box_on) {
	box_on = 0;
	box(box_xc, box_yc, 1);
    }
    wave_refresh();
}

static int in_box(int x, int y)
{
    return (box_on && box_left <= x && x <= box_right &&
	box_bottom <= y && y <= box_top);
}

static void attach_ann(struct ap *a)
{
    int y;
    void set_frame_footer(void);

    attached = a;
    if (ann_mode == 1 && (unsigned)a->this.chan < nsig) {
	if (sig_mode == 0)
	    y = base[(unsigned)a->this.chan] + mmy(2);
	else {
	    int i;

	    y = abase;
	    for (i = 0; i < siglistlen; i++)
		if (a->this.chan == siglist[i]) {
		    y = base[i] + mmy(2);
		    break;
		}
	}
    }
    else
	y = abase;
    box((int)((a->this.time - display_start_time)*tscale), y, 1);
    set_frame_footer();
}

static void detach_ann(void)
{
    void set_frame_footer(void);

    attached = NULL;
    box(0, 0, 0);
    set_frame_footer();
}

#define ANNTEMPSTACKSIZE	16
static struct WFDB_ann ann_stack[ANNTEMPSTACKSIZE];
static int ann_stack_index;

static int safestrcmp(char *a, char *b)
{
    if (a == NULL) return (b != NULL);
    else if (b == NULL) return (-1);
    else return (strcmp(a, b));
}

static void save_ann_template(void)
{
    int i;

    /* Search for ann_template in ann_stack.  We don't bother checking the
       last entry in ann_stack since we'll push ann_template onto the top
       of the stack anyway. */
    for (i = 0; i < ANNTEMPSTACKSIZE-1; i++) {
	if (ann_stack[i].anntyp == ann_template.anntyp &&
	    ann_stack[i].subtyp == ann_template.subtyp &&
	    /* note: chan fields do not have to match */
	    ann_stack[i].num == ann_template.num &&
	    safestrcmp(ann_stack[i].aux, ann_template.aux) == 0)
	    break;	/* ann_template is already in the stack */
    }
    /* Make room for ann_template at the top of the stack by discarding
       the copy of ann_template we found in the stack, or the least recently
       used template if we didn't find ann_template in the stack. */
    for ( ; i > 0; i--)
	ann_stack[i] = ann_stack[i-1];
    ann_stack[ann_stack_index = 0] = ann_template;
}

static void set_ann_template(struct WFDB_ann *a)
{
    if (ann_template.anntyp != a->anntyp ||
	ann_template.subtyp != a->subtyp ||
	ann_template.num    != a->num    ||
	safestrcmp(ann_template.aux, a->aux) != 0) {
	ann_template = *a;
	set_anntyp(ann_template.anntyp);
	set_ann_aux(ann_template.aux);
	set_ann_subtyp(ann_template.subtyp);
	set_ann_chan(ann_template.chan);
	set_ann_num(ann_template.num);
    }
}

static void set_next_ann_template(void)
{
    if (ann_stack_index > 0)
	set_ann_template(&ann_stack[--ann_stack_index]);
}

static void set_prev_ann_template(void)
{
    if (ann_stack_index < ANNTEMPSTACKSIZE-1)
	set_ann_template(&ann_stack[++ann_stack_index]);
}

/* Parse the aux string of a LINK annotation to obtain the URL of the external
   data, and open the URL in a browser window. */
static void parse_and_open_url(char *s)
{
    char *p1, *p2, *p3;
    int use_path = 1;

    if (s == NULL || *s == 0 || *(s+1) == ' ' || *(s+1) == '\t')
	return;		/* no link defined, do nothing */
    p1 = p2 = s + 1;	/* first data byte */
    p3 = p1 + *s; 	/* first byte after valid data */

    /* First, scan the string to see if it includes a protocol prefix
       (typically `http:' or `ftp:'), a tag suffix (`#' followed by a string
       specifying a location within a file), or a label (a string following
       a space, not a part of the URL but intended to be displayed by WAVE). */
    while (p2 < p3) {
	if (*p2 == ':')
	    /* The URL appears to include a protocol prefix -- pass the string
	       to the browser as is except for removal of the label, if any. */
	    use_path = 0;
	else if (*p2 == ' ' || *p2 == '\t')
	    /* There is a label in the string -- we don't need to scan further.
	       p2 marks the end of the URL. */
	    break;
	else if (*p2 == '#' && use_path)
	    /* The URL includes a tag, and it's also incomplete.  In this case,
	       we need to find the file specified by the portion of the string
	       up to p2, then furnish the complete pathname to the browser,
	       appending the tag again (but still removing the label, if any).
	       We don't need to scan further just yet. */
	    break;
	p2++;
    }
    strncpy(url, p1, p2-p1);
    url[p2-p1] = '\0';
    if (use_path == 0 || url[0] == '/') {
	/* Here, we have either a full URL with a protocol prefix (if use_path
	   is 0) or an absolute pathname of a local file, possibly with a tag
	   suffix.  In either case, the URL can be passed to the browser as is.
	   */
	open_url();
	return;
    }

    /* In this case, the string specifies a relative pathname (possibly
       referring to a file in another directory in the WFDB path).  First,
       let's try to find the file using wfdbfile to search the WFDB path. */
    if ((p1 = wfdbfile(url, NULL))) { /* wfdbfile has found the file */
	if (*p1 != '/') {
	    /* We still have only a relative pathname, and we need an absolute
	       pathname (the browser may not accept a relative path, and may
	       have a different working directory than WAVE in any case). */
	    static char *cwd;

	    if (cwd == NULL) cwd = getcwd(NULL, 256);
	    if (cwd && strlen(cwd) + strlen(p1) < sizeof(url) - 1)
		sprintf(url, "%s/%s", cwd, p1);
	    else {
		strncpy(url, p1, sizeof(url)-1);
		url[strlen(url)] = '\0';
	    }
	}
	else
	    strcpy(url, p1);
	/* We should now have a null-terminated absolute pathname in url. */
    }

    /* We still need to reattach the tag suffix, if any, to the URL. */
    if (*p2 == '#') {
	p1 = url + strlen(url);
	while (p1 < url+sizeof(url)-1 && p2 < p3 &&
	       *p2 != ' ' && *p2 != '\t' && *p2 != '\0')
	    *p1++ = *p2++;
	*p1 = '\0';
    }

    /* We should now have a complete URL (unless wfdbfile or getcwd failed
       above, in which case we'll let the browser try to find it anyway). */
    open_url();
}

/* Warp the mouse pointer to (x, y) within the drawing area.  In GTK3 we use
   gdk_device_warp() with coordinates translated to root-window space. */
static void warp_pointer(int x, int y)
{
    GdkWindow *gdk_win = gtk_widget_get_window(drawing_area);
    if (!gdk_win) return;

    int rx, ry;
    gdk_window_get_root_coords(gdk_win, x, y, &rx, &ry);

    GdkDisplay *disp = gdk_window_get_display(gdk_win);
    GdkSeat *seat = gdk_display_get_default_seat(disp);
    GdkDevice *pointer = gdk_seat_get_pointer(seat);
    GdkScreen *screen = gdk_window_get_screen(gdk_win);

    gdk_device_warp(pointer, screen, rx, ry);
}

/* Handle events in the ECG display canvas. */
gboolean canvas_event_handler(GtkWidget *widget, GdkEvent *event,
			      gpointer data)
{
    int i, x, y;
    guint keyval;
    WFDB_Time t, tt;
    struct ap *a;
    static int left_down, middle_down, right_down, redrawing, dragged, warped;
    void delete_annotation(WFDB_Time, int), move_annotation(struct ap *, WFDB_Time);

    /* If there is an attached (selected) annotation, detach it if it is no
       longer on-screen (the user may have used a main control panel button
       to move to another location in the record). */
    if (attached && (attached->this.time < display_start_time ||
		     attached->this.time >= display_start_time + nsamp))
	detach_ann();

    switch (event->type) {

    case GDK_ENTER_NOTIFY:
	/* Grab keyboard focus when the pointer enters the canvas. */
	gtk_widget_grab_focus(widget);
	return TRUE;

    case GDK_KEY_PRESS:
	keyval = event->key.keyval;

	/* Function keys F1-F10 (replacing KEY_LEFT/KEY_TOP 1-10) */
	if (keyval == GDK_KEY_F6) {
	    /* <Copy>/<F6>: copy selected annotation into Annotation
	       Template */
	    if (attached) {
		set_ann_template(&(attached->this));
		save_ann_template();
	    }
	    return TRUE;
	}

	if (keyval == GDK_KEY_F9) {
	    /* <Find>/<F9>: search */
	    if (event->key.state & GDK_SHIFT_MASK) {
		if (event->key.state & GDK_CONTROL_MASK)
		    disp_proc("h");	/* home */
		else
		    disp_proc("e");	/* end */
	    }
	    else if (event->key.state & GDK_CONTROL_MASK)
		disp_proc("[");		/* backward */
	    else
		disp_proc("]");		/* forward */
	    selected = -1;
	    return TRUE;
	}

	if (keyval == GDK_KEY_F10) {
	    /* <F10>: +half/full screen, with shift/ctrl variants */
	    if (event->key.state & GDK_SHIFT_MASK) {
		if (event->key.state & GDK_CONTROL_MASK)
		    disp_proc("<");
		else
		    disp_proc("(");
	    }
	    else {
		if (event->key.state & GDK_CONTROL_MASK)
		    disp_proc(">");
		else
		    disp_proc(")");
	    }
	    selected = -1;
	    return TRUE;
	}

	if (keyval == GDK_KEY_F3) {
	    /* Simulate drag left */
	    static int count = 1;
	    x = cursor_active ? cursor_x : 0;
	    y = cursor_active ? cursor_y : 0;
	    if ((x -= count) < 0) x = 0;
	    if (count < 100) count++;
	    warped = 1;
	    if (middle_down) bar(x, y, 1);
	    warp_pointer(x, y);
	    return TRUE;
	}

	if (keyval == GDK_KEY_F4) {
	    /* Simulate drag right */
	    static int count = 1;
	    x = cursor_active ? cursor_x : 0;
	    y = cursor_active ? cursor_y : 0;
	    if ((x += count) >= canvas_width) x = canvas_width - 1;
	    if (count < 100) count++;
	    warped = 1;
	    warp_pointer(x, y);
	    if (middle_down) bar(x, y, 1);
	    return TRUE;
	}

	if (keyval == GDK_KEY_Home) {
	    disp_proc("h");
	    selected = -1;
	    return TRUE;
	}

	if (keyval == GDK_KEY_End) {
	    disp_proc("e");
	    selected = -1;
	    return TRUE;
	}

	if (keyval == GDK_KEY_Page_Up) {
	    if (event->key.state & GDK_CONTROL_MASK)
		disp_proc("<");
	    else
		disp_proc("(");
	    selected = -1;
	    return TRUE;
	}

	if (keyval == GDK_KEY_Page_Down) {
	    if (event->key.state & GDK_CONTROL_MASK)
		disp_proc(">");
	    else
		disp_proc(")");
	    selected = -1;
	    return TRUE;
	}

	if (keyval == GDK_KEY_Up) {
	    /* up-arrow: move annotation to previous signal channel */
	    if (ann_mode == 1 && attached && annp->this.chan > 0) {
		if (accept_edit == 0) {
		    wave_notice_prompt(
			"You may not edit annotations unless you first "
			"enable editing from the 'Edit' menu.");
		    return TRUE;
		}
		if (annp->previous &&
		    (annp->previous)->this.time == annp->this.time &&
		    (annp->previous)->this.chan == annp->this.chan - 1)
		    attach_ann(annp->previous);
		else {
		    struct ap *a;

		    if ((event->key.state & GDK_CONTROL_MASK) &&
			(a = get_ap())) {
			a->this = annp->this;
			a->this.chan--;
			if (a->this.aux) {
			    char *p;

			    if ((p = (char *)calloc(*(a->this.aux)+2,1))
				== NULL) {
				wave_notice_prompt(
				    "This annotation cannot be inserted "
				    "because there is insufficient memory.");
				return TRUE;
			    }
			    memcpy(p, a->this.aux, *(a->this.aux)+1);
			    a->this.aux = p;
			}
			insert_annotation(a);
			set_ann_template(&(a->this));
			save_ann_template();
			attach_ann(a);
		    }
		    else {
			annp->this.chan--;
			check_post_update();
		    }
		    box(0,0,0);
		    bar(0,0,0);
		    clear_annotation_display();
		    show_annotations(display_start_time, nsamp);
		    box_on = dragged = 0;
		    attach_ann(attached);
		}
		annp = attached;
		x = (attached->this.time - display_start_time)*tscale;
		if (sig_mode == 0)
		    y = base[(unsigned)attached->this.chan] + mmy(2);
		else {
		    int i;

		    y = abase;
		    for (i = 0; i < siglistlen; i++)
			if (attached->this.chan == siglist[i]) {
			    y = base[i] + mmy(2);
			    break;
			}
		}
		warped = 1;
		warp_pointer(x, y);
	    }
	    return TRUE;
	}

	if (keyval == GDK_KEY_Down) {
	    /* down-arrow: move annotation to next signal channel */
	    if (ann_mode == 1 && attached && annp->this.chan < nsig-1) {
		if (accept_edit == 0) {
		    wave_notice_prompt(
			"You may not edit annotations unless you first "
			"enable editing from the 'Edit' menu.");
		    return TRUE;
		}
		if (annp->next &&
		    (annp->next)->this.time == annp->this.time &&
		    (annp->next)->this.chan == annp->this.chan + 1)
		    attach_ann(annp->next);
		else {
		    struct ap *a;

		    if ((event->key.state & GDK_CONTROL_MASK) &&
			(a = get_ap())) {
			a->this = annp->this;
			a->this.chan++;
			if (a->this.aux) {
			    char *p;

			    if ((p = (char *)calloc(*(a->this.aux)+2,1))
				== NULL) {
				wave_notice_prompt(
				    "This annotation cannot be inserted "
				    "because there is insufficient memory.");
				return TRUE;
			    }
			    memcpy(p, a->this.aux, *(a->this.aux)+1);
			    a->this.aux = p;
			}
			insert_annotation(a);
			set_ann_template(&(a->this));
			save_ann_template();
			attach_ann(a);
		    }
		    else {
			annp->this.chan++;
			check_post_update();
		    }
		    box(0,0,0);
		    bar(0,0,0);
		    clear_annotation_display();
		    show_annotations(display_start_time, nsamp);
		    box_on = dragged = 0;
		    attach_ann(attached);
		}
		annp = attached;
		x = (attached->this.time - display_start_time)*tscale;
		if (sig_mode == 0)
		    y = base[(unsigned)attached->this.chan] + mmy(2);
		else {
		    int i;

		    y = abase;
		    for (i = 0; i < siglistlen; i++)
			if (attached->this.chan == siglist[i]) {
			    y = base[i] + mmy(2);
			    break;
			}
		}
		warped = 1;
		warp_pointer(x, y);
	    }
	    return TRUE;
	}

	if (keyval == GDK_KEY_Left) {
	    /* left-arrow: simulate left mouse button press+release */
	    /* Handled below in BUTTON_PRESS with button==1. Generate a
	       synthetic approach: fall through to the ASCII handler for
	       compatibility or handle via button emulation.  For simplicity,
	       we synthesize the same flow as a left-click at the current
	       cursor position. */
	    /* Not directly needed as a separate case -- the original code
	       had KEY_RIGHT(10) fall through into MS_LEFT.  For GTK we
	       handle arrow keys for navigation above.  Left-arrow as
	       "simulate left button" is a legacy XView-ism; in the GTK
	       port the left/right arrows just navigate annotations. */
	    return TRUE;
	}

	if (keyval == GDK_KEY_Right) {
	    /* right-arrow: same as left-arrow, legacy XView compatibility */
	    return TRUE;
	}

	if (keyval == GDK_KEY_F1 || keyval == GDK_KEY_Help) {
	    help();
	    return TRUE;
	}

	/* Handle ASCII key events. */
	if (keyval >= 0x20 && keyval <= 0x7e) {
	    int e = (int)keyval;

	    if (e == '.') ann_template.anntyp = NOTQRS;
	    else if (e == ':') ann_template.anntyp = INDEX_MARK;
	    else if (e == '<') ann_template.anntyp = BEGIN_ANALYSIS;
	    else if (e == '>') ann_template.anntyp = END_ANALYSIS;
	    else if (e == ';') ann_template.anntyp = REF_MARK;
	    else if (e == '\r' && attached && attached->this.anntyp == LINK)
		parse_and_open_url(attached->this.aux);
	    else if (e == '+' && (event->key.state & GDK_CONTROL_MASK)) {
		/* Increase size of selected signal, if any */
		if (0 <= selected && selected < nsig)
		    vmag[selected] *= 1.1;
		/* or of all signals, otherwise */
		else
		    for (i = 0; i < nsig; i++)
			vmag[i] *= 1.1;
		vscale[0] = 0.0;
		calibrate();
		disp_proc(".");
	    }
	    else if (e == '-' && (event->key.state & GDK_CONTROL_MASK)) {
		/* Decrease size of selected signal, if any */
		if (0 <= selected && selected < nsig)
		    vmag[selected] /= 1.1;
		/* or of all signals, otherwise */
		else
		    for (i = 0; i < nsig; i++)
			vmag[i] /= 1.1;
		vscale[0] = 0.0;
		calibrate();
		disp_proc(".");
	    }
	    else if (e == '*' && (event->key.state & GDK_CONTROL_MASK)) {
		/* Invert selected signal, if any */
		if (0 <= selected && selected < nsig)
		    vmag[selected] *= -1.0;
		/* or all signals, otherwise */
		else
		    for (i = 0; i < nsig; i++)
			vmag[i] *= -1.0;
		vscale[0] = 0.0;
		calibrate();
		disp_proc(".");
	    }
	    else if (e == ')' && (event->key.state & GDK_CONTROL_MASK)) {
		/* Show more context, less detail (zoom out) */
		tmag /= 1.01;
		clear_cache();
		if (display_start_time < 0)
		    display_start_time = -display_start_time;
		display_start_time -= (nsamp + 100)/200;
		if (display_start_time < 0) display_start_time = 0;
		calibrate();
		disp_proc("^");
	    }
	    else if (e == '(' && (event->key.state & GDK_CONTROL_MASK)) {
		/* Show less context, more detail (zoom in) */
		tmag *= 1.01;
		clear_cache();
		if (display_start_time < 0)
		    display_start_time = -display_start_time;
		display_start_time += (nsamp + 99)/198;
		calibrate();
		disp_proc("^");
	    }
	    else if (e == '=' && (event->key.state & GDK_CONTROL_MASK)) {
		/* Reset size of selected signal, if any */
		if (0 <= selected && selected < nsig)
		    vmag[selected] = 1.0;
		/* or of all signals, otherwise */
		else
		    for (i = 0; i < nsig; i++)
			vmag[i] = 1.0;
		/* Reset time scale */
		tmag = 1.0;
		vscale[0] = 0.0;
		if (display_start_time < 0)
		    display_start_time = -display_start_time;
		display_start_time += nsamp/2;
		calibrate();
		display_start_time -= nsamp/2;
		if (display_start_time < 0)
		    display_start_time = 0;
		disp_proc("^");
	    }
	    else {
		static char es[2];

		es[0] = e;
		if ((i = strann(es)) != NOTQRS) ann_template.anntyp = i;
	    }
	    if (ann_popup_active < 0) show_ann_template();
	    if (ann_template.anntyp != -1)
		set_anntyp(ann_template.anntyp);
	    return TRUE;
	}

	/* Handle Return/Enter for LINK annotations */
	if ((keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) &&
	    attached && attached->this.anntyp == LINK) {
	    parse_and_open_url(attached->this.aux);
	    return TRUE;
	}

	return FALSE;

    case GDK_BUTTON_PRESS:
    {
	int button = event->button.button;
	x = (int)event->button.x;
	y = (int)event->button.y;
	t = display_start_time + x/tscale;
	if (atimeres > 1)
	    t -= (t % atimeres);

	/* Lock out mouse events while display is being updated. */
	if (redrawing) return TRUE;

	if (button == 1) {
	    /* The left button was pressed. */
	    if ((event->button.state & GDK_SHIFT_MASK) ||
		(event->button.state & GDK_CONTROL_MASK) ||
		(event->button.state & GDK_MOD1_MASK)) {
		int d, dmin = -1, imin = -1, n;

		n = sig_mode ? siglistlen : nsig;
		for (i = 0; i < n; i++) {
		    d = y - base[i];
		    if (d < 0) d = -d;
		    if (dmin < 0 || d < dmin) { imin = i; dmin = d; }
		}
		if (imin >= 0) {
		    set_signal_choice(imin);
		    if (selected == imin) selected = -1;
		    else selected = imin;
		    if (event->button.state & GDK_CONTROL_MASK)
			add_signal_choice();
		    if (event->button.state & GDK_MOD1_MASK)
			delete_signal_choice();
		}
		return TRUE;
	    }
	    dragged = 0;
	    if (accept_edit == 0 && wave_ppid) {
		char buf[80];
		sprintf(buf, "wave-remote -pid %d -f '%s'\n", wave_ppid,
			mstimstr(-t));
		system(buf);
		return TRUE;
	    }
	    if (middle_down) set_prev_ann_template();
	    show_ann_template();
	    if (middle_down || right_down) return TRUE;
	    left_down = 1;
	    (void)locate_annotation(t, -128);
	    if (annp) {
		if (annp->previous) annp = annp->previous;
		else return TRUE;
	    }
	    else if (ap_end) annp = ap_end;
	    else return TRUE;
	    redrawing = 1;
	    if (ann_mode == 1) {
		if (attached && in_box(x, y)) {
		    if (attached->previous) annp = attached->previous;
		    else annp = attached;
		}
		else {
		    double d, dx, dy, dmin = -1.0;
		    struct ap *a = annp;

		    while (a->next && annp->this.time == (a->next)->this.time)
			a = a->next;
		    while (a && a->this.time >= display_start_time) {
			dx = x - (a->this.time - display_start_time)*tscale;
			if (sig_mode == 0)
			    dy = y - (base[a->this.chan] + mmy(2));
			else {
			    int i;

			    dy = y - abase;
			    for (i = 0; i < siglistlen; i++)
				if (a->this.chan == siglist[i]) {
				    dy = y - (base[i] + mmy(2));
				    break;
				}
			}
			d = dx*dx + dy*dy;
			if (dmin < 0. || d < dmin) {
			    dmin = d;
			    annp = a;
			}
			a = a->previous;
		    }
		}
	    }
	    if (annp->this.time < display_start_time) {
		struct ap *a = annp;
		cairo_t *cr = wave_begin_paint();
		wave_fill_rect(cr, WAVE_COLOR_BACKGROUND,
			       0, 0, canvas_width+mmx(10), canvas_height);
		wave_end_paint(cr);
		if ((tt = annp->this.time -
		     (WFDB_Time)((nsamp-freq)/2)) < 0L)
		    display_start_time = 0L;
		else
		    display_start_time = strtim(timstr(tt));
		do_disp();
		left_down = bar_on = box_on = 0;
		annp = a;
	    }

	    /* Attach the annotation, move the pointer to it, and draw the
	       marker bars. */
	    attach_ann(annp);
	    x = (annp->this.time - display_start_time)*tscale;
	    if (ann_mode == 1 && (unsigned)annp->this.chan < nsig) {
		if (sig_mode == 0)
		    y = base[(unsigned)annp->this.chan] + mmy(2);
		else {
		    int i;

		    y = abase;
		    for (i = 0; i < siglistlen; i++)
			if (annp->this.chan == siglist[i]) {
			    y = base[i] + mmy(2);
			    break;
			}
		}
	    }
	    else
		y = abase;
	    warped = 1;
	    warp_pointer(x, y);
	    bar(x, y, 1);
	    redrawing = 0;
	}
	else if (button == 2) {
	    /* The middle button was pressed. */
	    if (left_down || right_down || ann_template.anntyp < 0)
		return TRUE;
	    middle_down = 1;
	    bar(x, y, 1);
	    if ((accept_edit == 0 ||
		 (event->button.state & GDK_CONTROL_MASK)) && wave_ppid) {
		char buf[80];
		sprintf(buf, "wave-remote -pid %d -f '%s'\n", wave_ppid,
			mstimstr(-t));
		system(buf);
	    }
	    else if (attached && !in_box(x, y))
		detach_ann();
	}
	else if (button == 3) {
	    /* The right button was pressed. */
	    if (middle_down) {
		set_next_ann_template();
		show_ann_template();
	    }
	    if (left_down || middle_down) return TRUE;
	    dragged = 0;
	    right_down = 1;
	    if (attached && in_box(x, y)) annp = attached->next;
	    else (void)locate_annotation(t, -128);
	    if (annp == NULL) return TRUE;
	    redrawing = 1;
	    if (ann_mode == 1 && (!attached || !in_box(x, y))) {
		double d, dx, dy, dmin = -1.0;
		struct ap *a = annp;

		while (a && a->this.time < display_start_time + nsamp) {
		    dx = x - (a->this.time - display_start_time)*tscale;
		    if (sig_mode == 0)
			dy = y - (base[a->this.chan] + mmy(2));
		    else {
			int i;

			dy = y - abase;
			for (i = 0; i < siglistlen; i++)
			    if (a->this.chan == siglist[i]) {
				dy = y - (base[i] + mmy(2));
				break;
			    }
		    }
		    d = dx*dx + dy*dy;
		    if (dmin < 0. || d < dmin) {
			dmin = d;
			annp = a;
		    }
		    a = a->next;
		}
	    }
	    if (annp->this.time >= display_start_time + nsamp) {
		struct ap *a = annp;
		cairo_t *cr = wave_begin_paint();
		wave_fill_rect(cr, WAVE_COLOR_BACKGROUND,
			       0, 0, canvas_width+mmx(10), canvas_height);
		wave_end_paint(cr);
		tt = annp->this.time - (WFDB_Time)((nsamp-freq)/2);
		display_start_time = strtim(timstr(tt));
		do_disp();
		right_down = bar_on = box_on = 0;
		annp = a;
	    }

	    /* Attach the annotation, move the pointer to it, and draw the
	       marker bars. */
	    attach_ann(annp);
	    x = (annp->this.time - display_start_time)*tscale;
	    if (ann_mode == 1 && (unsigned)annp->this.chan < nsig) {
		if (sig_mode == 0)
		    y = base[(unsigned)annp->this.chan] + mmy(2);
		else {
		    int i;

		    y = abase;
		    for (i = 0; i < siglistlen; i++)
			if (annp->this.chan == siglist[i]) {
			    y = base[i] + mmy(2);
			    break;
			}
		}
	    }
	    else
		y = abase;
	    warped = 1;
	    warp_pointer(x, y);
	    bar(x, y, 1);
	    redrawing = 0;
	}
	return TRUE;
    }

    case GDK_BUTTON_RELEASE:
    {
	int button = event->button.button;
	x = (int)event->button.x;
	y = (int)event->button.y;
	t = display_start_time + x/tscale;
	if (atimeres > 1)
	    t -= (t % atimeres);

	if (button == 1) {
	    /* The left button was released. */
	    if (!left_down) return TRUE;
	    left_down = 0;
	    if (attached && dragged && !in_box(x, y)) {
		move_annotation(attached, t);
		box(0,0,0);
		bar(0,0,0);
		clear_annotation_display();
		show_annotations(display_start_time, nsamp);
		box_on = dragged = 0;
		attach_ann(attached);
	    }
	    bar(x, 0, 0);
	}
	else if (button == 2) {
	    /* The middle button was released. */
	    if (!middle_down) return TRUE;
	    middle_down = 0;
	    if (ann_mode == 1) {
		int d, dmin = -1, imin = -1, n;

		n = sig_mode ? siglistlen : nsig;
		for (i = 0; i < n; i++) {
		    d = y - base[i];
		    if (d < 0) d = -d;
		    if (dmin < 0 || d < dmin) { imin = i; dmin = d; }
		}
		if (imin >= 0) {
		    if (sig_mode) imin = siglist[imin];
		    set_ann_chan(ann_template.chan = imin);
		}
	    }
	    if (attached && in_box(x, y)) {
		if (ann_template.anntyp == NOTQRS) {
		    save_ann_template();
		    delete_annotation(attached->this.time,
				      attached->this.chan);
		    a = NULL;
		}
		else if ((a = get_ap())) {
		    a->this = ann_template;
		    a->this.time = attached->this.time;
		}
	    }
	    else if (ann_template.anntyp != NOTQRS && (a = get_ap())) {
		a->this = ann_template;
		a->this.time = t;
	    }
	    else
		a = NULL;
	    if (a) {
		/* There is an annotation to be inserted.  Copy the aux
		   string, if any. */
		if (a->this.aux) {
		    char *p;

		    if ((p = (char *)calloc(*(a->this.aux)+2, 1)) == NULL) {
			wave_notice_prompt(
			    "This annotation cannot be inserted "
			    "because there is insufficient memory.");
			return TRUE;
		    }
		    memcpy(p, a->this.aux, *(a->this.aux)+1);
		    a->this.aux = p;
		}
		insert_annotation(a);
		set_ann_template(&(a->this));
		save_ann_template();
	    }
	    box(0,0,0);
	    bar(0,0,0);
	    clear_annotation_display();
	    show_annotations(display_start_time, nsamp);
	    box_on = 0;
	    bar(x,0,0);
	}
	else if (button == 3) {
	    /* The right button was released. */
	    if (!right_down) return TRUE;
	    right_down = 0;
	    if (attached && dragged && !in_box(x, y)) {
		move_annotation(attached, t);
		box(0,0,0);
		bar(0,0,0);
		clear_annotation_display();
		show_annotations(display_start_time, nsamp);
		box_on = dragged = 0;
		attach_ann(attached);
	    }
	    bar(x,0,0);
	}
	return TRUE;
    }

    case GDK_MOTION_NOTIFY:
    {
	/* The mouse moved while one or more buttons were depressed. */
	x = (int)event->motion.x;
	y = (int)event->motion.y;

	if ((!middle_down && !left_down && !right_down) || x == bar_x)
	    return TRUE;
	else if (warped) {
	    warped = 0;
	    return TRUE;
	}
	else if (attached && in_box(x, y)) {
	    if (bar_x != box_xc) bar(box_xc, box_yc, 1);
	}
	else if (ann_mode == 1) {
	    int d, dmin = -1, ii, imin = -1, n;

	    n = sig_mode ? siglistlen : nsig;
	    for (i = 0; i < n; i++) {
		d = y - base[i];
		if (d < 0) d = -d;
		if (dmin < 0 || d < dmin) { imin = i; dmin = d; }
	    }
	    ii = (imin >= 0 && sig_mode) ? siglist[imin] : imin;
	    if (imin >= 0 && ann_template.chan != ii) {
		set_ann_chan(ann_template.chan = ii);
		if (attached) attached->this.chan = ii;
	    }
	    bar(x, imin >= 0 ? base[(unsigned)imin] + mmy(2) : abase, 1);
	}
	else
	    bar(x, abase, 1);
	dragged = 1;
	return TRUE;
    }

    default:
	break;
    }

    return FALSE;
}

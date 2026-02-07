/* file: mainpan.c	G. Moody	30 April 1990
			Last revised:	2026
Functions for the main control panel of WAVE

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
#include <wfdb/ecgcodes.h>
#include "gtkwave.h"
#include <sys/time.h>
#include <unistd.h>

/* GtkEntry widgets for the main panel and sub-dialogs */
static GtkWidget *record_item;
static GtkWidget *annot_item;
static GtkWidget *time_item;
static GtkWidget *time2_item;
static GtkWidget *find_item;
static GtkWidget *findsig_item;

/* Sub-dialog widgets */
static GtkWidget *wfdbpath_item;
static GtkWidget *wfdbcal_item;
static GtkWidget *psprint_item;
static GtkWidget *textprint_item;

/* Sub-dialog windows */
static GtkWidget *load_window;
static GtkWidget *print_setup_window;
static GtkWidget *find_window;

char wfdbpath[512];	/* database path */
char wfdbcal[128];	/* WFDB calibration file name */

/* File viewer state */
static char filename[40], *title;

/* Forward declarations */
static void show_file(void);
static void wait_for_file(void);

/* --- WFDB path and calibration callbacks --- */

static void wfdbp_proc(GtkEntry *entry, gpointer data)
{
    const char *p = gtk_entry_get_text(GTK_ENTRY(wfdbpath_item));
    (void)entry; (void)data;

    if (p != wfdbpath)
	strncpy(wfdbpath, p, sizeof(wfdbpath) - 1);
    setwfdb(wfdbpath);
}

static void wfdbc_proc(GtkEntry *entry, gpointer data)
{
    const char *p = gtk_entry_get_text(GTK_ENTRY(wfdbcal_item));
    (void)entry; (void)data;

    if (strcmp(wfdbcal, p)) {
	strncpy(wfdbcal, p, sizeof(wfdbcal) - 1);
	if (calopen(cfname = wfdbcal) == 0)
	    calibrate();
    }
}

/* --- Load dialog --- */

static void reload_clicked(GtkButton *button, gpointer data)
{
    (void)button; (void)data;
    disp_proc(".");
}

static void create_load_window(void)
{
    GtkWidget *grid, *label, *button;
    char *p;

    strncpy(wfdbpath, getwfdb(), sizeof(wfdbpath) - 1);
    if (cfname != wfdbcal) {
	if (cfname != NULL)
	    strncpy(wfdbcal, cfname, sizeof(wfdbcal) - 1);
	else if ((p = getenv("WFDBCAL")))
	    strncpy(wfdbcal, p, sizeof(wfdbcal) - 1);
    }

    load_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(load_window), "File: Load");
    gtk_window_set_transient_for(GTK_WINDOW(load_window),
				 GTK_WINDOW(main_window));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(load_window), TRUE);
    g_signal_connect(load_window, "delete-event",
		     G_CALLBACK(gtk_widget_hide_on_delete), NULL);

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 4);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 6);
    gtk_container_add(GTK_CONTAINER(load_window), grid);

    /* Row 0: Record */
    label = gtk_label_new("Record:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    record_item = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(record_item), 32);
    gtk_entry_set_text(GTK_ENTRY(record_item), record);
    g_signal_connect(record_item, "activate",
		     G_CALLBACK(reload_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid), record_item, 1, 0, 1, 1);

    /* Row 0: Annotator */
    label = gtk_label_new("Annotator:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 2, 0, 1, 1);
    annot_item = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(annot_item), 8);
    gtk_entry_set_text(GTK_ENTRY(annot_item), annotator);
    g_signal_connect(annot_item, "activate",
		     G_CALLBACK(reload_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid), annot_item, 3, 0, 1, 1);

    /* Row 0: Reload button */
    button = gtk_button_new_with_label("Reload");
    g_signal_connect(button, "clicked", G_CALLBACK(reload_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 4, 0, 1, 1);

    /* Row 0: Calibration file */
    label = gtk_label_new("Calibration file:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 5, 0, 1, 1);
    wfdbcal_item = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(wfdbcal_item), 15);
    gtk_entry_set_text(GTK_ENTRY(wfdbcal_item), wfdbcal);
    g_signal_connect(wfdbcal_item, "activate", G_CALLBACK(wfdbc_proc), NULL);
    gtk_grid_attach(GTK_GRID(grid), wfdbcal_item, 6, 0, 1, 1);

    /* Row 1: WFDB Path (spans full width) */
    label = gtk_label_new("WFDB Path:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);
    wfdbpath_item = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(wfdbpath_item), 60);
    gtk_entry_set_text(GTK_ENTRY(wfdbpath_item), wfdbpath);
    g_signal_connect(wfdbpath_item, "activate", G_CALLBACK(wfdbp_proc), NULL);
    gtk_grid_attach(GTK_GRID(grid), wfdbpath_item, 1, 1, 6, 1);

    gtk_widget_show_all(grid);
}

static void show_load(GtkMenuItem *item, gpointer data)
{
    (void)item; (void)data;
    gtk_widget_show(load_window);
    gtk_window_present(GTK_WINDOW(load_window));
}

/* --- Print setup dialog --- */

static void print_setup_apply(GtkEntry *entry, gpointer data)
{
    (void)entry; (void)data;
    strncpy(psprint, gtk_entry_get_text(GTK_ENTRY(psprint_item)),
	    sizeof(psprint) - 1);
    strncpy(textprint, gtk_entry_get_text(GTK_ENTRY(textprint_item)),
	    sizeof(textprint) - 1);
}

static void create_print_setup_window(void)
{
    GtkWidget *grid, *label;
    char *p, *printer;

    if ((p = getenv("TEXTPRINT")) == NULL || strlen(p) > sizeof(textprint) - 1) {
	if ((printer = getenv("PRINTER")) == NULL)
	    sprintf(textprint, "lpr");
	else
	    sprintf(textprint, "lpr -P%s", printer);
    } else
	strcpy(textprint, p);

    if ((p = getenv("PSPRINT")) == NULL || strlen(p) > sizeof(psprint) - 1) {
	if ((printer = getenv("PRINTER")) == NULL)
	    sprintf(psprint, "lpr");
	else
	    sprintf(psprint, "lpr -P%s", printer);
    } else
	strcpy(psprint, p);

    print_setup_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(print_setup_window), "Print setup");
    gtk_window_set_transient_for(GTK_WINDOW(print_setup_window),
				 GTK_WINDOW(main_window));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(print_setup_window), TRUE);
    g_signal_connect(print_setup_window, "delete-event",
		     G_CALLBACK(gtk_widget_hide_on_delete), NULL);

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 4);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 6);
    gtk_container_add(GTK_CONTAINER(print_setup_window), grid);

    label = gtk_label_new("PostScript print command:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    psprint_item = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(psprint_item), 16);
    gtk_entry_set_text(GTK_ENTRY(psprint_item), psprint);
    g_signal_connect(psprint_item, "activate",
		     G_CALLBACK(print_setup_apply), NULL);
    gtk_grid_attach(GTK_GRID(grid), psprint_item, 1, 0, 1, 1);

    label = gtk_label_new("Text print command:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);
    textprint_item = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(textprint_item), 16);
    gtk_entry_set_text(GTK_ENTRY(textprint_item), textprint);
    g_signal_connect(textprint_item, "activate",
		     G_CALLBACK(print_setup_apply), NULL);
    gtk_grid_attach(GTK_GRID(grid), textprint_item, 1, 1, 1, 1);

    gtk_widget_show_all(grid);
}

static void show_print_setup(GtkMenuItem *item, gpointer data)
{
    (void)item; (void)data;
    gtk_widget_show(print_setup_window);
    gtk_window_present(GTK_WINDOW(print_setup_window));
}

/* --- Save callback --- */

static void save_clicked(GtkMenuItem *item, gpointer data)
{
    (void)item; (void)data;
    if (post_changes())
	set_frame_title();
}

/* --- File viewer (replaces Textsw) --- */

static void show_print(GtkButton *button, gpointer data)
{
    char print_command[128];
    (void)button; (void)data;

    if (*filename) {
	if (strncmp(filename, "/tmp/wave-s", 11) == 0)
	    sprintf(print_command, "wfdbdesc $RECORD | %s\n", textprint);
	else if (strncmp(filename, "/tmp/wave-a", 11) == 0)
	    sprintf(print_command, "sumann -r $RECORD -a $ANNOTATOR | %s\n",
		    textprint);
	else
	    sprintf(print_command, "%s <%s\n", textprint, filename);
	do_command(print_command);
    }
}

static void show_file(void)
{
    GtkWidget *window, *vbox, *scrolled, *textview, *button;
    GtkTextBuffer *buffer;
    FILE *fp;
    char line[256];

    fp = fopen(filename, "r");
    if (!fp) {
	GtkWidget *dialog = gtk_message_dialog_new(
	    GTK_WINDOW(main_window),
	    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	    GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
	    "Sorry, no property information\nis available for this topic.");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return;
    }

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_window_set_transient_for(GTK_WINDOW(window),
				 GTK_WINDOW(main_window));
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    button = gtk_button_new_with_label("Print");
    g_signal_connect(button, "clicked", G_CALLBACK(show_print), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 2);

    scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(textview), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(textview), TRUE);
    gtk_container_add(GTK_CONTAINER(scrolled), textview);

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    while (fgets(line, sizeof(line), fp))
	gtk_text_buffer_insert_at_cursor(buffer, line, -1);
    fclose(fp);

    gtk_widget_show_all(window);
    gtk_window_present(GTK_WINDOW(window));
}

/* Timer callback: check whether the command output file is ready.
   Once it contains readable data, wait one more tick, then display it. */
static gboolean check_file(gpointer data)
{
    char tstring[20];
    FILE *tfile;
    static int file_ready;
    (void)data;

    if (file_ready) {
	show_file();
	unlink(filename);
	file_ready = 0;
	return G_SOURCE_REMOVE;		/* stop the timer */
    }
    if ((tfile = fopen(filename, "r")) == NULL)
	return G_SOURCE_CONTINUE;
    if (fgets(tstring, 20, tfile) == NULL) {
	fclose(tfile);
	return G_SOURCE_CONTINUE;
    }
    fclose(tfile);
    file_ready++;
    return G_SOURCE_CONTINUE;
}

static void wait_for_file(void)
{
    g_timeout_add(1000, check_file, NULL);
}

static char command[80];

/* --- Properties menu callbacks --- */

static void prop_signals(GtkMenuItem *item, gpointer data)
{
    (void)item; (void)data;
    sprintf(filename, "/tmp/wave-s.XXXXXX");
    mkstemp(filename);
    sprintf(command, "(wfdbdesc $RECORD; echo =====) >%s\n", filename);
    do_command(command);
    title = "Signals";
    wait_for_file();
}

static void prop_annotations(GtkMenuItem *item, gpointer data)
{
    (void)item; (void)data;
    post_changes();
    sprintf(filename, "/tmp/wave-a.XXXXXX");
    mkstemp(filename);
    sprintf(command, "(sumann -r $RECORD -a $ANNOTATOR; echo =====) >%s\n",
	    filename);
    do_command(command);
    title = "Annotations";
    wait_for_file();
}

static void prop_wave(GtkMenuItem *item, gpointer data)
{
    (void)item; (void)data;
    sprintf(filename, "%s/wave/news.hlp", helpdir);
    title = "About WAVE";
    show_file();
}

/* --- Find dialog --- */

static void find_entry_activated(GtkEntry *entry, gpointer data)
{
    (void)entry; (void)data;
    disp_proc("]");
}

static void findsig_entry_activated(GtkEntry *entry, gpointer data)
{
    (void)entry; (void)data;
    disp_proc("}");
}

static void create_find_window(void)
{
    GtkWidget *grid, *label, *button;

    find_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(find_window), "Find");
    gtk_window_set_transient_for(GTK_WINDOW(find_window),
				 GTK_WINDOW(main_window));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(find_window), TRUE);
    g_signal_connect(find_window, "delete-event",
		     G_CALLBACK(gtk_widget_hide_on_delete), NULL);

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 4);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 6);
    gtk_container_add(GTK_CONTAINER(find_window), grid);

    /* Row 0: Start time */
    label = gtk_label_new("Start time:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    time_item = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(time_item), 15);
    gtk_entry_set_text(GTK_ENTRY(time_item), "0");
    g_signal_connect(time_item, "activate",
		     G_CALLBACK(reload_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid), time_item, 1, 0, 1, 1);

    /* Row 0: End time */
    label = gtk_label_new("End time:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 2, 0, 1, 1);
    time2_item = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(time2_item), 15);
    gtk_entry_set_text(GTK_ENTRY(time2_item), "10");
    gtk_grid_attach(GTK_GRID(grid), time2_item, 3, 0, 1, 1);

    /* Row 1: Search for annotation */
    label = gtk_label_new("Search for annotation:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);
    find_item = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(find_item), 6);
    g_signal_connect(find_item, "activate",
		     G_CALLBACK(find_entry_activated), NULL);
    gtk_grid_attach(GTK_GRID(grid), find_item, 1, 1, 1, 1);

    /* Row 1: More options button */
    button = gtk_button_new_with_label("More options...");
    g_signal_connect(button, "clicked",
		     G_CALLBACK(show_search_template), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 2, 1, 1, 1);

    /* Row 1: Find signal */
    label = gtk_label_new("Find signal:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 2, 1, 1);
    findsig_item = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(findsig_item), 6);
    g_signal_connect(findsig_item, "activate",
		     G_CALLBACK(findsig_entry_activated), NULL);
    gtk_grid_attach(GTK_GRID(grid), findsig_item, 1, 2, 1, 1);

    gtk_widget_show_all(grid);
}

static void show_find_window(GtkMenuItem *item, gpointer data)
{
    (void)item; (void)data;
    gtk_widget_show(find_window);
    gtk_window_present(GTK_WINDOW(find_window));
}

/* --- Edit menu callbacks --- */

static void allow_editing(GtkMenuItem *item, gpointer data)
{
    (void)item; (void)data;
    accept_edit = 1;
}

static void view_only(GtkMenuItem *item, gpointer data)
{
    (void)item; (void)data;
    accept_edit = 0;
}

/* --- Noise mnemonic helper --- */

/* This function converts a noise mnemonic string into a noise subtype.  It
   returns -2 if the input string is not a noise mnemonic string. */
static int noise_strsub(char *s)
{
    int i, imax, n = 0;

    if (('0' <= *s && *s <= '9') || strcmp(s, "-1") == 0)
	return (atoi(s));
    else if (strcmp(s, "U") == 0)
	return (-1);
    imax = (nsig <= 4) ? nsig : 4;
    if ((int)strlen(s) != imax)
	return (-2);
    for (i = 0; i < imax; i++) {
	if (s[i] == 'c')
	    continue;		/* signal i is clean */
	else if (s[i] == 'n')
	    n |= (1 << i);	/* signal i is noisy */
	else if (s[i] == 'u')
	    n |= (0x11 << i);	/* signal i is unreadable */
	else
	    return (-2);	/* the string is not a noise mnemonic */
    }
    return (n);
}

/* --- Navigation button callbacks --- */

static void nav_button_clicked(GtkButton *button, gpointer data)
{
    const char *action = (const char *)data;
    (void)button;
    disp_proc(action);
}

static void show_view_clicked(GtkButton *button, gpointer data)
{
    (void)button; (void)data;
    show_mode();
}

static void show_help_clicked(GtkButton *button, gpointer data)
{
    (void)button; (void)data;
    show_help();
}

static void quit_button_clicked(GtkButton *button, gpointer data)
{
    (void)button; (void)data;
    quit_proc();
}

static void sync_button_clicked(GtkButton *button, gpointer data)
{
    (void)button; (void)data;
    sync_other_wave_processes();
}

/* --- Reinitialize --- */

static int reload_signals, reload_annotations;

void reinitialize(void)
{
    reload_annotations = reload_signals = 1;
    disp_proc(".");
}

/* --- Main dispatch: handle a display request --- */

void disp_proc(const char *action)
{
    int etype, i;
    WFDB_Time cache_time;
    void set_frame_footer(void);

    /* Default action character */
    etype = action ? action[0] : '.';

    /* Reset display modes if necessary. */
    set_modes();

    /* If a new record has been selected, re-initialize. */
    if (reload_signals ||
	strncmp(record, gtk_entry_get_text(GTK_ENTRY(record_item)), RNLMAX)) {
	wfdbquit();

	/* Reclaim memory previously allocated for baseline labels, if any. */
	for (i = 0; i < nsig; i++)
	    if (blabel[i]) {
		free(blabel[i]);
		blabel[i] = NULL;
	    }

	if (!record_init((char *)gtk_entry_get_text(GTK_ENTRY(record_item))))
	    return;
	annotator[0] = '\0';	/* force re-initialization of annotator if
				   record was changed */
	savebackup = 1;
    }

    /* If a new annotator has been selected, re-initialize. */
    if (reload_annotations ||
	strncmp(annotator,
		gtk_entry_get_text(GTK_ENTRY(annot_item)), ANLMAX)) {
	strncpy(annotator,
		gtk_entry_get_text(GTK_ENTRY(annot_item)), ANLMAX);
	if (annotator[0]) {
	    af.name = annotator; af.stat = WFDB_READ;
	    nann = 1;
	} else
	    nann = 0;
	annot_init();
	savebackup = 1;
    }

    reload_signals = reload_annotations = 0;

    /* Find out which action was requested, and act on it. */
    switch (etype) {
      default:
      case '.':	/* Start at time specified on panel. */
      case '*':	/* (from scope_proc(), see scope.c) */
      case '!':	/* (from show_next_entry(), see logpan.c) */
	display_start_time =
	    wstrtim((char *)gtk_entry_get_text(GTK_ENTRY(time_item)));
	if (display_start_time < 0L) display_start_time = -display_start_time;
	cache_time = -1L;
	break;
      case '^':	/* Start at display_start_time. */
	cache_time = -1L;
	break;
      case ':':	/* End at time specified on panel. */
	display_start_time =
	    wstrtim((char *)gtk_entry_get_text(GTK_ENTRY(time2_item)));
	if (display_start_time < 0L) display_start_time = -display_start_time;
	if ((display_start_time -= nsamp) < 0L) display_start_time = 0L;
	cache_time = -1L;
	break;
      case 'h':	/* Go to the beginning of the record (see edit.c) */
	display_start_time = 0L;
	cache_time = -1L;
	break;
      case 'e':	/* Go to the end of the record (see edit.c) */
	if ((display_start_time = strtim("e") - nsamp) < 0L)
	    display_start_time = 0L;
	cache_time = -1L;
	break;
      case '}': /* Find next occurrence of specified signal. */
	if (1) {
	    const char *fp = gtk_entry_get_text(GTK_ENTRY(findsig_item));

	    if ((i = findsig(fp)) >= 0) {
		WFDB_Time tnext = tnextvec(i, display_start_time + nsamp);

		if (tnext >= 0L) {
		    display_start_time = tnext;
		    cache_time = -1L;
		    break;
		} else {
		    GtkWidget *dialog = gtk_message_dialog_new(
			GTK_WINDOW(main_window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
			"No match found!");
		    gtk_dialog_run(GTK_DIALOG(dialog));
		    gtk_widget_destroy(dialog);
		    break;
		}
	    }
	}
	break;
      case ']':	/* Find next occurrence of specified annotation. */
      case '[':	/* Find previous occurrence of specified annotation. */
	if (annotator[0]) {
	    char *fp = (char *)gtk_entry_get_text(GTK_ENTRY(find_item));
	    static char auxstr[256];
	    int mask, noise_mask, target;
	    WFDB_Time t;
	    struct WFDB_ann template;

	    if (*fp == '\0') {
		/* if find_item is empty, use the search template */
		template = search_template;
		mask = search_mask;
		if (template.aux == NULL) mask &= ~M_AUX;
	    } else if ((target = strann(fp))) {
		template.anntyp = target;
		mask = M_ANNTYP;
	    } else if ((noise_mask = noise_strsub(fp)) >= -1) {
		template.anntyp = NOISE;
		template.subtyp = noise_mask;
		mask = M_ANNTYP | M_SUBTYP;
	    } else if (strcmp(fp, ".") == 0) {
		template.anntyp = NOTQRS;
		mask = M_ANNTYP;
	    } else if (strcmp(fp, ":") == 0) {
		template.anntyp = INDEX_MARK;
		mask = M_ANNTYP;
	    } else if (strcmp(fp, "<") == 0) {
		template.anntyp = BEGIN_ANALYSIS;
		mask = M_ANNTYP;
	    } else if (strcmp(fp, ">") == 0) {
		template.anntyp = END_ANALYSIS;
		mask = M_ANNTYP;
	    } else if (strcmp(fp, "*n") == 0) {
		template.anntyp = NORMAL;
		mask = M_MAP2;
	    } else if (strcmp(fp, "*s") == 0) {
		template.anntyp = SVPB;
		mask = M_MAP2;
	    } else if (strcmp(fp, "*v") == 0) {
		template.anntyp = PVC;
		mask = M_MAP2;
	    } else if (strcmp(fp, "*") == 0 || *fp == '\0')
		mask = 0;	/* match annotation of any type */
	    else {
		strncpy(auxstr + 1, fp, 255);
		auxstr[0] = strlen(auxstr + 1);
		template.aux = auxstr;
		mask = M_AUX;
	    }
	    if ((etype == ']' && (t = next_match(&template, mask)) < 0L) ||
		(etype == '[' && (t = previous_match(&template, mask)) < 0L)) {
		GtkWidget *dialog = gtk_message_dialog_new(
		    GTK_WINDOW(main_window),
		    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		    GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
		    "No match found!");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	    } else {
		WFDB_Time halfwindow = (nsamp - freq) / 2;
		display_start_time = strtim(timstr(t - halfwindow));
		if (etype == ']') t = next_match(&template, mask);
		else t = previous_match(&template, mask);
		if (t > 0) cache_time = strtim(timstr(t - halfwindow));
		else cache_time = -1L;
	    }
	}
	break;
      case '<':	/* Go backwards one frame. */
	if ((display_start_time -= nsamp) < 0) display_start_time = 0;
	cache_time = display_start_time - nsamp;
	break;
      case '(':	/* Go backwards one-half frame. */
	if ((display_start_time -= nsamp / 2) < 0) display_start_time = 0;
	cache_time = display_start_time - nsamp / 2;
	break;
      case ')':	/* Go forwards one-half frame. */
	display_start_time += nsamp / 2;
	cache_time = display_start_time + nsamp / 2;
	break;
      case '>':	/* Go forwards one frame. */
	display_start_time += nsamp;
	cache_time = display_start_time + nsamp;
	break;
    }

    if (etype != '!' && description[0]) {
	description[0] = '\0';
	set_frame_title();
    }

    /* Clear the offscreen buffer. */
    bar(0, 0, 0);
    box(0, 0, 0);
    {
	cairo_t *cr = wave_begin_paint();
	wave_fill_rect(cr, WAVE_COLOR_BACKGROUND,
		       0, 0, canvas_width + mmx(10), canvas_height);
	wave_end_paint(cr);
    }

    /* Reset the pointer for the scope display unless the scope is running or
       has just been paused. */
    if (!scan_active && etype != '*') {
	(void)locate_annotation(display_start_time, -128);
	scope_annp = annp;
    }

    /* Display the selected data. */
    do_disp();
    set_frame_footer();

    /* Read ahead if possible. */
    if (cache_time >= 0) (void)find_display_list(cache_time);
}

/* --- Build the main panel (packed into main_panel_box) --- */

void create_main_panel(void)
{
    GtkWidget *menubar, *menu, *menuitem;
    GtkWidget *button, *label;

    /* Create sub-dialogs first (they create the entry widgets we need) */
    create_load_window();
    create_print_setup_window();
    create_find_window();

    /* --- Menu bar --- */
    menubar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(main_panel_box), menubar, FALSE, FALSE, 0);

    /* File menu */
    menu = gtk_menu_new();
    menuitem = gtk_menu_item_new_with_label("Load...");
    g_signal_connect(menuitem, "activate", G_CALLBACK(show_load), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label("Save");
    g_signal_connect(menuitem, "activate", G_CALLBACK(save_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label("Print");
    g_signal_connect(menuitem, "activate",
		     G_CALLBACK(print_proc), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label("Print setup...");
    g_signal_connect(menuitem, "activate",
		     G_CALLBACK(show_print_setup), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label("Analyze...");
    g_signal_connect(menuitem, "activate",
		     G_CALLBACK(analyze_proc), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label("Log...");
    g_signal_connect(menuitem, "activate",
		     G_CALLBACK(show_log), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label("File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menuitem);

    /* Edit menu */
    menu = gtk_menu_new();
    menuitem = gtk_menu_item_new_with_label("Allow editing");
    g_signal_connect(menuitem, "activate",
		     G_CALLBACK(allow_editing), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label("View only");
    g_signal_connect(menuitem, "activate",
		     G_CALLBACK(view_only), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label("Edit");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menuitem);

    /* Properties menu */
    menu = gtk_menu_new();
    menuitem = gtk_menu_item_new_with_label("Signals...");
    g_signal_connect(menuitem, "activate",
		     G_CALLBACK(prop_signals), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label("Annotations...");
    g_signal_connect(menuitem, "activate",
		     G_CALLBACK(prop_annotations), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label("About WAVE...");
    g_signal_connect(menuitem, "activate",
		     G_CALLBACK(prop_wave), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label("Properties");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menuitem);

    /* --- View button --- */
    button = gtk_button_new_with_label("View...");
    gtk_widget_set_tooltip_text(button, "Display options");
    g_signal_connect(button, "clicked",
		     G_CALLBACK(show_view_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(main_panel_box), button, FALSE, FALSE, 0);

    /* --- Navigation buttons --- */
    button = gtk_button_new_with_label("\342\237\250 Search");
    gtk_widget_set_tooltip_text(button, "Search backward for annotation");
    g_signal_connect(button, "clicked",
		     G_CALLBACK(nav_button_clicked), (gpointer)"[");
    gtk_box_pack_start(GTK_BOX(main_panel_box), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("<<");
    gtk_widget_set_tooltip_text(button, "Back one full screen");
    g_signal_connect(button, "clicked",
		     G_CALLBACK(nav_button_clicked), (gpointer)"<");
    gtk_box_pack_start(GTK_BOX(main_panel_box), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("<");
    gtk_widget_set_tooltip_text(button, "Back half a screen");
    g_signal_connect(button, "clicked",
		     G_CALLBACK(nav_button_clicked), (gpointer)"(");
    gtk_box_pack_start(GTK_BOX(main_panel_box), button, FALSE, FALSE, 0);

    /* Find button */
    button = gtk_button_new_with_label("Find...");
    gtk_widget_set_tooltip_text(button, "Open find dialog");
    g_signal_connect(button, "clicked",
		     G_CALLBACK(show_find_window), NULL);
    gtk_box_pack_start(GTK_BOX(main_panel_box), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label(">");
    gtk_widget_set_tooltip_text(button, "Forward half a screen");
    g_signal_connect(button, "clicked",
		     G_CALLBACK(nav_button_clicked), (gpointer)")");
    gtk_box_pack_start(GTK_BOX(main_panel_box), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label(">>");
    gtk_widget_set_tooltip_text(button, "Forward one full screen");
    g_signal_connect(button, "clicked",
		     G_CALLBACK(nav_button_clicked), (gpointer)">");
    gtk_box_pack_start(GTK_BOX(main_panel_box), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("Search \342\237\251");
    gtk_widget_set_tooltip_text(button, "Search forward for annotation");
    g_signal_connect(button, "clicked",
		     G_CALLBACK(nav_button_clicked), (gpointer)"]");
    gtk_box_pack_start(GTK_BOX(main_panel_box), button, FALSE, FALSE, 0);

    /* --- Help and Quit buttons --- */
    button = gtk_button_new_with_label("Help");
    gtk_widget_set_tooltip_text(button, "Open help");
    g_signal_connect(button, "clicked",
		     G_CALLBACK(show_help_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(main_panel_box), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("Quit");
    gtk_widget_set_tooltip_text(button, "Quit WAVE");
    g_signal_connect(button, "clicked",
		     G_CALLBACK(quit_button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(main_panel_box), button, FALSE, FALSE, 0);

    if (make_sync_button) {
	button = gtk_button_new_with_label("Sync");
	gtk_widget_set_tooltip_text(button, "Sync other WAVE processes");
	g_signal_connect(button, "clicked",
			 G_CALLBACK(sync_button_clicked), NULL);
	gtk_box_pack_start(GTK_BOX(main_panel_box), button, FALSE, FALSE, 0);
    }
}

/* --- Setter functions for entry widgets --- */

void set_record_item(char *s)
{
    gtk_entry_set_text(GTK_ENTRY(record_item), s);
}

void set_annot_item(char *s)
{
    gtk_entry_set_text(GTK_ENTRY(annot_item), s);
}

void set_start_time(char *s)
{
    gtk_entry_set_text(GTK_ENTRY(time_item), s);
}

void set_end_time(char *s)
{
    gtk_entry_set_text(GTK_ENTRY(time2_item), s);
}

void set_find_item(char *s)
{
    gtk_entry_set_text(GTK_ENTRY(find_item), s);
    gtk_entry_set_text(GTK_ENTRY(findsig_item), s);
}

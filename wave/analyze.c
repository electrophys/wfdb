/* file: analyze.c	G. Moody	10 August 1990
			Last revised:	2026
Functions for the analysis panel of WAVE

-------------------------------------------------------------------------------
WAVE: Waveform analyzer, viewer, and editor
Copyright (C) 2001 George B. Moody

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
#include <vte/vte.h>
#include <stdio.h>
#include <unistd.h>

#ifndef MENUDIR
#define MENUDIR		"/usr/local/lib"
#endif

#define MENUFILE	"wavemenu.def"	/* name of the default menu file (to
					   be found in MENUDIR) */
#define MAXLL		1024		/* maximum length of a line in the menu
					   file including continuation lines */

void do_analysis(GtkWidget *widget, gpointer data);
void edit_menu_file(void);
void set_signal(GtkWidget *widget, gpointer data);
void set_start_from_entry(GtkWidget *widget, gpointer data);
void set_stop_from_entry(GtkWidget *widget, gpointer data);
void set_back(void);
void set_ahead(void);
void set_siglist(void);
void set_siglist_from_string(char *s);
void show_command_window(void);
void add_signal_choice(void);
void delete_signal_choice(void);

char *wavemenu;
GtkWidget *analyze_window;	/* analysis controls window */
GtkWidget *tty_window;		/* terminal emulator window */
GtkWidget *tty;			/* VteTerminal widget */

GtkWidget *start_item;		/* elapsed start time entry */
GtkWidget *astart_item;		/* absolute start time entry */
GtkWidget *dstart_item;		/* date start entry */
GtkWidget *end_item;		/* elapsed end time entry */
GtkWidget *aend_item;		/* absolute end time entry */
GtkWidget *dend_item;		/* date end entry */
GtkWidget *signal_item;		/* signal number spin button */
GtkWidget *signal_name_item;	/* signal name label */
GtkWidget *siglist_item;	/* signal list entry */

int analyze_popup_active = -1;

struct MenuEntry {
    char *label;
    char *command;
    struct MenuEntry *nme;
} menu_head, *mep;

static char *menudir;

char *print_command = NULL;
int menu_read = 0;

void print_proc(void)
{
    char default_print_command[256];
    void read_menu(void);

    if (menu_read == 0)
	read_menu();
    if (print_command == NULL) {
	sprintf(default_print_command, "echo $RECORD $LEFT-$RIGHT |\
 pschart -a $ANNOTATOR -g -l -L -n 0 -R -t 20 -v 8 - | %s\n", psprint);
	print_command = default_print_command;
    }
    do_command(print_command);
}

char *open_url_command = NULL;

void open_url(void)
{
    char default_open_url_command[256];
    void read_menu(void);

    if (menu_read == 0)
	read_menu();
    if (open_url_command == NULL) {
	sprintf(default_open_url_command, "url_view $URL\n");
	open_url_command = default_open_url_command;
    }
    do_command(open_url_command);
}

void read_menu(void)
{
    char linebuf[MAXLL+1], *p, *p2;
    int l;
    FILE *ifile = NULL;

    /* Record that an attempt has been made to read the menu. */
    menu_read = 1;

    /* Clear the menu entry list. */
    mep = &menu_head;
    mep->label = mep->command = (char *)NULL;
    mep->nme = (struct MenuEntry *)NULL;

    /* Try to open the user's menu file, if one is specified. */
    if ((wavemenu != NULL || (wavemenu = getenv("WAVEMENU"))) &&
	(ifile = fopen(wavemenu, "r")) == NULL) {
	GtkWidget *dialog = gtk_message_dialog_new(
	    GTK_WINDOW(main_window),
	    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	    GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
	    "Can't read menu file: %s", wavemenu);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
    }

    /* If no user menu was specified, try to open "wavemenu" in the current
       directory. */
    if (ifile == NULL && (ifile = fopen("wavemenu", "r")) != NULL)
	wavemenu = "wavemenu";

    /* If the user's file wasn't specified or can't be read, and "wavemenu"
       can't be read in the current directory, try to open the default menu
       file. */
    if (ifile == NULL) {
	if ((menudir = getenv("MENUDIR")) == NULL) menudir = MENUDIR;
	if ((wavemenu = malloc(strlen(menudir)+strlen(MENUFILE)+2))) {
	    sprintf(wavemenu, "%s/%s", menudir, MENUFILE);
	    if ((ifile = fopen(wavemenu, "r")) == NULL) {
		GtkWidget *dialog = gtk_message_dialog_new(
		    GTK_WINDOW(main_window),
		    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		    GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
		    "Can't read default menu file: %s", wavemenu);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	    }
	    free(wavemenu);
	    wavemenu = NULL;
	}
    }

    /* Give up if no menu file can be opened.  In this case, commands may still
       be typed into the tty subwindow directly. */
    if (ifile == NULL) return;

    /* Read the menu file. */
    while (fgets(linebuf, MAXLL+1, ifile)) {	/* read the next line */
	while ((l = strlen(linebuf)) > 1 && l < MAXLL && linebuf[l-2]=='\\' &&
	       fgets(&linebuf[l-2], MAXLL+1 - (l-2), ifile))
	    /* append continuation line(s), if any */
	    if (linebuf[l-2] == '\t' || linebuf[l-2] == ' ') {
		/* discard initial whitespace in continuation lines, if any */
		char *p, *q;

		for (p = q = linebuf+l-2; *p == '\t' || *p == ' '; p++)
		    ;
		while (*p)
		    *q++ = *p++;
		*q = '\0';
	    }

	/* Find the first non-whitespace character (the beginning of the
	   button label). */
	for (p = linebuf; *p; p++) {
	    if (*p == '#') { *p = '\0'; break; }
	    else if (*p != ' ' && *p != '\t') break;
	}

	/* Skip comments and empty lines. */
	if (*p == '\0') continue;

	/* Find the first embedded tab (the end of the button label). */
	for (p2 = p; *p2 && *p2 != '\t'; p2++)
	    ;

	/* Skip lines without an embedded tab. */
	if (*p2 == '\0') continue;

	*p2++ = '\0';	/* Replace the embedded tab with a null. */

	/* Find the next non-whitespace character (the beginning of the
	   command). */
	for ( ; *p2 == '\t' || *p2 == ' '; p2++)
	    ;

	/* Skip lines without a command. */
	if (*p2 == '\n') continue;

	/* Test for special <Print> command definition. */
	if (strcmp(p, "<Print>") == 0) {
	    if ((p = (char *)malloc((unsigned)(strlen(p2)+1)))) {
		strcpy(p, p2);
		print_command = p;
	    }
	    continue;
	}

	/* Test for special <Open URL> command definition. */
	if (strcmp(p, "<Open URL>") == 0) {
	    if ((p = (char *)malloc((unsigned)(strlen(p2)+1)))) {
		strcpy(p, p2);
		open_url_command = p;
	    }
	    continue;
	}

	/* Allocate storage for the menu entry, button label, and command. */
	if (!(mep->nme=(struct MenuEntry *)malloc(sizeof(struct MenuEntry))) ||
	    !(mep->nme->label = (char *)malloc((unsigned)(strlen(p)+1))) ||
	    !(mep->nme->command = (char *)malloc((unsigned)(strlen(p2)+1)))) {
	    GtkWidget *dialog = gtk_message_dialog_new(
		GTK_WINDOW(main_window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
		"Out of memory while reading menu file");
	    gtk_dialog_run(GTK_DIALOG(dialog));
	    gtk_widget_destroy(dialog);
	    mep->nme = NULL;
	    break;
	}
	strcpy(mep->nme->label, p);
	strcpy(mep->nme->command, p2);
	mep = mep->nme;
	mep->nme = NULL;
    }
    fclose(ifile);
}

/* Helper: pass the 'e', 'a', or 'd' tag via the entry widget name.
   set_start and set_stop read this to decide which entry triggered them. */
static void set_entry_tag(GtkWidget *entry, char tag)
{
    char name[2] = { tag, '\0' };
    gtk_widget_set_name(entry, name);
}

static char get_entry_tag(GtkWidget *entry)
{
    const char *name = gtk_widget_get_name(entry);
    if (name && name[0] && name[1] == '\0')
	return name[0];
    return 'e';
}

/* Set up analyze menu and terminal emulator windows. */
void create_analyze_popup(void)
{
    int i;
    void recreate_analyze_popup(void);
    void reload(void);
    GtkWidget *vbox, *row1, *row2, *row3, *btn_box;
    GtkWidget *btn;

    if (menu_read == 0)
	read_menu();

    analyze_popup_active = 0;

    /* --- Analysis controls window --- */
    analyze_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(analyze_window), "Analyze");
    gtk_window_set_transient_for(GTK_WINDOW(analyze_window),
				 GTK_WINDOW(main_window));
    gtk_window_set_default_size(GTK_WINDOW(analyze_window), mmx(225), -1);
    g_signal_connect(analyze_window, "delete-event",
		     G_CALLBACK(gtk_widget_hide_on_delete), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
    gtk_container_add(GTK_CONTAINER(analyze_window), vbox);

    /* Row 1: < | Start (elapsed) | End (elapsed) | > */
    row1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX(vbox), row1, FALSE, FALSE, 0);

    btn = gtk_button_new_with_label("<");
    g_signal_connect_swapped(btn, "clicked", G_CALLBACK(set_back), NULL);
    gtk_box_pack_start(GTK_BOX(row1), btn, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(row1),
		       gtk_label_new("Start (elapsed):"), FALSE, FALSE, 0);
    start_item = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(start_item), 13);
    set_entry_tag(start_item, 'e');
    g_signal_connect(start_item, "activate",
		     G_CALLBACK(set_start_from_entry), NULL);
    gtk_box_pack_start(GTK_BOX(row1), start_item, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(row1),
		       gtk_label_new("End (elapsed):"), FALSE, FALSE, 0);
    end_item = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(end_item), 13);
    set_entry_tag(end_item, 'e');
    g_signal_connect(end_item, "activate",
		     G_CALLBACK(set_stop_from_entry), NULL);
    gtk_box_pack_start(GTK_BOX(row1), end_item, FALSE, FALSE, 0);

    btn = gtk_button_new_with_label(">");
    g_signal_connect_swapped(btn, "clicked", G_CALLBACK(set_ahead), NULL);
    gtk_box_pack_start(GTK_BOX(row1), btn, FALSE, FALSE, 0);

    /* Signal spinner + name label */
    {
	GtkWidget *sig_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(row1), sig_box, FALSE, FALSE, 8);

	gtk_box_pack_start(GTK_BOX(sig_box),
			   gtk_label_new("Signal:"), FALSE, FALSE, 0);
	signal_item = gtk_spin_button_new_with_range(
	    0, nsig > 0 ? nsig - 1 : 0, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(signal_item), signal_choice);
	gtk_widget_set_sensitive(signal_item, nsig > 0);
	g_signal_connect(signal_item, "value-changed",
			 G_CALLBACK(set_signal), NULL);
	gtk_box_pack_start(GTK_BOX(sig_box), signal_item, FALSE, FALSE, 0);

	signal_name_item = gtk_label_new(
	    nsig > 0 ? signame[signal_choice] : "");
	gtk_box_pack_start(GTK_BOX(sig_box), signal_name_item,
			   FALSE, FALSE, 0);
    }

    /* Row 2: From (absolute) | date | To (absolute) | date */
    row2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX(vbox), row2, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(row2),
		       gtk_label_new("From:"), FALSE, FALSE, 0);
    astart_item = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(astart_item), 13);
    set_entry_tag(astart_item, 'a');
    g_signal_connect(astart_item, "activate",
		     G_CALLBACK(set_start_from_entry), NULL);
    gtk_box_pack_start(GTK_BOX(row2), astart_item, FALSE, FALSE, 0);

    dstart_item = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(dstart_item), 11);
    set_entry_tag(dstart_item, 'd');
    g_signal_connect(dstart_item, "activate",
		     G_CALLBACK(set_start_from_entry), NULL);
    gtk_box_pack_start(GTK_BOX(row2), dstart_item, FALSE, FALSE, 0);

    reset_start();

    gtk_box_pack_start(GTK_BOX(row2),
		       gtk_label_new("To:"), FALSE, FALSE, 0);
    aend_item = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(aend_item), 13);
    set_entry_tag(aend_item, 'a');
    g_signal_connect(aend_item, "activate",
		     G_CALLBACK(set_stop_from_entry), NULL);
    gtk_box_pack_start(GTK_BOX(row2), aend_item, FALSE, FALSE, 0);

    dend_item = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(dend_item), 13);
    set_entry_tag(dend_item, 'd');
    g_signal_connect(dend_item, "activate",
		     G_CALLBACK(set_stop_from_entry), NULL);
    gtk_box_pack_start(GTK_BOX(row2), dend_item, FALSE, FALSE, 0);

    reset_stop();

    /* Signal list */
    row3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX(vbox), row3, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(row3),
		       gtk_label_new("Signal list:"), FALSE, FALSE, 0);
    siglist_item = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(siglist_item), 15);
    gtk_widget_set_sensitive(siglist_item, nsig > 0);
    g_signal_connect(siglist_item, "activate",
		     G_CALLBACK(set_siglist), NULL);
    gtk_box_pack_start(GTK_BOX(row3), siglist_item, TRUE, TRUE, 0);
    reset_siglist();

    /* Utility buttons row */
    btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX(vbox), btn_box, FALSE, FALSE, 0);

    btn = gtk_button_new_with_label("Show scope window");
    g_signal_connect_swapped(btn, "clicked",
			     G_CALLBACK(show_scope_window), NULL);
    gtk_box_pack_start(GTK_BOX(btn_box), btn, FALSE, FALSE, 0);

    btn = gtk_button_new_with_label("Show command window");
    g_signal_connect_swapped(btn, "clicked",
			     G_CALLBACK(show_command_window), NULL);
    gtk_box_pack_start(GTK_BOX(btn_box), btn, FALSE, FALSE, 0);

    btn = gtk_button_new_with_label("Edit menu");
    g_signal_connect_swapped(btn, "clicked",
			     G_CALLBACK(edit_menu_file), NULL);
    gtk_box_pack_start(GTK_BOX(btn_box), btn, FALSE, FALSE, 0);

    btn = gtk_button_new_with_label("Reread menu");
    g_signal_connect_swapped(btn, "clicked",
			     G_CALLBACK(recreate_analyze_popup), NULL);
    gtk_box_pack_start(GTK_BOX(btn_box), btn, FALSE, FALSE, 0);

    btn = gtk_button_new_with_label("Reload");
    g_signal_connect_swapped(btn, "clicked",
			     G_CALLBACK(reload), NULL);
    gtk_box_pack_start(GTK_BOX(btn_box), btn, FALSE, FALSE, 0);

    /* Analysis command buttons from menu file */
    {
	GtkWidget *cmd_box = gtk_flow_box_new();
	gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(cmd_box),
					GTK_SELECTION_NONE);
	gtk_box_pack_start(GTK_BOX(vbox), cmd_box, FALSE, FALSE, 0);

	for (i = 0, mep = &menu_head; mep->nme != NULL;
	     mep = mep->nme, i++) {
	    GtkWidget *abtn = gtk_button_new_with_label(mep->nme->label);
	    g_object_set_data(G_OBJECT(abtn), "menu-index",
			      GINT_TO_POINTER(i));
	    g_signal_connect(abtn, "clicked",
			     G_CALLBACK(do_analysis), NULL);
	    gtk_container_add(GTK_CONTAINER(cmd_box), abtn);
	}
    }

    /* --- Terminal emulator window --- */
    if (tty == NULL) {
	tty_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(tty_window), "Analysis commands");
	gtk_window_set_transient_for(GTK_WINDOW(tty_window),
				     GTK_WINDOW(main_window));
	gtk_window_set_default_size(GTK_WINDOW(tty_window), 600, 250);
	g_signal_connect(tty_window, "delete-event",
			 G_CALLBACK(gtk_widget_hide_on_delete), NULL);

	tty = vte_terminal_new();
	vte_terminal_set_size(VTE_TERMINAL(tty), 80, 10);
	gtk_container_add(GTK_CONTAINER(tty_window), tty);

	char *shell_argv[] = { "/bin/sh", NULL };
	vte_terminal_spawn_async(VTE_TERMINAL(tty),
	    VTE_PTY_DEFAULT, NULL, shell_argv, NULL,
	    G_SPAWN_DEFAULT, NULL, NULL, NULL, -1, NULL, NULL, NULL);
    }

    /* Set the signal name to the correct value now that the label has
       been created with a placeholder. */
    if (nsig > 0)
	gtk_label_set_text(GTK_LABEL(signal_name_item),
			   signame[signal_choice]);
}

/* Recreate analyze popup (if reread menu button was selected). */
void recreate_analyze_popup(void)
{
    if (analyze_window) {
	gtk_widget_destroy(analyze_window);
	analyze_window = NULL;

	struct MenuEntry *tmep;
	mep = menu_head.nme;
	while (mep != NULL) {
	    tmep = mep->nme;
	    free(mep->command);
	    free(mep->label);
	    free(mep);
	    mep = tmep;
	}
	menu_read = 0;
	create_analyze_popup();
	gtk_widget_show_all(analyze_window);
	gtk_window_present(GTK_WINDOW(analyze_window));
    }
}

/* Make the analysis menu window appear. */
void analyze_proc(void)
{
    if (analyze_popup_active < 0) create_analyze_popup();
    gtk_widget_show_all(tty_window);
    gtk_window_present(GTK_WINDOW(tty_window));
    gtk_widget_show_all(analyze_window);
    gtk_window_present(GTK_WINDOW(analyze_window));
    analyze_popup_active = 1;
}

/* Edit the menu file. */
void edit_menu_file(void)
{
    char *edit_command, *editor, *menu_filename;
    int clen, elen, result;

    if ((editor = getenv("EDITOR")) == NULL)
	editor = EDITOR;
    elen = strlen(editor);
    if (wavemenu == NULL) {
	result = wave_notice_prompt(
	    "You are now using the system default menu file,\n"
	    "which you may not edit directly.\n"
	    "Press 'Yes' to copy it into the current directory\n"
	    "as 'wavemenu' (and remember to set the WAVEMENU\n"
	    "environment variable next time),\n"
	    "or press 'No' if you prefer not to edit a menu file.");
	if (!result)
	    return;

	clen = strlen(menudir) + strlen(MENUFILE) + 4; /* "cp ... " */
	if (clen < elen) clen = elen;
	if ((edit_command = malloc(clen + 10))) {  /* "wavemenu\n" + null */
	    sprintf(edit_command, "cp %s/%s wavemenu\n", menudir, MENUFILE);
	    do_command(edit_command);
	    wavemenu = "wavemenu";
	}
    }
    else
	edit_command = malloc(elen + strlen(wavemenu) + 3);
    if (edit_command) {
	sprintf(edit_command, "%s %s\n", editor, wavemenu);
	show_command_window();
	do_command(edit_command);
	free(edit_command);
    }
}

/* Make the analysis command window appear. */
void show_command_window(void)
{
    if (analyze_popup_active < 0) create_analyze_popup();
    gtk_widget_show_all(tty_window);
    gtk_window_present(GTK_WINDOW(tty_window));
}

/* Set variables needed for analysis routines. */
void set_signal(GtkWidget *widget, gpointer data)
{
    (void)data;
    signal_choice = gtk_spin_button_get_value_as_int(
	GTK_SPIN_BUTTON(signal_item));
    gtk_label_set_text(GTK_LABEL(signal_name_item), signame[signal_choice]);
    sig_highlight(signal_choice);
}

void set_signal_choice(int i)
{
    int j;

    if (sig_mode == 0)
	j = i;
    else if (0 <= i && i < siglistlen)
	j = siglist[i];
    if (0 <= j && j < nsig) {
	signal_choice = j;
	if (analyze_popup_active >= 0) {
	    gtk_spin_button_set_value(GTK_SPIN_BUTTON(signal_item),
				     signal_choice);
	    gtk_label_set_text(GTK_LABEL(signal_name_item),
			       signame[signal_choice]);
	}
	sig_highlight(signal_choice);
    }
}

void set_siglist(void)
{
    set_siglist_from_string(
	(char *)gtk_entry_get_text(GTK_ENTRY(siglist_item)));
}

void set_siglist_from_string(char *s)
{
    char *p;

    /* Count the number of signals named in the string (s). */
    for (p = s, siglistlen = 0; *p; ) {
	while (*p && (*p == ' ' || *p == '\t'))
	    p++;
	if (*p) siglistlen++;
	while (*p && (*p != ' ' && *p != '\t'))
	    p++;
    }
    /* Allocate storage for siglist. */
    if (siglistlen > maxsiglistlen) {
	siglist = realloc(siglist, siglistlen * sizeof(int));
	base = realloc(base, siglistlen * sizeof(int));
	level = realloc(level, siglistlen * sizeof(WaveSegment));
	maxsiglistlen = siglistlen;
    }
    /* Now store the signal numbers in siglist. */
    for (p = s, siglistlen = 0; *p && siglistlen < maxsiglistlen; ) {
	while (*p && (*p == ' ' || *p == '\t'))
	    p++;
	if (*p) siglist[siglistlen++] = atoi(p);
	while (*p && (*p != ' ' && *p != '\t'))
	    p++;
    }
    reset_siglist();
}

/* set_start_from_entry: callback for GtkEntry "activate" signal.
   The entry's widget name (set via set_entry_tag) carries 'e', 'a', or 'd'
   to distinguish which entry triggered it. */
void set_start_from_entry(GtkWidget *widget, gpointer data)
{
    (void)data;
    struct ap *a;
    char *start_string, *astart_string, *dstart_string, *p;
    char buf[30];
    int i;
    WFDB_Time t;

    i = get_entry_tag(widget);
    if ((a = get_ap())) {
	int redraw;

	redraw = (display_start_time <= begin_analysis_time &&
		  begin_analysis_time < display_start_time + nsamp);
	switch (i) {
	  case 'e':
	    start_string = (char *)gtk_entry_get_text(GTK_ENTRY(start_item));
	    t = strtim(start_string);
	    p = mstimstr(-t);
	    if (*p == '[') {
		*(p+13) = *(p+24) = '\0';
		gtk_entry_set_text(GTK_ENTRY(astart_item), p+1);
		gtk_widget_set_sensitive(astart_item, TRUE);
		gtk_entry_set_text(GTK_ENTRY(dstart_item), p+14);
		gtk_widget_set_sensitive(dstart_item, TRUE);
	    }
	    else {
		gtk_entry_set_text(GTK_ENTRY(astart_item), "");
		gtk_widget_set_sensitive(astart_item, FALSE);
		gtk_entry_set_text(GTK_ENTRY(dstart_item), "");
		gtk_widget_set_sensitive(dstart_item, FALSE);
	    }
	    break;
	  case 'a':
	    astart_string = (char *)gtk_entry_get_text(
		GTK_ENTRY(astart_item));
	    dstart_string = (char *)gtk_entry_get_text(
		GTK_ENTRY(dstart_item));
	    sprintf(buf, "[%s %s]", astart_string, dstart_string);
	    if ((t = -strtim(buf)) <= 0L) {
		t = 0L;
		gtk_entry_set_text(GTK_ENTRY(start_item), "beginning");
	    }
	    else
		gtk_entry_set_text(GTK_ENTRY(start_item), mstimstr(t));
	    /* Recurse through the 'e' path to update absolute fields. */
	    set_start_from_entry(start_item, NULL);
	    break;
	  case 'd':
	    dstart_string = (char *)gtk_entry_get_text(
		GTK_ENTRY(dstart_item));
	    sprintf(buf, "[0:0:0 %s]", dstart_string);
	    if ((t = -strtim(buf)) <= 0L) {
		t = 0L;
		gtk_entry_set_text(GTK_ENTRY(start_item), "beginning");
	    }
	    else
		gtk_entry_set_text(GTK_ENTRY(start_item), mstimstr(t));
	    set_start_from_entry(start_item, NULL);
	    break;
	}

	a->this.anntyp = BEGIN_ANALYSIS;
	a->this.subtyp = a->this.num = 0;
	a->this.chan = 127;
	a->this.aux = NULL;
	a->this.time = t;
	insert_annotation(a);
	if (redraw || (display_start_time <= begin_analysis_time &&
		  begin_analysis_time < display_start_time + nsamp)) {
	    clear_annotation_display();
	    show_annotations(display_start_time, nsamp);
	}
    }
}

/* set_stop_from_entry: callback for end-time GtkEntry "activate" signal. */
void set_stop_from_entry(GtkWidget *widget, gpointer data)
{
    (void)data;
    struct ap *a;
    char *end_string, *aend_string, *dend_string, *p;
    char buf[30];
    int i;
    WFDB_Time t;

    i = get_entry_tag(widget);
    if ((a = get_ap())) {
	int redraw;

	redraw = (display_start_time <= end_analysis_time &&
		  end_analysis_time < display_start_time + nsamp);
	switch (i) {
	  case 'e':
	    end_string = (char *)gtk_entry_get_text(GTK_ENTRY(end_item));
	    t = strtim(end_string);
	    p = mstimstr(-t);
	    if (*p == '[') {
		*(p+13) = *(p+24) = '\0';
		gtk_entry_set_text(GTK_ENTRY(aend_item), p+1);
		gtk_widget_set_sensitive(aend_item, TRUE);
		gtk_entry_set_text(GTK_ENTRY(dend_item), p+14);
		gtk_widget_set_sensitive(dend_item, TRUE);
	    }
	    else {
		gtk_entry_set_text(GTK_ENTRY(aend_item), "");
		gtk_widget_set_sensitive(aend_item, FALSE);
		gtk_entry_set_text(GTK_ENTRY(dend_item), "");
		gtk_widget_set_sensitive(dend_item, FALSE);
	    }
	    break;
	  case 'a':
	    aend_string = (char *)gtk_entry_get_text(GTK_ENTRY(aend_item));
	    dend_string = (char *)gtk_entry_get_text(GTK_ENTRY(dend_item));
	    sprintf(buf, "[%s %s]", aend_string, dend_string);
	    if ((t = -strtim(buf)) <= 0L) {
		t = 0L;
		gtk_entry_set_text(GTK_ENTRY(end_item), "beginning");
	    }
	    else
		gtk_entry_set_text(GTK_ENTRY(end_item), mstimstr(t));
	    set_stop_from_entry(end_item, NULL);
	    break;
	  case 'd':
	    dend_string = (char *)gtk_entry_get_text(GTK_ENTRY(dend_item));
	    sprintf(buf, "[0:0:0 %s]", dend_string);
	    if ((t = -strtim(buf)) <= 0L) {
		t = 0L;
		gtk_entry_set_text(GTK_ENTRY(end_item), "beginning");
	    }
	    else
		gtk_entry_set_text(GTK_ENTRY(end_item), mstimstr(t));
	    set_stop_from_entry(end_item, NULL);
	    break;
	}

	a->this.anntyp = END_ANALYSIS;
	a->this.subtyp = a->this.num = 0;
	a->this.chan = 127;
	a->this.aux = NULL;
	a->this.time = t;
	insert_annotation(a);
	if (redraw || (display_start_time <= end_analysis_time &&
		  end_analysis_time < display_start_time + nsamp)) {
	    clear_annotation_display();
	    show_annotations(display_start_time, nsamp);
	}
    }
}

void set_back(void)
{
    WFDB_Time step = end_analysis_time - begin_analysis_time, t0, t1;

    if (begin_analysis_time <= 0L || step <= 0L) return;
    if ((t0 = begin_analysis_time - step) < 0L) t0 = 0L;
    t1 = t0 + step;
    gtk_entry_set_text(GTK_ENTRY(start_item), mstimstr(t0));
    set_start_from_entry(start_item, NULL);
    gtk_entry_set_text(GTK_ENTRY(end_item), mstimstr(t1));
    set_stop_from_entry(end_item, NULL);
}

void set_ahead(void)
{
    WFDB_Time step = end_analysis_time - begin_analysis_time, t0, t1,
        te = strtim("e");

    if ((te > 0L && end_analysis_time >= te) || step <= 0L) return;
    t0 = begin_analysis_time + step;
    t1 = t0 + step;
    gtk_entry_set_text(GTK_ENTRY(end_item), mstimstr(t1));
    set_stop_from_entry(end_item, NULL);
    gtk_entry_set_text(GTK_ENTRY(start_item), mstimstr(t0));
    set_start_from_entry(start_item, NULL);
}


void reset_start(void)
{
    if (analyze_popup_active >= 0) {
	char *p;

	if (begin_analysis_time == -1L) begin_analysis_time = 0L;
	gtk_entry_set_text(GTK_ENTRY(start_item),
	    begin_analysis_time == 0L ? "beginning"
				      : mstimstr(begin_analysis_time));
	p = mstimstr(-begin_analysis_time);
	if (*p == '[') {
	    *(p+13) = *(p+24) = '\0';
	    gtk_entry_set_text(GTK_ENTRY(astart_item), p+1);
	    gtk_widget_set_sensitive(astart_item, TRUE);
	    gtk_entry_set_text(GTK_ENTRY(dstart_item), p+14);
	    gtk_widget_set_sensitive(dstart_item, TRUE);
	}
	else {
	    gtk_entry_set_text(GTK_ENTRY(astart_item), "");
	    gtk_widget_set_sensitive(astart_item, FALSE);
	    gtk_entry_set_text(GTK_ENTRY(dstart_item), "");
	    gtk_widget_set_sensitive(dstart_item, FALSE);
	}
    }
}

void reset_stop(void)
{
    if (analyze_popup_active >= 0) {
	char *p;

	if (end_analysis_time == -1L) end_analysis_time = strtim("e");
	gtk_entry_set_text(GTK_ENTRY(end_item),
	    end_analysis_time == 0L ? "end"
				    : mstimstr(end_analysis_time));
	p = mstimstr(-end_analysis_time);
	if (*p == '[') {
	    *(p+13) = *(p+24) = '\0';
	    gtk_entry_set_text(GTK_ENTRY(aend_item), p+1);
	    gtk_widget_set_sensitive(aend_item, TRUE);
	    gtk_entry_set_text(GTK_ENTRY(dend_item), p+14);
	    gtk_widget_set_sensitive(dend_item, TRUE);
	}
	else {
	    gtk_entry_set_text(GTK_ENTRY(aend_item), "");
	    gtk_widget_set_sensitive(aend_item, FALSE);
	    gtk_entry_set_text(GTK_ENTRY(dend_item), "");
	    gtk_widget_set_sensitive(dend_item, FALSE);
	}
    }
}

void reset_siglist(void)
{
    if (analyze_popup_active >= 0) {
        char *p;
	int i;
	static char *sigliststring;
	static int maxrssiglistlen;

	if (siglistlen > maxrssiglistlen) {
	    sigliststring = realloc(sigliststring, 8 * siglistlen);
	    maxrssiglistlen = siglistlen;
	}
	p = sigliststring;
	if (p) *p = '\0';
	for (i = 0; i < siglistlen; i++) {
	    sprintf(p, "%d ", siglist[i]);
	    p += strlen(p);
	}
	gtk_entry_set_text(GTK_ENTRY(siglist_item),
			   sigliststring ? sigliststring : "");
    }
    if (sig_mode)
	set_baselines();
}

void reset_maxsig(void)
{
    if (analyze_popup_active >= 0) {
	gtk_widget_set_sensitive(signal_item, nsig > 0);
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(signal_item),
				  0, nsig > 0 ? nsig - 1 : 0);
	if (signal_choice >= nsig || signal_choice < 0) {
	    signal_choice = 0;
	    gtk_spin_button_set_value(GTK_SPIN_BUTTON(signal_item), 0);
	}
	gtk_widget_set_sensitive(signal_name_item, nsig > 0);
	gtk_label_set_text(GTK_LABEL(signal_name_item),
			   signame[signal_choice]);
    }
}

/* Signal-list manipulation. */
void add_to_siglist(int i)
{
    if (0 <= i && i < nsig) {
	if (++siglistlen >= maxsiglistlen) {
	    siglist = realloc(siglist, siglistlen * sizeof(int));
	    base = realloc(base, nsig * sizeof(int));
	    level = realloc(level, nsig * sizeof(WaveSegment));
	    maxsiglistlen = siglistlen;
	}
	siglist[siglistlen-1] = i;
    }
    reset_siglist();
}

void delete_from_siglist(int i)
{
    int nsl;

    for (nsl = 0; nsl < siglistlen; nsl++) {
	if (siglist[nsl] == i) {
	    siglistlen--;
	    for ( ; nsl < siglistlen; nsl++)
		siglist[nsl] = siglist[nsl+1];
	    reset_siglist();
	}
    }
}

void add_signal_choice(void)
{
    add_to_siglist(signal_choice);
}

void delete_signal_choice(void)
{
    delete_from_siglist(signal_choice);
}

/* This function executes the command string provided as its argument, after
   substituting for WAVE's internal variables (RECORD, ANNOTATOR, etc.). */

static void feed_tty(const char *str, int len)
{
    vte_terminal_feed_child(VTE_TERMINAL(tty), str, len);
}

void do_command(char *p1)
{
    char *p2, *tp;

    post_changes();	/* make sure that the annotation file is up-to-date */
    finish_log();	/* make sure that the log file is up-to-date */
    if (analyze_popup_active < 0) create_analyze_popup();
    for (p2 = p1; *p2; p2++) {
	if (*p2 == '$') {
	    feed_tty(p1, p2-p1);
	    p1 = ++p2;
	    if (strncmp(p1, "RECORD", 6) == 0) {
		feed_tty(record, strlen(record));
		p1 = p2 += 6;
	    }
	    else if (strncmp(p1, "ANNOTATOR", 9) == 0) {
		if (annotator[0])
		    feed_tty(annotator, strlen(annotator));
		else
		    feed_tty("\"\"", 2);
		p1 = p2 += 9;
	    }
	    else if (strncmp(p1, "START", 5) == 0) {
		if (begin_analysis_time != -1L && begin_analysis_time != 0L) {
		    tp = mstimstr(begin_analysis_time);
		    while (*tp == ' ') tp++;
		}
		else tp = "0";
		feed_tty(tp, strlen(tp));
		p1 = p2 += 5;
	    }
	    else if (strncmp(p1, "END", 3) == 0) {
		if (end_analysis_time != -1L)
		    tp = mstimstr(end_analysis_time);
		else if (end_analysis_time == 0L)
		    tp = "0";
		else tp = mstimstr(strtim("e"));
		while (*tp == ' ') tp++;
		feed_tty(tp, strlen(tp));
		p1 = p2 += 3;
	    }
	    else if (strncmp(p1, "DURATION", 8) == 0) {
		WFDB_Time t0 = begin_analysis_time, t1 = end_analysis_time;

		/* If end_analysis_time is unspecified, determine the
		   record length. */
		if (t1 == -1L) t1 = strtim("e");
		/* If the record length is also unspecified, use zero
		   in place of the duration.  Programs that accept
		   duration arguments must be written to accept zero
		   as meaning "unspecified". */
		if (t1 == 0L) feed_tty("0", 1);
		else {
		    if (t0 == -1L) t0 = 0L;
		    tp = mstimstr(t1-t0);
		    while (*tp == ' ') tp++;
		    feed_tty(tp, strlen(tp));
		}
		p1 = p2 += 8;
	    }
	    else if (strncmp(p1, "SIGNALS", 7) == 0) {
		char s[4];
		int nsl;

		for (nsl = 0; nsl < siglistlen; nsl++) {
		    sprintf(s, "%d%s", siglist[nsl],
			    nsl < siglistlen ? " " : "");
		    feed_tty(s, strlen(s));
		}
		p1 = p2 += 7;
	    }
	    else if (strncmp(p1, "SIGNAL", 6) == 0) {
		char s[3];

		sprintf(s, "%d", signal_choice);
		feed_tty(s, strlen(s));
		p1 = p2 += 6;
	    }
	    else if (strncmp(p1, "LEFT", 4) == 0) {
		if (display_start_time < 1L) tp = "0";
		else {
		    tp = mstimstr(display_start_time);
		    while (*tp == ' ') tp++;
		}
		feed_tty(tp, strlen(tp));
		p1 = p2 += 4;
	    }
	    else if (strncmp(p1, "RIGHT", 5) == 0) {
		tp = mstimstr(display_start_time + nsamp);
		while (*tp == ' ') tp++;
		feed_tty(tp, strlen(tp));
		p1 = p2 += 5;
	    }
	    else if (strncmp(p1, "WIDTH", 5) == 0) {
		tp = mstimstr(nsamp);
		while (*tp == ' ') tp++;
		feed_tty(tp, strlen(tp));
		p1 = p2 += 5;
	    }
	    else if (strncmp(p1, "LOG", 3) == 0) {
		if (*log_file_name == '0')
		    sprintf(log_file_name, "log.%s", record);
		feed_tty(log_file_name, strlen(log_file_name));
		p1 = p2 += 3;
	    }
	    else if (strncmp(p1, "WFDBCAL", 7) == 0) {
		feed_tty(cfname, strlen(cfname));
		p1 = p2 += 7;
	    }
	    else if (strncmp(p1, "WFDB", 4) == 0) {
		feed_tty(getwfdb(), strlen(getwfdb()));
		p1 = p2 += 4;
	    }
	    else if (strncmp(p1, "TSCALE", 6) == 0) {
		char s[10];

		sprintf(s, "%g", mmpersec);
		feed_tty(s, strlen(s));
		p1 = p2 += 6;
	    }
	    else if (strncmp(p1, "VSCALE", 6) == 0) {
		char s[10];

		sprintf(s, "%g", mmpermv);
		feed_tty(s, strlen(s));
		p1 = p2 += 6;
	    }
	    else if (strncmp(p1, "DISPMODE", 8) == 0) {
		char s[3];

		sprintf(s, "%d", (ann_mode << 1) | show_marker);
		feed_tty(s, strlen(s));
		p1 = p2 += 8;
	    }
	    else if (strncmp(p1, "PSPRINT", 7) == 0) {
		feed_tty(psprint, strlen(psprint));
		p1 = p2 += 7;
	    }
	    else if (strncmp(p1, "TEXTPRINT", 7) == 0) {
		feed_tty(textprint, strlen(textprint));
		p1 = p2 += 9;
	    }
	    else if (strncmp(p1, "URL", 3) == 0) {
		feed_tty(url, strlen(url));
		p1 = p2 += 3;
	    }
	    else
		p1--;
	}
    }
    feed_tty(p1, p2-p1);
}

void do_analysis(GtkWidget *widget, gpointer data)
{
    (void)data;
    int i, j;

    i = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "menu-index"));
    for (j=0, mep = &menu_head; j<i && mep->nme != NULL; j++, mep = mep->nme)
	;
    if (j == i && mep->nme != NULL && mep->nme->command != NULL)
	do_command(mep->nme->command);
}

static char fname[20];

/* This function gets called periodically while the timer is running.
   Once the temporary file named by fname contains readable data, it
   waits one more cycle, removes the timer, and then deletes the file. */

static guint check_timer_id;

static gboolean check_if_done(gpointer data)
{
    (void)data;
    FILE *tfile;
    static int file_ready;
    void reinitialize(void);

    if (file_ready) {
	check_timer_id = 0;
	unlink(fname);
	file_ready = 0;
	reinitialize();
	set_start_from_entry(start_item, NULL);
	set_stop_from_entry(end_item, NULL);
	return G_SOURCE_REMOVE;
    }
    if ((tfile = fopen(fname, "r")) == NULL)
	return G_SOURCE_CONTINUE;
    fclose(tfile);
    file_ready++;
    return G_SOURCE_CONTINUE;
}

void reload(void)
{
    static char command[80];

    if (fname[0] == '\0') {
	sprintf(fname, "/tmp/wave.XXXXXX");
	mkstemp(fname);
	sprintf(command, "touch %s\n", fname);
    }
    do_command(command);
    if (check_timer_id == 0)
	check_timer_id = g_timeout_add(1000, check_if_done, NULL);
}

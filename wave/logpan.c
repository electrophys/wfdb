/* file: logpan.c	G. Moody	 1 May 1990
			Last revised:	2026
Log panel functions for WAVE (GTK 3 version)

-------------------------------------------------------------------------------
WAVE: Waveform analyzer, viewer, and editor
Copyright (C) 1999 George B. Moody

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
#include <ctype.h>
#include <sys/time.h>

#define LLLMAX	(RNLMAX+40+DSLMAX)	/* max length of line in log file */

static GtkWidget *log_window;

/* A WAVE log file contains one-line entries.  Each entry specifies a record
   and a time or time interval in that record, and contains an associated
   text string, which (typically) contains a user-entered description of
   features of the signals at that time.  The format is:
      <record><whitespace><time_spec><whitespace><text>
   where
      <record> is the record name (if empty, use the record name given in the
	previous entry, or the name of the currently displayed record if this
	is the first entry)
      <time_spec> is either the start and end times separated by a hyphen, or
	the time of the center of the region of interest, in strtim format
      <text> is the text of the log entry, which cannot begin with whitespace
	but which may contain embedded whitespace (this field may be empty)
   and <whitespace> is a sequence of one or more space or tab characters.
   This format is compatible with `pschart' and `psfd' script file format.

   The log is kept internally as a doubly-linked list of log_entry structures.
*/

struct log_entry {
    struct log_entry *prev, *next;
    char *record, *time_spec, *text;
} *first_entry, *current_entry, *last_entry;

/* This function selects an annotation at a specified time.  If no such
   annotation exists, it moves the log marker (an index mark pseudo-annotation)
   to the specified time and selects the marker.
*/
void set_marker(WFDB_Time t)
{
    static struct ap *log_marker;

    if (locate_annotation(t, 0))
	attached = annp;
    else if (log_marker) {
	move_annotation(log_marker, t);
	attached = log_marker;
    }
    else if ((log_marker = get_ap())) {
	log_marker->this.time = t;
	log_marker->this.anntyp = INDEX_MARK;
	insert_annotation(log_marker);
	attached = log_marker;
    }
}

/* add_entry allocates memory for a log_entry structure, fills it in
   using the input arguments, and inserts it in the linked list at the
   location after the current entry.  (If current_entry is NULL, the
   new entry is inserted at the head of the list.)  Upon successful
   (non-zero) return, current_entry points to the newly inserted
   entry, and first_entry and last_entry may have been updated; if
   memory cannot be allocated, add_entry returns 0, and the log entry
   pointers are left unchanged. */

int add_entry(char *recp, char *timep, char *textp)
{
    struct log_entry *new_entry;
    char *p;

    /* Allocate memory for structure and strings. */
    if ((new_entry = (struct log_entry *)malloc(sizeof(struct log_entry)))
	== NULL ||
	(new_entry->record = (char *)malloc(strlen(recp)+1)) == NULL ||
	(new_entry->time_spec = (char *)malloc(strlen(timep)+1)) == NULL ||
	(textp &&
	 (new_entry->text = (char *)malloc(strlen(textp)+1)) == NULL)) {
	wave_notice_prompt("Error in allocating memory for log\n");
	if (new_entry) {
	    if (new_entry->time_spec) free(new_entry->time_spec);
	    if (new_entry->record) free(new_entry->record);
	    free(new_entry);
	}
	return (0);
    }

    /* Fill in the log_entry structure. */
    strcpy(new_entry->record, recp);
    strcpy(new_entry->time_spec, timep);
    if (textp) strcpy(new_entry->text, textp);
    else new_entry->text = NULL;

    /* Insert the new entry into the linked list. */
    if (current_entry) {
	if (current_entry->next)	/* insert into middle of list */
	    (current_entry->next)->prev = new_entry;
	else				/* append at tail */
	    last_entry = new_entry;
	new_entry->next = current_entry->next;
	new_entry->prev = current_entry;
	current_entry->next = new_entry;
    }
    else if (first_entry) {		/* insert at head of list */
	new_entry->next = first_entry;
	new_entry->prev = NULL;
	first_entry->prev = new_entry;
	first_entry = new_entry;
    }
    else {				/* list is empty, initialize it */
	new_entry->next = new_entry->prev = NULL;
	first_entry = last_entry = new_entry;
    }

    current_entry = new_entry;
    p = strchr(timep, '-');
    if (p) *p = '\0';
    return (1);
}

/* delete_entry removes the current entry from the linked list, and resets the
   current_entry pointer so that it points to the next entry in the linked
   list if there is one, or to the previous entry otherwise. */

void delete_entry(void)
{
    struct log_entry *p;

    if (current_entry) {
	if (current_entry->prev)
	    (current_entry->prev)->next = current_entry->next;
	else				/* deleting entry at head */
	    first_entry = current_entry->next;
	if ((p = current_entry->next))
	    (current_entry->next)->prev = current_entry->prev;
	else				/* deleting entry at tail */
	    p = last_entry = current_entry->prev;
	free(current_entry->record);
	free(current_entry->time_spec);
	if (current_entry->text) free(current_entry->text);
	free(current_entry);
	current_entry = p;
    }
}

/* read_log appends the contents of the log file named by its argument to
   the linked list.  It returns 1 if completely successful, 0 if the file
   cannot be read or if not all properly formatted entries can be stored. */

int read_log(char *logfname)
{
    char buf[LLLMAX+1], *p, *recp = record, *timep, *textp = NULL;
    FILE *logfile;
    int ignore;

    if ((logfile = fopen(logfname, "r")) == NULL)
	return (0);
    while (fgets(buf, LLLMAX, logfile)) {	/* read and parse an entry */
	/* Check that entry is properly formatted -- if not, skip it. */
	for (p = buf, ignore = 0; *p; p++) {
	    if (!isprint(*p) && !isspace(*p)) { ignore = 1; break; }
	}
	if (ignore) continue;
	if (buf[0] != ' ' && buf[0] != '\t') {
	    recp = strtok(buf, " \t");	/* first token is record name */
	    timep = strtok(NULL, " \t\n");/* second token is time spec */
	}
	else			/* record name missing, use previous value */
	    timep = strtok(buf, " \t");	/* first token is time spec */
	if (recp == NULL || timep == NULL) continue;
	/* skip improperly formatted entries */
	textp = strtok(NULL, "\n");	/* remainder of line is text */

	if (add_entry(recp, timep, textp) == 0) {
	    fclose(logfile);
	    return (0);
	}
    }
    fclose(logfile);
    return (1);
}

/* write_log copies the contents of the linked list to the log file named by
   its argument.  It returns 1 if successful, 0 otherwise. */

static int log_changes, save_log_backup;

int write_log(char *logfname)
{
    int result;
    struct log_entry *p;
    FILE *logfile;

    /* Do we need to back up ? */
    if (save_log_backup) {
	char backfname[LNLMAX+2];

	snprintf(backfname, sizeof(backfname), "%s~", logfname);
	if (rename(logfname, backfname)) {
	    char msg[LNLMAX + 256];
	    snprintf(msg, sizeof(msg),
		     "Your log cannot be saved unless you remove the "
		     "file named %s\n\n"
		     "You may attempt to correct this problem from "
		     "another window after pressing 'Yes', or "
		     "you may exit immediately and discard your "
		     "changes by pressing 'No'.", backfname);
	    result = wave_notice_prompt(msg);
	    if (result) return (0);  /* user chose "Yes" = continue */
	    else if (post_changes()) {
		gtk_widget_destroy(main_window);
		exit(1);
	    }
	    else return (0);
	}
	save_log_backup = 0;
    }

    if ((logfile = fopen(logfname, "w")) == NULL) {
	char msg[LNLMAX + 256];
	snprintf(msg, sizeof(msg),
		 "Your log cannot be saved until you obtain "
		 "write permission for %s\n\n"
		 "You may attempt to correct this problem from "
		 "another window after pressing 'Yes', or "
		 "you may exit immediately and discard your "
		 "changes by pressing 'No'.", logfname);
	result = wave_notice_prompt(msg);
	if (result) return (0);  /* user chose "Yes" = continue */
	else if (post_changes()) {
	    gtk_widget_destroy(main_window);
	    exit(1);
	}
	else return (0);
    }

    for (p = first_entry; p; p = p->next) {
	fprintf(logfile, "%s %s", p->record, p->time_spec);
	if (p->text) fprintf(logfile, " %s", p->text);
	fprintf(logfile, "\n");
    }

    log_changes = 0;
    fclose(logfile);
    return (1);
}

static GtkWidget *log_name_item, *log_text_item;
static GtkWidget *load_button, *add_button, *replace_button, *delete_button;
static GtkWidget *edit_button, *first_button, *rreview_button, *prev_button;
static GtkWidget *pause_button, *next_button, *review_button, *last_button;
static GtkWidget *delay_scale;

static guint review_timer_id;

void show_current_entry(void)
{
    char *p;
    int record_changed = 0;
    WFDB_Time t0;

    if (current_entry) {
	if (current_entry->text)
	    strncpy(description, current_entry->text, DSLMAX);
	else
	    description[0] = '\0';
	if (strcmp(record, current_entry->record)) {
	    set_record_item(current_entry->record);
	    record_changed = 1;
	}
	p = strchr(current_entry->time_spec, '-');
	if (p) *p = '\0';
	if ((t0 = strtim(current_entry->time_spec)) < 0L) t0 = -t0;
	if (p)		/* start and end times are given */
	    *p = '-';
	else {		/* only one time given -- place mark at that time */
	    set_marker(t0);
	    if ((t0 -= nsamp/2) < 0L) t0 = 0L;
	}
	if (!record_changed) {
	    set_frame_title();
	    (void)find_display_list(t0);
	}
	set_start_time(timstr(t0));
	set_end_time(timstr(t0 + nsamp));
	gtk_entry_set_text(GTK_ENTRY(log_text_item), description);
	disp_proc(".");
	if (attached)
	  box((int)((attached->this.time - display_start_time)*tscale),
	    (ann_mode==1 && (unsigned)attached->this.chan < nsig) ?
	    (int)(base[(unsigned)attached->this.chan] + mmy(2)) : abase,
	    1);
    }
}

static gboolean show_next_entry_cb(gpointer data)
{
    char *p;
    WFDB_Time t0;
    (void)data;

    if (current_entry->next) current_entry = current_entry->next;
    else current_entry = first_entry;
    show_current_entry();
    if (current_entry->next &&
	strcmp(current_entry->next->record, current_entry->record) == 0) {
	p = strchr(current_entry->next->time_spec, '-');
	if (p) *p = '\0';
	if ((t0 = strtim(current_entry->next->time_spec)) < 0L) t0 = -t0;
	if (p) *p = '-';
	else if ((t0 -= nsamp/2) < 0L) t0 = 0L;
	(void)find_display_list(t0);
    }
    return G_SOURCE_CONTINUE;
}

static gboolean show_prev_entry_cb(gpointer data)
{
    char *p;
    WFDB_Time t0;
    (void)data;

    if (current_entry->prev) current_entry = current_entry->prev;
    else current_entry = last_entry;
    show_current_entry();
    if (current_entry->prev &&
	strcmp(current_entry->prev->record, current_entry->record) == 0) {
	p = strchr(current_entry->prev->time_spec, '-');
	if (p) *p = '\0';
	if ((t0 = strtim(current_entry->prev->time_spec)) < 0L) t0 = -t0;
	if (p) *p = '-';
	else if ((t0 -= nsamp/2) < 0L) t0 = 0L;
	(void)find_display_list(t0);
    }
    return G_SOURCE_CONTINUE;
}

static int review_delay = 5;
static int review_in_progress;

void log_review(int direction)
{
    review_in_progress = direction;
    if (review_timer_id) {
	g_source_remove(review_timer_id);
	review_timer_id = 0;
    }
    if (direction == 1)
	review_timer_id = g_timeout_add_seconds(review_delay,
						show_next_entry_cb, NULL);
    else if (direction == -1)
	review_timer_id = g_timeout_add_seconds(review_delay,
						show_prev_entry_cb, NULL);
}

void pause_review(void)
{
    review_in_progress = 0;
    if (review_timer_id) {
	g_source_remove(review_timer_id);
	review_timer_id = 0;
    }
}

/* Handle enabling/disabling log navigation buttons for non-review modes. */
void set_buttons(void)
{
    gtk_widget_set_sensitive(pause_button, FALSE);
    if (log_file_name[0]) {
	gtk_widget_set_sensitive(load_button, TRUE);
	gtk_widget_set_sensitive(add_button, TRUE);
	gtk_widget_set_sensitive(edit_button, TRUE);
    }
    if (current_entry) {
	gtk_widget_set_sensitive(replace_button, TRUE);
	gtk_widget_set_sensitive(delete_button, TRUE);
	gtk_widget_set_sensitive(first_button, TRUE);
	gtk_widget_set_sensitive(last_button, TRUE);
	gtk_widget_set_sensitive(next_button,
				 current_entry->next ? TRUE : FALSE);
	gtk_widget_set_sensitive(prev_button,
				 current_entry->prev ? TRUE : FALSE);
	gtk_widget_set_sensitive(review_button,
		(current_entry->next || current_entry->prev) ? TRUE : FALSE);
	gtk_widget_set_sensitive(rreview_button,
		(current_entry->next || current_entry->prev) ? TRUE : FALSE);
    }
    else {
	gtk_widget_set_sensitive(replace_button, FALSE);
	gtk_widget_set_sensitive(delete_button, FALSE);
	gtk_widget_set_sensitive(first_button, FALSE);
	gtk_widget_set_sensitive(rreview_button, FALSE);
	gtk_widget_set_sensitive(prev_button, FALSE);
	gtk_widget_set_sensitive(next_button, FALSE);
	gtk_widget_set_sensitive(review_button, FALSE);
	gtk_widget_set_sensitive(last_button, FALSE);
    }
}

void disable_buttons(void)
{
    gtk_widget_set_sensitive(load_button, FALSE);
    gtk_widget_set_sensitive(add_button, FALSE);
    gtk_widget_set_sensitive(replace_button, FALSE);
    gtk_widget_set_sensitive(delete_button, FALSE);
    gtk_widget_set_sensitive(edit_button, FALSE);
    gtk_widget_set_sensitive(first_button, FALSE);
    gtk_widget_set_sensitive(rreview_button, FALSE);
    gtk_widget_set_sensitive(prev_button, FALSE);
    gtk_widget_set_sensitive(pause_button, FALSE);
    gtk_widget_set_sensitive(next_button, FALSE);
    gtk_widget_set_sensitive(review_button, FALSE);
    gtk_widget_set_sensitive(last_button, FALSE);
}

/* Edit the log file. */
void edit_log_file(void)
{
    char *edit_command, *editor;

    if ((editor = getenv("EDITOR")) == NULL)
	editor = EDITOR;
    edit_command = malloc(strlen(editor) + strlen(log_file_name) + 3);
    if (edit_command) {
	sprintf(edit_command, "%s %s\n", editor, log_file_name);
	analyze_proc();
	do_command(edit_command);
	free(edit_command);
    }
}

/* Callback: file name entry activated (Enter pressed). */
static void on_log_name_activate(GtkEntry *entry, gpointer data)
{
    const char *new_name;
    (void)data;

    new_name = gtk_entry_get_text(entry);
    /* Do nothing unless the name has been changed. */
    if (strncmp(log_file_name, new_name, LNLMAX)) {
	/* If edits have been made, write the current log to the old file.
	   If the write fails, reset the file name (after write_log has
	   alerted the user). */
	if (log_changes && write_log(log_file_name) == 0)
	    gtk_entry_set_text(GTK_ENTRY(log_name_item), log_file_name);
	else {
	    /* Clear out the old entries. */
	    for (current_entry = first_entry; current_entry; )
		delete_entry();
	    strncpy(log_file_name, new_name, LNLMAX);
	    /* Reinitialize from the new log file. */
	    if (read_log(log_file_name))
		save_log_backup = 1;
	    log_changes = 0;
	    current_entry = first_entry;
	}
	set_buttons();
	show_current_entry();
    }
}

/* Generic button-click callback dispatching on the character stored
   as user data (same convention as the original XView PANEL_CLIENT_DATA). */
static void on_log_button_clicked(GtkButton *button, gpointer data)
{
    int client_data = GPOINTER_TO_INT(data);
    char timestring[25];
    (void)button;

    switch (client_data) {
      case 'a': {	/* add an entry */
	if (attached && display_start_time < attached->this.time &&
	    attached->this.time < display_start_time + nsamp)
	    strcpy(timestring, mstimstr(attached->this.time));
	else {
	    strcpy(timestring, strtok(timstr(display_start_time), " "));
	    strcat(timestring, "-");
	    strcat(timestring, strtok(timstr(display_start_time+nsamp), " "));
	}
	if (add_entry(record, timestring,
		      (char *)gtk_entry_get_text(GTK_ENTRY(log_text_item)))) {
	    if (++log_changes > 10) write_log(log_file_name);
	    set_buttons();
	}
	break;
      }
      case 'd':		/* delete the current entry */
	delete_entry();
	if (++log_changes > 10) write_log(log_file_name);
	set_buttons();
	show_current_entry();
	break;
      case 'e':		/* edit the log file */
	if (log_file_name[0]) {
	    disable_buttons();
	    if (log_changes > 0) write_log(log_file_name);
	    edit_log_file();
	    /* Clear out the old entries. */
	    for (current_entry = first_entry; current_entry; )
		delete_entry();
	    /* Reinitialize from the new log file. */
	    if (read_log(log_file_name))
		save_log_backup = 1;
	    log_changes = 0;
	    current_entry = first_entry;
	    set_buttons();
	}
	break;
      case 'l': {	/* force reload of log file */
	const char *name_val;
	/* If edits have been made, write the current log to the old file.
	   If the write fails, reset the file name (after write_log has
	   alerted the user). */
	if (log_changes) {
	    char backfname[LNLMAX+2];
	    snprintf(backfname, sizeof(backfname), "%s~", log_file_name);
	    save_log_backup = 0;
	    write_log(backfname);
	}
	/* Clear out the old entries. */
	for (current_entry = first_entry; current_entry; )
	    delete_entry();
	name_val = gtk_entry_get_text(GTK_ENTRY(log_name_item));
	strncpy(log_file_name, name_val, LNLMAX);
	/* Reinitialize from the new log file. */
	if (read_log(log_file_name))
	    save_log_backup = 1;
	log_changes = 0;
	current_entry = first_entry;
	set_buttons();
	show_current_entry();
	break;
      }
      case 'p':		/* pause review */
	pause_review();
	set_buttons();
	break;
      case 'r':		/* replace description field of current entry */
	if (current_entry) {
	    const char *newtext =
		gtk_entry_get_text(GTK_ENTRY(log_text_item));
	    char *newtextp;

	    if (strcmp(newtext, current_entry->text) &&
		(newtextp = (char *)malloc(strlen(newtext)+1))) {
		strcpy(newtextp, newtext);
		free(current_entry->text);
		current_entry->text = newtextp;
		if (++log_changes > 10) write_log(log_file_name);
	    }
	}
	break;
      case 'A':		/* show first entry */
	if (first_entry) {
	    current_entry = first_entry;
	    set_buttons();
	    show_current_entry();
	}
	break;
      case '<':		/* show previous entry */
	if (current_entry->prev) {
	    current_entry = current_entry->prev;
	    set_buttons();
	    show_current_entry();
	}
	break;
      case '>':		/* show next entry */
	if (current_entry->next) {
	    current_entry = current_entry->next;
	    set_buttons();
	    show_current_entry();
	}
	break;
      case 'Z':		/* show last entry */
	if (last_entry) {
	    current_entry = last_entry;
	    set_buttons();
	    show_current_entry();
	}
	break;
      case '+':		/* review log entries */
      case '-':		/* review log entries in reverse order */
	disable_buttons();
	gtk_widget_set_sensitive(pause_button, TRUE);
	log_review(client_data == '+' ? 1 : -1);
	break;
    }
}

static void on_delay_changed(GtkRange *range, gpointer data)
{
    (void)data;
    review_delay = (int)gtk_range_get_value(range);
    if (review_in_progress) log_review(review_in_progress);
}

/* Helper to create a button with a label, tooltip, click handler, and
   client-data character. */
static GtkWidget *make_log_button(const char *label, const char *tooltip,
				  int client_data)
{
    GtkWidget *btn = gtk_button_new_with_label(label);
    gtk_widget_set_tooltip_text(btn, tooltip);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_log_button_clicked),
		     GINT_TO_POINTER(client_data));
    gtk_widget_set_sensitive(btn, FALSE);
    return btn;
}

/* Set up log window. */
static void create_log_popup(void)
{
    GtkWidget *vbox, *hbox, *label, *nav_box;

    log_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(log_window), "WAVE log");
    gtk_window_set_transient_for(GTK_WINDOW(log_window),
				 GTK_WINDOW(main_window));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(log_window), TRUE);
    /* Hide on close instead of destroying, so it can be re-shown. */
    g_signal_connect(log_window, "delete-event",
		     G_CALLBACK(gtk_widget_hide_on_delete), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
    gtk_container_add(GTK_CONTAINER(log_window), vbox);

    /* Row 0: File name entry */
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("File:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    log_name_item = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(log_name_item), 60);
    gtk_widget_set_tooltip_text(log_name_item,
				"Name of the log file");
    g_signal_connect(log_name_item, "activate",
		     G_CALLBACK(on_log_name_activate), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), log_name_item, TRUE, TRUE, 0);

    load_button = make_log_button("Load",
				  "Reload the log file from disk", 'l');
    gtk_box_pack_start(GTK_BOX(hbox), load_button, FALSE, FALSE, 0);

    /* Row 1: Description entry + delay slider */
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("Description:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    log_text_item = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(log_text_item), 50);
    gtk_widget_set_tooltip_text(log_text_item,
				"Description text for the current log entry");
    gtk_box_pack_start(GTK_BOX(hbox), log_text_item, TRUE, TRUE, 0);

    label = gtk_label_new("Delay:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    delay_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
					   1.0, 10.0, 1.0);
    gtk_range_set_value(GTK_RANGE(delay_scale), 5.0);
    gtk_scale_set_draw_value(GTK_SCALE(delay_scale), FALSE);
    gtk_widget_set_size_request(delay_scale, 100, -1);
    gtk_widget_set_tooltip_text(delay_scale,
				"Review delay in seconds (1-10)");
    g_signal_connect(delay_scale, "value-changed",
		     G_CALLBACK(on_delay_changed), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), delay_scale, FALSE, FALSE, 0);

    /* Row 2: Add / Replace / Delete / Edit buttons */
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    add_button = make_log_button("Add",
				 "Add a new log entry at the current position",
				 'a');
    gtk_box_pack_start(GTK_BOX(hbox), add_button, FALSE, FALSE, 0);

    replace_button = make_log_button("Replace",
		"Replace the description of the current log entry", 'r');
    gtk_box_pack_start(GTK_BOX(hbox), replace_button, FALSE, FALSE, 0);

    delete_button = make_log_button("Delete",
				    "Delete the current log entry", 'd');
    gtk_box_pack_start(GTK_BOX(hbox), delete_button, FALSE, FALSE, 0);

    edit_button = make_log_button("Edit",
		"Edit the log file in an external text editor", 'e');
    gtk_box_pack_start(GTK_BOX(hbox), edit_button, FALSE, FALSE, 0);

    /* Row 3: Navigation buttons */
    nav_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(vbox), nav_box, FALSE, FALSE, 0);

    first_button = make_log_button("|<",
				   "Go to the first log entry", 'A');
    gtk_box_pack_start(GTK_BOX(nav_box), first_button, FALSE, FALSE, 0);

    rreview_button = make_log_button("<<",
		"Auto-review log entries in reverse order", '-');
    gtk_box_pack_start(GTK_BOX(nav_box), rreview_button, FALSE, FALSE, 0);

    prev_button = make_log_button("<",
				  "Go to the previous log entry", '<');
    gtk_box_pack_start(GTK_BOX(nav_box), prev_button, FALSE, FALSE, 0);

    pause_button = make_log_button("Pause",
				   "Pause the auto-review", 'p');
    gtk_box_pack_start(GTK_BOX(nav_box), pause_button, FALSE, FALSE, 0);

    next_button = make_log_button(">",
				  "Go to the next log entry", '>');
    gtk_box_pack_start(GTK_BOX(nav_box), next_button, FALSE, FALSE, 0);

    review_button = make_log_button(">>",
		"Auto-review log entries in forward order", '+');
    gtk_box_pack_start(GTK_BOX(nav_box), review_button, FALSE, FALSE, 0);

    last_button = make_log_button(">|",
				  "Go to the last log entry", 'Z');
    gtk_box_pack_start(GTK_BOX(nav_box), last_button, FALSE, FALSE, 0);
}

static int log_popup_active = -1;

/* Make the log popup window appear. */
void show_log(void)
{
    if (log_popup_active < 0) create_log_popup();
    gtk_widget_show_all(log_window);
    gtk_window_present(GTK_WINDOW(log_window));
    log_popup_active = 1;
}

/* Update and close the log file. */
void finish_log(void)
{
    if (log_changes > 0) write_log(log_file_name);
}

/* Enter demonstration mode. */
void start_demo(void)
{
    char *filename, *p, *title;
    int c, r, x, y;
    extern void mode_undo(void);

    if ((filename = malloc(strlen(helpdir) + strlen("wave/demo.txt") + 2))) {
        if ((title = getenv("DEMOTITLE")) == NULL)
	    title = "Demonstration of WAVE";
	if ((p = getenv("DEMOX")) == NULL)
	    x = 10;
	else
	    x = atoi(p);
	if ((p = getenv("DEMOY")) == NULL)
	    y = 700;
	else
	    y = atoi(p);
	if ((p = getenv("DEMOCOLS")) == NULL)
	    c = 80;
	else
	    c = atoi(p);
	if ((p = getenv("DEMOROWS")) == NULL)
	    r = 20;
	else
	    r = atoi(p);

	sprintf(filename, "%s/wave/demo.txt", helpdir);

	/* Create a text window to display the demo description. */
	{
	    GtkWidget *text_window, *scrolled, *textview;
	    GtkTextBuffer *buffer;
	    FILE *fp;

	    text_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	    gtk_window_set_title(GTK_WINDOW(text_window), title);
	    gtk_window_move(GTK_WINDOW(text_window), x, y);
	    gtk_window_set_default_size(GTK_WINDOW(text_window),
					c * 8, r * 16);

	    scrolled = gtk_scrolled_window_new(NULL, NULL);
	    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
					   GTK_POLICY_AUTOMATIC,
					   GTK_POLICY_AUTOMATIC);
	    gtk_container_add(GTK_CONTAINER(text_window), scrolled);

	    textview = gtk_text_view_new();
	    gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
	    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(textview), FALSE);
	    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview),
					GTK_WRAP_WORD);
	    gtk_container_add(GTK_CONTAINER(scrolled), textview);

	    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

	    fp = fopen(filename, "r");
	    if (fp) {
		char buf[4096];
		size_t nread;
		GtkTextIter end;

		while ((nread = fread(buf, 1, sizeof(buf) - 1, fp)) > 0) {
		    buf[nread] = '\0';
		    gtk_text_buffer_get_end_iter(buffer, &end);
		    gtk_text_buffer_insert(buffer, &end, buf, (gint)nread);
		}
		fclose(fp);
		gtk_widget_show_all(text_window);
	    }
	}
	free(filename);
    }

    create_log_popup();
    log_popup_active = 0;
    gtk_entry_set_text(GTK_ENTRY(log_name_item), log_file_name);
    show_mode();
    ghflag = gvflag = visible = 1;
    show_signame = 16;
    mode_undo();
    dismiss_mode();
    if (read_log(log_file_name)) {
	gtk_widget_set_sensitive(pause_button, TRUE);
	log_review(1);
    }
}

/* file: helppan.c	G. Moody        1 May 1990
			Last revised:	2026
Help panel functions for WAVE (GTK 3 version)

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

/* On-line help files for WAVE are located in *(helpdir)/wave.  Their names are
   of the form *(helpdir)/wave/TOPIC.hlp, where TOPIC is one of the topics
   given in the help popup menu (possibly abbreviated).  The master help file
   is *(helpdir)/wave/HELPFILE. */
#define HELPFILE	"wave.hlp"

void find_user_guide(void)
{
    FILE *ifile;

    sprintf(url, "%s/html/wug/wug.htm", helpdir);
    if ((ifile = fopen(url, "r")) != NULL) fclose(ifile);
    else if ((ifile = fopen("wug.htm", "r")) != NULL) {
	fclose(ifile);
	sprintf(url, "%s/wug.htm", getcwd(NULL, 256));
    }
    else
	strcpy(url, "http://www.physionet.org/physiotools/wug/");
}

void find_faq(void)
{
    FILE *ifile;

    sprintf(url, "%s/html/wug/wave-faq.htm", helpdir);
    if ((ifile = fopen(url, "r")) != NULL) fclose(ifile);
    else if ((ifile = fopen("wave-faq.htm", "r")) != NULL) {
	fclose(ifile);
	sprintf(url, "%s/wave-faq.htm", getcwd(NULL, 256));
    }
    else
	strcpy(url, "http://www.physionet.org/physiotools/wug/wave-faq.htm");
}

void help(void)
{
    find_user_guide();
    fprintf(stderr, "WAVE version %s (%s)\n %s", WAVEVERSION, __DATE__,
	    wfdberror());
    fprintf(stderr, "usage: %s -r RECORD[+RECORD] [ options ]\n", pname);
    fprintf(stderr, "\nOptions are:\n");
    fprintf(stderr, " -a annotator-name  Open an annotation file\n");
    fprintf(stderr," -dpi XX[xYY]       Calibrate for XX [by YY] dots/inch\n");
    fprintf(stderr, " -f TIME            Open the record beginning at TIME\n");
    fprintf(stderr, " -g                 Use shades of grey only\n");
    fprintf(stderr, " -H                 Use high-resolution mode\n");
    fprintf(stderr, " -m                 Use black and white only\n");
    fprintf(stderr, " -O                 Use overlay graphics\n");
    fprintf(stderr, " -p PATH            Search for input files in PATH\n");
    fprintf(stderr, "                     (if not found in $WFDB)\n");
    fprintf(stderr, " -s SIGNAL [SIGNAL ...]  Initialize the signal list\n");
    fprintf(stderr, " -S                 Use a shared colormap\n");
    fprintf(stderr, " -Vx                Set initial display option x\n");
    if (getenv("DISPLAY") == NULL) {
	fprintf(stderr, "\n%s is an X11 client.  ", pname);
	fprintf(stderr, "You must specify the X server\n");
	fprintf(stderr, "connection for it in the DISPLAY environment ");
	fprintf(stderr, "variable.\n");
    }
    if (getenv("WFDB") == NULL) {
	fprintf(stderr, "\nBe sure to set the WFDB environment variable to\n");
	fprintf(stderr, "indicate a list of locations that contain\n");
	fprintf(stderr, "input files for %s.\n", pname);
    }
    fprintf(stderr, "\nFor more information, type `more %s/wave/%s',\n",
	    helpdir, HELPFILE);
    fprintf(stderr, "or open `%s' using\nyour web browser.\n", url);
    exit(1);
}

static GtkWidget *help_window;	/* help topics popup window */
static char *topic;

static void help_print(void)
{
    char *print_command;

    if (topic &&
	(print_command =
	 malloc(strlen(textprint) + strlen(helpdir) + strlen(topic) + 14))) {
	sprintf(print_command, "%s %s/wave/%s.hlp\n",
		textprint, helpdir, topic);
	do_command(print_command);
	free(print_command);
    }
}

/* Open the WAVE User's Guide in a web browser. */
static void help_user_guide_cb(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    find_user_guide();
    open_url();
}

/* Open the WAVE FAQ in a web browser. */
static void help_faq_cb(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    find_faq();
    open_url();
}

/* Load a help file into a GtkTextView and display it in a sub-window. */
static void show_help_topic(const char *topic_key, const char *topic_label)
{
    GtkWidget *subwindow, *vbox, *scrolled, *textview, *button;
    GtkTextBuffer *buffer;
    char *filename;
    FILE *fp;

    filename = malloc(strlen(helpdir) + strlen(topic_key) + 11);
    if (filename == NULL)
	return;
    /* strlen("/wave/") + strlen(".hlp") + 1 for trailing null = 11 */
    sprintf(filename, "%s/wave/%s.hlp", helpdir, topic_key);

    fp = fopen(filename, "r");
    if (fp == NULL) {
	GtkWidget *dialog = gtk_message_dialog_new(
	    GTK_WINDOW(help_window),
	    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	    GTK_MESSAGE_WARNING,
	    GTK_BUTTONS_OK,
	    "Sorry, help is not available for this topic.");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	free(filename);
	return;
    }

    /* Read entire file into a buffer. */
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);
    char *contents = malloc(fsize + 1);
    if (contents == NULL) {
	fclose(fp);
	free(filename);
	return;
    }
    size_t nread = fread(contents, 1, fsize, fp);
    contents[nread] = '\0';
    fclose(fp);

    /* Create a sub-window for this topic. */
    subwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(subwindow), topic_label);
    gtk_window_set_transient_for(GTK_WINDOW(subwindow),
				 GTK_WINDOW(help_window));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(subwindow), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(subwindow), 700, 500);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
    gtk_container_add(GTK_CONTAINER(subwindow), vbox);

    /* Print button. */
    button = gtk_button_new_with_label("Print");
    gtk_widget_set_tooltip_text(button, "Print this help topic");
    g_signal_connect_swapped(button, "clicked",
			     G_CALLBACK(help_print), NULL);
    gtk_widget_set_halign(button, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

    /* Scrolled text view with help content. */
    scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(textview), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(textview), 4);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(textview), 4);
    gtk_container_add(GTK_CONTAINER(scrolled), textview);

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    gtk_text_buffer_set_text(buffer, contents, -1);

    /* Scroll to the top. */
    GtkTextIter start;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_place_cursor(buffer, &start);

    gtk_widget_show_all(subwindow);
    gtk_window_present(GTK_WINDOW(subwindow));

    free(contents);
    free(filename);
}

/* Callback for help topic buttons. The topic key is passed as user data. */
static void help_select_cb(GtkWidget *widget, gpointer data)
{
    const char *key = (const char *)data;
    const char *label;

    (void)widget;

    /* Map key character to topic name. */
    switch (key[0]) {
      case 'a': topic = "analysis";  label = "Analysis";            break;
      case 'b': topic = "buttons";   label = "Buttons";             break;
      case 'e': topic = "editing";   label = "Annotation Editing";  break;
      case 'l': topic = "log";       label = "WAVE Logs";           break;
      case 'n': topic = "news";      label = "What's new";          break;
      case 'p': topic = "printing";  label = "Printing";            break;
      case 'r': topic = "resource";  label = "Resources";           break;
      default:
      case 'i': topic = "intro";     label = "Introduction";        break;
    }

    show_help_topic(topic, label);
}

/* Handle window close (delete-event) by hiding instead of destroying. */
static gboolean help_window_delete(GtkWidget *widget, GdkEvent *event,
				   gpointer data)
{
    (void)widget;
    (void)event;
    (void)data;
    dismiss_help();
    return TRUE;	/* prevent destruction */
}

/* Callback wrapper for the Dismiss button. */
static void dismiss_help_cb(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    dismiss_help();
}

/* Set up popup window for help. */
void create_help_popup(void)
{
    GtkWidget *vbox, *label_box, *label, *button;
    char version_str[128];

    /* Create the popup window. */
    help_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(help_window), "Help Topics");
    gtk_window_set_transient_for(GTK_WINDOW(help_window),
				 GTK_WINDOW(main_window));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(help_window), TRUE);
    g_signal_connect(help_window, "delete-event",
		     G_CALLBACK(help_window_delete), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);
    gtk_container_add(GTK_CONTAINER(help_window), vbox);

    /* Title labels: "WAVE" version and copyright. */
    label_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(vbox), label_box, FALSE, FALSE, 0);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>WAVE</b>");
    gtk_box_pack_start(GTK_BOX(label_box), label, FALSE, FALSE, 0);

    snprintf(version_str, sizeof(version_str), "<b>%s</b>", WAVEVERSION);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), version_str);
    gtk_box_pack_start(GTK_BOX(label_box), label, FALSE, FALSE, 0);

    label = gtk_label_new("Copyright \302\251 1990-2010 George B. Moody.");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    /* Help topic buttons. */
    button = gtk_button_new_with_label("Introduction");
    gtk_widget_set_tooltip_text(button, "Introduction to WAVE");
    g_signal_connect(button, "clicked", G_CALLBACK(help_select_cb), "i");
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("Buttons");
    gtk_widget_set_tooltip_text(button, "WAVE button reference");
    g_signal_connect(button, "clicked", G_CALLBACK(help_select_cb), "b");
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("Annotation Editing");
    gtk_widget_set_tooltip_text(button, "Annotation editing help");
    g_signal_connect(button, "clicked", G_CALLBACK(help_select_cb), "e");
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("WAVE Logs");
    gtk_widget_set_tooltip_text(button, "WAVE log file help");
    g_signal_connect(button, "clicked", G_CALLBACK(help_select_cb), "l");
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("Printing");
    gtk_widget_set_tooltip_text(button, "Printing help");
    g_signal_connect(button, "clicked", G_CALLBACK(help_select_cb), "p");
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("Analysis");
    gtk_widget_set_tooltip_text(button, "Analysis tools help");
    g_signal_connect(button, "clicked", G_CALLBACK(help_select_cb), "a");
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("Resources");
    gtk_widget_set_tooltip_text(button, "WAVE resources help");
    g_signal_connect(button, "clicked", G_CALLBACK(help_select_cb), "r");
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("What's new");
    gtk_widget_set_tooltip_text(button, "Recent changes in WAVE");
    g_signal_connect(button, "clicked", G_CALLBACK(help_select_cb), "n");
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("Frequently asked questions");
    gtk_widget_set_tooltip_text(button, "WAVE FAQ");
    g_signal_connect(button, "clicked", G_CALLBACK(help_faq_cb), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("User's Guide");
    gtk_widget_set_tooltip_text(button, "Open the WAVE User's Guide");
    g_signal_connect(button, "clicked", G_CALLBACK(help_user_guide_cb), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("Quit from Help");
    gtk_widget_set_tooltip_text(button, "Close this help window");
    g_signal_connect(button, "clicked", G_CALLBACK(dismiss_help_cb), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
}

int help_popup_active = -1;

/* Make the help popup window appear. */
void show_help(void)
{
    if (help_popup_active < 0) create_help_popup();
    gtk_widget_show_all(help_window);
    gtk_window_present(GTK_WINDOW(help_window));
    help_popup_active = 1;
}

/* Make the help popup window disappear. */
void dismiss_help(void)
{
    if (help_popup_active > 0) {
	gtk_widget_hide(help_window);
	help_popup_active = 0;
    }
}

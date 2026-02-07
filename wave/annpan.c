/* file: annpan.c	G. Moody	1 May 1990
			Last revised:	2026
Annotation template panel functions for WAVE (GTK 3 version)

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

static GtkWidget *ann_window;		/* annotation template popup window */
static GtkWidget *anntyp_item;		/* type combo box */
static GtkWidget *aux_item;		/* aux text entry */
static GtkWidget *subtyp_item;		/* subtype spin button */
static GtkWidget *chan_item;		/* chan spin button */
static GtkWidget *num_item;		/* num spin button */

int ann_popup_active = -1;

/* Make the annotation template popup window disappear. */
static void dismiss_ann_template(void)
{
    if (ann_popup_active > 0) {
	gtk_widget_hide(ann_window);
	ann_popup_active = 0;
    }
}

/* Callback wrapper for the Dismiss button. */
static void dismiss_ann_template_cb(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    dismiss_ann_template();
}

/* Handle window close (delete-event) by hiding instead of destroying. */
static gboolean ann_window_delete(GtkWidget *widget, GdkEvent *event,
				  gpointer data)
{
    (void)widget;
    (void)event;
    (void)data;
    dismiss_ann_template();
    return TRUE;	/* prevent destruction */
}

/* Update ann_template when the type combo box changes. */
static void anntyp_changed(GtkComboBoxText *combo, gpointer data)
{
    (void)data;
    ann_template.anntyp = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
}

/* Update ann_template when the aux entry changes. */
static void aux_changed(GtkEntry *entry, gpointer data)
{
    static char tmp_aux[257];

    (void)data;
    const char *text = gtk_entry_get_text(entry);
    strncpy(tmp_aux + 1, text, 255);
    tmp_aux[256] = '\0';
    if ((tmp_aux[0] = (char)strlen(tmp_aux + 1)) != 0)
	ann_template.aux = tmp_aux;
    else
	ann_template.aux = NULL;
}

/* Update ann_template when the subtype spin button changes. */
static void subtyp_changed(GtkSpinButton *spin, gpointer data)
{
    (void)data;
    ann_template.subtyp = gtk_spin_button_get_value_as_int(spin);
}

/* Update ann_template when the chan spin button changes. */
static void chan_changed(GtkSpinButton *spin, gpointer data)
{
    (void)data;
    ann_template.chan = gtk_spin_button_get_value_as_int(spin);
}

/* Update ann_template when the num spin button changes. */
static void num_changed(GtkSpinButton *spin, gpointer data)
{
    (void)data;
    ann_template.num = gtk_spin_button_get_value_as_int(spin);
}

/* Callback wrapper for the "Change all in range" button. */
static void change_annotations_cb(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    change_annotations();
}

static char *mstr[54];

static void create_mstr_array(void)
{
    char *dp, mbuf[5], *mp;
    int i;
    unsigned int l, lm;

    mstr[0] = ".    (Deleted annotation)";
    for (i = 1; i <= ACMAX; i++) {
	if ((mp = annstr(i)) == NULL) {
	    snprintf(mbuf, sizeof(mbuf), "[%d]", i);
	    mp = mbuf;
	}
	if ((dp = anndesc(i)) == NULL)
	    dp = "(unassigned annotation type)";
	lm = strlen(mp);
	l = strlen(dp) + 6;
	if (lm > 4) l += lm - 4;
	if ((mstr[i] = (char *)malloc(l)) == NULL)
	    break;
	strcpy(mstr[i], mp);
	do {
	    mstr[i][lm] = ' ';
	} while (++lm < 5);
	strcpy(mstr[i] + lm, dp);
    }
    mstr[ACMAX+1] = ":    (Index mark)";
    mstr[ACMAX+2] = "<    (Start of analysis)";
    mstr[ACMAX+3] = ">    (End of analysis)";
    mstr[ACMAX+4] = ";    (Reference mark)";
}

/* Set up popup window for defining an annotation template. */
static void create_ann_template_popup(void)
{
    GtkWidget *grid, *label, *button;
    int i, row;

    create_mstr_array();
    ann_template.anntyp = 1;

    /* Create the popup window. */
    ann_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(ann_window), "Annotation Template");
    gtk_window_set_transient_for(GTK_WINDOW(ann_window),
				 GTK_WINDOW(main_window));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(ann_window), TRUE);
    g_signal_connect(ann_window, "delete-event",
		     G_CALLBACK(ann_window_delete), NULL);

    /* Use a grid for layout. */
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 8);
    gtk_container_add(GTK_CONTAINER(ann_window), grid);

    row = 0;

    /* Type: combo box with 54 annotation type entries. */
    label = gtk_label_new("Type:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

    anntyp_item = gtk_combo_box_text_new();
    gtk_widget_set_tooltip_text(anntyp_item,
	"Select the annotation type for the template");
    for (i = 0; i < 54; i++) {
	if (mstr[i])
	    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(anntyp_item),
					   mstr[i]);
	else
	    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(anntyp_item),
					   "(undefined)");
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(anntyp_item), 1);
    g_signal_connect(anntyp_item, "changed",
		     G_CALLBACK(anntyp_changed), NULL);
    gtk_widget_set_hexpand(anntyp_item, TRUE);
    gtk_grid_attach(GTK_GRID(grid), anntyp_item, 1, row, 1, 1);
    row++;

    /* Text: entry for aux string. */
    label = gtk_label_new("Text:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

    aux_item = gtk_entry_new();
    gtk_widget_set_tooltip_text(aux_item,
	"Enter auxiliary text for the annotation template");
    gtk_entry_set_max_length(GTK_ENTRY(aux_item), 255);
    gtk_entry_set_width_chars(GTK_ENTRY(aux_item), 20);
    g_signal_connect(aux_item, "changed",
		     G_CALLBACK(aux_changed), NULL);
    gtk_widget_set_hexpand(aux_item, TRUE);
    gtk_grid_attach(GTK_GRID(grid), aux_item, 1, row, 1, 1);
    row++;

    /* Subtype: spin button, range -128 to 127. */
    label = gtk_label_new("Subtype:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

    subtyp_item = gtk_spin_button_new_with_range(-128, 127, 1);
    gtk_widget_set_tooltip_text(subtyp_item,
	"Set the annotation subtype field");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(subtyp_item), 0);
    g_signal_connect(subtyp_item, "value-changed",
		     G_CALLBACK(subtyp_changed), NULL);
    gtk_grid_attach(GTK_GRID(grid), subtyp_item, 1, row, 1, 1);
    row++;

    /* Chan: spin button, range -128 to 127. */
    label = gtk_label_new("'Chan' field:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

    chan_item = gtk_spin_button_new_with_range(-128, 127, 1);
    gtk_widget_set_tooltip_text(chan_item,
	"Set the annotation 'chan' field");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(chan_item), 0);
    g_signal_connect(chan_item, "value-changed",
		     G_CALLBACK(chan_changed), NULL);
    gtk_grid_attach(GTK_GRID(grid), chan_item, 1, row, 1, 1);
    row++;

    /* Num: spin button, range -128 to 127. */
    label = gtk_label_new("'Num' field:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

    num_item = gtk_spin_button_new_with_range(-128, 127, 1);
    gtk_widget_set_tooltip_text(num_item,
	"Set the annotation 'num' field");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(num_item), 0);
    g_signal_connect(num_item, "value-changed",
		     G_CALLBACK(num_changed), NULL);
    gtk_grid_attach(GTK_GRID(grid), num_item, 1, row, 1, 1);
    row++;

    /* "Change all in range" button. */
    button = gtk_button_new_with_label("Change all in range");
    gtk_widget_set_tooltip_text(button,
	"Change all annotations in the selected range to match the template");
    g_signal_connect(button, "clicked",
		     G_CALLBACK(change_annotations_cb), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 0, row, 1, 1);

    /* Dismiss button. */
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_tooltip_text(button,
	"Hide the annotation template window");
    g_signal_connect(button, "clicked",
		     G_CALLBACK(dismiss_ann_template_cb), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 1, row, 1, 1);
}

/* Make the annotation template popup window appear. */
void show_ann_template(void)
{
    if (ann_popup_active < 0) create_ann_template_popup();
    gtk_widget_show_all(ann_window);
    gtk_window_present(GTK_WINDOW(ann_window));
    ann_popup_active = 1;
}

/* Set the annotation template window 'Type' item. */
void set_anntyp(int i)
{
    if (ann_popup_active < 0) create_ann_template_popup();
    gtk_combo_box_set_active(GTK_COMBO_BOX(anntyp_item), i);
}

/* Set the annotation template window 'Aux' item. */
void set_ann_aux(char *s)
{
    if (ann_popup_active >= 0)
	gtk_entry_set_text(GTK_ENTRY(aux_item), s ? s + 1 : "");
}

/* Set the annotation template window 'Subtype' item. */
void set_ann_subtyp(int i)
{
    if (ann_popup_active >= 0)
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(subtyp_item), i);
}

/* Set the annotation template window 'Chan' item. */
void set_ann_chan(int i)
{
    if (ann_popup_active >= 0)
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(chan_item), i);
}

/* Set the annotation template window 'Num' item. */
void set_ann_num(int i)
{
    if (ann_popup_active >= 0)
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(num_item), i);
}

/* file: search.c	G. Moody	17 October 1996
			Last revised:	2026
Search template functions for WAVE (GTK 3 version)

-------------------------------------------------------------------------------
WAVE: Waveform analyzer, viewer, and editor
Copyright (C) 1996-2010 George B. Moody

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

static GtkWidget *search_window;
static GtkWidget *s_anntyp_item, *s_subtyp_item, *s_num_item,
		 *s_chan_item, *s_aux_item;
static GtkWidget *s_anntyp_mask, *s_subtyp_mask, *s_num_mask,
		 *s_chan_mask, *s_aux_mask;

int search_popup_active = -1;

/* Make the search template popup window disappear. */
static void dismiss_search_template(void)
{
    if (search_popup_active > 0) {
	gtk_widget_hide(search_window);
	search_popup_active = 0;
    }
}

/* Callback for the dismiss button. */
static void dismiss_search_template_cb(GtkButton *button, gpointer data)
{
    (void)button; (void)data;
    dismiss_search_template();
}

/* Handle window close (delete-event) by hiding instead of destroying. */
static gboolean search_window_delete(GtkWidget *widget, GdkEvent *event,
				     gpointer data)
{
    (void)widget; (void)event; (void)data;
    dismiss_search_template();
    return TRUE;	/* prevent destruction */
}

/* Callback for mask combo boxes (Ignore/Match). */
static void mask_changed_cb(GtkComboBox *combo, gpointer data)
{
    int bit = GPOINTER_TO_INT(data);
    int active = gtk_combo_box_get_active(combo);

    if (active == 1)
	search_mask |= bit;
    else
	search_mask &= ~bit;
}

/* Callback for the annotation type combo. */
static void anntyp_changed_cb(GtkComboBox *combo, gpointer data)
{
    (void)data;
    search_template.anntyp = gtk_combo_box_get_active(combo);
}

/* Callback for subtyp spin button. */
static void subtyp_changed_cb(GtkSpinButton *spin, gpointer data)
{
    (void)data;
    search_template.subtyp = gtk_spin_button_get_value_as_int(spin);
}

/* Callback for chan spin button. */
static void chan_changed_cb(GtkSpinButton *spin, gpointer data)
{
    (void)data;
    search_template.chan = gtk_spin_button_get_value_as_int(spin);
}

/* Callback for num spin button. */
static void num_changed_cb(GtkSpinButton *spin, gpointer data)
{
    (void)data;
    search_template.num = gtk_spin_button_get_value_as_int(spin);
}

/* Callback for aux text entry. */
static void aux_changed_cb(GtkEditable *editable, gpointer data)
{
    static char tmp_aux[257];

    (void)data;
    const char *text = gtk_entry_get_text(GTK_ENTRY(editable));
    strncpy(tmp_aux + 1, text, 255);
    tmp_aux[256] = '\0';
    if ((tmp_aux[0] = (char)strlen(tmp_aux + 1)))
	search_template.aux = tmp_aux;
    else
	search_template.aux = NULL;
}

static char *mstr[53];

static void s_create_mstr_array(void)
{
    char *dp, mbuf[5], *mp;
    int i;
    unsigned int l, lm;

    mstr[0] = ".    (Deleted annotation)";
    for (i = 1; i <= ACMAX; i++) {
	if ((mp = annstr(i)) == NULL) {
	    sprintf(mbuf, "[%d]", i);
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
	strcpy(mstr[i]+lm, dp);
    }
    mstr[ACMAX+1] = ":    (Index mark)";
    mstr[ACMAX+2] = "<    (Start of analysis)";
    mstr[ACMAX+3] = ">    (End of analysis)";
}

/* Set the search template to match the currently selected annotation. */
static void match_selected_annotation_cb(GtkButton *button, gpointer data)
{
    (void)button; (void)data;

    if (attached == NULL) return;

    gtk_combo_box_set_active(GTK_COMBO_BOX(s_anntyp_item),
			     attached->this.anntyp);
    gtk_entry_set_text(GTK_ENTRY(s_aux_item),
		       attached->this.aux ?
		       (const char *)(attached->this.aux + 1) : "");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(s_subtyp_item),
			      attached->this.subtyp);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(s_chan_item),
			      attached->this.chan);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(s_num_item),
			      attached->this.num);

    gtk_combo_box_set_active(GTK_COMBO_BOX(s_anntyp_mask), 1);
    gtk_combo_box_set_active(GTK_COMBO_BOX(s_aux_mask), 1);
    gtk_combo_box_set_active(GTK_COMBO_BOX(s_subtyp_mask), 1);
    gtk_combo_box_set_active(GTK_COMBO_BOX(s_chan_mask), 1);
    gtk_combo_box_set_active(GTK_COMBO_BOX(s_num_mask), 1);

    search_template = attached->this;
    search_mask = M_ANNTYP | M_SUBTYP | M_CHAN | M_NUM;
    if (search_template.aux) search_mask |= M_AUX;
}

/* Create a GtkComboBoxText with "Ignore" and "Match" entries. */
static GtkWidget *create_mask_combo(int mask_bit)
{
    GtkWidget *combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Ignore");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Match");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    g_signal_connect(combo, "changed", G_CALLBACK(mask_changed_cb),
		     GINT_TO_POINTER(mask_bit));
    return combo;
}

/* Set up popup window for defining the search template. */
static void create_search_template_popup(void)
{
    GtkWidget *grid, *label, *button, *bbox;
    int i, row;

    s_create_mstr_array();
    search_template.anntyp = 1;

    search_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(search_window), "Search Template");
    gtk_window_set_transient_for(GTK_WINDOW(search_window),
				 GTK_WINDOW(main_window));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(search_window), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(search_window), 6);
    g_signal_connect(search_window, "delete-event",
		     G_CALLBACK(search_window_delete), NULL);

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
    gtk_container_add(GTK_CONTAINER(search_window), grid);
    row = 0;

    /* Type: mask combo + annotation type combo */
    s_anntyp_mask = create_mask_combo(M_ANNTYP);
    gtk_grid_attach(GTK_GRID(grid), s_anntyp_mask, 0, row, 1, 1);

    label = gtk_label_new("Type:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 1, row, 1, 1);

    s_anntyp_item = gtk_combo_box_text_new();
    for (i = 0; i < 53; i++) {
	if (mstr[i])
	    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(s_anntyp_item),
					   mstr[i]);
	else
	    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(s_anntyp_item),
					   "");
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(s_anntyp_item), 1);
    g_signal_connect(s_anntyp_item, "changed",
		     G_CALLBACK(anntyp_changed_cb), NULL);
    gtk_grid_attach(GTK_GRID(grid), s_anntyp_item, 2, row, 1, 1);
    row++;

    /* Text (aux): mask combo + text entry */
    s_aux_mask = create_mask_combo(M_AUX);
    gtk_grid_attach(GTK_GRID(grid), s_aux_mask, 0, row, 1, 1);

    label = gtk_label_new("Text:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 1, row, 1, 1);

    s_aux_item = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(s_aux_item), 255);
    gtk_entry_set_width_chars(GTK_ENTRY(s_aux_item), 20);
    gtk_entry_set_text(GTK_ENTRY(s_aux_item), "");
    g_signal_connect(s_aux_item, "changed",
		     G_CALLBACK(aux_changed_cb), NULL);
    gtk_grid_attach(GTK_GRID(grid), s_aux_item, 2, row, 1, 1);
    row++;

    /* Subtype: mask combo + spin button */
    s_subtyp_mask = create_mask_combo(M_SUBTYP);
    gtk_grid_attach(GTK_GRID(grid), s_subtyp_mask, 0, row, 1, 1);

    label = gtk_label_new("Subtype:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 1, row, 1, 1);

    s_subtyp_item = gtk_spin_button_new_with_range(-128, 127, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(s_subtyp_item), 0);
    g_signal_connect(s_subtyp_item, "value-changed",
		     G_CALLBACK(subtyp_changed_cb), NULL);
    gtk_grid_attach(GTK_GRID(grid), s_subtyp_item, 2, row, 1, 1);
    row++;

    /* Chan: mask combo + spin button */
    s_chan_mask = create_mask_combo(M_CHAN);
    gtk_grid_attach(GTK_GRID(grid), s_chan_mask, 0, row, 1, 1);

    label = gtk_label_new("'Chan' field:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 1, row, 1, 1);

    s_chan_item = gtk_spin_button_new_with_range(-128, 127, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(s_chan_item), 0);
    g_signal_connect(s_chan_item, "value-changed",
		     G_CALLBACK(chan_changed_cb), NULL);
    gtk_grid_attach(GTK_GRID(grid), s_chan_item, 2, row, 1, 1);
    row++;

    /* Num: mask combo + spin button */
    s_num_mask = create_mask_combo(M_NUM);
    gtk_grid_attach(GTK_GRID(grid), s_num_mask, 0, row, 1, 1);

    label = gtk_label_new("'Num' field:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 1, row, 1, 1);

    s_num_item = gtk_spin_button_new_with_range(-128, 127, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(s_num_item), 0);
    g_signal_connect(s_num_item, "value-changed",
		     G_CALLBACK(num_changed_cb), NULL);
    gtk_grid_attach(GTK_GRID(grid), s_num_item, 2, row, 1, 1);
    row++;

    /* Button row */
    bbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_START);
    gtk_box_set_spacing(GTK_BOX(bbox), 6);
    gtk_grid_attach(GTK_GRID(grid), bbox, 0, row, 3, 1);

    button = gtk_button_new_with_label("Match selected annotation");
    g_signal_connect(button, "clicked",
		     G_CALLBACK(match_selected_annotation_cb), NULL);
    gtk_container_add(GTK_CONTAINER(bbox), button);

    button = gtk_button_new_with_label("Dismiss");
    g_signal_connect(button, "clicked",
		     G_CALLBACK(dismiss_search_template_cb), NULL);
    gtk_container_add(GTK_CONTAINER(bbox), button);
}

/* Make the search template popup window appear. */
void show_search_template(void)
{
    if (search_popup_active < 0) create_search_template_popup();
    gtk_widget_show_all(search_window);
    gtk_window_present(GTK_WINDOW(search_window));
    search_popup_active = 1;
    set_find_item("");
}

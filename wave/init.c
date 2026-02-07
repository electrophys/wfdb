/* file: init.c		G. Moody	 1 May 1990
			Last revised:	2026
Initialization functions for WAVE

-------------------------------------------------------------------------------
WAVE: Waveform analyzer, viewer, and editor
Copyright (C) 1990-2013 George B. Moody

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

static WFDB_Siginfo *df;
static int maxnsig;

static void memerr(void)
{
    GtkWidget *dialog = gtk_message_dialog_new(
	GTK_WINDOW(main_window),
	GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
	"Insufficient memory");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void alloc_sigdata(int ns)
{
    int i;

    if ((df = realloc(df, ns * sizeof(WFDB_Siginfo))) == NULL ||
	(signame = realloc(signame, ns * sizeof(char *))) == NULL ||
	(sigunits = realloc(sigunits, ns * sizeof(char *))) == NULL ||
	(calibrated = realloc(calibrated, ns * sizeof(char))) == NULL ||
	(scope_v = realloc(scope_v, ns * sizeof(WFDB_Sample))) == NULL ||
	(vref = realloc(vref, ns * sizeof(WFDB_Sample))) == NULL ||
	(level_v = realloc(level_v, ns * sizeof(WFDB_Sample))) == NULL ||
	(v = realloc(v, ns * sizeof(WFDB_Sample))) == NULL ||
	(v0 = realloc(v0, ns * sizeof(WFDB_Sample))) == NULL ||
	(vmax = realloc(vmax, ns * sizeof(WFDB_Sample))) == NULL ||
	(vmin = realloc(vmin, ns * sizeof(WFDB_Sample))) == NULL ||
	(vvalid = realloc(vvalid, ns * sizeof(int))) == NULL ||
	(level_name_string =
		realloc(level_name_string, ns * sizeof(char **))) == NULL ||
	(level_value_string =
		realloc(level_value_string, ns * sizeof(char **))) == NULL ||
	(level_units_string =
		realloc(level_units_string, ns * sizeof(char **))) == NULL ||
	(vscale = realloc(vscale, ns * sizeof(double))) == NULL ||
	(vmag = realloc(vmag, ns * sizeof(double))) == NULL ||
	(dc_coupled = realloc(dc_coupled, ns * sizeof(int))) == NULL ||
	(sigbase = realloc(sigbase, ns * sizeof(int))) == NULL ||
	(blabel = realloc(blabel, ns * sizeof(char *))) == NULL ||
	(level_name =
		realloc(level_name, ns * sizeof(GtkWidget *))) == NULL ||
	(level_value =
		realloc(level_value, ns * sizeof(GtkWidget *))) == NULL ||
	(level_units =
		realloc(level_units, ns * sizeof(GtkWidget *))) == NULL) {
	memerr();
    }
    for (i = maxnsig; i < ns; i++) {
	signame[i] = sigunits[i] = blabel[i] = NULL;
	level_name[i] = level_value[i] = level_units[i] = NULL;
	dc_coupled[i] = scope_v[i] = vref[i] = level_v[i] = v[i] = v0[i] =
	    vmax[i] = vmin[i] = 0;
	vscale[i] = vmag[i] = 1.0;
	if ((level_name_string[i] = calloc(1, 12)) == NULL ||
	    (level_value_string[i] = calloc(1, 12)) == NULL ||
	    (level_units_string[i] = calloc(1, 12)) == NULL) {
	    memerr();
	}
    }
    maxnsig = ns;
}

/* Open up a new ECG record. */
int record_init(char *s)
{
    char ts[RNLMAX+30];
    int i, rebuild_list, tl;

    /* Suppress error messages from the WFDB library. */
    wfdbquiet();

    /* Do nothing if the current annotation list has been edited and the
       changes can't be saved. */
    if (post_changes() == 0)
	return (0);

    /* Check to see if the signal list must be rebuilt. */
    if (freeze_siglist)
	freeze_siglist = rebuild_list = 0;
    else
	rebuild_list = (siglistlen == 0) | strcmp(record, s);

    /* Save the name of the new record in local storage. */
    strncpy(record, s, RNLMAX);

    /* Reset the frame title. */
    set_frame_title();

    /* Open as many signals as possible. */
    nsig = isigopen(record, NULL, 0);
    if (nsig > maxnsig)
	alloc_sigdata(nsig);
    nsig = isigopen(record, df, nsig);
    atimeres = getspf();
    if (nsig < 0 || (freq = sampfreq(NULL)) <= 0.) freq = WFDB_DEFFREQ;
    setifreq(freq);
    if ((getgvmode() & WFDB_HIGHRES) == 0) setafreq(0.);

    /* Quit if isigopen failed. */
    if (nsig < 0) {
	sprintf(ts, "Record %s is unavailable\n", record);
	GtkWidget *dialog = gtk_message_dialog_new(
	    GTK_WINDOW(main_window),
	    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	    GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
	    "%s", ts);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return (0);
    }

    /* If the record has a low sampling rate, use coarse time scale. */
    if (freq <= 10.0) {
	tsa_index = coarse_tsa_index;
	grid_mode = coarse_grid_mode;
	mode_undo();
	set_modes();
    }
    else {
	tsa_index = fine_tsa_index;
	grid_mode = fine_grid_mode;
	mode_undo();
	set_modes();
    }

    /* Set signal name pointers. */
    sprintf(ts, "record %s, ", record);
    tl = strlen(ts);
    for (i = 0; i < nsig; i++) {
	if (strncmp(df[i].desc, ts, tl) == 0)
	    signame[i] = df[i].desc + tl;
	else
	    signame[i] = df[i].desc;
	if (df[i].units == NULL || *df[i].units == '\0')
	    sigunits[i] = "mV";
	else
	    sigunits[i] = df[i].units;
	if (df[i].gain == 0) {
	    calibrated[i] = 0;
	    df[i].gain = WFDB_DEFGAIN;
	}
	else
	    calibrated[i] = 1;
    }

    /* Set range for signal selection on analyze panel. */
    reset_maxsig();

    /* Initialize the signal list unless the new record name matches the
       old one. */
    if (rebuild_list) {
	if (nsig > maxsiglistlen) {
	    siglist = realloc(siglist, nsig * sizeof(int));
	    base = realloc(base, nsig * sizeof(int));
	    level = realloc(level, nsig * sizeof(WaveSegment));
	    maxsiglistlen = nsig;
	}
	for (i = 0; i < nsig; i++)
	    siglist[i] = i;
	siglistlen = nsig;
	reset_siglist();
    }

    /* Calculate the base levels for each signal. */
    set_baselines();
    tmag = 1.0;
    vscale[0] = 0.;
    calibrate();

    /* Rebuild the level window (see edit.c) */
    recreate_level_popup();
    return (1);
}

void set_baselines(void)
{
    int i;

    if (sig_mode == 0)
	for (i = 0; i < nsig; i++)
	    base[i] = canvas_height*(2*i+1.)/(2.*nsig);
    else
	for (i = 0; i < siglistlen; i++)
	    base[i] = canvas_height*(2*i+1.)/(2.*siglistlen);
    if (i > 1)
	abase = (base[i/2] + base[i/2-1])/2;
    else if (nsig > 0)
	abase = canvas_height*4/5;
    else
	abase = canvas_height/2;
}

void calibrate(void)
{
    int i;
    struct WFDB_calinfo ci;

    if (vscale[0] == 0.0) {
	clear_cache();

	if ((cfname == (char *)NULL) && (cfname = getenv("WFDBCAL")))
	    calopen(cfname);

	for (i = 0; i < nsig; i++) {
	    vscale[i] = - vmag[i] * millivolts(1) / df[i].gain;
	    dc_coupled[i] = 0;
	    if (getcal(df[i].desc, df[i].units, &ci) == 0 && ci.scale != 0.0) {
		vscale[i] /= ci.scale;
		if (ci.caltype & 1) {
		    dc_coupled[i] = 1;
		    sigbase[i] = df[i].baseline;
		    if ((blabel[i] = (char *)malloc(strlen(ci.units) +
						   strlen(df[i].desc) + 6)))
			sprintf(blabel[i], "0 %s (%s)", ci.units, df[i].desc);
		}
	    }
	}
    }
    vscalea = - millivolts(1);
    if (af.name && getcal(af.name, "units", &ci) == 0 && ci.scale != 0)
	vscalea /= ci.scale;
    else if (getcal("ann", "units", &ci) == 0 && ci.scale != 0)
	vscalea /= ci.scale;
    else
	vscalea /= WFDB_DEFGAIN;

    if (freq == 0.0) freq = WFDB_DEFFREQ;
    if (tmag <= 0.0) tmag = 1.0;
    nsamp = canvas_width_sec * freq / tmag;
    tscale = tmag * seconds(1) / freq;
}

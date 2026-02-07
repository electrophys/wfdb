/* file: sig.c		G. Moody	 27 April 1990
			Last revised:	 2026
Signal display functions for WAVE

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

static void show_signal_names(cairo_t *cr);
static void show_signal_baselines(cairo_t *cr, struct display_list *lp);

/* Show_display_list() plots the display list pointed to by its argument. */

static struct display_list *lp_current;
static int highlighted = -1;

int in_siglist(int i)
{
    int nsl;

    for (nsl = 0; nsl < siglistlen; nsl++)
	if (siglist[nsl] == i) return (1);
    return (0);
}

/* Draw a signal trace from absolute-coordinate WavePoint array.
   ybase is added to each y value.  Segments with WFDB_INVALID_SAMPLE
   are skipped (gaps in the trace). */
static void drawtrace(cairo_t *cr, WavePoint *b, int n, int ybase,
		      WaveColorIndex color)
{
    int j;
    WavePoint *p, *q;

    if (ybase == -9999) return;

    wave_set_color(cr, color);
    cairo_set_line_width(cr, 1.0);

    for (j = 0, p = q = b; j <= n; j++) {
	if (j == n || q->y == WFDB_INVALID_SAMPLE) {
	    if (p < q) {
		/* Skip leading invalid samples */
		while (p < q && p->y == WFDB_INVALID_SAMPLE)
		    p++;
		if (p < q) {
		    cairo_move_to(cr, p->x + 0.5, p->y + ybase + 0.5);
		    for (WavePoint *r = p + 1; r < q; r++) {
			if (r->y != WFDB_INVALID_SAMPLE)
			    cairo_line_to(cr, r->x + 0.5, r->y + ybase + 0.5);
		    }
		    cairo_stroke(cr);
		}
	    }
	    p = ++q;
	}
	else
	    ++q;
    }
}

static void show_display_list(cairo_t *cr, struct display_list *lp)
{
    int i, j;

    lp_current = lp;
    if (!lp) return;
    if (sig_mode == 0)
	for (i = 0; i < nsig; i++) {
	    if (lp->vlist[i])
		drawtrace(cr, lp->vlist[i], lp->ndpts, base[i],
			  WAVE_COLOR_SIGNAL);
	}
    else if (sig_mode == 1)
	for (i = 0; i < siglistlen; i++) {
	    if (0 <= siglist[i] && siglist[i] < nsig &&
		lp->vlist[siglist[i]])
		drawtrace(cr, lp->vlist[siglist[i]], lp->ndpts, base[i],
			  WAVE_COLOR_SIGNAL);
	}
    else {	/* sig_mode == 2 (show valid signals only) */
	int nvsig;
	for (i = nvsig = 0; i < nsig; i++)
	    if (lp->vlist[i] && vvalid[i]) nvsig++;
	for (i = j = 0; i < nsig; i++) {
	    if (lp->vlist[i] && vvalid[i]) {
		base[i] = canvas_height*(2*(j++)+1.)/(2.*nvsig);
		drawtrace(cr, lp->vlist[i], lp->ndpts, base[i],
			  WAVE_COLOR_SIGNAL);
	    }
	    else
		base[i] = -9999;
	}
    }
    highlighted = -1;
}

void sig_highlight(int i)
{
    cairo_t *cr;

    if (!lp_current) return;
    cr = wave_begin_paint();

    if (0 <= highlighted && highlighted < lp_current->nsig) {
	if (sig_mode != 1) {
	    /* Erase old highlight by redrawing in signal color */
	    drawtrace(cr, lp_current->vlist[highlighted], lp_current->ndpts,
		      base[highlighted], WAVE_COLOR_SIGNAL);
	}
	else {
	    int j;
	    for (j = 0; j < siglistlen; j++) {
		if (siglist[j] == highlighted) {
		    drawtrace(cr, lp_current->vlist[highlighted],
			      lp_current->ndpts, base[j], WAVE_COLOR_SIGNAL);
		}
	    }
	}
    }
    highlighted = i;
    if (0 <= highlighted && highlighted < lp_current->nsig) {
	if (sig_mode != 1) {
	    drawtrace(cr, lp_current->vlist[highlighted], lp_current->ndpts,
		      base[highlighted], WAVE_COLOR_HIGHLIGHT);
	}
	else {
	    int j;
	    for (j = 0; j < siglistlen; j++) {
		if (siglist[j] == highlighted) {
		    drawtrace(cr, lp_current->vlist[highlighted],
			      lp_current->ndpts, base[j],
			      WAVE_COLOR_HIGHLIGHT);
		}
	    }
	}
    }

    wave_end_paint(cr);
    wave_refresh();
}

/* Do_disp() executes a display request.  The display will show nsamp samples
of nsig signals, starting at display_start_time. */

void do_disp(void)
{
    char *tp;
    int x0, x1, y0;
    struct display_list *lp;
    cairo_t *cr;

    wave_set_busy(1);

    cr = wave_begin_paint();

    /* Clear the osb */
    wave_fill_rect(cr, WAVE_COLOR_BACKGROUND, 0, 0,
		   canvas_width + mmx(10), canvas_height);

    /* Show the grid if requested. */
    show_grid();

    /* Make sure that the requested time is reasonable. */
    if (display_start_time < 0) display_start_time = 0;

    /* Update the panel items that indicate the start and end times. */
    tp = wtimstr(display_start_time);
    set_start_time(tp);
    while (*tp == ' ') tp++;
    y0 = canvas_height - mmy(2);
    x0 = mmx(2);
    wave_draw_string(cr, time_mode == 1 ? WAVE_COLOR_ANNOTATION
					: WAVE_COLOR_SIGNAL,
		     x0, y0, tp);
    tp = wtimstr(display_start_time + nsamp);
    set_end_time(tp);
    while (*tp == ' ') tp++;
    x1 = canvas_width - wave_text_width(tp) - mmx(2);
    wave_draw_string(cr, time_mode == 1 ? WAVE_COLOR_ANNOTATION
					: WAVE_COLOR_SIGNAL,
		     x1, y0, tp);

    /* Show the annotations, if available. */
    show_annotations(display_start_time, nsamp);

    /* Get a display list for the requested screen, and show it. */
    lp = find_display_list(display_start_time);
    show_display_list(cr, lp);

    /* If requested, show the signal names. */
    if (show_signame) show_signal_names(cr);

    /* If requested, show the signal baselines. */
    if (show_baseline) show_signal_baselines(cr, lp);

    wave_end_paint(cr);

    wave_set_busy(0);
    wave_refresh();
}

static int nlists;
static int vlist_size;

/* Get_display_list() obtains storage for display lists from the heap (via
malloc) or by recycling a display list in the cache. */

static struct display_list *get_display_list(void)
{
    int i, maxx, x;
    static int max_nlists = MAX_DISPLAY_LISTS;
    struct display_list *lp = NULL, *lpl;

    if (nlists < max_nlists &&
	(lp = (struct display_list *)calloc(1, sizeof(struct display_list))))
	    nlists++;
    if (lp == NULL)
	switch (nlists) {
	    case 0: return (NULL);
	    case 1: lp = first_list; break;
	    default: lpl = first_list; lp = lpl->next;
		while (lp->next) {
		    lpl = lp;
		    lp = lp->next;
		}
		lpl->next = NULL;
		break;
	}
    if (lp->nsig < nsig) {
	lp->sb = realloc(lp->sb, nsig * sizeof(int));
	lp->vlist = realloc(lp->vlist, nsig * sizeof(WavePoint *));
	for (i = lp->nsig; i < nsig; i++) {
	    lp->sb[i] = 0;
	    lp->vlist[i] = NULL;
	}
	lp->nsig = nsig;
    }

    if (canvas_width > vlist_size) vlist_size = canvas_width;

    for (i = 0; i < nsig; i++) {
	if (lp->vlist[i] == NULL) {
	    if ((lp->vlist[i] =
		 (WavePoint *)calloc(vlist_size, sizeof(WavePoint)))==NULL) {
		while (--i >= 0)
		    free(lp->vlist[i]);
		free(lp);
		max_nlists = --nlists;
		return (get_display_list());
	    }
	}

	/* Set the x-coordinates as absolute pixel values. */
	if (nsamp > canvas_width) {
	    x = maxx = canvas_width - 1;
	    for (x = 0; x <= maxx; x++)
		lp->vlist[i][x].x = x;
	}
	else {
	    maxx = nsamp - 1;
	    for (x = 0; x < vlist_size; x++)
		lp->vlist[i][x].x = (int)(x * tscale);
	}
    }
    lp->next = first_list;
    lp->npoints = nsamp;
    lp->xmax = (nsamp > canvas_width) ? canvas_width - 1 : nsamp - 1;
    return (first_list = lp);
}

/* Find_display_list() obtains a display list beginning at the sample number
specified by its argument.  Coordinates are stored as absolute values. */

struct display_list *find_display_list(WFDB_Time fdl_time)
{
    int c, i, j, x, x0, y, ymax, ymin;
    struct display_list *lp;
    WavePoint *tp;

    if (fdl_time < 0L) fdl_time = -fdl_time;
    /* If the requested display list is in the cache, return it at once. */
    for (lp = first_list; lp; lp = lp->next)
	if (lp->start == fdl_time && lp->npoints == nsamp) return (lp);

    /* Give up if we can't skip to the requested segment, or if we
       can't read at least one sample. */
    if ((fdl_time != strtim("i") && isigsettime(fdl_time) < 0) ||
	getvec(v0) < 0)
	    return (NULL);

    /* Allocate a new display list; give up if we can't do so. */
    if ((lp = get_display_list()) == NULL)
	return (NULL);

    /* Record the starting time. */
    lp->start = fdl_time;

    /* Set the starting point for each signal. */
    for (c = 0; c < nsig; c++) {
	vmin[c] = vmax[c] = v0[c];
	vvalid[c] = 0;
	if (v0[c] == WFDB_INVALID_SAMPLE)
	    lp->vlist[c][0].y = WFDB_INVALID_SAMPLE;
	else
	    lp->vlist[c][0].y = v0[c]*vscale[c];
    }

    /* If there are more than canvas_width samples to be shown, compress. */
    if (nsamp > canvas_width) {
	for (i = 1, x0 = 0; i < nsamp && getvec(v) > 0; i++) {
	    for (c = 0, vvalid[c] = 0; c < nsig; c++) {
		if (v[c] != WFDB_INVALID_SAMPLE) {
		    if (v[c] > vmax[c]) vmax[c] = v[c];
		    if (v[c] < vmin[c]) vmin[c] = v[c];
		    vvalid[c] = 1;
		}
	    }
	    if ((x = i*tscale) > x0) {
		x0 = x;
		for (c = 0; c < nsig; c++) {
		    if (vvalid[c]) {
			if (vmax[c] - v0[c] > v0[c] - vmin[c])
			    v0[c] = vmin[c] = vmax[c];
			else
			    v0[c] = vmax[c] = vmin[c];
			lp->vlist[c][x0].y = v0[c]*vscale[c];
		    }
		    else
			lp->vlist[c][x0].y = WFDB_INVALID_SAMPLE;
		}
	    }
	}
	i = x0+1;
    }
    /* If there are canvas_width or fewer samples to be shown, no
       compression is necessary. */
    else
	for (i = 1; i < nsamp && getvec(v) > 0; i++)
	    for (c = 0; c < nsig; c++) {
		if (v[c] == WFDB_INVALID_SAMPLE)
		    lp->vlist[c][i].y = WFDB_INVALID_SAMPLE;
		else
		    lp->vlist[c][i].y = v[c]*vscale[c];
	    }

    /* Record the number of displayed points. */
    lp->ndpts = i;

    /* Set the y-offset so that the signal will be vertically centered about
       the nominal baseline. */
    for (c = 0; c < nsig; c++) {
	int dy;
	int n;
	int ymid;
	long ymean;
	double w;

	tp = lp->vlist[c];

	/* Find the first valid sample in the trace, if any. */
	for (j = 0; j < i && tp[j].y == WFDB_INVALID_SAMPLE; j++)
	    ;
	ymean = ymax = ymin = (j < i) ? tp[j].y : 0;
	for (n = 1; j < i; j++) {
	    if ((y = tp[j].y) != WFDB_INVALID_SAMPLE) {
	        if (y > ymax) ymax = y;
		else if (y < ymin) ymin = y;
		ymean += y;
		n++;
	    }
	}
	ymean /= n;
	ymid = (ymax + ymin)/2;
	if (ymid > ymean)
	    w = (ymid - ymean)/(ymax - ymean);
	else if (ymid < ymean)
	    w = (ymean - ymid)/(ymean - ymin);
	else w = 1.0;
	dy = -(ymid + ((double)ymean-ymid)*w);

	/* Apply the y-offset to all valid samples (absolute coordinates). */
	for (j = 0; j < i; j++) {
	    if (tp[j].y == WFDB_INVALID_SAMPLE) continue;
	    tp[j].y += dy;
	    if (tp[j].y < -canvas_height) tp[j].y = -canvas_height;
	    else if (tp[j].y > canvas_height) tp[j].y = canvas_height;
	}
	if (dc_coupled[c]) lp->sb[c] = sigbase[c]*vscale[c] + dy;
    }
    return (lp);
}

/* Clear_cache() marks all of the display lists in the cache as invalid. */
void clear_cache(void)
{
    int i;
    struct display_list *lp;

    if (canvas_width > vlist_size) {
	for (lp = first_list; lp; lp = lp->next) {
	    for (i = 0; i < lp->nsig && lp->vlist[i] != NULL; i++) {
		free(lp->vlist[i]);
		lp->vlist[i] = NULL;
	    }
	}
	vlist_size = 0;
    }
    else {
	for (lp = first_list; lp; lp = lp->next) {
	    lp->start = -1;
	    lp->npoints = 0;
	}
    }
}

static void show_signal_names(cairo_t *cr)
{
    int i, xoff, yoff;

    xoff = mmx(5);
    yoff = (nsig > 1) ? (base[1] - base[0])/3 : canvas_height/3;
    if (sig_mode == 0)
	for (i = 0; i < nsig; i++)
	    wave_draw_string(cr, WAVE_COLOR_SIGNAL, xoff, base[i] - yoff,
			     signame[i]);
    else if (sig_mode == 1) {
	for (i = 0; i < siglistlen; i++)
	    if (0 <= siglist[i] && siglist[i] < nsig)
		wave_draw_string(cr, WAVE_COLOR_SIGNAL, xoff, base[i] - yoff,
				 signame[siglist[i]]);
    }
    else {	/* sig_mode == 2 (show valid signals only) */
	int j, nvsig;
	for (i = nvsig = 0; i < nsig; i++)
	    if (vvalid[i]) nvsig++;
	for (i = j = 0; i < nsig; i++) {
	    if (vvalid[i]) {
		base[i] = canvas_height*(2*(j++)+1.)/(2.*nvsig);
		wave_draw_string(cr, WAVE_COLOR_SIGNAL, xoff, base[i] - yoff,
				 signame[i]);
	    }
	}
    }
}

static void show_signal_baselines(cairo_t *cr, struct display_list *lp)
{
    int i, l, xoff, yoff;

    if (!lp) return;

    yoff = mmy(2);
    for (i = 0; i < nsig; i++) {
	if (base[i] == -9999) continue;
	if (dc_coupled[i] && 0 <= lp->sb[i] && lp->sb[i] < canvas_height) {
	    wave_draw_line(cr, WAVE_COLOR_ANNOTATION,
			   0, lp->sb[i]+base[i],
			   canvas_width, lp->sb[i]+base[i]);
	    if (blabel[i]) {
		l = strlen(blabel[i]);
		xoff = canvas_width - wave_text_width(blabel[i]) - mmx(2);
		wave_draw_string(cr, WAVE_COLOR_SIGNAL,
				 xoff, lp->sb[i]+base[i] - yoff, blabel[i]);
	    }
	}
    }
}

/* Return window y-coordinate corresponding to the level of displayed trace
   i at abscissa x.  Uses absolute coordinates stored in the display list. */
int sigy(int i, int x)
{
    int ix, j = -1;

    if (sig_mode != 1) j = i;
    else if (0 <= i && i < siglistlen) j = siglist[i];
    if (j < 0 || j >= nsig || lp_current->vlist[j] == NULL) return (-1);
    if (nsamp > canvas_width) ix = x;
    else ix = (int)(x/tscale);
    if (ix >= lp_current->ndpts) ix = lp_current->ndpts - 1;
    if (ix < 0) ix = 0;

    /* With absolute coordinates, just look up the y value directly. */
    if (lp_current->vlist[j][ix].y == WFDB_INVALID_SAMPLE) return (-1);
    return (lp_current->vlist[j][ix].y + base[i]);
}

/* file: grid.c		G. Moody	 1 May 1990
			Last revised:	2026
Grid drawing functions for WAVE

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

static int grid_plotted;

/* Call this function from the repaint procedure to restore the grid after
   the window has been cleared. */
void restore_grid(void)
{
    grid_plotted = 0;
    show_grid();
}

/* Show_grid() draws the grid in the requested style on the offscreen buffer. */
void show_grid(void)
{
    int i, ii, x, xx, y, yy;
    double dx, dxfine, dy, dyfine, vm;
    static int oghf, ogvf;
    static double odx, ody;
    cairo_t *cr;

    /* If the grid should not be visible, nothing to do. */
    if (!visible)
	return;

    /* Calculate the grid spacing in pixels */
    if (tmag <= 0.0) tmag = 1.0;
    switch (gvflag) {
      case 0:
      case 1: dx = tmag * seconds(0.2); break;
      case 2: dx = tmag * seconds(0.2); dxfine = tmag * seconds(0.04); break;
      case 3: dx = tmag * seconds(300.0); dxfine = tmag * seconds(60.0); break;
    }
    if (vmag == NULL || vmag[0] == 0.0) vm = 1.0;
    else vm = vmag[0];
    switch (ghflag) {
      case 0:
      case 1: dy = vm * millivolts(0.5); break;
      case 2: dy = vm * millivolts(0.5);
	  dyfine = vm * millivolts(0.1); break;
    }

    /* The grid must be drawn if it has not been plotted already, or if the
       grid spacing or style has changed. */
    if (!grid_plotted || ghflag != oghf || gvflag != ogvf ||
	(ghflag && dy != ody) || (gvflag && dx != odx)) {

	cr = wave_begin_paint();

	/* If horizontal grid lines are enabled, draw them. */
	if (ghflag)
	    for (i = y = 0; y < canvas_height + dy; i++, y = i*dy) {
		if (0 < y && y < canvas_height)
		    wave_draw_line(cr,
				   (ghflag > 1) ? WAVE_COLOR_GRID_COARSE
						: WAVE_COLOR_GRID,
				   0, y, canvas_width, y);
		if (ghflag > 1)		/* Draw fine horizontal grid lines. */
		    for (ii = 1; ii < 5; ii++) {
			yy = y + ii*dyfine;
			wave_draw_line(cr, WAVE_COLOR_GRID,
				       0, yy, canvas_width, yy);
		    }
	    }

	/* If vertical grid lines are enabled, draw them. */
	if (gvflag)
	    for (i = x = 0; x < canvas_width + dx; i++, x = i*dx) {
		if (0 < x && x < canvas_width)
		    wave_draw_line(cr,
				   (gvflag > 1) ? WAVE_COLOR_GRID_COARSE
						: WAVE_COLOR_GRID,
				   x, 0, x, canvas_height);
		if (gvflag > 1)		/* Draw fine vertical grid lines. */
		    for (ii = 1; ii < 5; ii++) {
			xx = x + ii*dxfine;
			wave_draw_line(cr, WAVE_COLOR_GRID,
				       xx, 0, xx, canvas_height);
		    }
	    }

	wave_end_paint(cr);

        oghf = ghflag; ogvf = gvflag; odx = dx; ody = dy;
	grid_plotted = 1;
    }
}

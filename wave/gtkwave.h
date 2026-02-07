/* file: gtkwave.h	G. Moody	27 April 1990
			Last revised:	2026

GTK 3 constants, macros, function prototypes, and global variables for WAVE

This file replaces the original xvwave.h (XView/X11 toolkit layer).

-------------------------------------------------------------------------------
WAVE: Waveform analyzer, viewer, and editor
Copyright (C) 1990-2005 George B. Moody

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

#ifndef GTKWAVE_H
#define GTKWAVE_H

#include <gtk/gtk.h>
#include <cairo.h>

/* Default screen resolution.  All known X servers seem to know something about
   the screen size, even if what they know isn't true.  Just in case, these
   constants define default values to be used if the monitor info gives
   non-positive results. */
#ifndef DPMM
#define DPMM	4.0	/* default screen resolution (pixels per millimeter) */
#endif
#ifndef DPMMX
#define DPMMX	DPMM
#endif
#ifndef DPMMY
#define DPMMY	DPMM
#endif

/* Annotations are displayed in this font if no other choice is specified. */
#ifndef DEFANNFONT
#define DEFANNFONT	"Monospace 10"
#endif

/* --- Replacement types for XPoint and XSegment --- */

typedef struct {
    int x, y;
} WavePoint;

typedef struct {
    int x1, y1, x2, y2;
} WaveSegment;

/* --- Color management --- */

typedef enum {
    WAVE_COLOR_BACKGROUND,
    WAVE_COLOR_GRID,
    WAVE_COLOR_GRID_COARSE,
    WAVE_COLOR_CURSOR,
    WAVE_COLOR_ANNOTATION,
    WAVE_COLOR_SIGNAL,
    WAVE_COLOR_HIGHLIGHT,
    WAVE_COLOR_COUNT
} WaveColorIndex;

typedef struct {
    double r, g, b, a;
} WaveColor;

/* --- Display lists ---

A display list contains all information needed to draw a screenful of signals.
For each of the nsig signals, a display list contains a list of the
(x, y) pixel coordinate pairs that specify the vertices of the polyline
that represents the signal.  Coordinates are stored as absolute values
(not relative as in the old XView/X11 code).

A cache of recently-used display lists (up to MAX_DISPLAY_LISTS of them) is
maintained as a singly-linked list; the first display list is pointed
to by first_list.
*/

struct display_list {
    struct display_list *next;	/* link to next display list */
    WFDB_Time start;		/* time of first sample */
    int nsig;			/* number of signals */
    int npoints;		/* number of (input) points per signal */
    int ndpts;			/* number of (output) points per signal */
    int xmax;			/* largest x, expressed as window abscissa */
    int *sb;			/* signal baselines, expressed as window ordinates */
    WavePoint **vlist;		/* vertex list pointers for each signal */
};

/* --- Globally visible GTK objects --- */

COMMON GtkWidget *main_window;		/* main application window */
COMMON GtkWidget *drawing_area;		/* signal drawing area */
COMMON GtkWidget *main_panel_box;	/* main toolbar container */
COMMON GtkWidget *status_bar;		/* status bar (replaces frame footer) */
COMMON cairo_surface_t *osb;		/* offscreen buffer surface */
COMMON PangoFontDescription *ann_font;	/* font for annotations */
COMMON PangoLayout *ann_layout;		/* reusable layout for text measurement */

COMMON WaveColor wave_colors[WAVE_COLOR_COUNT];
COMMON struct display_list *first_list;

COMMON GtkWidget **level_name;		/* see edit.c */
COMMON GtkWidget **level_value;
COMMON GtkWidget **level_units;
COMMON WaveSegment *level;

extern int ann_popup_active;	/* <0: annotation template popup not created,
				    0: popup not visible, >0: popup visible */

/* Cursor overlay state: stored separately from osb so we can draw
   cursors as an overlay in the draw callback without modifying osb. */
COMMON int cursor_x, cursor_y;		/* current cursor bar position */
COMMON int cursor_active;		/* nonzero if cursor bar is visible */
COMMON int box_x0, box_y0;		/* box corner */
COMMON int box_x1, box_y1;		/* opposite box corner */
COMMON int box_active;			/* nonzero if selection box is visible */

/* --- Drawing helper API ---
   All wave/ files call these instead of Cairo directly. */

void wave_set_color(cairo_t *cr, WaveColorIndex idx);
void wave_draw_line(cairo_t *cr, WaveColorIndex color,
		    int x1, int y1, int x2, int y2);
void wave_draw_lines(cairo_t *cr, WaveColorIndex color,
		     WavePoint *pts, int npts);
void wave_draw_string(cairo_t *cr, WaveColorIndex color,
		      int x, int y, const char *str);
void wave_draw_segments(cairo_t *cr, WaveColorIndex color,
			WaveSegment *segs, int n);
void wave_fill_rect(cairo_t *cr, WaveColorIndex color,
		    int x, int y, int w, int h);
cairo_t *wave_begin_paint(void);	/* get cairo_t for osb */
void wave_end_paint(cairo_t *cr);	/* release */
int wave_text_width(const char *str);	/* replaces XTextWidth */
int wave_text_height(void);		/* font line height */
void wave_refresh(void);		/* queue redraw of drawing_area */

/* --- Functions from gtkwave.c (replacing xvwave.c) --- */

extern void create_main_panel(void);			/* in mainpan.c */
extern void disp_proc(const char *action);		/* in mainpan.c */
extern gboolean canvas_event_handler(GtkWidget *widget,	/* in edit.c */
				     GdkEvent *event,
				     gpointer data);
extern struct display_list *find_display_list(		/* in signal.c */
					     WFDB_Time time);

/* Functions provided by gtkwave.c */
extern void strip_x_args(int *argc, char **argv);
extern int initialize_graphics(int mode);
extern void hide_grid(void);
extern void unhide_grid(void);
extern void display_and_process_events(void);
extern void quit_proc(void);
extern void sync_other_wave_processes(void);
extern void save_defaults(void);

/* Utility: show a modal yes/no dialog.  Returns nonzero if "yes". */
extern int wave_notice_prompt(const char *message);

/* Set frame title and footer text */
extern void wave_set_frame_title(const char *title);
extern void wave_set_left_footer(const char *text);
extern void wave_set_right_footer(const char *text);
extern void wave_set_busy(int busy);

/* Grid visibility (replaces colormap manipulation) */
extern int wave_grid_is_hidden(void);

#endif /* GTKWAVE_H */

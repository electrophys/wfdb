/* file: wave.c		G. Moody	27 April 1990
			Last revised:	2026
main() function for WAVE

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

#include "wave.h"
#include "gtkwave.h"
#include <wfdb/wfdblib.h>
#include <unistd.h>

#ifndef HELPDIR
#define HELPDIR		"/usr/local/help"
#endif

int main(int argc, char *argv[])
{
    char *wfdbp, *hp, *p, *start_string = NULL, *tp;
    int do_demo = 0, i, j, mode = 0;
    static char *helppath;
    static int wave_procno;

    /* Extract program name for use in error messages. */
    for (p = pname = argv[0]; *p; p++)
	if (*p == '/') pname = p+1;

    /* Initialize non-zero global variables. */
    begin_analysis_time = end_analysis_time = -1L;
    tsa_index = vsa_index = ann_mode = overlap = sig_mode = time_mode =
	grid_mode = -1;
    tscale = 1.0;

    if ((helpdir = getenv("HELPDIR")) == NULL) helpdir = HELPDIR;

    /* Find the record name argument. */
    for (i = 1; i < argc-1; i++) {
	if (strcmp(argv[i], "-r") == 0)
	    break;
    }

    /* Provide (non-window based) usage information if needed. */
    if (i == argc-1 || argc == 1)
	help();	/* print info and quit */

    /* Set the help path. */
    if ((hp = getenv("HELPPATH")) == NULL)
	hp = "/usr/lib/help";
    helppath = (char *)malloc(strlen(hp) + strlen(helpdir) + 16);
    if (helppath) {
	sprintf(helppath, "HELPPATH=%s:%s/wave", hp, helpdir);
	putenv(helppath);
    }

    /* Check for requests to open more than one record. */
    strcpy(record, argv[++i]);
    while ((p = strchr(record, '+')) != NULL) {
	static int wave_pid;

	wave_pid = (int)getpid();
	if (fork() == 0) {		/* child */
	    wave_ppid = wave_pid;
	    wave_procno++;
	    p++; tp = record;
	    while ((*tp++ = *p++))
		;
	    strcpy(argv[i], record);
	    if (strchr(record, '+') == NULL)
		make_sync_button = 1;
	}
	else {	/* parent */
	    *p = '\0';
	    strcpy(argv[i], record);
	}
    }

    /* Remove any command-line arguments that are specific to another WAVE
       process, and remove the `+n/' prefix from any that are specific to
       this process. */
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '+') {
	    if ((atoi(argv[i]+1) == wave_procno) &&
		(p = strchr(argv[i], '/')))
	      argv[i] = p+1;
	    else {
		for (j = i+1; j < argc; j++)
		    argv[j-1] = argv[j];
		argc--;
	    }
	}
    }

    /* Process window system related command-line arguments (gtk_init). */
    strip_x_args(&argc, argv);

    /* Process WAVE-specific arguments. */
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-')
	    switch (*(argv[i] + 1)) {
	      case 'a':
		if (++i >= argc) {
		    fprintf(stderr, "%s: annotator name must follow -a\n",
			    pname);
		    exit(1);
		}
		if (strlen(argv[i]) > ANLMAX) {
		    fprintf(stderr, "%s: annotator name is too long\n",
			    pname);
		    exit(1);
		}
		strcpy(annotator, argv[i]);
		break;
	      case 'd':
		if (strcmp(argv[i], "-dpi")) break;
		if (++i >= argc) {
		    fprintf(stderr, "%s: resolution must follow -dpi\n",
			    pname);
		    exit(1);
		}
		(void)sscanf(argv[i], "%lfx%lf", &dpmmx, &dpmmy);
		if (dpmmx < 0.) dpmmx = 0.;
		if (dpmmy <= 0.) dpmmy = dpmmx;
		dpmmx /= 25.4; dpmmy /= 25.4;
		break;
	      case 'D':
		if (++i >= argc) {
		    fprintf(stderr, "%s: log file name must follow -D\n",
			    pname);
		    exit(1);
		}
		if (strlen(argv[i]) > LNLMAX) {
		    fprintf(stderr, "%s: log file name is too long\n", pname);
		    exit(1);
		}
		strcpy(log_file_name, argv[i]);
		do_demo = 1;
		break;
	      case 'f':
		if (++i >= argc) {
		    fprintf(stderr, "%s: start time must follow -f\n",
			    pname);
		    exit(1);
		}
		start_string = argv[i];
		if (start_string[0] == '[')
		    time_mode = 1;
		else if (start_string[0] == 's')
		    time_mode = 2;
		break;
	      case 'g':
		mode |= MODE_GREY;
		break;
	      case 'H':
	        setgvmode(WFDB_HIGHRES);
		break;
	      case 'm':
		mode |= MODE_MONO;
		break;
	      case 'O':
		mode |= MODE_OVERLAY;
		break;
	      case 'p':
		if (++i >= argc) {
		    fprintf(stderr,
			    "%s: input file location(s) must follow -p\n",
			    pname);
		    exit(1);
		}
		wfdb_addtopath(argv[i]);
		break;
	      case 'r':
		if (++i >= argc) {
		    fprintf(stderr, "%s: record name must follow -r\n",
			    pname);
		    exit(1);
		}
		if (strlen(argv[i]) > RNLMAX) {
		    fprintf(stderr, "%s: record name is too long\n",
			    pname);
		    exit(1);
		}
		strcpy(record, argv[i]);
		break;
	      case 's':
		for (j = 0; ++i < argc && *argv[i] != '-'; j++)
		    ;
		if (j == 0) {
		    (void)fprintf(stderr,
			     "%s: one or more signal numbers must follow -s\n",
				  pname);
		    exit(1);
		}
		if (siglistlen + j > maxsiglistlen) {
		    if ((siglist = realloc(siglist,
				      (siglistlen+j) * sizeof(int))) == NULL ||
			(base = realloc(base,
				      (siglistlen+j) * sizeof(int))) == NULL ||
			(level = realloc(level,
				 (siglistlen+j) * sizeof(WaveSegment))) == NULL) {
			(void)fprintf(stderr, "%s: insufficient memory\n",
				      pname);
			exit(2);
		    }
		    maxsiglistlen = siglistlen + j;
		}
		for (i -= j; i < argc && *argv[i] != '-'; )
		    siglist[siglistlen++] = atoi(argv[i++]);
		i--;
		sig_mode = 1;
		break;
	      case 'S':
		mode |= MODE_SHARED;
		break;
	      case 'V':
		switch (*(argv[i]+2)) {
		  case 'a':
		    show_aux = 1;
		    break;
		  case 'A':
		    if (++i >= argc) {
			fprintf(stderr,
			       "%s: annotation display mode must follow -VA\n",
				pname);
			exit(1);
		    }
		    ann_mode = atoi(argv[i]);
		    break;
		  case 'b':
		    show_baseline = 1;
		    break;
		  case 'c':
		    show_chan = 1;
		    break;
		  case 'G':
		    if (++i >= argc) {
			fprintf(stderr,
			       "%s: grid display mode must follow -VG\n",
				pname);
			exit(1);
		    }
		    grid_mode = coarse_grid_mode = fine_grid_mode =
			atoi(argv[i]);
		    break;
		  case 'l':
		    show_level = 1;
		    break;
		  case 'm':
		    show_marker = 1;
		    break;
		  case 'n':
		    show_num = 1;
		    break;
		  case 'N':
		    show_signame = 1;
		    break;
		  case 's':
		    show_subtype = 1;
		    break;
		  case 'S':
		    if (++i >= argc) {
			fprintf(stderr,
				"%s: signal display mode must follow -VS\n",
				pname);
			exit(1);
		    }
		    sig_mode = atoi(argv[i]);
		    break;
		  case 't':
		    if (++i >= argc) {
			fprintf(stderr,
				"%s: time scale choice must follow -Vt\n",
				pname);
			exit(1);
		    }
		    tsa_index = coarse_tsa_index = fine_tsa_index =
			atoi(argv[i]);
		    break;
		  case 'T':
		    if (++i >= argc) {
			fprintf(stderr,
			       "%s: time display mode must follow -VT\n",
				pname);
			exit(1);
		    }
		    time_mode = atoi(argv[i]);
		    break;
		  case 'v':
		    if (++i >= argc) {
			fprintf(stderr,
				"%s: amplitude scale choice must follow -Vv\n",
				pname);
			exit(1);
		    }
		    vsa_index = atoi(argv[i]);
		    break;
		}
		break;
	    }
    }

    /* Provide help if no record was specified. */
    if (record[0] == '\0') help();

    /* Set up base frame, quit if unsuccessful. */
    if (initialize_graphics(mode)) exit(1);

    /* Make sure that the WFDB path begins with an empty component. */
    wfdbp = getwfdb();
    if (*wfdbp != ':') {
	char *nwfdbp;

	if ((nwfdbp = (char *)malloc(strlen(wfdbp)+2)) == NULL) {
	    fprintf(stderr, "%s: memory allocation error\n", pname);
	    exit(1);
	}
	sprintf(nwfdbp, ":%s", wfdbp);
	setwfdb(nwfdbp);
    }

    /* Make sure there is a calibration database defined. */
    if (!getenv("WFDBCAL"))
	putenv("WFDBCAL=wfdbcal");
    /* Initialize the annotation table. */
    (void)read_anntab();

    /* Open the selected record (and annotation file, if specified). */
    if (record_init(record)) {
	if (annotator[0]) {
	    af.name = annotator; af.stat = WFDB_READ;
	    nann = 1;
	    annot_init();
	}
    }

    /* Indicate that the input annotation file must be saved before writing
       any edits. */
    savebackup = 1;

    /* If requested, set the start time. */
    if (start_string) {
	display_start_time = strtim(start_string);
	if (start_string[0] == '[') {
	    if (display_start_time > 0L)
		display_start_time = 0;
	    else
		display_start_time = -display_start_time;
	}
    }
    set_frame_footer();

    /* If requested, start demonstration. */
    if (do_demo)
      start_demo();

    /* Do initial display */
    do_disp();

    /* Enter the main loop. */
    display_and_process_events();

    exit(0);
}

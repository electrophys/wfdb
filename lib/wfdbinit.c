/* file: wfdbinit.c	G. Moody	 23 May 1983
			Last revised:   9 February 2026   	wfdblib 11.0.0
WFDB library functions wfdbinit, wfdbquit, and wfdbflush
_______________________________________________________________________________
wfdb: a library for reading and writing annotated waveforms (time series data)
Copyright (C) 1983-2012 George B. Moody

This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Library General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option) any
later version.

This library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU Library General Public License for more
details.

You should have received a copy of the GNU Library General Public License along
with this library; if not, see <http://www.gnu.org/licenses/>.

You may contact the author by e-mail (wfdb@physionet.org) or postal mail
(MIT Room E25-505A, Cambridge, MA 02139 USA).  For updates to this software,
please visit PhysioNet (http://www.physionet.org/).
_______________________________________________________________________________

This file contains definitions of the following WFDB library functions:
 wfdbinit	(opens annotation files and input signals)
 wfdbquit	(closes all annotation and signal files)
 wfdbflush	(writes all buffered output annotation and signal files)
*/

#include "wfdb_context.h"

int wfdbinit_ctx(WFDB_Context *ctx, char *record,
		  const WFDB_Anninfo *aiarray, unsigned int nann,
		  WFDB_Siginfo *siarray, unsigned int nsig)
{
    int stat;

    if ((stat = annopen_ctx(ctx, record, aiarray, nann)) == 0)
	stat = isigopen_ctx(ctx, record, siarray, (int)nsig);
    return (stat);
}

int wfdbinit(char *record, const WFDB_Anninfo *aiarray, unsigned int nann,
	      WFDB_Siginfo *siarray, unsigned int nsig)
{
    return wfdbinit_ctx(wfdb_get_default_context(), record, aiarray, nann,
			siarray, nsig);
}

void wfdbquit_ctx(WFDB_Context *ctx)
{
    wfdb_anclose();	/* close annotation files, reset variables */
    wfdb_oinfoclose();	/* close info file */
    wfdb_sigclose();	/* close signals, reset variables */
    resetwfdb_ctx(ctx);	/* restore the WFDB path */
    wfdb_sampquit();	/* release sample data buffer */
    wfdb_freeinfo();	/* release info strings */
}

void wfdbquit(void)
{
    wfdbquit_ctx(wfdb_get_default_context());
}

void wfdbflush_ctx(WFDB_Context *ctx)
{
    wfdb_oaflush();	/* flush buffered output annotations */
    wfdb_osflush();	/* flush buffered output samples */
}

void wfdbflush(void)	/* write all buffered output to files */
{
    wfdbflush_ctx(wfdb_get_default_context());
}

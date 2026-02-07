/* file: signal.c	G. Moody	13 April 1989
			Last revised:    18 May 2022		wfdblib 10.7.0
WFDB library functions for signals

_______________________________________________________________________________
wfdb: a library for reading and writing annotated waveforms (time series data)
Copyright (C) 1989-2013 George B. Moody

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

This file contains definitions of the following functions, which are not
visible outside of this file:
 allocisig	(sets max number of simultaneously open input signals)
 allocigroup	(sets max number of simultaneously open input signal groups)
 allocosig	(sets max number of simultaneously open output signals)
 allocogroup	(sets max number of simultaneously open output signal groups)
 isfmt		(checks if argument is a legal signal format type)
 isflacfmt	(checks if argument refers to a FLAC signal format)
 copysi		(deep-copies a WFDB_Siginfo structure)
 sigmap_cleanup (deallocates memory used by sigmap)
 make_vsd	(makes a virtual signal object)
 sigmap_init	(manages the signal maps)
 sigmap		(creates a virtual signal vector from a raw sample vector)
 edfparse [10.4.5](gets header info from an EDF file)
 readheader	(reads a header file)
 hsdfree	(deallocates memory used by readheader)
 flac_getsamp	(reads the next sample from a FLAC input file)
 flac_isopen	(opens a FLAC input file)
 flac_isclose	(closes a FLAC input file)
 flac_isseek	(skips to a specified location in a FLAC input file)
 flac_putsamp	(writes a sample to a FLAC output file)
 flac_osinit	(prepares to encode a FLAC output file)
 flac_osopen	(opens a FLAC output file)
 flac_osclose	(closes a FLAC output file)
 isigclose	(closes input signals)
 osigclose	(closes output signals)
 isgsetframe	(skips to a specified frame number in a specified signal group)
 getskewedframe	(reads an input frame, without skew correction)
 meansamp       (calculates mean of an array of samples)
 rgetvec        (reads a sample from each input signal without resampling)
 openosig       (opens output signals)

This file also contains low-level I/O routines for signals in various formats;
typically, the input routine for format N signals is named rN(), and the output
routine is named wN().  Most of these routines are implemented as macros for
efficiency.  These low-level I/O routines are not visible outside of this file.

This file also contains definitions of the following WFDB library functions:
 isigopen	(opens input signals)
 osigopen	(opens output signals according to a header file)
 osigfopen	(opens output signals by name)
 findsig [10.4.12] (find an input signal with a specified name)
 getspf [9.6]	(returns number of samples returned by getvec per frame)
 setgvmode [9.0](sets getvec operating mode)
 getgvmode [10.5.3](returns getvec operating mode)
 setifreq [10.2.6](sets the getvec sampling frequency)
 getifreq [10.2.6](returns the getvec sampling frequency)
 getvec		(reads a (possibly resampled) sample from each input signal)
 getframe [9.0]	(reads an input frame)
 putvec		(writes a sample to each output signal)
 isigsettime	(skips to a specified time in each signal)
 isgsettime	(skips to a specified time in a specified signal group)
 tnextvec [10.4.13] (skips to next valid sample of a specified signal)
 setibsize [5.0](sets the default buffer size for getvec)
 setobsize [5.0](sets the default buffer size for putvec)
 newheader	(creates a new header file)
 setheader [5.0](creates or rewrites a header file given signal specifications)
 setmsheader [9.1] (creates or rewrites a header for a multi-segment record)
 wfdbgetskew [9.4](returns skew)
 wfdbsetiskew[9.8](sets skew for input signals)
 wfdbsetskew [9.4](sets skew to be written by setheader)
 wfdbgetstart [9.4](returns byte offset of sample 0 within signal file)
 wfdbsetstart [9.4](sets byte offset to be written by setheader)
 wfdbputprolog [10.4.15](writes a prolog to a signal file)
 setinfo [10.5.11] (creates a .info file for a record)
 putinfo [4.0]	(writes a line of info for a record)
 getinfo [4.0]	(reads a line of info for a record)
 sampfreq	(returns the sampling frequency of the specified record)
 setsampfreq	(sets the putvec sampling frequency)
 setbasetime	(sets the base time and date)
 timstr		(converts sample intervals to time strings)
 mstimstr	(converts sample intervals to time strings with milliseconds)
 getcfreq [5.2]	(gets the counter frequency)
 setcfreq [5.2]	(sets the counter frequency)
 getbasecount [5.2] (gets the base counter value)
 setbasecount [5.2] (sets the base counter value)
 strtim		(converts time strings to sample intervals)
 datstr		(converts Julian dates to date strings)
 strdat		(converts date strings to Julian dates)
 adumuv		(converts ADC units to microvolts)
 muvadu		(converts microvolts to ADC units)
 aduphys [6.0]	(converts ADC units to physical units)
 physadu [6.0]	(converts physical units to ADC units)
 sample [10.3.0](get a sample from a given signal at a given time)
 sample_valid [10.3.0](verify that last value returned by sample was valid)

(Numbers in brackets in the list above indicate the first version of the WFDB
library that included the corresponding function.  Functions not so marked
have been included in all published versions of the WFDB library.)

These functions, also defined here, are intended only for the use of WFDB
library functions defined elsewhere:
 wfdb_sampquit  (frees memory allocated by sample() and sigmap_init())
 wfdb_sigclose 	(closes signals and resets variables)
 wfdb_osflush	(flushes output signals)
 wfdb_freeinfo [10.5.11] (releases resources allocated for info string handling)

The function setbasetime() uses the C library functions localtime() and time(),
and definitions from <time.h>.

Beginning with version 6.1, header files are written by newheader() and
setheader() with \r\n line separators (earlier versions used \n only).  Earlier
versions of the WFDB library can read header files written by these functions,
but signal descriptions and info strings will be terminated by \r.

Multifrequency records (i.e., those for which not all signals are digitized at
the same frequency) are supported by version 9.0 and later versions.  Multi-
segment records (constructed by concatenating single-segment records) and null
(format 0) signals are supported by version 9.1 and later versions.

Beginning with version 10.2, there are no longer any fixed limits on the
number of signals in a record or the number of samples per signal per frame.
Older applications used the symbols WFDB_MAXSIG and WFDB_MAXSPF (which are
still defined in wfdb.h for compatibility) to determine the sizes needed for
arrays of WFDB_Siginfo structures passed into, e.g., isiginfo, and the lengths
of arrays of WFDB_Samples passed into getvec and getframe.  Newly-written
applications should use the methods illustrated immediately below instead.
*/

#include "signal_internal.h"

#include <time.h>

/* Local functions. */

/* Allocate workspace for up to n input signals. */
int allocisig(unsigned int n)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    if (maxisig < n) {
	unsigned m = maxisig;

	SREALLOC(isd, n, sizeof(struct isdata *));
	while (m < n) {
	    SUALLOC(isd[m], 1, sizeof(struct isdata));
	    m++;
	}
	maxisig = n;
    }
    return (maxisig);
}

/* Allocate workspace for up to n input signal groups. */
int allocigroup(unsigned int n)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    if (maxigroup < n) {
	unsigned m = maxigroup;

	SREALLOC(igd, n, sizeof(struct igdata *));
	while (m < n) {
	    SUALLOC(igd[m], 1, sizeof(struct igdata));
	    m++;
	}
	maxigroup = n;
    }
    return (maxigroup);
}

/* Allocate workspace for up to n output signals. */
int allocosig(unsigned int n)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    if (maxosig < n) {
	unsigned m = maxosig;

	SREALLOC(osd, n, sizeof(struct osdata *));
	while (m < n) {
	    SUALLOC(osd[m], 1, sizeof(struct osdata));
	    m++;
	}
	maxosig = n;
    }
    return (maxosig);
}

/* Allocate workspace for up to n output signal groups. */
int allocogroup(unsigned int n)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    if (maxogroup < n) {
	unsigned m = maxogroup;

	SREALLOC(ogd, n, sizeof(struct ogdata *));
	while (m < n) {
	    SUALLOC(ogd[m], 1, sizeof(struct ogdata));
	    m++;
	}
	maxogroup = n;
    }
    return (maxogroup);
}

int isfmt(int f)
{
    int i;
    static const int fmt_list[WFDB_NFMTS] = WFDB_FMT_LIST;

    for (i = 0; i < WFDB_NFMTS; i++)
	if (f == fmt_list[i]) return (1);
    return (0);
}

int isflacfmt(int f)
{
    return (f >= 500 && f <= 532);
}

int copysi(WFDB_Siginfo *to, const WFDB_Siginfo *from)
{
    if (to == NULL || from == NULL) return (0);
    *to = *from;
    to->fname = to->desc = to->units = NULL;
    SSTRCPY(to->fname, from->fname);
    SSTRCPY(to->desc, from->desc);
    SSTRCPY(to->units, from->units);
    return (1);
}

void isigclose(void)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    struct isdata *is;
    struct igdata *ig;

    if (sbuf && !in_msrec) {
	SFREE(sbuf);
	sample_vflag = 0;
    }
    if (isd) {
	while (maxisig)
	    if (is = isd[--maxisig]) {
		SFREE(is->info.fname);
		SFREE(is->info.units);
		SFREE(is->info.desc);
		SFREE(is);
	    }
	SFREE(isd);
    }
    maxisig = nisig = 0;
    framelen = 0;

    if (igd) {
	while (maxigroup)
	    if (ig = igd[--maxigroup]) {
		if (ig->flacdec)
		    flac_isclose(ig);
		if (ig->fp) (void)wfdb_fclose(ig->fp);
		SFREE(ig->buf);
		SFREE(ig);
	    }
	SFREE(igd);
    }
    maxigroup = nigroup = 0;

    istime = 0L;
    gvc = ispfmax = 1;
    if (hheader) {
	(void)wfdb_fclose(hheader);
	hheader = NULL;
    }
    SFREE(linebuf);
    if (nosig == 0 && maxhsig != 0)
	hsdfree();
}

int osigclose(void)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    struct osdata *os;
    struct ogdata *og;
    WFDB_Group g;
    int stat = 0, errflag;

    for (g = 0; g < nogroup; g++)
	if (ogd && (og = ogd[g]))
	    og->force_flush = 1;

    wfdb_osflush();

    if (osd) {
	while (maxosig)
	    if (os = osd[--maxosig]) {
		SFREE(os->info.fname);
		SFREE(os->info.units);
		SFREE(os->info.desc);
		SFREE(os);
	    }
	SFREE(osd);
    }
    nosig = 0;

    if (ogd) {
	while (maxogroup)
	    if (og = ogd[--maxogroup]) {
		if (og->fp) {
		    if (og->flacenc)
			flac_osclose(og);

		    /* If a block size has been defined, null-pad the buffer */
		    if (og->bsize)
			while (og->bp != og->be)
			    *(og->bp++) = '\0';
		    /* Flush the last block unless it's empty. */
		    if (og->bp != og->buf)
			(void)wfdb_fwrite(og->buf, 1, og->bp-og->buf, og->fp);
		    if (og->fp->fp == stdout)
			wfdb_fflush(og->fp);
		    errflag = wfdb_ferror(og->fp);
		    /* Close file (except stdout, which is closed on exit). */
		    if (og->fp->fp != stdout) {
			if (wfdb_fclose(og->fp))
			    errflag = 1;
			og->fp = NULL;
		    }
		    if (errflag) {
			wfdb_error("osigclose: write error"
				   " in signal group %d\n", maxogroup);
			stat = -4;
		    }
		}
		SFREE(og->buf);
		SFREE(og);
	    }
	SFREE(ogd);
    }
    maxogroup = nogroup = 0;

    ostime = 0L;
    if (oheader) {
	errflag = wfdb_ferror(oheader);
	if (wfdb_fclose(oheader))
	    errflag = 1;
	if (errflag) {
	    wfdb_error("osigclose: write error in header file\n");
	    stat = -4;
	}
	if (outinfo == oheader) outinfo = NULL;
	oheader = NULL;
    }
    if (nisig == 0 && maxhsig != 0)
	hsdfree();

    return (stat);
}

/* WFDB library functions. */

int isigopen_ctx(WFDB_Context *ctx, char *record, WFDB_Siginfo *siarray, int nsig)
{
    int navail, nn, spflimit;
    int first_segment = 0;
    struct hsdata *hs;
    struct isdata *is;
    struct igdata *ig;
    WFDB_Signal s, si, sj;
    WFDB_Group g;

    /* Close previously opened input signals unless otherwise requested. */
    if (*record == '+') record++;
    else isigclose();

    /* Remove trailing .hea, if any, from record name. */
    wfdb_striphea(record);

    /* Save the current record name. */
    if (!in_msrec) wfdb_setirec(record);

    /* Read the header and determine how many signals are available. */
    if ((navail = readheader(record)) <= 0) {
	if (navail == 0 && segments) {	/* this is a multi-segment record */
	    in_msrec = 1;
	    first_segment = 1;
	    /* Open the first segment to get signal information. */
	    if (segp && (navail = readheader(segp->recname)) >= 0) {
		if (msbtime == 0L) msbtime = btime;
		if (msbdate == (WFDB_Date)0) msbdate = bdate;
	    }
	    if (nsig <= 0)
		in_msrec = 0;
	}
	if (navail == 0 && nsig)
	    wfdb_error("isigopen: record %s has no signals\n", record);
	if (navail <= 0)
	    return (navail);
    }

    /* If nsig <= 0, isigopen fills in up to (-nsig) members of siarray based
       on the contents of the header, but no signals are actually opened.  The
       value returned is the number of signals named in the header. */
    if (nsig <= 0) {
	nsig = -nsig;
	if (navail < nsig) nsig = navail;
	if (siarray != NULL)
	    for (s = 0; s < nsig; s++)
		siarray[s] = hsd[s]->info;
	in_msrec = 0;	/* necessary to avoid errors when reopening */
	return (navail);
    }

    /* Determine how many new signals we should attempt to open.  The caller's
       upper limit on this number is nsig, and the upper limit defined by the
       header is navail. */
    if (nsig > navail) nsig = navail;

    /* Allocate input signals and signal group workspace. */
    nn = nisig + nsig;
    if (allocisig(nn) != nn)
	return (-1);	/* failed, nisig is unchanged, allocisig emits error */
    nn = nigroup + hsd[navail-1]->info.group + 1;
    if (nn > nigroup + nsig)
	nn = nigroup + nsig;
    if (allocigroup(nn) != nn)
	return (-1);	/* failed, allocigroup emits error */

    /* Set default buffer size (if not set already by setibsize). */
    if (ibsize <= 0) ibsize = BUFSIZ;

    /* Open the signal files.  One signal group is handled per iteration.  In
       this loop, si counts through the entries that have been read from hsd,
       and s counts the entries that have been added to isd. */
    for (g = si = s = 0; si < navail && s < nsig; si = sj) {
        hs = hsd[si];
	is = isd[nisig+s];
	ig = igd[nigroup+g];

	/* Find out how many signals are in this group. */
        for (sj = si + 1; sj < navail; sj++)
	  if (hsd[sj]->info.group != hs->info.group) break;

	/* Skip this group if there are too few slots in the caller's array. */
	if (sj - si > nsig - s) continue;

	/* Set the buffer size and the seek capability flag. */
	if (hs->info.bsize < 0) {
	    ig->bsize = hs->info.bsize = -hs->info.bsize;
	    ig->seek = 0;
	}
	else {
	    if ((ig->bsize = hs->info.bsize) == 0) ig->bsize = ibsize;
	    ig->seek = 1;
	}
	SALLOC(ig->buf, 1, ig->bsize);

	/* Check that the signal file is readable. */
	if (hs->info.fmt == 0)
	    ig->fp = NULL;	/* Don't open a file for a null signal. */
	else {
	    ig->fp = wfdb_open(hs->info.fname, (char *)NULL, WFDB_READ);
	    /* Skip this group if the signal file can't be opened. */
	    if (ig->fp == NULL) {
	        SFREE(ig->buf);
		continue;
	    }
	}

	if (isflacfmt(hs->info.fmt)) {
	    if (flac_isopen(ig, &hsd[si], sj - si) < 0) {
		SFREE(ig->buf);
		wfdb_fclose(ig->fp);
		continue;
	    }
	}

	/* All tests passed -- fill in remaining data for this group. */
	ig->be = ig->bp = ig->buf + ig->bsize;
	ig->start = hs->start;
	ig->initial_skip = (ig->start > 0);
	ig->stat = 1;
	while (si < sj && s < nsig) {
	    copysi(&is->info, &hs->info);
	    is->info.group = nigroup + g;
	    is->skew = hs->skew;
	    ++s;
	    if (++si < sj) {
		hs = hsd[si];
		is = isd[nisig + s];
	    }
	}
	g++;
    }

    /* Produce a warning message if none of the requested signals could be
       opened. */
    if (s == 0 && nsig)
	wfdb_error("isigopen: none of the signals for record %s is readable\n",
		 record);

    /* Check that the total number of samples per frame is less than
       or equal to INT_MAX. */
    spflimit = INT_MAX - framelen;
    for (si = 0; si < s; si++) {
	spflimit -= isd[nisig + si]->info.spf;
	if (spflimit < 0) {
	    wfdb_error("isigopen: total frame size too large in record %s\n",
		       record);
	    isigclose();
	    return (-3);
	}
    }

    /* Copy the WFDB_Siginfo structures to the caller's array.  Use these
       data to construct the initial sample vector, and to determine the
       maximum number of samples per signal per frame and the maximum skew. */
    for (si = 0; si < s; si++) {
        is = isd[nisig + si];
	if (siarray)
	    copysi(&siarray[si], &is->info);
	is->samp = is->info.initval;
	if (ispfmax < is->info.spf) ispfmax = is->info.spf;
	if (skewmax < is->skew) skewmax = is->skew;
    }
    nisig += s;		/* Update the count of open input signals. */
    nigroup += g;	/* Update the count of open input signal groups. */
    if (sigmap_init(first_segment) < 0) {
	isigclose();
	return (-3);
    }
    spfmax = ispfmax;
    setgvmode(gvmode);	/* Reset sfreq if appropriate. */
    gvc = ispfmax;	/* Initialize getvec's sample-within-frame counter. */

    /* Determine the total number of samples per frame. */
    for (si = framelen = 0; si < nisig; si++)
	framelen += isd[si]->info.spf;

    /* Allocate workspace for getvec, isgsettime, and tnextvec. */
    if (tspf > tuvlen) {
	SALLOC(tvector, tspf, sizeof(WFDB_Sample));
	SALLOC(uvector, tspf, sizeof(WFDB_Sample));
	SALLOC(vvector, tspf, sizeof(WFDB_Sample));
	tuvlen = tspf;
    }

    /* If deskewing is required, allocate the deskewing buffer (unless this is
       a multi-segment record and dsbuf has been allocated already). */
    if (skewmax != 0 && (!in_msrec || dsbuf == NULL)) {
	if (tspf > INT_MAX / (skewmax + 1)) {
	    wfdb_error("isigopen: maximum skew too large in record %s\n",
		       record);
	    isigclose();
	    return (-3);
	}
	dsbi = -1;	/* mark buffer contents as invalid */
	dsblen = tspf * (skewmax + 1);
	SALLOC(dsbuf, dsblen, sizeof(WFDB_Sample));
    }
    return (s);
}

int isigopen(char *record, WFDB_Siginfo *siarray, int nsig)
{
    return isigopen_ctx(wfdb_get_default_context(), record, siarray, nsig);
}

static int openosig(const char *func, WFDB_Siginfo *si_out,
		    const WFDB_Siginfo *si_in, unsigned int nsig)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    struct osdata *os, *op;
    struct ogdata *og;
    WFDB_Signal s;
    unsigned int ga;

    /* Allocate workspace for output signals. */
    if (allocosig(nosig + nsig) < 0) return (-3);
    /* Allocate workspace for output signal groups. */
    if (allocogroup(nogroup + si_in[nsig-1].group + 1) < 0) return (-3);

    /* Initialize local variables. */
    if (obsize <= 0) obsize = BUFSIZ;

    /* Set the group number adjustment.  This quantity is added to the group
       numbers of signals which are opened below;  it accounts for any output
       signals which were left open from previous calls. */
    ga = nogroup;

    /* Open the signal files.  One signal is handled per iteration. */
    for (s = 0, os = osd[nosig]; s < nsig; s++, nosig++, si_in++) {
	op = os;
	os = osd[nosig];

	/* Copy signal information from the caller's array. */
	copysi(&os->info, si_in);
	if (os->info.spf < 1) os->info.spf = 1;
	os->info.cksum = 0;
	os->info.nsamp = (WFDB_Time)0L;
	os->info.group += ga;
	if (si_out) {
	    copysi(si_out, &os->info);
	    si_out++;
	}

	/* Check if this signal is in the same group as the previous one. */
	if (s == 0 || os->info.group != op->info.group) {
	    size_t obuflen;

	    og = ogd[os->info.group];
	    og->bsize = os->info.bsize;
	    if (isflacfmt(os->info.fmt)) {
		unsigned ns = 1;
		while (s + ns < nsig && si_in[ns].group == si_in[0].group)
		    ns++;
		if (flac_osinit(og, si_in, ns) < 0) {
		    osigclose();
		    return (-3);
		}
	    }

	    obuflen = og->bsize ? og->bsize : obsize;
	    /* This is the first signal in a new group; allocate buffer. */
	    SALLOC(og->buf, 1, obuflen);
	    og->bp = og->buf;
	    og->be = og->buf + obuflen;
	    if (os->info.fmt == 0) {
		/* If the signal file name was NULL or "~", don't create a
		   signal file. */
		if (os->info.fname == NULL || strcmp("~", os->info.fname) == 0)
		    og->fp = NULL;
		/* Otherwise, assume that the user wants to write a signal
		   file in the default format (16). */
		else
		    os->info.fmt = 16;
	    }
	    if (os->info.fmt != 0) {
		/* An error in opening an output file is fatal. */
		og->fp = wfdb_open(os->info.fname,(char *)NULL, WFDB_WRITE);
		if (og->fp == NULL) {
		    wfdb_error("%s: can't open %s\n", func, os->info.fname);
		    SFREE(og->buf);
		    osigclose();
		    return (-3);
		}
	    }
	    if (isflacfmt(os->info.fmt) && flac_osopen(og) < 0) {
		SFREE(og->buf);
		osigclose();
		return (-3);
	    }
	    nogroup++;
	}
	else {
	    /* This signal belongs to the same group as the previous signal. */
	    if (os->info.fmt != op->info.fmt ||
		os->info.bsize != op->info.bsize) {
		wfdb_error(
		    "%s: error in specification of signal %d or %d\n",
		    func, s-1, s);
		return (-2);
	    }
	}
    }
    return (s);
}

int osigopen_ctx(WFDB_Context *ctx, char *record, WFDB_Siginfo *siarray, unsigned int nsig)
{
    int n;
    WFDB_Signal s;
    WFDB_Siginfo *hsi;

    /* Close previously opened output signals unless otherwise requested. */
    if (*record == '+') record++;
    else osigclose();

    /* Remove trailing .hea, if any, from record name. */
    wfdb_striphea(record);

    if ((n = readheader(record)) < 0)
	return (n);
    if (n < nsig) {
	wfdb_error("osigopen: record %s has fewer signals than needed\n",
		 record);
	return (-3);
    }

    SUALLOC(hsi, nsig, sizeof(WFDB_Siginfo));
    if (!hsi) return (-3);
    for (s = 0; s < nsig; s++)
	hsi[s] = hsd[s]->info;
    s = openosig("osigopen", siarray, hsi, nsig);
    SFREE(hsi);
    return (s);
}

int osigopen(char *record, WFDB_Siginfo *siarray, unsigned int nsig)
{
    return osigopen_ctx(wfdb_get_default_context(), record, siarray, nsig);
}

int osigfopen_ctx(WFDB_Context *ctx, const WFDB_Siginfo *siarray, unsigned int nsig)
{
    int s, stat;
    const WFDB_Siginfo *si;

    /* Close any open output signals. */
    stat = osigclose();

    /* Do nothing further if there are no signals to open. */
    if (siarray == NULL || nsig == 0) return (stat);

    if (obsize <= 0) obsize = BUFSIZ;

    /* Prescan siarray to check the signal specifications and to determine
       the number of signal groups. */
    for (s = 0, si = siarray; s < nsig; s++, si++) {
	/* The combined lengths of the fname and desc strings should be 200
	   characters or less, the bsize field must not be negative, the
	   format should be legal, group numbers should be the same if and
	   only if file names are the same, and group numbers should begin
	   at zero and increase in steps of 1. */
	if (strlen(si->fname) + strlen(si->desc) > 200 ||
	    si->bsize < 0 || !isfmt(si->fmt)) {
	    wfdb_error("osigfopen: error in specification of signal %d\n",
		       s);
	    return (-2);
	}
	if (!((s == 0 && si->group == 0) ||
	    (s && si->group == (si-1)->group &&
	     strcmp(si->fname, (si-1)->fname) == 0) ||
	    (s && si->group == (si-1)->group + 1 &&
	     strcmp(si->fname, (si-1)->fname) != 0))) {
	    wfdb_error(
		     "osigfopen: incorrect file name or group for signal %d\n",
		     s);
	    return (-2);
	}
    }

    return (openosig("osigfopen", NULL, siarray, nsig));
}

int osigfopen(const WFDB_Siginfo *siarray, unsigned int nsig)
{
    return osigfopen_ctx(wfdb_get_default_context(), siarray, nsig);
}

/* Function findsig finds an open input signal with the name specified by its
(string) argument, and returns the associated signal number.  If the argument
is a decimal numeral and is less than the number of open input signals, it is
assumed to represent a signal number, which is returned.  Otherwise, findsig
looks for a signal with a description matching the string, and returns the
first match if any, or -1 if not. */

int findsig_ctx(WFDB_Context *ctx, const char *p)
{
  const char *q = p;
  int s;

  while ('0' <= *q && *q <= '9')
      q++;
  if (*q == 0) {	/* all digits, probably a signal number */
      s = strtol(p, NULL, 10);
      if (s < nisig || s < nvsig) return (s);
  }
  /* Otherwise, p is either an integer too large to be a signal number or a
     string containing a non-digit character.  Assume it's a signal name. */
  if (need_sigmap) {
      for (s = 0; s < nvsig; s++)
	  if ((q = vsd[s]->info.desc) && strcmp(p, q) == 0) return (s);
  }
  else {
      for (s = 0; s < nisig; s++)
	  if ((q = isd[s]->info.desc) && strcmp(p, q) == 0) return (s);
  }
  /* No match found. */
  return (-1);
}

int findsig(const char *p)
{
    return findsig_ctx(wfdb_get_default_context(), p);
}

/* Function getvec can operate in two different modes when reading
multifrequency records.  In WFDB_LOWRES mode, getvec returns one sample of each
signal per frame (decimating any oversampled signal by returning the average of
all of its samples within the frame).  In WFDB_HIGHRES mode, each sample of any
oversampled signal is returned by successive invocations of getvec; samples of
signals sampled at lower frequencies are returned on two or more successive
invocations of getvec as appropriate.  Function setgvmode can be used to change
getvec's operating mode, which is determined by environment variable
WFDBGVMODE or constant DEFWFDBGVMODE by default.  When reading
ordinary records (with all signals sampled at the same frequency), getvec's
behavior is independent of its operating mode.

Since sfreq and ffreq are always positive, the effect of adding 0.5 to their
quotient before truncating it to an int (see below) is to round the quotient
to the nearest integer.  Although sfreq should always be an exact multiple
of ffreq, loss of precision in representing non-integer frequencies can cause
problems if this rounding is omitted.  Thanks to Guido Muesch for pointing out
this problem.
 */

int getspf_ctx(WFDB_Context *ctx)
{
    return ((sfreq != ffreq) ? (int)(sfreq/ffreq + 0.5) : 1);
}

int getspf(void)
{
    return getspf_ctx(wfdb_get_default_context());
}

void setgvmode_ctx(WFDB_Context *ctx, int mode)
{
    if (mode < 0) {	/* (re)set to default mode */
	char *p;

        if (p = getenv("WFDBGVMODE"))
	    mode = strtol(p, NULL, 10);
	else
	    mode = DEFWFDBGVMODE;
    }

    gvmode = mode & (WFDB_HIGHRES | WFDB_GVPAD);

    if ((mode & WFDB_HIGHRES) == WFDB_HIGHRES) {
	if (spfmax == 0) spfmax = 1;
	sfreq = ffreq * spfmax;
    }
    else {
	sfreq = ffreq;
    }
}

void setgvmode(int mode)
{
    setgvmode_ctx(wfdb_get_default_context(), mode);
}

int getgvmode_ctx(WFDB_Context *ctx)
{
    return (gvmode);
}

int getgvmode(void)
{
    return getgvmode_ctx(wfdb_get_default_context());
}

/* An application can specify the input sampling frequency it prefers by
   calling setifreq after opening the input record. */

int setifreq_ctx(WFDB_Context *ctx, WFDB_Frequency f)
{
    WFDB_Frequency error, g = sfreq;

    if (g <= 0.0) {
	ifreq = 0.0;
	wfdb_error("setifreq: no open input record\n");
	return (-1);
    }
    if (f > 0.0) {
	if (nvsig > 0) {
	    SREALLOC(gv0, nvsig, sizeof(WFDB_Sample));
	    SREALLOC(gv1, nvsig, sizeof(WFDB_Sample));
	}
	setafreq(ifreq = f);
	/* The 0.005 below is the maximum tolerable error in the resampling
	   frequency (in Hz).  The code in the while loop implements Euclid's
	   algorithm for finding the greatest common divisor of two integers,
	   but in this case the integers are (implicit) multiples of 0.005. */
	while ((error = f - g) > 0.005 || error < -0.005)
	    if (f > g) f -= g;
	    else g -= f;
	/* f is now the GCD of sfreq and ifreq in the sense described above.
	   We divide each raw sampling interval into mticks subintervals. */
        mticks = (long)(sfreq/f + 0.5);
	/* We divide each resampled interval into nticks subintervals. */
	nticks = (long)(ifreq/f + 0.5);
	/* Raw and resampled intervals begin simultaneously once every mnticks
	   subintervals; we say an epoch begins at these times. */
	mnticks = mticks * nticks;
	/* gvtime is the number of subintervals from the beginning of the
	   current epoch to the next sample to be returned by getvec(). */
	gvtime = 0;
	rgvstat = rgetvec(gv0);
	rgvstat = rgetvec(gv1);
	/* rgvtime is the number of subintervals from the beginning of the
	   current epoch to the most recent sample returned by rgetvec(). */
	rgvtime = nticks;
	return (0);
    }
    else {
	ifreq = 0.0;
	wfdb_error("setifreq: improper frequency %g (must be > 0)\n", f);
	return (-1);
    }
}

int setifreq(WFDB_Frequency f)
{
    return setifreq_ctx(wfdb_get_default_context(), f);
}

WFDB_Frequency getifreq_ctx(WFDB_Context *ctx)
{
    return (ifreq > (WFDB_Frequency)0 ? ifreq : sfreq);
}

WFDB_Frequency getifreq(void)
{
    return getifreq_ctx(wfdb_get_default_context());
}

int getvec_ctx(WFDB_Context *ctx, WFDB_Sample *vector)
{
    int i, nsig;

    if (ifreq == 0.0 || ifreq == sfreq)	/* no resampling necessary */
	return (rgetvec(vector));

    /* Resample the input. */
    if (rgvtime > mnticks) {
	rgvtime -= mnticks;
	gvtime  -= mnticks;
    }
    nsig = (nvsig > nisig) ? nvsig : nisig;
    while (gvtime > rgvtime) {
	for (i = 0; i < nsig; i++)
	    gv0[i] = gv1[i];
	rgvstat = rgetvec(gv1);
	rgvtime += nticks;
    }
    for (i = 0; i < nsig; i++) {
	vector[i] = gv0[i] + (gvtime%nticks)*(gv1[i]-gv0[i])/nticks;
        gv0[i] = gv1[i];
    }
    gvtime += mticks;
    return (rgvstat);
}

int getvec(WFDB_Sample *vector)
{
    return getvec_ctx(wfdb_get_default_context(), vector);
}

int getframe_ctx(WFDB_Context *ctx, WFDB_Sample *vector)
{
    int stat = -1;

    if (dsbuf) {	/* signals must be deskewed */
	int c, i, j, s;

	/* First, obtain the samples needed. */
	if (dsbi < 0) {	/* dsbuf contents are invalid -- refill dsbuf */
	    for (dsbi = i = 0; i < dsblen; dsbi = i += tspf) {
		stat = getskewedframe(dsbuf + dsbi);
		if (stat < 0)
		    break;
	    }
	    dsbi = 0;
	}
	else {		/* replace oldest frame in dsbuf only */
	    stat = getskewedframe(dsbuf + dsbi);
	    if ((dsbi += tspf) >= dsblen) dsbi = 0;
	}

	/* Assemble the deskewed frame from the data in dsbuf. */
	for (j = s = 0; s < nvsig; s++) {
	    if ((i = j + dsbi + vsd[s]->skew*tspf) >= dsblen) i %= dsblen;
	    for (c = 0; c < vsd[s]->info.spf; c++)
		vector[j++] = dsbuf[i++];
	}
    }
    else		/* no deskewing necessary */
	stat = getskewedframe(vector);
    istime++;
    return (stat);
}

int getframe(WFDB_Sample *vector)
{
    return getframe_ctx(wfdb_get_default_context(), vector);
}

int putvec_ctx(WFDB_Context *ctx, const WFDB_Sample *vector)
{
    int c, dif, stat = (int)nosig;
    struct osdata *os;
    struct ogdata *og;
    WFDB_Signal s;
    WFDB_Group g;
    WFDB_Sample samp;

    for (s = 0; s < nosig; s++) {
	os = osd[s];
	g = os->info.group;
	og = ogd[os->info.group];
	if (os->info.nsamp++ == (WFDB_Time)0L)
	    os->info.initval = os->samp = *vector;
	for (c = 0; c < os->info.spf; c++, vector++) {
	    /* Replace invalid samples with lowest possible value for format */
	    if ((samp = *vector) == WFDB_INVALID_SAMPLE)
		switch (os->info.fmt) {
		  case 0:
		  case 8:
		  case 16:
		  case 61:
		  case 160:
		  case 516:
		  default:
		    samp = -1 << 15; break;
		  case 80:
		  case 508:
		    samp = -1 << 7; break;
		  case 212:
		    samp = -1 << 11; break;
		  case 310:
		  case 311:
		    samp = -1 << 9; break;
		  case 24:
		  case 524:
		    samp = -1 << 23; break;
		  case 32:
		    samp = -1 << 31; break;
		}
	    switch (os->info.fmt) {
	      case 0:	/* null signal (do not write) */
		os->samp = samp; break;
	      case 8:	/* 8-bit first differences */
	      default:
		/* Handle large slew rates sensibly. */
		if ((dif = samp - os->samp) < -128) { dif = -128; stat=0; }
		else if (dif > 127) { dif = 127; stat = 0; }
		os->samp += dif;
		w8(dif, og);
		break;
	      case 16:	/* 16-bit amplitudes */
		w16(samp, og); os->samp = samp; break;
	      case 61:	/* 16-bit amplitudes, bytes swapped */
		w61(samp, og); os->samp = samp; break;
	      case 80:	/* 8-bit offset binary amplitudes */
		w80(samp, og); os->samp = samp; break;
	      case 160:	/* 16-bit offset binary amplitudes */
		w160(samp, og); os->samp = samp; break;
	      case 212:	/* 2 12-bit amplitudes bit-packed in 3 bytes */
		w212(samp, og); os->samp = samp; break;
	      case 310:	/* 3 10-bit amplitudes bit-packed in 4 bytes */
		w310(samp, og); os->samp = samp; break;
	      case 311:	/* 3 10-bit amplitudes bit-packed in 4 bytes */
		w311(samp, og); os->samp = samp; break;
	      case 24: /* 24-bit amplitudes */
	        w24(samp, og); os->samp = samp; break;
	      case 32: /* 32-bit amplitudes */
	        w32(samp, og); os->samp = samp; break;

	      case 508: /* 8-bit compressed FLAC */
	      case 516:	/* 16-bit compressed FLAC */
	      case 524:	/* 24-bit compressed FLAC */
		if (flac_putsamp(samp, os->info.fmt, og) < 0)
		    stat = -1;
		os->samp = samp;
		break;
	    }
	    if (wfdb_ferror(og->fp)) {
		wfdb_error("putvec: write error in signal %d\n", s);
		stat = -1;
	    }
	    else
		os->info.cksum += os->samp;
	}
    }
    ostime++;
    return (stat);
}

int putvec(const WFDB_Sample *vector)
{
    return putvec_ctx(wfdb_get_default_context(), vector);
}

int isigsettime_ctx(WFDB_Context *ctx, WFDB_Time t)
{
    WFDB_Group g;
    WFDB_Time curtime;
    int stat = 0;

    /* Return immediately if no seek is needed. */
    if (nisig == 0) return (0);
    if (ifreq <= (WFDB_Frequency)0) {
	if (!(gvmode & WFDB_HIGHRES) || ispfmax < 2)
	    curtime = istime;
	else
	    curtime = (istime - 1) * ispfmax + gvc;
	if (t == curtime) return (0);
    }

    for (g = 1; g < nigroup; g++)
        if ((stat = isgsettime(g, t)) < 0) break;
    /* Seek on signal group 0 last (since doing so updates istime and would
       confuse isgsettime if done first). */
    if (stat == 0) stat = isgsettime(0, t);
    return (stat);
}

int isigsettime(WFDB_Time t)
{
    return isigsettime_ctx(wfdb_get_default_context(), t);
}
    
int isgsettime_ctx(WFDB_Context *ctx, WFDB_Group g, WFDB_Time t)
{
    int spf, stat, trem = 0;
    double tt;

    /* Handle negative arguments as equivalent positive arguments. */
    if (t < 0L) {
	if (t < -WFDB_TIME_MAX) {
	    wfdb_error("isigsettime: improper seek on signal group %d\n", g);
	    return (-1);
	}
	t = -t;
    }

    /* Convert t to raw sample intervals if we are resampling. */
    if (ifreq > (WFDB_Frequency)0) {
	tt = t * sfreq/ifreq;
	if (tt > WFDB_TIME_MAX) {
	    wfdb_error("isigsettime: improper seek on signal group %d\n", g);
	    return (-1);
	}
	t = (WFDB_Time) tt;
    }

    /* If we're in WFDB_HIGHRES mode, convert t from samples to frames, and
       save the remainder (if any) in trem. */
    if (gvmode & WFDB_HIGHRES) {
	trem = t % ispfmax;
	t /= ispfmax;
    }

    /* Mark the contents of the deskewing buffer (if any) as invalid. */
    dsbi = -1;

    if ((stat = isgsetframe(g, t)) == 0 && g == 0) {
	while (trem-- > 0) {
	    if (rgetvec(uvector) < 0) {
		wfdb_error("isigsettime: improper seek on signal group %d\n",
			   g);
		return (-1);
	    }
	}
	if (ifreq > (WFDB_Frequency)0 && ifreq != sfreq) {
	    gvtime = 0;
	    rgvstat = rgetvec(gv0);
	    rgvstat = rgetvec(gv1);
	    rgvtime = nticks;
	}
    }

    return (stat);
}

int isgsettime(WFDB_Group g, WFDB_Time t)
{
    return isgsettime_ctx(wfdb_get_default_context(), g, t);
}

WFDB_Time tnextvec_ctx(WFDB_Context *ctx, WFDB_Signal s, WFDB_Time t)
{
    int stat = 0;
    WFDB_Time tf;

    if (in_msrec && need_sigmap) { /* variable-layout multi-segment record */
	if (s >= nvsig) {
	    wfdb_error("nextvect: illegal signal number %d\n", s);
	    return ((WFDB_Time) -1);
	}
	/* Go to the start (t) if not already there. */
	if (t != istime && isigsettime(t) < 0) return ((WFDB_Time) -1);
	while (stat >= 0) {
	    char *p = vsd[s]->info.desc, *q;
	    int ss;

	    tf = segp->samp0 + segp->nsamp;  /* end of current segment */
	    /* Check if signal s is available in the current segment. */
	    for (ss = 0; ss < nisig; ss++)
		if ((q = isd[ss]->info.desc) && strcmp(p, q) == 0)
		    break;
	    if (ss < nisig) {
		/* The current segment contains the desired signal.
		   Read samples until we find a valid one or reach
		   the end of the segment. */
		for ( ; t <= tf && (stat = getvec(vvector)) > 0; t++)
		    if (vvector[s] != WFDB_INVALID_SAMPLE) {
			isigsettime(t);
			return (t);
		    }
		if (stat < 0) return ((WFDB_Time) -1);
	    }
	    /* Go on to the next segment. */
	    if (t != tf) stat = isigsettime(t = tf);
	}
    }
    else {	/* single-segment or fixed-layout multi-segment record */
	/* Go to the start (t) if not already there. */
	if (t != istime && isigsettime(t) < 0) return ((WFDB_Time) -1);
	if (s >= nisig) {
	    wfdb_error("nextvect: illegal signal number %d\n", s);
	    return ((WFDB_Time) -1);
	}
	for ( ; (stat = getvec(vvector)) > 0; t++)
	    /* Read samples until we find a valid one or reach the end of the
	       record. */
	    if (vvector[s] != WFDB_INVALID_SAMPLE) {
		isigsettime(t);
		return (t);
	    }
    }
    /* Error or end of record without finding another sample of signal s. */
    return ((WFDB_Time) stat);
}

WFDB_Time tnextvec(WFDB_Signal s, WFDB_Time t)
{
    return tnextvec_ctx(wfdb_get_default_context(), s, t);
}

int setibsize_ctx(WFDB_Context *ctx, int n)
{
    if (nisig) {
	wfdb_error("setibsize: can't change buffer size after isigopen\n");
	return (-1);
    }
    if (n < 0) {
	wfdb_error("setibsize: illegal buffer size %d\n", n);
	return (-2);
    }
    if (n == 0) n = BUFSIZ;
    return (ibsize = n);
}

int setibsize(int n)
{
    return setibsize_ctx(wfdb_get_default_context(), n);
}

int setobsize_ctx(WFDB_Context *ctx, int n)
{
    if (nosig) {
	wfdb_error("setobsize: can't change buffer size after osig[f]open\n");
	return (-1);
    }
    if (n < 0) {
	wfdb_error("setobsize: illegal buffer size %d\n", n);
	return (-2);
    }
    if (n == 0) n = BUFSIZ;
    return (obsize = n);
}

int setobsize(int n)
{
    return setobsize_ctx(wfdb_get_default_context(), n);
}

int newheader_ctx(WFDB_Context *ctx, char *record)
{
    int stat;
    WFDB_Signal s;
    WFDB_Siginfo *osi;

    /* Remove trailing .hea, if any, from record name. */
    wfdb_striphea(record);

    SUALLOC(osi, nosig, sizeof(WFDB_Siginfo));
    for (s = 0; s < nosig; s++)
	copysi(&osi[s], &osd[s]->info);
    stat = setheader(record, osi, nosig);
    for (s = 0; s < nosig; s++) {
	SFREE(osi[s].fname);
	SFREE(osi[s].desc);
	SFREE(osi[s].units);
    }
    SFREE(osi);
    return (stat);
}

int newheader(char *record)
{
    return newheader_ctx(wfdb_get_default_context(), record);
}

int setheader_ctx(WFDB_Context *ctx, char *record, const WFDB_Siginfo *siarray, unsigned int nsig)
{
    char *p;
    WFDB_Signal s;

    /* If another output header file was opened, close it. */
    if (oheader) {
	(void)wfdb_fclose(oheader);
	if (outinfo == oheader) outinfo = NULL;
	oheader = NULL;
    }

    /* Remove trailing .hea, if any, from record name. */
    wfdb_striphea(record);

    /* Quit (with message from wfdb_checkname) if name is illegal. */
    if (wfdb_checkname(record, "record"))
	return (-1);

    /* Try to create the header file. */
    if ((oheader = wfdb_open("hea", record, WFDB_WRITE)) == NULL) {
	wfdb_error("newheader: can't create header for record %s\n", record);
	return (-1);
    }

    /* Write the general information line. */
    (void)wfdb_fprintf(oheader, "%s %d %.12g", record, nsig, ffreq);
    if ((cfreq > 0.0 && cfreq != ffreq) || bcount != 0.0) {
	(void)wfdb_fprintf(oheader, "/%.12g", cfreq);
	if (bcount != 0.0)
	    (void)wfdb_fprintf(oheader, "(%.12g)", bcount);
    }
    (void)wfdb_fprintf(oheader, " %ld", nsig > 0 ? siarray[0].nsamp : 0L);
    if (btime != 0L || bdate != (WFDB_Date)0) {
	if (btime == 0L)
	    (void)wfdb_fprintf(oheader, " 0:00");
        else if (btime % 1000 == 0)
	    (void)wfdb_fprintf(oheader, " %s",
			       ftimstr(btime, 1000.0));
	else
	    (void)wfdb_fprintf(oheader, " %s",
			       fmstimstr(btime, 1000.0));
    }
    if (bdate)
	(void)wfdb_fprintf(oheader, "%s", datstr(bdate));
    (void)wfdb_fprintf(oheader, "\r\n");

    /* Write a signal specification line for each signal. */
    for (s = 0; s < nsig; s++) {
	(void)wfdb_fprintf(oheader, "%s %d", siarray[s].fname, siarray[s].fmt);
	if (siarray[s].spf > 1)
	    (void)wfdb_fprintf(oheader, "x%d", siarray[s].spf);
	if (osd && osd[s]->skew)
	    (void)wfdb_fprintf(oheader, ":%d", osd[s]->skew*siarray[s].spf);
	if (ogd && ogd[osd[s]->info.group]->start)
	    (void)wfdb_fprintf(oheader, "+%ld",
			       ogd[osd[s]->info.group]->start);
	else if (prolog_bytes)
	    (void)wfdb_fprintf(oheader, "+%ld", prolog_bytes);
	(void)wfdb_fprintf(oheader, " %.12g", siarray[s].gain);
	if (siarray[s].baseline != siarray[s].adczero)
	    (void)wfdb_fprintf(oheader, "(%d)", siarray[s].baseline);
	if (siarray[s].units && (p = strtok(siarray[s].units, " \t\n\r")))
	    (void)wfdb_fprintf(oheader, "/%s", p);
	(void)wfdb_fprintf(oheader, " %d %d %d %d %d",
		     siarray[s].adcres, siarray[s].adczero, siarray[s].initval,
		     (short int)(siarray[s].cksum & 0xffff), siarray[s].bsize);
	if (siarray[s].desc && (p = strtok(siarray[s].desc, "\n\r")))
	    (void)wfdb_fprintf(oheader, " %s", p);
	(void)wfdb_fprintf(oheader, "\r\n");
    }
    prolog_bytes = 0L;
    (void)wfdb_fflush(oheader);
    return (0);
}

int setheader(char *record, const WFDB_Siginfo *siarray, unsigned int nsig)
{
    return setheader_ctx(wfdb_get_default_context(), record, siarray, nsig);
}

int getseginfo_ctx(WFDB_Context *ctx, WFDB_Seginfo **sarray)
{
    *sarray = segarray;
    return (segments);
}

int getseginfo(WFDB_Seginfo **sarray)
{
    return getseginfo_ctx(wfdb_get_default_context(), sarray);
}

int setmsheader_ctx(WFDB_Context *ctx, char *record, char **segment_name, unsigned int nsegments)
{
    WFDB_Frequency msfreq = 0, mscfreq = 0;
    double msbcount = 0;
    int n, nsig = 0, old_in_msrec = in_msrec;
    WFDB_Time *ns;
    unsigned i;

    isigclose();	/* close any open input signals */

    /* If another output header file was opened, close it. */
    if (oheader) {
	(void)wfdb_fclose(oheader);
	if (outinfo == oheader) outinfo = NULL;
	oheader = NULL;
    }

    /* Remove trailing .hea, if any, from record name. */
    wfdb_striphea(record);

    /* Quit (with message from wfdb_checkname) if name is illegal. */
    if (wfdb_checkname(record, "record"))
	return (-1);

    if (nsegments < 1) {
	wfdb_error("setmsheader: record must contain at least one segment\n");
	return (-1);
    }

    SUALLOC(ns, nsegments, sizeof(WFDB_Time));
    for (i = 0; i < nsegments; i++) {
	if (strlen(segment_name[i]) > WFDB_MAXRNL) {
	    wfdb_error(
	     "setmsheader: `%s' is too long for a segment name in record %s\n",
		     segment_name[i], record);
	    SFREE(ns);
	    return (-2);
	}
	in_msrec = 1;
	nsamples = 0;
	n = readheader(segment_name[i]);
	in_msrec = old_in_msrec;
	if (n < 0) {
	    wfdb_error("setmsheader: can't read segment %s header\n",
		     segment_name[i]);
	    SFREE(ns);
	    return (-3);
	}
	if ((ns[i] = nsamples) <= 0L) {
	    wfdb_error("setmsheader: length of segment %s must be specified\n",
		     segment_name[i]);
	    SFREE(ns);
	    return (-4);
	}
	if (i == 0) {
	    nsig = n;
	    msfreq = ffreq;
	    mscfreq = cfreq;
	    msbcount = bcount;
	    msbtime = btime;
	    msbdate = bdate;
	    msnsamples = ns[i];
	}
	else {
	    if (nsig != n) {
		wfdb_error(
		    "setmsheader: incorrect number of signals in segment %s\n",
			 segment_name[i]);
		SFREE(ns);
		return (-4);
	    }
	    if (msfreq != ffreq) {
		wfdb_error(
		   "setmsheader: incorrect sampling frequency in segment %s\n",
			 segment_name[i]);
		SFREE(ns);
		return (-4);
	    }
	    msnsamples += ns[i];
	}
    }

    /* Try to create the header file. */
    if ((oheader = wfdb_open("hea", record, WFDB_WRITE)) == NULL) {
	wfdb_error("setmsheader: can't create header file for record %s\n",
		 record);
	SFREE(ns);
	return (-1);
    }

    /* Write the first line of the master header. */
    (void)wfdb_fprintf(oheader,"%s/%u %d %.12g", record, nsegments, nsig, msfreq);
    if ((mscfreq > 0.0 && mscfreq != msfreq) || msbcount != 0.0) {
	(void)wfdb_fprintf(oheader, "/%.12g", mscfreq);
	if (msbcount != 0.0)
	    (void)wfdb_fprintf(oheader, "(%.12g)", msbcount);
    }
    (void)wfdb_fprintf(oheader, " %"WFDB_Pd_TIME, msnsamples);
    if (msbtime != 0L || msbdate != (WFDB_Date)0) {
        if (msbtime % 1000 == 0)
	    (void)wfdb_fprintf(oheader, " %s",
			       ftimstr(msbtime, 1000.0));
	else
	    (void)wfdb_fprintf(oheader, " %s",
			       fmstimstr(msbtime, 1000.0));
    }
    if (msbdate)
	(void)wfdb_fprintf(oheader, "%s", datstr(msbdate));
    (void)wfdb_fprintf(oheader, "\r\n");

    /* Write a line for each segment. */
    for (i = 0; i < nsegments; i++)
	(void)wfdb_fprintf(oheader, "%s %"WFDB_Pd_TIME"\r\n",
			   segment_name[i], ns[i]);

    SFREE(ns);
    return (0);
}

int setmsheader(char *record, char **segment_name, unsigned int nsegments)
{
    return setmsheader_ctx(wfdb_get_default_context(), record, segment_name, nsegments);
}

int wfdbgetskew_ctx(WFDB_Context *ctx, WFDB_Signal s)
{
    if (s < nvsig)
	return (vsd[s]->skew);
    else
	return (0);
}

int wfdbgetskew(WFDB_Signal s)
{
    return wfdbgetskew_ctx(wfdb_get_default_context(), s);
}

/* Careful!!  This function is dangerous, and should be used only to restore
   skews when they have been reset as a side effect of using, e.g., sampfreq */
void wfdbsetiskew_ctx(WFDB_Context *ctx, WFDB_Signal s, int skew)
{
    if (s < nvsig && skew >= 0 && skew < dsblen / tspf)
        vsd[s]->skew = skew;
}

void wfdbsetiskew(WFDB_Signal s, int skew)
{
    wfdbsetiskew_ctx(wfdb_get_default_context(), s, skew);
}

/* Note: wfdbsetskew affects *only* the skew to be written by setheader.
   It does not affect how getframe deskews input signals, nor does it
   affect the value returned by wfdbgetskew. */
void wfdbsetskew_ctx(WFDB_Context *ctx, WFDB_Signal s, int skew)
{
    if (s < nosig)
	osd[s]->skew = skew;
}

void wfdbsetskew(WFDB_Signal s, int skew)
{
    wfdbsetskew_ctx(wfdb_get_default_context(), s, skew);
}

long wfdbgetstart_ctx(WFDB_Context *ctx, WFDB_Signal s)
{
    if (s < nisig)
        return (igd[vsd[s]->info.group]->start);
    else if (s == 0 && hsd != NULL)
	return (hsd[0]->start);
    else
	return (0L);
}

long wfdbgetstart(WFDB_Signal s)
{
    return wfdbgetstart_ctx(wfdb_get_default_context(), s);
}

/* Note: wfdbsetstart affects *only* the byte offset to be written by
   setheader.  It does not affect how isgsettime calculates byte offsets, nor
   does it affect the value returned by wfdbgetstart. */
void wfdbsetstart_ctx(WFDB_Context *ctx, WFDB_Signal s, long int bytes)
{
    if (s < nosig)
        ogd[osd[s]->info.group]->start = bytes;
    prolog_bytes = bytes;
}

void wfdbsetstart(WFDB_Signal s, long int bytes)
{
    wfdbsetstart_ctx(wfdb_get_default_context(), s, bytes);
}

int wfdbputprolog_ctx(WFDB_Context *ctx, const char *buf, long int size, WFDB_Signal s)
{
    long int n;
    WFDB_Group g = osd[s]->info.group;

    n = wfdb_fwrite(buf, 1, size, ogd[g]->fp);
    wfdbsetstart(s, n);
    if (n != size)
	wfdb_error("wfdbputprolog: only %ld of %ld bytes written\n", n, size);
    return (n == size ? 0 : -1);
}

int wfdbputprolog(const char *buf, long int size, WFDB_Signal s)
{
    return wfdbputprolog_ctx(wfdb_get_default_context(), buf, size, s);
}

/* Create a .info file (or open it for appending) */
int setinfo_ctx(WFDB_Context *ctx, char *record)
{
    /* Close any previously opened output info file. */
    int stat = wfdb_oinfoclose();

    /* Quit unless a record name has been specified. */
    if (record == NULL) return (stat);

    /* Remove trailing .hea, if any, from record name. */
    wfdb_striphea(record);

    /* Quit (with message from wfdb_checkname) if name is illegal. */
    if (wfdb_checkname(record, "record"))
	return (-1);

    /* Try to create the .info file. */
    if ((outinfo = wfdb_open("info", record, WFDB_APPEND)) == NULL) {
	wfdb_error("setinfo: can't create info file for record %s\n", record);
	return (-1);
    }

    /* Success! */
    return (0);
}

int setinfo(char *record)
{
    return setinfo_ctx(wfdb_get_default_context(), record);
}

/* Write an info string to the open output .hea or .info file */
int putinfo_ctx(WFDB_Context *ctx, const char *s)
{
    if (outinfo == NULL) {
	if (oheader) outinfo = oheader;
	else {
	    wfdb_error("putinfo: caller has not specified a record name\n");
	    return (-1);
	}
    }
    (void)wfdb_fprintf(outinfo, "#%s\r\n", s);
    (void)wfdb_fflush(outinfo);
    return (0);
}

int putinfo(const char *s)
{
    return putinfo_ctx(wfdb_get_default_context(), s);
}

/* getinfo: On the first call, read all info strings from the .hea file and (if
available) the .info file for the specified record, and return a pointer to the
first one.  On subsequent calls, return a pointer to the next info string.
Return NULL if there are no more info strings. */

char *getinfo_ctx(WFDB_Context *ctx, char *record)
{
    char *buf = NULL, *p;
    size_t bufsize = 0;
    WFDB_FILE *ifile;

    if (record)
	wfdb_freeinfo();

    if (pinfo == NULL) {	/* info for record has not yet been read */
	if (record == NULL && (record = wfdb_getirec()) == NULL) {
	    wfdb_error("getinfo: caller did not specify record name\n");
	    return (NULL);
	}

	if (ninfo) {
	    wfdb_freeinfo();  /* free memory allocated previously to info */
	    ninfo = 0;
	}

	getinfo_index = 0;
	nimax = 16;	       /* initial allotment of info string pointers */
	SALLOC(pinfo, nimax, sizeof(char *));

	/* Read info from the .hea file, if available (skip for EDF files) */
	if (!isedf) {
	    /* Remove trailing .hea, if any, from record name. */
	    wfdb_striphea(record);
	    if ((ifile = wfdb_open("hea", record, WFDB_READ))) {
		while (wfdb_getline(&buf, &bufsize, ifile))
		    if (*buf != '#') break; /* skip initial comments, if any */
		while (wfdb_getline(&buf, &bufsize, ifile))
		    if (*buf == '#') break; /* skip header content */
		while (*buf) {	/* read and save info */
		    if (*buf == '#') {	    /* skip anything that isn't info */
			p = buf + strlen(buf) - 1;
			if (*p == '\n') *p-- = '\0';
			if (*p == '\r') *p = '\0';
			if (ninfo >= nimax) {
			    int j = nimax;
			    nimax += 16;
			    SREALLOC(pinfo, nimax, sizeof(char *));
			    memset(pinfo + j, 0, (size_t)(16*sizeof(char *)));
			}
			SSTRCPY(pinfo[ninfo], buf+1);
			ninfo++;
		    }
		    if (wfdb_getline(&buf, &bufsize, ifile) == 0) break;
		}
		wfdb_fclose(ifile);
	    }
	}
	/* Read more info from the .info file, if available */
	if ((ifile = wfdb_open("info", record, WFDB_READ))) {
	    while (wfdb_getline(&buf, &bufsize, ifile)) {
		if (*buf == '#') {
		    p = buf + strlen(buf) - 1;
		    if (*p == '\n') *p-- = '\0';
		    if (*p == '\r') *p = '\0';
		    if (ninfo >= nimax) {
			int j = nimax;
			nimax += 16;
			SREALLOC(pinfo, nimax, sizeof(char *));
			memset(pinfo + j, 0, (size_t)(16*sizeof(char *)));
		    }
		    SSTRCPY(pinfo[ninfo], buf+1);
		    ninfo++;
		}
	    }
	    wfdb_fclose(ifile);
	}
	SFREE(buf);
    }
    if (getinfo_index < ninfo)
	return pinfo[getinfo_index++];
    else
	return (NULL);
}

char *getinfo(char *record)
{
    return getinfo_ctx(wfdb_get_default_context(), record);
}

/* sample(s, t) provides buffered random access to the input signals.  The
arguments are the signal number (s) and the sample number (t); the returned
value is the sample from signal s at time t.  On return, the global variable
sample_vflag is true (non-zero) if the requested sample is not beyond the end
of the record, false (zero) otherwise.  The caller must open the input signals
and must set the global variable nisig to the number of input signals before
invoking sample().  Once this has been done, the caller may request samples in
any order. */

#define BUFLN   4096	/* must be a power of 2, see sample() */

WFDB_Sample sample_ctx(WFDB_Context *ctx, WFDB_Signal s, WFDB_Time t)
{
    WFDB_Sample v;
    int nsig = (nvsig > nisig) ? nvsig : nisig;

    /* Allocate the sample buffer on the first call. */
    if (sbuf == NULL) {
	SALLOC(sbuf, nsig, BUFLN*sizeof(WFDB_Sample));
	sample_tt = (WFDB_Time)-1L;
    }

    /* If the caller requested a sample from an unavailable signal, return
       an invalid value.  Note that sample_vflag is not cleared in this
       case.  */
    if (s < 0 || s >= nsig) {
        sample_vflag = -1;
	return (WFDB_INVALID_SAMPLE);
    }

    /* If the caller specified a negative sample number, prepare to return
       sample 0.  This behavior differs from the convention that only the
       absolute value of the sample number matters. */
    if (t < 0L) t = 0L;

    /* If the caller has requested a sample that is no longer in the buffer,
       or if the caller has requested a sample that is further ahead than the
       length of the buffer, we need to reset the signal file pointer(s).
       If we do this, we must be sure that the buffer is refilled so that
       any subsequent requests for samples between t - BUFLN+1 and t will
       receive correct responses. */
    if (t <= sample_tt - BUFLN || t > sample_tt + BUFLN) {
	sample_tt = t - BUFLN;
	if (sample_tt < 0L) sample_tt = -1L;
	if (isigsettime(sample_tt+1) < 0) {
	    sample_vflag = 0;
	    return (WFDB_INVALID_SAMPLE);
	}
    }
    /* If the requested sample is not yet in the buffer, read and buffer
       more samples.  If we reach the end of the record, clear sample_vflag
       and return the last valid value. */
    while (t > sample_tt)
        if (getvec(sbuf + nsig * ((++sample_tt)&(BUFLN-1))) < 0) {
	    --sample_tt;
	    sample_vflag = 0;
	    return (*(sbuf + nsig * (sample_tt&(BUFLN-1)) + s));
	}

    /* The requested sample is in the buffer.  Set sample_vflag and
       return the requested sample. */
    if ((v = *(sbuf + nsig * (t&(BUFLN-1)) + s)) == WFDB_INVALID_SAMPLE)
        sample_vflag = -1;
    else
        sample_vflag = 1;
    return (v);
}

WFDB_Sample sample(WFDB_Signal s, WFDB_Time t)
{
    return sample_ctx(wfdb_get_default_context(), s, t);
}

int sample_valid_ctx(WFDB_Context *ctx)
{
    return (sample_vflag);
}

int sample_valid(void)
{
    return sample_valid_ctx(wfdb_get_default_context());
}

/* Private functions (for use by other WFDB library functions only). */

void wfdb_sampquit(void)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    if (sbuf) {
	SFREE(sbuf);
	sample_vflag = 0;
    }
}

void wfdb_sigclose(void)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    isigclose();
    osigclose();
    btime = bdate = nsamples = msbtime = msbdate = msnsamples = 0;
    sfreq = ifreq = ffreq = 0;
    pdays = -1;
    segments = in_msrec = skewmax = 0;
    if (dsbuf) {
	SFREE(dsbuf);
	dsbi = -1;
    }
    if (segarray) {
	int i;

	SFREE(segarray);
	segp = segend = (WFDB_Seginfo *)NULL;
	for (i = 0; i < maxisig; i++) {
	    SFREE(isd[i]->info.fname);  /* missing before 10.4.6 */
	    SFREE(isd[i]->info.desc);
	    SFREE(isd[i]->info.units);
	}
    }
    SFREE(gv0);
    SFREE(gv1);
    SFREE(tvector);
    SFREE(uvector);
    SFREE(vvector);
    tuvlen = 0;

    sigmap_cleanup();
}

void wfdb_osflush(void)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    WFDB_Group g;
    WFDB_Signal s;
    struct ogdata *og;
    struct osdata *os;

    if (!osd || !ogd)
	return;

    for (s = 0; s < nosig; s++) {
	if ((os = osd[s]) && (og = ogd[os->info.group]) && og->nrewind == 0) {
	    if (!og->force_flush && og->seek == 0) {
		if (og->bsize == 0 && !wfdb_fseek(og->fp, 0L, SEEK_CUR))
		    og->seek = 1;
		else
		    og->seek = -1;
	    }
	    if (og->force_flush || og->seek > 0) {
		/* Either we are closing the file (osigclose() was called),
		   or the file is seekable: write out any
		   partially-completed sets of bit-packed samples. */
		switch (os->info.fmt) {
		  case 212: f212(og); break;
		  case 310: f310(og); break;
		  case 311: f311(og); break;
		  default: break;
		}
	    }
	}
    }

    for (g = 0; g < nogroup; g++) {
	og = ogd[g];
	if (og->bsize == 0 && og->bp != og->buf) {
	    (void)wfdb_fwrite(og->buf, 1, og->bp - og->buf, og->fp);
	    og->bp = og->buf;
	}
	(void)wfdb_fflush(og->fp);

	if (!og->force_flush && og->nrewind != 0) {
	    /* Rewind the file so that subsequent samples will be
	       written in the right place. */
	    wfdb_fseek(og->fp, -((long) og->nrewind), SEEK_CUR);
	    og->nrewind = 0;
	}
    }
}

/* Release resources allocated for info string handling */
void wfdb_freeinfo_ctx(WFDB_Context *ctx)
{
    int i;

    for (i = 0; i < nimax; i++)
	SFREE(pinfo[i]);
    SFREE(pinfo);
    nimax = ninfo = 0;
}

void wfdb_freeinfo(void)
{
    wfdb_freeinfo_ctx(wfdb_get_default_context());
}

/* Close any previously opened output info file. */
int wfdb_oinfoclose(void)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    int stat = 0, errflag;

    if (outinfo && outinfo != oheader) {
	errflag = wfdb_ferror(outinfo);
	if (wfdb_fclose(outinfo))
	    errflag = 1;
	if (errflag) {
	    wfdb_error("setinfo: write error in info file\n");
	    stat = -2;
	}
    }
    outinfo = NULL;
    return (stat);
}

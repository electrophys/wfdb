/* file: sigmap.c	G. Moody	 2026
   Variable-layout multi-segment signal mapping.

   This module handles transparent signal mapping for variable-layout
   multi-segment records, so that WFDB applications can assume all signals
   are always present in the same order with constant gains and baselines.
*/

#include "signal_internal.h"

/* Variable-layout multi-segment support */

/* For handling records in which the weights and calculation scheme may vary
   from one segment to the next:

   "Variable-layout" multi-segment records are handled by the code below,
   transparently, so that a WFDB application can assume that all signals
   are always present in the same order, with constant gains and baselines
   as specified in the .hea file for the first segment.  This .hea file
   is in the same format as an ordinary .hea file, but if its length is
   specified as zero samples, it is recognized as a "layout header".  In
   this case, the signal file name is conventionally given as "~" (although
   any name is acceptable), and the format is given as 0 (required).

   The "layout header" contains a signal description line for each signal
   of interest that appears anywhere in the record.  The gain and baseline
   specified in the layout header are those that will apply to samples
   throughout the record (sigmap, below, scales and shifts the raw samples
   as needed).

   If a gap occurs between the end of one segment and the beginning of the
   next, a special "null" segment can be listed in the master .hea file,
   like this:
       ~ 4590
   The segment name, "~", does not correspond to a real .hea file, but the
   number that follows indicates the length of the gap in sample intervals.
 */

void sigmap_cleanup(void)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    int i;

    need_sigmap = nvsig = tspf = vspfmax = 0;
    SFREE(ovec);
    if (smi) {
	for (i = 0; i < tspf; i += smi[i].spf)
	    SFREE(smi[i].desc);
	SFREE(smi);
    }

    if (vsd) {
	struct isdata *is;

	while (maxvsig)
	    if (is = vsd[--maxvsig]) {
		SFREE(is->info.fname);
		SFREE(is->info.units);
		SFREE(is->info.desc);
		SFREE(is);
	    }
    	SFREE(vsd);
    }
}

int make_vsd(void)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    int i;

    if (nvsig != nisig) {
	wfdb_error("make_vsd: oops! nvsig = %d, nisig = %d\n", nvsig, nisig);
	return (-1);
    }
    for (i = 0; i < maxvsig; i++) {
        SFREE(vsd[i]->info.fname);
        SFREE(vsd[i]->info.desc);
        SFREE(vsd[i]->info.units);
    }
    if (maxvsig < nvsig) {
	unsigned m = maxvsig;

	SREALLOC(vsd, nvsig, sizeof(struct isdata *));
	while (m < nvsig) {
	    SUALLOC(vsd[m], 1, sizeof(struct isdata));
	    m++;
	}
	maxvsig = nvsig;
    }

    for (i = 0; i < nvsig; i++) {
	copysi(&vsd[i]->info, &isd[i]->info);
	vsd[i]->skew = isd[i]->skew;
    }

    return (nvsig);
}

int sigmap_init(int first_segment)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    int i, j, k, kmax, s, ivmin, ivmax;
    double ovmin, ovmax;
    struct sigmapinfo *ps;

    /* is this the layout segment?  if so, set up output side of map */
    if (in_msrec && first_segment && segarray && segarray[0].nsamp == 0) {
	need_sigmap = 1;

	/* The number of virtual signals is the number of signals defined
	   in the layout segment. */
	nvsig = nisig;
	vspfmax = ispfmax;
	for (s = tspf = 0; s < nisig; s++)
	    tspf += isd[s]->info.spf;
	SALLOC(smi, tspf, sizeof(struct sigmapinfo));
	for (i = s = 0; i < nisig; i++) {
	    SSTRCPY(smi[s].desc, isd[i]->info.desc);
	    smi[s].gain = isd[i]->info.gain;
	    smi[s].baseline = isd[i]->info.baseline;
	    k = smi[s].spf = isd[i]->info.spf;
	    for (j = 1; j < k; j++)
		smi[s + j] = smi[s];
	    s += k;
	}
	SALLOC(ovec, tspf, sizeof(WFDB_Sample));
	return (make_vsd());
    }

    else if (need_sigmap) {	/* set up the input side of the map */
	for (s = 0; s < tspf; s++) {
	    smi[s].index = 0;
	    smi[s].scale = 0.;
	    smi[s].offset = 0.;
	    smi[s].sample_offset = WFDB_INVALID_SAMPLE;
	}
	ispfmax = vspfmax;

	if (isd[0]->info.fmt == 0 && nisig == 1)
	    return (0);    /* the current segment is a null record */

	for (i = j = 0; i < nisig; j += isd[i++]->info.spf)
	    for (s = 0; s < tspf; s += smi[s].spf)
		if (strcmp(smi[s].desc, isd[i]->info.desc) == 0) {
		    if ((kmax = smi[s].spf) != isd[i]->info.spf) {
			wfdb_error(
		   "sigmap_init: unexpected spf for signal %d in segment %s\n",
		                   i, segp->recname);
			if (kmax > isd[i]->info.spf)
			    kmax = isd[i]->info.spf;
		    }
		    for (k = 0; k < kmax; k++) {
			ps = &smi[s + k];
			ps->index = j + k;
			ps->scale = ps->gain / isd[i]->info.gain;
			if (ps->scale < 1.0)
			    wfdb_error(
	       "sigmap_init: loss of precision in signal %d in segment %s\n",
				       i, segp->recname);
			ps->offset = ps->baseline -
			             ps->scale * isd[i]->info.baseline + 0.5;

			/* If it is possible to add an additional
			   offset such that all possible output values
			   will fit into a positive signed integer, we
			   can use the "fast" case in sigmap, below. */
			switch (isd[i]->info.fmt) {
			  case 508:
			  case 80: ivmin = -0x80; ivmax = 0x7f; break;
			  case 310:
			  case 311: ivmin = -0x200; ivmax = 0x1ff; break;
			  case 212: ivmin = -0x800; ivmax = 0x7ff; break;
			  case 16:
			  case 61:
			  case 516:
			  case 160: ivmin = -0x8000; ivmax = 0x7fff; break;
			  case 524:
			  case 24: ivmin = -0x800000; ivmax = 0x7fffff; break;
			  default:
			    ivmin = WFDB_SAMPLE_MIN;
			    ivmax = WFDB_SAMPLE_MAX;
			    break;
			}
			ovmin = ivmin * ps->scale + ps->offset;
			ovmax = ivmax * ps->scale + ps->offset;
			if (ovmin < ovmax &&
			    ovmin >= WFDB_SAMPLE_MIN + 1 &&
			    ovmax <= WFDB_SAMPLE_MAX &&
			    ovmax - ovmin + 1 < WFDB_SAMPLE_MAX) {
			    ps->sample_offset = ovmin - 1;
			    ps->offset -= ps->sample_offset;
			}
			else {
			    ps->sample_offset = 0;
			}
		    }
		    break;
		}
	if (j > tspf) {
	    wfdb_error("sigmap_init: frame size too large in segment %s\n",
		       segp->recname);
	    return (-1);
	}
    }

    else if (in_msrec && !first_segment && framelen == 0) {
	/* opening a new segment of a fixed-layout multisegment
	   record */
	ispfmax = vspfmax;
	if (nisig > nvsig) {
	    wfdb_error("sigmap_init: wrong number of signals in segment %s\n",
		       segp->recname);
	    return (-1);
	}
	for (i = 0; i < nisig; i++) {
	    if (isd[i]->info.spf != vsd[i]->info.spf) {
		wfdb_error(
		    "sigmap_init: wrong spf for signal %d in segment %s\n",
		    i, segp->recname);
		return (-1);
	    }
	}
    }

    else {	/* normal record, or multisegment record without a dummy
		   header */
	nvsig = nisig;
	vspfmax = ispfmax;
	for (s = tspf = 0; s < nisig; s++)
	    tspf += isd[s]->info.spf;
	return (make_vsd());
    }

    return (0);
}

int sigmap(WFDB_Sample *vector, const WFDB_Sample *ivec)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    int i;
    double v;

    for (i = 0; i < tspf; i++) {
	if (ivec[smi[i].index] == WFDB_INVALID_SAMPLE)
	    vector[i] = WFDB_INVALID_SAMPLE;
	else {
	    /* Scale the input sample and round it to the nearest
	       integer.  Halfway cases are always rounded up (10.5 is
	       rounded to 11, but -10.5 is rounded to -10.)  Note that
	       smi[i].offset already includes an extra 0.5, so we
	       simply need to calculate the floor of v. */
	    v = ivec[smi[i].index] * smi[i].scale + smi[i].offset;
	    if (smi[i].sample_offset) {
		/* Fast case: if we can guarantee that v is always
		   positive and the following calculation cannot
		   overflow, we can avoid additional floating-point
		   operations. */
		vector[i] = (WFDB_Sample) v + smi[i].sample_offset;
	    }
	    else {
		/* Slow case: we need to check bounds and handle
		   negative values. */
		if (v >= 0) {
		    if (v <= WFDB_SAMPLE_MAX)
			vector[i] = (WFDB_Sample) v;
		    else
			vector[i] = WFDB_SAMPLE_MAX;
		}
		else {
		    if (v >= WFDB_SAMPLE_MIN) {
			vector[i] = (WFDB_Sample) v;
			if (vector[i] > v)
			    vector[i]--;
		    }
		    else {
			vector[i] = WFDB_SAMPLE_MIN;
		    }
		}
	    }
	}
    }
    return (tspf);
}

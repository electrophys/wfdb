/* file: sigformat.c   2026  Signal format-specific I/O for WFDB library. */
#include "signal_internal.h"

/* r212: read and return the next sample from a format 212 signal file
   (2 12-bit samples bit-packed in 3 bytes) */
static int r212(struct igdata *g)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    int v;

    /* Obtain the next 12-bit value right-justified in v. */
    switch (g->count++) {
      case 0:	v = g->data = r16(g); break;
      case 1:
      default:	g->count = 0;
	        v = ((g->data >> 4) & 0xf00) | (r8(g) & 0xff); break;
    }
    /* Sign-extend from the twelfth bit. */
    if (v & 0x800) v |= ~(0xfff);
    else v &= 0xfff;
    return (v);
}

/* w212: write the next sample to a format 212 signal file */
void w212(WFDB_Sample v, struct ogdata *g)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    /* Samples are buffered here and written in pairs, as three bytes. */
    switch (g->count++) {
      case 0:	g->data = v & 0xfff; break;
      case 1:	g->count = 0;
	  g->data |= (v << 4) & 0xf000;
	  w16(g->data, g);
	  w8(v, g);
		break;
    }
}

/* f212: flush output to a format 212 signal file */
void f212(struct ogdata *g)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    /* If we have one leftover sample, write it as two bytes. */
    if (g->count == 1) {
	w16(g->data, g);
	g->nrewind = 2;
    }
}

/* r310: read and return the next sample from a format 310 signal file
   (3 10-bit samples bit-packed in 4 bytes) */
static int r310(struct igdata *g)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    int v;

    /* Obtain the next 10-bit value right-justified in v. */
    switch (g->count++) {
      case 0:	v = (g->data = r16(g)) >> 1; break;
      case 1:	v = (g->datb = r16(g)) >> 1; break;
      case 2:
      default:	g->count = 0;
		v = ((g->data & 0xf800) >> 11) | ((g->datb & 0xf800) >> 6);
		break;
    }
    /* Sign-extend from the tenth bit. */
    if (v & 0x200) v |= ~(0x3ff);
    else v &= 0x3ff;
    return (v);
}

/* w310: write the next sample to a format 310 signal file */
void w310(WFDB_Sample v, struct ogdata *g)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    /* Samples are buffered here and written in groups of three, as two
       left-justified 15-bit words. */
    switch (g->count++) {
      case 0:	g->data = (v << 1) & 0x7fe; break;
      case 1:	g->datb = (v << 1) & 0x7fe; break;
      case 2:
      default:	g->count = 0;
	        g->data |= (v << 11); w16(g->data, g);
	        g->datb |= ((v <<  6) & ~0x7fe); w16(g->datb, g);
		break;
    }
}

/* f310: flush output to a format 310 signal file */
void f310(struct ogdata *g)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    switch (g->count) {
      case 0:  break;
      /* If we have one leftover sample, write it as two bytes. */
      case 1:  w16(g->data, g);
	       g->nrewind = 2;
	       break;
      /* If we have two leftover samples, write them as four bytes.
	 In this case, the file will appear to have an extra (zero)
	 sample at the end. */
      case 2:
      default: w16(g->data, g);
	       w16(g->datb, g);
	       g->nrewind = 4;
	       break;
    }
}

/* r311: read and return the next sample from a format 311 signal file
   (3 10-bit samples bit-packed in 4 bytes; note that formats 310 and 311
   differ in the layout of the bit-packed data) */
static int r311(struct igdata *g)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    int v;

    /* Obtain the next 10-bit value right-justified in v. */
    switch (g->count++) {
      case 0:	v = (g->data = r16(g)); break;
      case 1:	g->datb = (r8(g) & 0xff);
	        v = ((g->data & 0xfc00) >> 10) | ((g->datb & 0xf) << 6);
		break;
      case 2:
      default:	g->count = 0;
		g->datb |= r8(g) << 8;
		v = g->datb >> 4; break;
    }
    /* Sign-extend from the tenth bit. */
    if (v & 0x200) v |= ~(0x3ff);
    else v &= 0x3ff;
    return (v);
}

/* w311: write the next sample to a format 311 signal file */
void w311(WFDB_Sample v, struct ogdata *g)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    /* Samples are buffered here and written in groups of three, bit-packed
       into the 30 low bits of a 32-bit word. */
    switch (g->count++) {
      case 0:	g->data = v & 0x3ff; break;
      case 1:	g->data |= (v << 10); w16(g->data, g);
	        g->datb = (v >> 6) & 0xf; break;
      case 2:
      default:	g->count = 0;
	        g->datb |= (v << 4); g->datb &= 0x3fff; w16(g->datb, g);
		break;
    }
}

/* f311: flush output to a format 311 signal file */
void f311(struct ogdata *g)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    switch (g->count) {
      case 0:	break;
      /* If we have one leftover sample, write it as two bytes. */
      case 1:	w16(g->data, g);
		g->nrewind = 2;
		break;
      /* If we have two leftover samples, write them as four bytes
	 (note that the first two bytes will already have been written
	 by w311(), above.)  The file will appear to have an extra
	 (zero) sample at the end.  It would be possible to write only
	 three bytes here, but older versions of WFDB would not be
	 able to read the resulting file. */
      case 2:
      default:	w16(g->datb, g);
		g->nrewind = 2;
		break;
    }
}

int isgsetframe(WFDB_Group g, WFDB_Time t)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    int i, trem = 0;
    long nb, tt;
    struct igdata *ig;
    WFDB_Signal s;
    unsigned int b, d = 1, n, nn, j;

    /* Do nothing if there is no more than one input signal group and
       the input pointer is correct already. */
    if (nigroup < 2 && istime == t && gvc == ispfmax &&
	igd[g]->start == 0)
	return (0);

    /* Find the first signal that belongs to group g. */
    for (s = 0; s < nisig && g != isd[s]->info.group; s++)
	;
    if (s == nisig) {
	wfdb_error("isgsettime: incorrect signal group number %d\n", g);
	return (-2);
    }

    /* If the current record contains multiple segments, locate the segment
       containing the desired sample. */
    if (in_msrec) {
	WFDB_Seginfo *tseg = segp;
	WFDB_Group h;

	if (t >= msnsamples) {
	    wfdb_error("isigsettime: improper seek on signal group %d\n", g);
	    return (-1);
	}
	while (t < tseg->samp0)
	    tseg--;
	while (t >= tseg->samp0 + tseg->nsamp && tseg < segend)
	    tseg++;
	if (segp != tseg) {
	    segp = tseg;
	    if (isigopen(segp->recname, NULL, (int)nvsig) <= 0) {
	        wfdb_error("isigsettime: can't open segment %s\n",
			   segp->recname);
		return (-1);
	    }
	    /* Following isigopen(), nigroup may have changed and
	       group numbers may not make any sense anymore.  However,
	       isigsettime() will still call isgsettime() once for
	       each non-zero group (if the previous segment had
	       multiple groups) and then once for group zero.

	       Calling isgsetframe() multiple times for a non-zero
	       group is mostly harmless, but seeking on group 0 should
	       only be done once.  Thus, when we jump to a new
	       segment, implicitly seek on all non-zero groups
	       (regardless of g), but only seek on group 0 if g is 0.

	       (Note that isgsettime() is not and has never been fully
	       functional for multi-segment records, because it cannot
	       read signals from two different segments at once.) */
	    for (h = 1; h < nigroup; h++)
		if (i = isgsetframe(h, t))
		    return (i);
	    if (g == 0)
		return (isgsetframe(0, t));
	    else
		return (0);
	}
	t -= segp->samp0;
    }

    ig = igd[g];
    ig->initial_skip = 0;
    /* Determine the number of samples per frame for signals in the group. */
    for (n = nn = 0; s+n < nisig && isd[s+n]->info.group == g; n++)
	nn += isd[s+n]->info.spf;
    /* Determine the number of bytes per sample interval in the file. */
    switch (isd[s]->info.fmt) {
      case 0:
	if (t < nsamples) {
	    gvc = ispfmax;
	    if (s == 0) istime = (in_msrec) ? t + segp->samp0 : t;
	    isd[s]->info.nsamp = nsamples - t;
	    ig->stat = 1;
	    return (0);
	}
	else {
	    if (s == 0) istime = (in_msrec) ? msnsamples : nsamples;
	    isd[s]->info.nsamp = 0L;
	    return (-1);
	}

      case 508:
      case 516:
      case 524:
	if (flac_isseek(ig, t) < 0) {
	    wfdb_error("isigsettime: improper seek on signal group %d\n", g);
	    return (-1);
	}

	/* Reset the getvec sample-within-frame counter. */
	gvc = ispfmax;

	/* Reset the time (if signal 0 belongs to the group) and
	   disable checksum testing (by setting the number of samples
	   remaining to 0). */
	if (s == 0) istime = in_msrec ? t + segp->samp0 : t;
	while (n-- != 0)
	    isd[s+n]->info.nsamp = (WFDB_Time)0L;
	return (0);

      case 8:
      case 80:
      default: b = nn; break;
      case 16:
      case 61:
      case 160:
	if (nn > UINT_MAX / 2) {
	    wfdb_error("isigsettime: overflow in signal group %d\n", g);
	    return (-1);
	}
	b = 2*nn;
	break;
      case 212:
	if (nn > UINT_MAX / 3) {
	    wfdb_error("isigsettime: overflow in signal group %d\n", g);
	    return (-1);
	}
	/* Reset the input counter. */
	ig->count = 0;
	/* If the desired sample does not lie on a byte boundary, seek to
	   the previous sample and then read ahead. */
	if ((nn & 1) && (t & 1)) {
	    if (in_msrec)
		t += segp->samp0;	/* restore absolute time */
	    if (i = isgsetframe(g, t - 1))
		return (i);
	    for (j = 0; j < nn; j++)
		(void)r212(ig);
	    istime++;
	    for (n = 0; s+n < nisig && isd[s+n]->info.group == g; n++)
		isd[s+n]->info.nsamp = (WFDB_Time)0L;
	    return (0);
	}
	b = 3*nn; d = 2; break;
      case 310:
	if (nn > UINT_MAX / 4) {
	    wfdb_error("isigsettime: overflow in signal group %d\n", g);
	    return (-1);
	}
	/* Reset the input counter. */
	ig->count = 0;
	/* If the desired sample does not lie on a byte boundary, seek to
	   the closest previous sample that does, then read ahead. */
	if ((nn % 3) && (trem = (t % 3))) {
	    if (in_msrec)
		t += segp->samp0;	/* restore absolute time */
	    if (i = isgsetframe(g, t - trem))
		return (i);
	    for (j = nn*trem; j > 0; j--)
		(void)r310(ig);
	    istime += trem;
	    for (n = 0; s+n < nisig && isd[s+n]->info.group == g; n++)
		isd[s+n]->info.nsamp = (WFDB_Time)0L;
	    return (0);
	}
	b = 4*nn; d = 3; break;
      case 311:
	if (nn > UINT_MAX / 4) {
	    wfdb_error("isigsettime: overflow in signal group %d\n", g);
	    return (-1);
	}
	/* Reset the input counter. */
	ig->count = 0;
	/* If the desired sample does not lie on a byte boundary, seek to
	   the closest previous sample that does, then read ahead. */
	if ((nn % 3) && (trem = (t % 3))) {
	    if (in_msrec)
		t += segp->samp0;	/* restore absolute time */
	    if (i = isgsetframe(g, t - trem))
		return (i);
	    for (j = nn*trem; j > 0; j--)
		(void)r311(ig);
	    istime += trem;
	    for (n = 0; s+n < nisig && isd[s+n]->info.group == g; n++)
		isd[s+n]->info.nsamp = (WFDB_Time)0L;
	    return (0);
	}
	b = 4*nn; d = 3; break;
      case 24:
	if (nn > UINT_MAX / 3) {
	    wfdb_error("isigsettime: overflow in signal group %d\n", g);
	    return (-1);
	}
	b = 3*nn;
	break;
      case 32:
	if (nn > UINT_MAX / 4) {
	    wfdb_error("isigsettime: overflow in signal group %d\n", g);
	    return (-1);
	}
	b = 4*nn;
	break;
    }

    if (t > (LONG_MAX / b) || ((long) (t * b))/d > (LONG_MAX - ig->start)) {
	wfdb_error("isigsettime: improper seek on signal group %d\n", g);
	return (-1);
    }

    /* Seek to the beginning of the block which contains the desired sample.
       For normal files, use fseek() to do so. */
    if (ig->seek) {
	tt = t*b;
	nb = tt/d + ig->start;
	if ((i = ig->bsize) == 0) i = ibsize;
	/* Seek to a position such that the next block read will contain the
	   desired sample. */
	tt = nb/i;
	if (wfdb_fseek(ig->fp, tt*i, 0)) {
	    wfdb_error("isigsettime: improper seek on signal group %d\n", g);
	    return (-1);
	}
	nb %= i;
    }
    /* For special files, rewind if necessary and then read ahead. */
    else {
	WFDB_Time t0, t1;

	/* Get the time of the earliest buffered sample ... */
	t0 = istime - (ig->bp - ig->buf)/b;
	/* ... and that of the earliest unread sample. */
	t1 = t0 + (ig->be - ig->buf)/b;
	/* There are three possibilities:  either the desired sample has been
	   read and has passed out of the buffer, requiring a rewind ... */
	if (t < t0) {
	    if (wfdb_fseek(ig->fp, 0L, 0)) {
		wfdb_error("isigsettime: improper seek on signal group %d\n",
			   g);
		return (-1);
	    }
	    tt = t*b;
	    nb = tt/d + ig->start;
	}
	/* ... or it is in the buffer already ... */
	else if (t < t1) {
	    tt = (t - t0)*b;
	    ig->bp = ig->buf + tt/d;
	    return (0);
	}
	/* ... or it has not yet been read. */
	else {
	    tt = (t - t1) * b;
	    nb = tt/d;
	}
	while (nb > ig->bsize && !wfdb_feof(ig->fp))
	    nb -= wfdb_fread(ig->buf, 1, ig->bsize, ig->fp);
    }

    /* Reset the block pointer to indicate nothing has been read in the
       current block. */
    ig->bp = ig->be;
    ig->stat = 1;
    /* Read any bytes in the current block that precede the desired sample. */
    while (nb-- > 0 && ig->stat > 0)
	i = r8(ig);
    if (ig->stat <= 0) return (-1);

    /* Reset the getvec sample-within-frame counter. */
    gvc = ispfmax;

    /* Reset the time (if signal 0 belongs to the group) and disable checksum
       testing (by setting the number of samples remaining to 0). */
    if (s == 0) istime = in_msrec ? t + segp->samp0 : t;
    while (n-- != 0)
	isd[s+n]->info.nsamp = (WFDB_Time)0L;
    return (0);
}

/* VFILL provides the value returned by getskewedframe() for a missing or
   invalid sample */
#define VFILL	((gvmode & WFDB_GVPAD) ? is->samp : WFDB_INVALID_SAMPLE)

int getskewedframe(WFDB_Sample *vector)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    int c, stat;
    struct isdata *is;
    struct igdata *ig;
    WFDB_Group g;
    WFDB_Sample v, *vecstart = vector;
    WFDB_Signal s;

    if ((stat = (int)nisig) == 0) return (nvsig > 0 ? -1 : 0);
    if (istime == 0L) {
	for (s = 0; s < nisig; s++)
	    isd[s]->samp = isd[s]->info.initval;
    }
    for (g = nigroup; g; ) {
	/* Go through groups in reverse order since seeking on group 0
	   should always be done last. */
	if (igd[--g]->initial_skip)
	    isgsetframe(g, (in_msrec ? segp->samp0 : 0));
    }

    /* If the vector needs to be rearranged (variable-layout record),
       then read samples into a temporary buffer. */
    if (need_sigmap)
	vector = ovec;

    for (s = 0; s < nisig; s++) {
	is = isd[s];
	ig = igd[is->info.group];
	for (c = 0; c < is->info.spf; c++, vector++) {
	    switch (is->info.fmt) {
	      case 0:	/* null signal: return sample tagged as invalid */
		  *vector = v = VFILL;
		if (is->info.nsamp == 0) ig->stat = -1;
		break;
	      case 8:	/* 8-bit first differences */
	      default:
		*vector = v = is->samp += r8(ig); break;
	      case 16:	/* 16-bit amplitudes */
		*vector = v = r16(ig);
		if (v == -1 << 15)
		    *vector = VFILL;
		else
		    is->samp = *vector;
		break;
	      case 61:	/* 16-bit amplitudes, bytes swapped */
		*vector = v = r61(ig);
		if (v == -1 << 15)
		    *vector = VFILL;
		else
		    is->samp = *vector;
		break;
	      case 80:	/* 8-bit offset binary amplitudes */
		*vector = v = r80(ig);
		if (v == -1 << 7)
		    *vector = VFILL;
		else
		    is->samp = *vector;
		break;
	      case 160:	/* 16-bit offset binary amplitudes */
		*vector = v = r160(ig);
		if (v == -1 << 15)
		    *vector = VFILL;
		else
		    is->samp = *vector;
		break;
	      case 212:	/* 2 12-bit amplitudes bit-packed in 3 bytes */
		*vector = v = r212(ig);
		if (v == -1 << 11)
		    *vector = VFILL;
		else
		    is->samp = *vector;
		break;
	      case 310:	/* 3 10-bit amplitudes bit-packed in 4 bytes */
		*vector = v = r310(ig);
		if (v == -1 << 9)
		    *vector = VFILL;
		else
		    is->samp = *vector;
		break;
	      case 311:	/* 3 10-bit amplitudes bit-packed in 4 bytes */
		*vector = v = r311(ig);
		if (v == -1 << 9)
		    *vector = VFILL;
		else
		    is->samp = *vector;
		break;
	      case 24:	/* 24-bit amplitudes */
		*vector = v = r24(ig);
		if (v == -1 << 23)
		    *vector = VFILL;
		else
		    is->samp = *vector;
		break;
	      case 32:	/* 32-bit amplitudes */
		*vector = v = r32(ig);
		if (v == -1 << 31)
		    *vector = VFILL;
		else
		    is->samp = *vector;
		break;
	      case 508:	/* 8-bit compressed FLAC */
		*vector = v = flac_getsamp(ig);
		if (v == -1 << 7)
		    *vector = VFILL;
		else
		    is->samp = *vector;
		break;
	      case 516:	/* 16-bit compressed FLAC */
		*vector = v = flac_getsamp(ig);
		if (v == -1 << 15)
		    *vector = VFILL;
		else
		    is->samp = *vector;
		break;
	      case 524:	/* 24-bit compressed FLAC */
		*vector = v = flac_getsamp(ig);
		if (v == -1 << 23)
		    *vector = VFILL;
		else
		    is->samp = *vector;
		break;
	    }
	    if (ig->stat <= 0) {
		/* End of file -- reset input counter. */
		ig->count = 0;
		if (ig->stat == -2) {
		    /* error in decoding compressed data */
		    stat = -3;
		}
		else if (is->info.nsamp > (WFDB_Time)0L) {
		    wfdb_error("getvec: unexpected EOF in signal %d\n", s);
		    stat = -3;
		}
		else if (in_msrec && segp && segp < segend) {
		    segp++;
		    if (isigopen(segp->recname, NULL, (int)nvsig) <= 0) {
			wfdb_error("getvec: error opening segment %s\n",
				   segp->recname);
			stat = -3;
			return (stat);  /* avoid looping if segment is bad */
		    }
		    else {
			istime = segp->samp0;
			return (getskewedframe(vecstart));
		    }
		}
		else
		    stat = -1;
	    }
	    is->info.cksum -= v;
	}
	if (is->info.nsamp >= 0 && --is->info.nsamp == 0 &&
	    (is->info.cksum & 0xffff) &&
	    !in_msrec && !isedf &&
	    is->info.fmt != 0) {
	    wfdb_error("getvec: checksum error in signal %d\n", s);
	    stat = -4;
	}
    }

    if (need_sigmap)
	sigmap(vecstart, ovec);
    else if (framelen != tspf)
	for (s = framelen; s < tspf; s++)
	    vecstart[s] = WFDB_INVALID_SAMPLE;

    return (stat);
}

/* meansamp: calculate the mean of n sample values.  The result is
   rounded to the nearest integer, with halfway cases always rounded
   up. */
WFDB_Sample meansamp(const WFDB_Sample *s, int n)
{
    /* If a WFDB_Time is large enough to hold the sum of the sample
       values, then simply add them up and divide by n. */
    if (WFDB_SAMPLE_MAX <= WFDB_TIME_MAX / INT_MAX) {
	WFDB_Time sum = n / 2;
	int i;

	for (i = 0; i < n; i++) {
	    if (*s == WFDB_INVALID_SAMPLE)
		return (WFDB_INVALID_SAMPLE);
	    sum += *s++;
	}
	if (n & (n - 1)) {	/* if n is not a power of two */
	    return (sum + (sum < 0)) / n - (sum < 0);
	}
	else {
	    while ((n /= 2) != 0)
		sum = (sum - (sum < 0)) / 2;
	    return sum;
	}
    }
    /* Otherwise, calculate the quotient and remainder separately to
       avoid overflows. */
    else {
	WFDB_Sample q, qq = 0;
	int i, r, rr = n / 2;

	for (i = 0; i < n; i++) {
	    if (*s == WFDB_INVALID_SAMPLE)
		return (WFDB_INVALID_SAMPLE);
	    q = *s / n;
	    r = *s % n;
	    if (r >= 0) {
		q++;
		r -= n;
	    }
	    qq += q;
	    rr += r;
	    if (rr < 0) {
		rr += n;
		qq--;
	    }
	    s++;
	}
	return (qq);
    }
}

int rgetvec(WFDB_Sample *vector)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    WFDB_Sample *tp;
    WFDB_Signal s;
    static int stat;

    if (ispfmax < 2)	/* all signals at the same frequency */
	return (getframe(vector));

    if ((gvmode & WFDB_HIGHRES) != WFDB_HIGHRES) {
	/* return one sample per frame, decimating by averaging if necessary */
	stat = getframe(tvector);
	for (s = 0, tp = tvector; s < nvsig; s++) {
	    int sf = vsd[s]->info.spf;
	    *vector++ = meansamp(tp, sf);
	    tp += sf;
	}
    }
    else {			/* return ispfmax samples per frame, using
				   zero-order interpolation if necessary */
	if (gvc >= ispfmax) {
	    stat = getframe(tvector);
	    gvc = 0;
	}
	for (s = 0, tp = tvector; s < nvsig; s++) {
	    int sf = vsd[s]->info.spf;
	    if (gvc == 0) {
		vsd[s]->gvindex = 0;
		vsd[s]->gvcount = -ispfmax;
	    }
	    else {
		vsd[s]->gvcount += sf;
		if (vsd[s]->gvcount >= 0) {
		    vsd[s]->gvindex++;
		    vsd[s]->gvcount -= ispfmax;
		}
	    }
	    *vector++ = tp[vsd[s]->gvindex];
	    tp += sf;
	}
	gvc++;
    }
    return (stat);
}

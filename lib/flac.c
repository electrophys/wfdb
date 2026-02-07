/* file: flac.c   2026  FLAC signal compression support for WFDB library. */

#include "signal_internal.h"

/* Routines for reading FLAC signal files.

   Terminology note: the FLAC format refers to an encoded block of
   samples as a "frame".  In WFDB terminology, a "frame" is a group of
   samples representing a fixed time interval (specified by the frame
   frequency in the header file.)  A FLAC frame may represent the
   contents of multiple WFDB frames, or only part of one.  Unless
   otherwise noted, the word "frame" refers to a WFDB frame. */

#ifdef WFDB_FLAC_SUPPORT

/* The following functions (iflac_read, iflac_seek, iflac_tell,
   iflac_length, iflac_eof, iflac_error, and iflac_samples) are used
   as callbacks for the FLAC library.  The client_data argument is the
   value passed to FLAC__stream_decoder_init_stream, which is a
   pointer to a struct igdata. */

/* iflac_read is called by the FLAC library in order to read a chunk of
   data from the input file. */
static FLAC__StreamDecoderReadStatus
iflac_read(const FLAC__StreamDecoder *dec, FLAC__byte buffer[],
	   size_t *bytes, void *client_data)
{
    struct igdata *g = client_data;
    if (*bytes == 0)
	return (FLAC__STREAM_DECODER_READ_STATUS_ABORT);
    *bytes = wfdb_fread(buffer, sizeof(FLAC__byte), *bytes, g->fp);
    if (wfdb_ferror(g->fp))
	return (FLAC__STREAM_DECODER_READ_STATUS_ABORT);
    else if (*bytes == 0)
	return (FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM);
    else
	return (FLAC__STREAM_DECODER_READ_STATUS_CONTINUE);
}

/* iflac_seek is called by the FLAC library in order to move to a
   different position in the input file. */
static FLAC__StreamDecoderSeekStatus
iflac_seek(const FLAC__StreamDecoder *dec, FLAC__uint64 pos,
	   void *client_data)
{
    struct igdata *g = client_data;
    if (!g->seek)
	return (FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED);
    else if (pos > LONG_MAX || wfdb_fseek(g->fp, pos, SEEK_SET))
	return (FLAC__STREAM_DECODER_SEEK_STATUS_ERROR);
    else
	return (FLAC__STREAM_DECODER_SEEK_STATUS_OK);
}

/* iflac_tell is called by the FLAC library in order to determine the
   current position in the input file. */
static FLAC__StreamDecoderTellStatus
iflac_tell(const FLAC__StreamDecoder *dec, FLAC__uint64 *pos,
	   void *client_data)
{
    struct igdata *g = client_data;
    long p;
    if (!g->seek)
	return (FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED);
    else if ((p = wfdb_ftell(g->fp)) < 0)
	return (FLAC__STREAM_DECODER_TELL_STATUS_ERROR);
    else {
	*pos = p;
	return (FLAC__STREAM_DECODER_TELL_STATUS_OK);
    }
}

/* iflac_length is called by the FLAC library in order to determine the
   total size of the input file. */
static FLAC__StreamDecoderLengthStatus
iflac_length(const FLAC__StreamDecoder *dec, FLAC__uint64 *len,
	     void *client_data)
{
    struct igdata *g = client_data;
    long pos, end;

    if (!g->seek)
	return (FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED);
    else if ((pos = wfdb_ftell(g->fp)) < 0)
	return (FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR);
    else if (wfdb_fseek(g->fp, 0L, SEEK_END) ||
	     (end = wfdb_ftell(g->fp)) < 0) {
	wfdb_fseek(g->fp, pos, SEEK_SET);
	return (FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR);
    }
    if (wfdb_fseek(g->fp, pos, SEEK_SET))
	return (FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR);
    *len = end;
    return (FLAC__STREAM_DECODER_LENGTH_STATUS_OK);
}

/* iflac_eof is called by the FLAC library in order to check whether
   we have reached the end of the input file. */
static FLAC__bool iflac_eof(const FLAC__StreamDecoder *dec, void *client_data)
{
    struct igdata *g = client_data;
    return (wfdb_feof(g->fp));
}

/* iflac_error is called by the FLAC library when the input stream
   appears invalid or corrupted. */
static void iflac_error(const FLAC__StreamDecoder *decoder,
			FLAC__StreamDecoderErrorStatus status,
			void *client_data)
{
    struct igdata *g = client_data;

    switch (status) {
      case FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC:
	wfdb_error("getvec: unable to decode FLAC (lost sync)\n");
	break;
      case FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER:
	wfdb_error("getvec: unable to decode FLAC (invalid header)\n");
	break;
      case FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH:
	wfdb_error("getvec: unable to decode FLAC (CRC mismatch)\n");
	break;
      case FLAC__STREAM_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM:
	wfdb_error("getvec: unable to decode FLAC (unsupported format)\n");
	break;
      default:
	wfdb_error("getvec: unable to decode FLAC\n");
	break;
    }
    /* Note that if an error is detected, the FLAC library will still
       subsequently invoke iflac_samples, with a buffer of zeroes
       rather than valid data. */
    g->stat = -2;
}

/* iflac_samples is called by the FLAC library when a block of samples
   has been decoded. */
static FLAC__StreamDecoderWriteStatus
iflac_samples(const FLAC__StreamDecoder *dec, const FLAC__Frame *ffrm,
	      const FLAC__int32 *const buf[], void *client_data)
{
    struct igdata *g = client_data;
    size_t nsig = ffrm->header.channels;
    size_t nsamp = ffrm->header.blocksize;
    size_t oldsize, newsize, frmsize, bufsize, spf, ipos, orem, s, n;
    char *nbuf;
    FLAC__int32 *p;

    /* g->data is the number of signals in the group. */
    if (nsig != g->data) {
	wfdb_error("getvec: wrong number of signals in FLAC signal file\n");
	g->stat = -2;
    }
    /* g->datb is the group sample resolution. */
    if (ffrm->header.bits_per_sample > g->datb) {
	wfdb_error("getvec: wrong sample resolution in FLAC signal file\n");
	g->stat = -2;
    }
    /* If the resolution or number of signals is incorrect, or if an
       error was previously detected by flac_error, then stop decoding
       immediately (and return an error from getskewedframe or
       isgsetframe.) */
    if (g->stat < 0)
	return (FLAC__STREAM_DECODER_WRITE_STATUS_ABORT);

    /* g->buf is the start of the input buffer.

       g->bp points to the next sample (FLAC__int32) to be retrieved.
       This advances by four bytes with each call to flac_getsamp.
       (When iflac_samples is called, g->bp should be located at the
       start of a frame.)

       g->packptr points to the next frame to be decoded.  This
       advances by 4*nsig*spf bytes when a complete frame has been
       decoded.

       g->packcount is the number of samples (of each signal) that
       have been decoded and stored at g->packptr.  This is initially
       zero; when it reaches spf, it is reset to zero and g->packptr
       advances.

       g->be is the end of the input buffer.  We should always have
       g->buf <= g->bp <= g->packptr <= g->be. */

    spf = g->packspf;
    newsize = nsig * nsamp * sizeof(FLAC__int32);
    frmsize = nsig * spf * sizeof(FLAC__int32);

    bufsize = g->be - g->buf;
    oldsize = g->packptr - g->bp;

    /* If the existing buffer is not large enough to hold old + new
       data, plus one extra frame, it must be reallocated.  Typically
       the FLAC block size is constant, so this reallocation will only
       be needed once per file. */
    if (bufsize < oldsize + newsize + frmsize) {
	bufsize = oldsize + newsize + frmsize;
	SUALLOC(nbuf, bufsize, 1);
	if (!nbuf)
	    return (FLAC__STREAM_DECODER_WRITE_STATUS_ABORT);

	n = oldsize + (g->packcount ? frmsize : 0);
	if (n)
	    memcpy(nbuf, g->bp, n);
	SFREE(g->buf);
	g->buf = g->bp = nbuf;
	g->be = nbuf + bufsize;
	g->packptr = nbuf + oldsize;
    }
    /* Otherwise, shift existing data (if any) to the beginning of the
       buffer. */
    else if (g->bp != g->buf) {
	n = oldsize + (g->packcount ? frmsize : 0);
	if (n)
	    memmove(g->buf, g->bp, n);
	g->bp = g->buf;
	g->packptr = g->buf + oldsize;
    }

    /* g->count is the number of initial samples to skip, following a
       seek (see flac_isseek). */
    if (g->count > nsamp) {
	g->count -= nsamp;
	return (FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE);
    }
    else {
	ipos = g->count;
	nsamp -= ipos;
	g->count = 0;
    }

    /* Rearrange data from the buffer(s) provided by the decoder, and
       store the samples in frame order. */
    p = ((FLAC__int32 *) g->packptr) + g->packcount;
    orem = spf - g->packcount;
    while (nsamp != 0) {
	n = (nsamp < orem ? nsamp : orem);
	for (s = 0; s < nsig; s++) {
	    memcpy(p, buf[s] + ipos, n * sizeof(FLAC__int32));
	    p += spf;
	}
	nsamp -= n;
	orem -= n;
	if (orem == 0) {
	    g->packptr += frmsize;
	    p = (FLAC__int32 *) g->packptr;
	    orem = spf;
	    ipos += n;
	}
    }
    g->packcount = spf - orem;

    return (FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE);
}

/* Read and return the next sample from a FLAC signal file. */
int flac_getsamp(struct igdata *g)
{
    FLAC__int32 *ibp;
    FLAC__StreamDecoderState state;
    unsigned oldcount;

    /* If the next frame has not yet been decoded, read more data from
       the input file. */
    while (g->bp == g->packptr) {
	oldcount = g->packcount;
	if (!FLAC__stream_decoder_process_single(g->flacdec)) {
	    if (g->stat != -2) {
		wfdb_error("getvec: unexpected FLAC decoding error\n");
		g->stat = -2;
	    }
	    return (0);
	}
	if (g->stat <= 0) {
	    return (0);
	}
	if (g->bp == g->packptr && g->packcount == oldcount) {
	    state = FLAC__stream_decoder_get_state(g->flacdec);
	    switch (state) {
	      case FLAC__STREAM_DECODER_END_OF_STREAM:
		if (g->packcount != 0)
		    wfdb_error("getvec: warning: %d samples left over"
			       " at end of file\n", g->packcount);
		g->packcount = 0;
		g->stat = 0;
		return (0);
	      case FLAC__STREAM_DECODER_SEARCH_FOR_METADATA:
	      case FLAC__STREAM_DECODER_READ_METADATA:
	      case FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC:
		break;
	      default:
		wfdb_error("getvec: unknown FLAC decoding state\n");
		g->stat = -2;
		return (0);
	    }
	}
    }

    /* Retrieve the next decoded sample. */
    ibp = (FLAC__int32 *) g->bp;
    g->bp = (char *) &ibp[1];
    return (*ibp);
}

/* Open a FLAC stream decoder for an input signal group.  The input
   file (ig->fp) has already been opened and the buffer (ig->buf) has
   already been allocated. */
int flac_isopen(struct igdata *ig, struct hsdata **hs, unsigned ns)
{
    unsigned int i;
    char *p;

    if (ns > FLAC__MAX_CHANNELS) {
	wfdb_error(
	    "isigopen: cannot store %u signals in a single FLAC file\n",
	    ns);
	return (-1);
    }
    for (i = 1; i < ns; i++) {
	if (hs[i]->info.spf != hs[0]->info.spf) {
	    wfdb_error(
	       "isigopen: every signal in a FLAC file must have\n"
	       "  the same sampling frequency\n");
	    return (-1);
	}
    }

    ig->flacdec = FLAC__stream_decoder_new();
    if (!ig->flacdec) {
	wfdb_error("isigopen: cannot initialize stream decoder\n");
	return (-1);
    }
    /* If the WFDB_FLAC_CHECK_MD5 environment variable is defined, try
       to verify the MD5 hash of the stream.  Note that this will only
       work if the application reads the entire record, never calls
       isigsettime, and calls wfdbquit afterwards. */
    if ((p = getenv("WFDB_FLAC_CHECK_MD5")) && *p)
	FLAC__stream_decoder_set_md5_checking(ig->flacdec, 1);

    ig->data = ns;
    ig->datb = hs[0]->info.fmt - 500;
    ig->count = 0;
    ig->packspf = hs[0]->info.spf;
    ig->packptr = ig->be = ig->bp = ig->buf + ig->bsize;
    ig->packcount = 0;
    if (FLAC__stream_decoder_init_stream(ig->flacdec, &iflac_read,
					 &iflac_seek, &iflac_tell,
					 &iflac_length, &iflac_eof,
					 &iflac_samples, NULL,
					 &iflac_error, ig)) {
	wfdb_error("isigopen: cannot open stream decoder\n");
	FLAC__stream_decoder_delete(ig->flacdec);
	return (-1);
    }
    return (0);
}

/* Close a stream decoder. */
int flac_isclose(struct igdata *ig)
{
    int stat = 0;

    if (!FLAC__stream_decoder_finish(ig->flacdec)) {
	wfdb_error("isigclose: warning: incorrect MD5 hash in FLAC input\n");
	stat = -1;
    }
    FLAC__stream_decoder_delete(ig->flacdec);
    return (stat);
}

/* Seek to the given frame number in an input stream. */
int flac_isseek(struct igdata *ig, WFDB_Time t)
{
    FLAC__StreamDecoderState state;
    WFDB_Time tt;

    /* If there was a previous seek error, clear the decoder state so
       we can try again.  (Calling flush at other times can break
       decoding for some reason.) */
    state = FLAC__stream_decoder_get_state(ig->flacdec);
    if (state == FLAC__STREAM_DECODER_SEEK_ERROR)
	FLAC__stream_decoder_flush(ig->flacdec);

    /* Reset the block pointers to indicate nothing has been read
       in the current block. */
    ig->packptr = ig->bp = ig->be;
    ig->packcount = 0;
    ig->stat = 1;
    ig->count = 0;

    /* Seek to the desired sample.  Note that seek_absolute will
       return an error if the given sample number is greater than or
       equal to the length of the stream; isgsetframe should succeed
       if tt is exactly the length of the stream.  Therefore, we
       subtract one from the target sample number, and set ig->count
       to 1 so that iflac_samples will discard the first sample from
       each signal. */
    tt = t * ig->packspf + ig->start;
    if (tt != 0) {
	tt--;
	ig->count = 1;
    }
    /* This will invoke iflac_samples, with a (partial) block of
       samples starting at the requested sample number. */
    if (!FLAC__stream_decoder_seek_absolute(ig->flacdec, tt))
	ig->stat = -1;
    return (ig->stat);
}

#else /* !WFDB_FLAC_SUPPORT */

int flac_getsamp(struct igdata *ig)
{
    ig->stat = -1;
    return (0);
}

int flac_isopen(struct igdata *ig, struct hsdata **hs, unsigned ns)
{
    wfdb_error("isigopen: libwfdb was compiled without FLAC support\n");
    return (-1);
}

int flac_isclose(struct igdata *ig)
{
    return (-1);
}

int flac_isseek(struct igdata *ig, WFDB_Time t)
{
    return (-1);
}

#endif

/* Routines for writing FLAC signal files. */

#ifdef WFDB_FLAC_SUPPORT

/* The following functions (oflac_write, oflac_seek, and oflac_tell)
   are used as callbacks for the FLAC library.  The client_data
   argument is the value passed to FLAC__stream_encoder_init_stream,
   which is a pointer to a struct ogdata. */

/* oflac_write is called by the FLAC library in order to write a
   chunk of data to the output file. */
static FLAC__StreamEncoderWriteStatus
oflac_write(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[],
	    size_t bytes, unsigned samples, unsigned current_frame,
	    void *client_data)
{
    struct ogdata *g = client_data;
    if (wfdb_fwrite(buffer, 1, bytes, g->fp) != bytes)
	return (FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR);
    else
	return (FLAC__STREAM_ENCODER_WRITE_STATUS_OK);
}

/* oflac_seek is called by the FLAC library in order to move to a
   different position in the output file. */
static FLAC__StreamEncoderSeekStatus
oflac_seek(const FLAC__StreamEncoder *encoder,
	   FLAC__uint64 pos, void *client_data)
{
    struct ogdata *g = client_data;
    if (pos > LONG_MAX)
	return (FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR);
    else if (wfdb_fseek(g->fp, pos, SEEK_SET))
	return (FLAC__STREAM_ENCODER_SEEK_STATUS_UNSUPPORTED);
    else
	return (FLAC__STREAM_ENCODER_SEEK_STATUS_OK);
}

/* oflac_tell is called by the FLAC library in order to determine
   the current position in the output file. */
static FLAC__StreamEncoderTellStatus
oflac_tell(const FLAC__StreamEncoder *encoder,
	   FLAC__uint64 *pos, void *client_data)
{
    struct ogdata *g = client_data;
    long p;
    if ((p = wfdb_ftell(g->fp)) < 0)
	return (FLAC__STREAM_ENCODER_TELL_STATUS_UNSUPPORTED);
    else {
	*pos = p;
	return (FLAC__STREAM_ENCODER_TELL_STATUS_OK);
    }
}

/* Write the next sample to a FLAC signal file. */
int flac_putsamp(WFDB_Sample v, int fmt, struct ogdata *g)
{
    FLAC__int32 *obp;
    const FLAC__int32 *channels[FLAC__MAX_CHANNELS];
    unsigned spf, i;
    struct { WFDB_Sample v : 8; } v8;
    struct { WFDB_Sample v : 16; } v16;
    struct { WFDB_Sample v : 24; } v24;

    /* If sample values passed to the FLAC encoder exceed the
       specified number of "bits per sample", the result is
       unpredictable: the encoder might discard the upper bits, or it
       might actually encode some values outside the stated range.  To
       ensure that the results are consistent and predictable, even if
       the caller supplies invalid sample values to putvec, we discard
       the upper bits by sign-extending values before passing them to
       the encoder. */
    switch (fmt) {
      case 508: v = v8.v = v; break;
      case 516: v = v16.v = v; break;
      case 524: v = v24.v = v; break;
    }

    obp = (FLAC__int32 *) g->bp;
    *obp++ = v;

    /* The output buffer is exactly the size of one frame, so when obp
       reaches g->be we have one frame's worth of data to encode. */
    if (obp == (FLAC__int32 *) g->be) {
	obp = (FLAC__int32 *) g->buf;
	spf = g->packspf;
	for (i = 0; obp != (FLAC__int32 *) g->be; i++) {
	    channels[i] = obp;
	    obp += spf;
	}
	if (!FLAC__stream_encoder_process(g->flacenc, channels, spf)) {
	    wfdb_error("putvec: error writing FLAC signal data\n");
	    return (-1);
	}
	g->bp = g->buf;
	return (0);
    }
    else {
	g->bp = (char *) obp;
	return (0);
    }
}

int flac_osinit(struct ogdata *og, const WFDB_Siginfo *si, unsigned ns)
{
    FLAC__StreamEncoder *enc;
    char *p;
    int min = 0, max = 0;
    unsigned int i;

    if (ns > FLAC__MAX_CHANNELS) {
	wfdb_error(
	    "osigfopen: cannot store %u signals in a single FLAC file\n",
	    ns);
	return (-1);
    }
    for (i = 1; i < ns; i++) {
	if (si[i].spf != si[0].spf) {
	    wfdb_error(
	       "osigfopen: every signal in a FLAC file must have\n"
	       "  the same sampling frequency\n");
	    return (-1);
	}
    }

    og->flacenc = enc = FLAC__stream_encoder_new();
    og->packspf = si->spf;
    if (!enc) {
	wfdb_error("osigfopen: cannot initialize stream encoder\n");
	return (-1);
    }
    FLAC__stream_encoder_set_channels(enc, ns);
    FLAC__stream_encoder_set_bits_per_sample(enc, si->fmt - 500);
    FLAC__stream_encoder_set_sample_rate(enc, 96000);

    /* If the output file is not seekable, set the STREAMINFO length
       to the maximum possible value.  If the length is zero, libFLAC
       will have problems when trying to read the file. */
    FLAC__stream_encoder_set_total_samples_estimate(enc, (FLAC__uint64) -1);

    /* Make the buffer exactly large enough to hold a single frame.
       (Also note that setting og->bsize to a non-zero value prevents
       wfdb_osflush from trying to flush the output buffer.) */
    og->bsize = ns * si->spf * sizeof(FLAC__int32);

    /* The following environment variables may be used to set
       parameters for the FLAC compression algorithm. */

    if (p = getenv("WFDB_FLAC_COMPRESSION_LEVEL"))
	FLAC__stream_encoder_set_compression_level(enc, atoi(p));
    else
	FLAC__stream_encoder_set_compression_level(enc, 5);

    if (p = getenv("WFDB_FLAC_BLOCK_SIZE"))
	FLAC__stream_encoder_set_blocksize(enc, atoi(p));
    if (p = getenv("WFDB_FLAC_STEREO")) {
	if (p[0] == 'a' || p[0] == 'A') { /* auto */
	    FLAC__stream_encoder_set_do_mid_side_stereo(enc, 1);
	    FLAC__stream_encoder_set_loose_mid_side_stereo(enc, 1);
	}
	else if (p[0] == 'b' || p[0] == 'B') { /* best */
	    FLAC__stream_encoder_set_do_mid_side_stereo(enc, 1);
	    FLAC__stream_encoder_set_loose_mid_side_stereo(enc, 0);
	}
	else {
	    FLAC__stream_encoder_set_do_mid_side_stereo(enc, 0);
	}
    }
    if (p = getenv("WFDB_FLAC_APODIZATION"))
	FLAC__stream_encoder_set_apodization(enc, p);
    if (p = getenv("WFDB_FLAC_MAX_LPC_ORDER"))
	FLAC__stream_encoder_set_max_lpc_order(enc, atoi(p));
    if (p = getenv("WFDB_FLAC_QLP_COEFF_PRECISION")) {
	if (p[0] == 'a' || p[0] == 'A') { /* auto */
	    FLAC__stream_encoder_set_qlp_coeff_precision(enc, 0);
	    FLAC__stream_encoder_set_do_qlp_coeff_prec_search(enc, 0);
	}
	else if (p[0] == 'b' || p[0] == 'B') { /* best */
	    FLAC__stream_encoder_set_qlp_coeff_precision(enc, 0);
	    FLAC__stream_encoder_set_do_qlp_coeff_prec_search(enc, 1);
	}
	else {
	    FLAC__stream_encoder_set_qlp_coeff_precision(enc, atoi(p));
	    FLAC__stream_encoder_set_do_qlp_coeff_prec_search(enc, 0);
	}
    }
    if (p = getenv("WFDB_FLAC_EXHAUSTIVE_MODEL_SEARCH")) {
	if (p[0] == 'y' || p[0] == 'Y')
	    FLAC__stream_encoder_set_do_exhaustive_model_search(enc, 1);
	else
	    FLAC__stream_encoder_set_do_exhaustive_model_search(enc, 0);
    }
    if (p = getenv("WFDB_FLAC_RICE_PARTITION_ORDER")) {
	min = strtol(p, &p, 10);
	if (p && *p == ',') {
	    max = strtol(p + 1, NULL, 10);
	}
	else {
	    max = min;
	    min = 0;
	}
	FLAC__stream_encoder_set_min_residual_partition_order(enc, min);
	FLAC__stream_encoder_set_max_residual_partition_order(enc, max);
    }

    return (0);
}

int flac_osopen(struct ogdata *og)
{
    if (FLAC__stream_encoder_init_stream(og->flacenc, &oflac_write,
					 &oflac_seek, &oflac_tell,
					 NULL, og)) {
	wfdb_error("osigfopen: cannot open stream encoder\n");
	return (-1);
    }
    return (0);
}

int flac_osclose(struct ogdata *og)
{
    int stat = 0;

    if (!FLAC__stream_encoder_finish(og->flacenc)) {
	wfdb_error("osigclose: error writing FLAC signal file\n");
	stat = -1;
    }
    FLAC__stream_encoder_delete(og->flacenc);
    og->bp = og->be = og->buf;
    return (stat);
}

#else /* !WFDB_FLAC_SUPPORT */

int flac_putsamp(WFDB_Sample v, int fmt, struct ogdata *g)
{
    return (-1);
}

int flac_osinit(struct ogdata *og, const WFDB_Siginfo *si, unsigned ns)
{
    wfdb_error("osigfopen: libwfdb was compiled without FLAC support\n");
    return (-1);
}

int flac_osopen(struct ogdata *og)
{
    return (-1);
}

int flac_osclose(struct ogdata *og)
{
    return (-1);
}

#endif

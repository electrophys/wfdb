/* file: signal_internal.h	G. Moody	2026
   Internal header for shared state between signal.c modules.

   This header declares the struct types and provides access macros for the
   shared state used by the signal processing modules (signal.c, sigformat.c,
   flac.c, header.c, sigmap.c, timeconv.c).

   All shared state is now held in the WFDB_Context structure.  Each function
   that uses these variables must declare a local variable:
       WFDB_Context *ctx = wfdb_get_default_context();
   The macros below then provide transparent access to the context fields.
*/

#ifndef WFDB_SIGNAL_INTERNAL_H
#define WFDB_SIGNAL_INTERNAL_H

#include "wfdb_context.h"
#include <limits.h>

_Static_assert(CHAR_BIT == 8, "WFDB format I/O requires 8-bit bytes");

/* Mark internal symbols as hidden to prevent collisions with
   identically-named symbols in application code */
#if defined(__GNUC__) && __GNUC__ >= 4
#define WFDB_INTERNAL __attribute__((visibility("hidden")))
#else
#define WFDB_INTERNAL
#endif

#ifdef WFDB_FLAC_SUPPORT
#include <FLAC/stream_encoder.h>
#include <FLAC/stream_decoder.h>
#else
#define FLAC__StreamEncoder struct dummy
#define FLAC__StreamDecoder struct dummy
#endif

/* ---- Struct definitions ---- */

struct hsdata {
    WFDB_Siginfo info;		/* info about signal from header */
    long start;			/* signal file byte offset to sample 0 */
    int skew;			/* intersignal skew (in frames) */
};

struct isdata {			/* unique for each input signal */
    WFDB_Siginfo info;		/* input signal information */
    WFDB_Sample samp;		/* most recent sample read */
    int skew;			/* intersignal skew (in frames) */
    int gvindex;		/* current high-resolution sample number */
    int gvcount;		/* counter for updating gvindex */
};

struct igdata {			/* shared by all signals in a group (file) */
    int data;			/* raw data read by r*() */
    int datb;			/* more raw data used for bit-packed formats */
    WFDB_FILE *fp;		/* file pointer for an input signal group */
    long start;			/* signal file byte offset to sample 0 */
    int bsize;			/* if non-zero, all reads from the input file
				   are in multiples of bsize bytes */
    char *buf;			/* pointer to input buffer */
    char *bp;			/* pointer to next location in buf[] */
    char *be;			/* pointer to input buffer endpoint */
    FLAC__StreamDecoder *flacdec; /* internal state for FLAC decoder */
    char *packptr;		/* pointer to next partially-decoded frame */
    unsigned packspf;		/* number of samples per signal per frame */
    unsigned packcount; 	/* number of samples decoded in this frame */
    char count;			/* input counter for bit-packed signal */
    char seek;			/* 0: do not seek on file, 1: seeks permitted */
    char initial_skip;		/* 1 if isgsetframe is needed before reading */
    int stat;			/* signal file status flag */
};

struct osdata {			/* unique for each output signal */
    WFDB_Siginfo info;		/* output signal information */
    WFDB_Sample samp;		/* most recent sample written */
    int skew;			/* skew to be written by setheader() */
};

struct ogdata {			/* shared by all signals in a group (file) */
    int data;			/* raw data to be written by w*() */
    int datb;			/* more raw data used for bit-packed formats */
    WFDB_FILE *fp;		/* file pointer for output signal */
    long start;			/* byte offset to be written by setheader() */
    int bsize;			/* if non-zero, all writes to the output file
				   are in multiples of bsize bytes */
    char *buf;			/* pointer to output buffer */
    char *bp;			/* pointer to next location in buf[]; */
    char *be;			/* pointer to output buffer endpoint */
    FLAC__StreamEncoder *flacenc; /* internal state for FLAC encoder */
    unsigned packspf;		/* number of samples per frame */
    char count;			/* output counter for bit-packed signal */
    signed char seek;		/* 1: seek works, -1: seek doesn't work,
				   0: unknown */
    char force_flush;		/* flush even if seek doesn't work */
    char nrewind;		/* number of bytes to seek backwards
				   after flushing */
};

struct sigmapinfo {
    char *desc;
    double gain, scale, offset;
    WFDB_Sample sample_offset;
    WFDB_Sample baseline;
    int index;
    int spf;
};

/* ---- Shared state access macros ----
   These require a local `WFDB_Context *ctx` to be in scope. */

/* Header data */
#define maxhsig		(ctx->maxhsig)
#define hheader		(ctx->hheader)
#define linebuf		(ctx->linebuf)
#define linebufsize	(ctx->linebufsize)
#define hsd		(ctx->hsd)

/* Time/frequency/conversion */
#define ffreq		(ctx->ffreq)
#define ifreq		(ctx->ifreq)
#define sfreq		(ctx->sfreq)
#define cfreq		(ctx->cfreq)
#define spfmax		(ctx->spfmax)
#define btime		(ctx->btime)
#define bdate		(ctx->bdate)
#define nsamples	(ctx->nsamples)
#define bcount		(ctx->bcount)
#define prolog_bytes	(ctx->prolog_bytes)

/* Multi-segment record */
#define segments	(ctx->segments)
#define in_msrec	(ctx->in_msrec)
#define msbtime		(ctx->msbtime)
#define msbdate		(ctx->msbdate)
#define msnsamples	(ctx->msnsamples)
#define segarray	(ctx->segarray)
#define segp		(ctx->segp)
#define segend		(ctx->segend)

/* Input signals */
#define maxisig		(ctx->maxisig)
#define maxigroup	(ctx->maxigroup)
#define nisig		(ctx->nisig)
#define nigroup		(ctx->nigroup)
#define ispfmax		(ctx->ispfmax)
#define isd		(ctx->isd)
#define igd		(ctx->igd)
#define tvector		(ctx->tvector)
#define uvector		(ctx->uvector)
#define vvector		(ctx->vvector)
#define tuvlen		(ctx->tuvlen)
#define istime		(ctx->istime)
#define ibsize		(ctx->ibsize)
#define skewmax		(ctx->skewmax)
#define dsbuf		(ctx->dsbuf)
#define dsbi		(ctx->dsbi)
#define dsblen		(ctx->dsblen)
#define framelen	(ctx->framelen)
#define gvmode		(ctx->gvmode)
#define gvc		(ctx->gvc)
#define isedf		(ctx->isedf)
#define sbuf		(ctx->sbuf)
#define sample_vflag	(ctx->sample_vflag)

/* Resampling state */
#define mticks		(ctx->mticks)
#define nticks		(ctx->nticks)
#define mnticks		(ctx->mnticks)
#define rgvstat		(ctx->rgvstat)
#define rgvtime		(ctx->rgvtime)
#define gvtime		(ctx->gvtime)
#define gv0		(ctx->gv0)
#define gv1		(ctx->gv1)

/* getinfo iteration index */
#define getinfo_index	(ctx->getinfo_index)

/* sample() function cache */
#define sample_tt	(ctx->sample_tt)

/* Output signals */
#define maxosig		(ctx->maxosig)
#define maxogroup	(ctx->maxogroup)
#define nosig		(ctx->nosig)
#define nogroup		(ctx->nogroup)
#define oheader		(ctx->oheader)
#define outinfo		(ctx->outinfo)
#define osd		(ctx->osd)
#define ogd		(ctx->ogd)
#define ostime		(ctx->ostime)
#define obsize		(ctx->obsize)

/* Info strings */
#define pinfo		(ctx->pinfo)
#define nimax		(ctx->nimax)
#define ninfo		(ctx->ninfo)

/* Sigmap */
#define need_sigmap	(ctx->need_sigmap)
#define maxvsig		(ctx->maxvsig)
#define nvsig		(ctx->nvsig)
#define tspf		(ctx->tspf)
#define vspfmax		(ctx->vspfmax)
#define vsd		(ctx->vsd)
#define ovec		(ctx->ovec)
#define smi		(ctx->smi)

/* Format I/O helpers */
#define _l		(ctx->_l)
#define _lw		(ctx->_lw)
#define _n		(ctx->_n)

/* Format I/O macros (require ctx in scope via the helpers above) */
#define r8(G)	((G->bp < G->be) ? *(G->bp++) : \
		  ((_n = (G->bsize > 0) ? G->bsize : ibsize), \
		   (G->stat = _n = wfdb_fread(G->buf, 1, _n, G->fp)), \
		   (G->be = (G->bp = G->buf) + _n),\
		  *(G->bp++)))

#define w8(V,G)	(((*(G->bp++) = (char)V)), \
		  (_l = (G->bp != G->be) ? 0 : \
		   ((_n = (G->bsize > 0) ? G->bsize : obsize), \
		    wfdb_fwrite((G->bp = G->buf), 1, _n, G->fp))))

#define r16(G)	    (_l = r8(G), ((int)((short)((r8(G) << 8) | (_l & 0xff)))))
#define w16(V,G)    (w8((V), (G)), w8(((V) >> 8), (G)))
#define r61(G)      (_l = r8(G), ((int)((short)((r8(G) & 0xff) | (_l << 8)))))
#define w61(V,G)    (w8(((V) >> 8), (G)), w8((V), (G)))
#define r24(G)	    (_lw = r16(G), ((int)((r8(G) << 16) | (_lw & 0xffff))))
#define w24(V,G)    (w16((V), (G)), w8(((V) >> 16), (G)))
#define r32(G)	    (_lw = r16(G), ((int)((r16(G) << 16) | (_lw & 0xffff))))
#define w32(V,G)    (w16((V), (G)), w16(((V) >> 16), (G)))

#define r80(G)		((r8(G) & 0xff) - (1 << 7))
#define w80(V, G)	(w8(((V) & 0xff) + (1 << 7), G))

#define r160(G)		((r16(G) & 0xffff) - (1 << 15))
#define w160(V, G)	(w16(((V) & 0xffff) + (1 << 15), G))

/* Time conversion state (from timeconv.c) */
#define date_string	(ctx->date_string)
#define time_string	(ctx->time_string)
#define pdays		(ctx->pdays)

/* ---- Internal function declarations ---- */

/* From timeconv.c */
WFDB_INTERNAL char *ftimstr(WFDB_Time t, WFDB_Frequency f);
WFDB_INTERNAL char *fmstimstr(WFDB_Time t, WFDB_Frequency f);
WFDB_INTERNAL WFDB_Time fstrtim(const char *string, WFDB_Frequency f);

/* From header.c */
WFDB_INTERNAL void read_edf_str(char *buf, int size, WFDB_FILE *ifile);
WFDB_INTERNAL int edfparse(WFDB_FILE *ifile);
WFDB_INTERNAL int readheader(const char *record);
WFDB_INTERNAL void hsdfree(void);

/* From sigformat.c */
WFDB_INTERNAL int isgsetframe(WFDB_Group g, WFDB_Time t);
WFDB_INTERNAL int getskewedframe(WFDB_Sample *vector);
WFDB_INTERNAL void w212(WFDB_Sample v, struct ogdata *g);
WFDB_INTERNAL void f212(struct ogdata *g);
WFDB_INTERNAL void w310(WFDB_Sample v, struct ogdata *g);
WFDB_INTERNAL void f310(struct ogdata *g);
WFDB_INTERNAL void w311(WFDB_Sample v, struct ogdata *g);
WFDB_INTERNAL void f311(struct ogdata *g);

/* From flac.c */
WFDB_INTERNAL int flac_getsamp(struct igdata *g);
WFDB_INTERNAL int flac_isopen(struct igdata *ig, struct hsdata **hs, unsigned ns);
WFDB_INTERNAL int flac_isclose(struct igdata *ig);
WFDB_INTERNAL int flac_isseek(struct igdata *ig, WFDB_Time t);
WFDB_INTERNAL int flac_putsamp(WFDB_Sample v, int fmt, struct ogdata *g);
WFDB_INTERNAL int flac_osinit(struct ogdata *og, const WFDB_Siginfo *si, unsigned ns);
WFDB_INTERNAL int flac_osopen(struct ogdata *og);
WFDB_INTERNAL int flac_osclose(struct ogdata *og);

/* From sigmap.c */
WFDB_INTERNAL void sigmap_cleanup(void);
WFDB_INTERNAL int make_vsd(void);
WFDB_INTERNAL int sigmap_init(int first_segment);
WFDB_INTERNAL int sigmap(WFDB_Sample *vector, const WFDB_Sample *ivec);

/* From signal.c */
WFDB_INTERNAL int allocisig(unsigned int n);
WFDB_INTERNAL int allocigroup(unsigned int n);
WFDB_INTERNAL int allocosig(unsigned int n);
WFDB_INTERNAL int allocogroup(unsigned int n);
WFDB_INTERNAL int isfmt(int f);
WFDB_INTERNAL int isflacfmt(int f);
WFDB_INTERNAL int copysi(WFDB_Siginfo *to, const WFDB_Siginfo *from);
WFDB_INTERNAL void isigclose(void);
WFDB_INTERNAL int osigclose(void);
WFDB_INTERNAL WFDB_Sample meansamp(const WFDB_Sample *s, int n);
WFDB_INTERNAL int rgetvec(WFDB_Sample *vector);

#endif /* WFDB_SIGNAL_INTERNAL_H */

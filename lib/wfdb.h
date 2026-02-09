/* file: wfdb.h		G. Moody	13 June 1983
			Last revised:    9 February 2026    	wfdblib 11.0.0
WFDB library type, constant, structure, and function interface definitions
_______________________________________________________________________________
wfdb: a library for reading and writing annotated waveforms (time series data)
Copyright (C) 1983-2013 George B. Moody

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

*/

#ifndef wfdb_WFDB_H	/* avoid multiple definitions */
#define wfdb_WFDB_H

/* Library version.  Update these at each release. */
#define WFDB_MAJOR   10
#define WFDB_MINOR   7
#define WFDB_RELEASE 0

/* Simple data types */
typedef int	     WFDB_Sample;   /* units are adus */
typedef long	     WFDB_Date;	    /* units are days */
typedef double	     WFDB_Frequency;/* units are Hz (samples/second/signal) */
typedef double	     WFDB_Gain;	    /* units are adus per physical unit */
typedef unsigned int WFDB_Group;    /* signal group number */
typedef unsigned int WFDB_Signal;   /* signal number */
typedef unsigned int WFDB_Annotator;/* annotator number */

/* The following macros can be used to determine the ranges of the
   above data types.  You will also need to include <limits.h> and/or
   <float.h>. */
#define WFDB_SAMPLE_MIN             INT_MIN
#define WFDB_SAMPLE_MAX             INT_MAX
#define WFDB_DATE_MIN               LONG_MIN
#define WFDB_DATE_MAX               LONG_MAX
#define WFDB_FREQUENCY_MAX          DBL_MAX
#define WFDB_FREQUENCY_DIG          DBL_DIG
#define WFDB_FREQUENCY_MAX_10_EXP   DBL_MAX_10_EXP
#define WFDB_FREQUENCY_EPSILON      DBL_EPSILON
#define WFDB_GAIN_MAX               DBL_MAX
#define WFDB_GAIN_DIG               DBL_DIG
#define WFDB_GAIN_MAX_10_EXP        DBL_MAX_10_EXP
#define WFDB_GAIN_EPSILON           DBL_EPSILON
#define WFDB_GROUP_MAX              UINT_MAX
#define WFDB_SIGNAL_MAX             UINT_MAX
#define WFDB_ANNOTATOR_MAX          UINT_MAX

/* WFDB_Time is unconditionally 64-bit. */
typedef long long    WFDB_Time;	    /* units are sample intervals */
_Static_assert(sizeof(WFDB_Time) >= 8, "WFDB_Time must be at least 64 bits");
#define WFDB_TIME_MIN             LLONG_MIN
#define WFDB_TIME_MAX             LLONG_MAX
#ifdef _WIN32
# define WFDB_TIME_PRINTF_MODIFIER "I64"
# define WFDB_TIME_SCANF_MODIFIER  "I64"
#else
# define WFDB_TIME_PRINTF_MODIFIER "ll"
# define WFDB_TIME_SCANF_MODIFIER  "ll"
#endif

/* The following macros can be used to construct format strings for
   printf and scanf, and related standard library functions. */

#define WFDB_SAMPLE_PRINTF_MODIFIER ""
#define WFDB_SAMPLE_SCANF_MODIFIER ""
#define WFDB_FREQUENCY_PRINTF_MODIFIER ""
#define WFDB_FREQUENCY_SCANF_MODIFIER "l"
#define WFDB_GAIN_PRINTF_MODIFIER ""
#define WFDB_GAIN_SCANF_MODIFIER "l"

#define WFDB_Pd_SAMP WFDB_SAMPLE_PRINTF_MODIFIER "d"
#define WFDB_Pi_SAMP WFDB_SAMPLE_PRINTF_MODIFIER "i"
#define WFDB_Po_SAMP WFDB_SAMPLE_PRINTF_MODIFIER "o"
#define WFDB_Pu_SAMP WFDB_SAMPLE_PRINTF_MODIFIER "u"
#define WFDB_Px_SAMP WFDB_SAMPLE_PRINTF_MODIFIER "x"
#define WFDB_PX_SAMP WFDB_SAMPLE_PRINTF_MODIFIER "X"
#define WFDB_Sd_SAMP WFDB_SAMPLE_SCANF_MODIFIER "d"
#define WFDB_Si_SAMP WFDB_SAMPLE_SCANF_MODIFIER "i"
#define WFDB_So_SAMP WFDB_SAMPLE_SCANF_MODIFIER "o"
#define WFDB_Su_SAMP WFDB_SAMPLE_SCANF_MODIFIER "u"
#define WFDB_Sx_SAMP WFDB_SAMPLE_SCANF_MODIFIER "x"
#define WFDB_SX_SAMP WFDB_SAMPLE_SCANF_MODIFIER "X"
#define WFDB_Pd_TIME WFDB_TIME_PRINTF_MODIFIER "d"
#define WFDB_Pi_TIME WFDB_TIME_PRINTF_MODIFIER "i"
#define WFDB_Po_TIME WFDB_TIME_PRINTF_MODIFIER "o"
#define WFDB_Pu_TIME WFDB_TIME_PRINTF_MODIFIER "u"
#define WFDB_Px_TIME WFDB_TIME_PRINTF_MODIFIER "x"
#define WFDB_PX_TIME WFDB_TIME_PRINTF_MODIFIER "X"
#define WFDB_Sd_TIME WFDB_TIME_SCANF_MODIFIER "d"
#define WFDB_Si_TIME WFDB_TIME_SCANF_MODIFIER "i"
#define WFDB_So_TIME WFDB_TIME_SCANF_MODIFIER "o"
#define WFDB_Su_TIME WFDB_TIME_SCANF_MODIFIER "u"
#define WFDB_Sx_TIME WFDB_TIME_SCANF_MODIFIER "x"
#define WFDB_SX_TIME WFDB_TIME_SCANF_MODIFIER "X"
#define WFDB_Pe_FREQ WFDB_FREQUENCY_PRINTF_MODIFIER "e"
#define WFDB_PE_FREQ WFDB_FREQUENCY_PRINTF_MODIFIER "E"
#define WFDB_Pf_FREQ WFDB_FREQUENCY_PRINTF_MODIFIER "f"
#define WFDB_Pg_FREQ WFDB_FREQUENCY_PRINTF_MODIFIER "g"
#define WFDB_PG_FREQ WFDB_FREQUENCY_PRINTF_MODIFIER "G"
#define WFDB_Se_FREQ WFDB_FREQUENCY_SCANF_MODIFIER "e"
#define WFDB_SE_FREQ WFDB_FREQUENCY_SCANF_MODIFIER "E"
#define WFDB_Sf_FREQ WFDB_FREQUENCY_SCANF_MODIFIER "f"
#define WFDB_Sg_FREQ WFDB_FREQUENCY_SCANF_MODIFIER "g"
#define WFDB_SG_FREQ WFDB_FREQUENCY_SCANF_MODIFIER "G"
#define WFDB_Pe_GAIN WFDB_GAIN_PRINTF_MODIFIER "e"
#define WFDB_PE_GAIN WFDB_GAIN_PRINTF_MODIFIER "E"
#define WFDB_Pf_GAIN WFDB_GAIN_PRINTF_MODIFIER "f"
#define WFDB_Pg_GAIN WFDB_GAIN_PRINTF_MODIFIER "g"
#define WFDB_PG_GAIN WFDB_GAIN_PRINTF_MODIFIER "G"
#define WFDB_Se_GAIN WFDB_GAIN_SCANF_MODIFIER "e"
#define WFDB_SE_GAIN WFDB_GAIN_SCANF_MODIFIER "E"
#define WFDB_Sf_GAIN WFDB_GAIN_SCANF_MODIFIER "f"
#define WFDB_Sg_GAIN WFDB_GAIN_SCANF_MODIFIER "g"
#define WFDB_SG_GAIN WFDB_GAIN_SCANF_MODIFIER "G"

/* getvec and getframe return a sample with a value of WFDB_INVALID_SAMPLE
   when the amplitude of a signal is undefined (e.g., the input is clipped or
   the signal is not available) and padding is disabled (see WFDB_GVPAD, below).
   WFDB_INVALID_SAMPLE is the minimum value for a 16-bit WFDB_Sample.  In
   a format 24 or 32 signal, this value will be near mid-range, but it should
   occur only rarely in such cases;  if this is a concern, WFDB_INVALID_SAMPLE
   can be redefined and the WFDB library can be recompiled. */
#define WFDB_INVALID_SAMPLE (-32768)

/* Array sizes
   Many older applications use the values of WFDB_MAXANN, WFDB_MAXSIG, and
   WFDB_MAXSPF to determine array sizes, but (since WFDB library version 10.2)
   there are no longer any fixed limits imposed by the WFDB library.
*/
#define WFDB_MAXANN    2   /* maximum number of input or output annotators */
#define	WFDB_MAXSIG   32   /* maximum number of input or output signals */
#define WFDB_MAXSPF    4   /* maximum number of samples per signal per frame */
#define WFDB_MAXRNL   50   /* maximum length of record name */
#define WFDB_MAXUSL   50   /* maximum length of WFDB_siginfo `.units' string */
#define WFDB_MAXDSL  100   /* maximum length of WFDB_siginfo `.desc' string */

/* wfdb_fopen mode values (WFDB_anninfo '.stat' values) */
#define WFDB_READ      0   /* standard input annotation file */
#define WFDB_WRITE     1   /* standard output annotation file */
#define WFDB_AHA_READ  2   /* AHA-format input annotation file */
#define WFDB_AHA_WRITE 3   /* AHA-format output annotation file */
#define WFDB_APPEND    4   /* for output info files */

/* WFDB_siginfo '.fmt' values
FMT_LIST is suitable as an initializer for a static array; it lists all of
the legal values for the format field in a WFDB_siginfo structure.
 fmt    meaning
   0	null signal (nothing read or written)
   8	8-bit first differences
  16	16-bit 2's complement amplitudes, low byte first
  61	16-bit 2's complement amplitudes, high byte first
  80	8-bit offset binary amplitudes
 160	16-bit offset binary amplitudes
 212	2 12-bit amplitudes bit-packed in 3 bytes
 310	3 10-bit amplitudes bit-packed in 4 bytes
 311    3 10-bit amplitudes bit-packed in 4 bytes
  24	24-bit 2's complement amplitudes, low byte first
  32	32-bit 2's complement amplitudes, low byte first
 508    FLAC, 8 bits per sample
 516    FLAC, 16 bits per sample
 524    FLAC, 24 bits per sample
*/
#define WFDB_FMT_LIST {0, 8, 16, 61, 80, 160, 212, 310, 311, 24, 32, \
      508, 516, 524}
#define WFDB_NFMTS	  14    /* number of items in WFDB_FMT_LIST */

/* Default signal specifications */
#define WFDB_DEFFREQ	250.0  /* default sampling frequency (Hz) */
#define WFDB_DEFGAIN	200.0  /* default value for gain (adu/physical unit) */
#define WFDB_DEFRES	12     /* default value for ADC resolution (bits) */

/* getvec operating modes */
#define WFDB_LOWRES   	0	/* return one sample per signal per frame */
#define WFDB_HIGHRES	1	/* return each sample of oversampled signals,
				   duplicating samples of other signals */
#define WFDB_GVPAD	2	/* replace invalid samples with previous valid
				   samples */

/* calinfo '.caltype' values
WFDB_AC_COUPLED and WFDB_DC_COUPLED are used in combination with the pulse
shape definitions below to characterize calibration pulses. */
#define WFDB_AC_COUPLED	0	/* AC coupled signal */
#define WFDB_DC_COUPLED	1	/* DC coupled signal */
#define WFDB_CAL_SQUARE	2	/* square wave pulse */
#define WFDB_CAL_SINE	4	/* sine wave pulse */
#define WFDB_CAL_SAWTOOTH	6	/* sawtooth pulse */
#define WFDB_CAL_UNDEF	8	/* undefined pulse shape */

/* Structure definitions */
struct WFDB_siginfo {	/* signal information structure */
    char *fname;	/* filename of signal file */
    char *desc;		/* signal description */
    char *units;	/* physical units (mV unless otherwise specified) */
    WFDB_Gain gain;	/* gain (ADC units/physical unit, 0: uncalibrated) */
    WFDB_Sample initval; 	/* initial value (that of sample number 0) */
    WFDB_Group group;	/* signal group number */
    int fmt;		/* format (8, 16, etc.) */
    int spf;		/* samples per frame (>1 for oversampled signals) */
    int bsize;		/* block size (for character special files only) */
    int adcres;		/* ADC resolution in bits */
    int adczero;	/* ADC output given 0 VDC input */
    int baseline;	/* ADC output given 0 physical units input */
    long nsamp;		/* number of samples (0: unspecified) */
    int cksum;		/* 16-bit checksum of all samples */
};

struct WFDB_calinfo {	/* calibration information structure */
    double low;		/* low level of calibration pulse in physical units */
    double high;	/* high level of calibration pulse in physical units */
    double scale;	/* customary plotting scale (physical units per cm) */
    char *sigtype;	/* signal type */
    char *units;	/* physical units */
    int caltype;	/* calibration pulse type (see definitions above) */
};

struct WFDB_anninfo {	/* annotator information structure */
    char *name;		/* annotator name */
    int stat;		/* file type/access code (READ, WRITE, etc.) */
};

struct WFDB_ann {	/* annotation structure */
    WFDB_Time time;	/* annotation time, in sample intervals from
			   the beginning of the record */
    char anntyp;	/* annotation type (< ACMAX, see <wfdb/ecgcodes.h> */
    signed char subtyp;	/* annotation subtype */
    unsigned char chan;	/* channel number */
    signed char num;	/* annotator number */
    unsigned char *aux;	/* pointer to auxiliary information */
};

struct WFDB_seginfo {	/* segment record structure */
    char recname[WFDB_MAXRNL+1];   /* segment name */
    WFDB_Time nsamp;		   /* number of samples in segment */
    WFDB_Time samp0;		   /* sample number of first sample */
};

/* Composite data types */
typedef struct WFDB_siginfo WFDB_Siginfo;
typedef struct WFDB_calinfo WFDB_Calinfo;
typedef struct WFDB_anninfo WFDB_Anninfo;
typedef struct WFDB_ann WFDB_Annotation;
typedef struct WFDB_seginfo WFDB_Seginfo;

/* Opaque context type for thread-safe concurrent use of the WFDB library.
   Each context holds independent library state, allowing multiple records
   to be processed simultaneously without interference. */
typedef struct WFDB_Context WFDB_Context;

/* Dynamic memory allocation macros. */
#define MEMERR(P, N, S)                                                 \
    do {                                                                \
        wfdb_error("WFDB: can't allocate (%lu*%lu) bytes for %s\n",     \
                   (unsigned long)N, (unsigned long)S, #P);             \
        if (wfdb_me_fatal()) exit(1);                                   \
    } while (0)
#define SFREE(P) do { if (P) { free (P); P = 0; } } while (0)
#define SUALLOC(P, N, S)                                                \
    do {                                                                \
        size_t WFDB_tmp1 = (N), WFDB_tmp2 = (S);                        \
        if (WFDB_tmp1 == 0 || WFDB_tmp2 == 0)                           \
            WFDB_tmp1 = WFDB_tmp2 = 1;                                  \
        if (!(P = calloc(WFDB_tmp1, WFDB_tmp2)))                        \
            MEMERR(P, WFDB_tmp1, WFDB_tmp2);                            \
    } while (0)
#define SALLOC(P, N, S) do { SFREE(P); SUALLOC(P, (N), (S)); } while (0)
#define SREALLOC(P, N, S) \
    do {                                                                \
        size_t WFDB_tmp1 = (N), WFDB_tmp2 = (S);                        \
        if (!(P = ((WFDB_tmp1 == 0 || WFDB_tmp2 == 0)                   \
                   ? realloc(P, 1)                                      \
                   : ((WFDB_tmp1 > (size_t) -1 / WFDB_tmp2)             \
                      ? 0 : realloc(P, WFDB_tmp1 * WFDB_tmp2)))))       \
            MEMERR(P, WFDB_tmp1, WFDB_tmp2);                            \
    } while (0)
#define SSTRCPY(P, Q)                                                   \
    do {                                                                \
        const char *WFDB_tmp = (Q);                                     \
        if (WFDB_tmp) {                                                 \
            SALLOC(P, (size_t)strlen(WFDB_tmp)+1, 1);                   \
            strcpy(P, WFDB_tmp);                                        \
        }                                                               \
    } while (0)

/* Specify C linkage for C++ compilers. */
#ifdef __cplusplus
extern "C" {
#endif

/* Function prototypes */
extern int annopen(char *record, const WFDB_Anninfo *aiarray,
		    unsigned int nann);
extern int isigopen(char *record, WFDB_Siginfo *siarray, int nsig);
extern int osigopen(char *record, WFDB_Siginfo *siarray,
		     unsigned int nsig);
extern int osigfopen(const WFDB_Siginfo *siarray, unsigned int nsig);
extern int wfdbinit(char *record,
		     const WFDB_Anninfo *aiarray, unsigned int nann,
		     WFDB_Siginfo *siarray, unsigned int nsig);
extern int findsig(const char *signame);
extern int getspf(void);
extern void setgvmode(int mode);
extern int getgvmode(void);
extern int setifreq(WFDB_Frequency freq);
extern WFDB_Frequency getifreq(void);
extern int getvec(WFDB_Sample *vector);
extern int getframe(WFDB_Sample *vector);
extern int putvec(const WFDB_Sample *vector);
extern int getann(WFDB_Annotator a, WFDB_Annotation *annot);
extern int ungetann(WFDB_Annotator a, const WFDB_Annotation *annot);
extern int putann(WFDB_Annotator a, const WFDB_Annotation *annot);
extern int isigsettime(WFDB_Time t);
extern int isgsettime(WFDB_Group g, WFDB_Time t);
extern WFDB_Time tnextvec(WFDB_Signal s, WFDB_Time t);
extern int iannsettime(WFDB_Time t);
extern char *ecgstr(int annotation_code);
extern int strecg(const char *annotation_mnemonic_string);
extern int setecgstr(int annotation_code,
		      const char *annotation_mnemonic_string);
extern char *annstr(int annotation_code);
extern int strann(const char *annotation_mnemonic_string);
extern int setannstr(int annotation_code,
		      const char *annotation_mnemonic_string);
extern char *anndesc(int annotation_code);
extern int setanndesc(int annotation_code,
		       const char *annotation_description);
extern void setafreq(WFDB_Frequency f);
extern WFDB_Frequency getafreq(void);
extern void setiafreq(WFDB_Annotator a, WFDB_Frequency f);
extern WFDB_Frequency getiafreq(WFDB_Annotator a);
extern WFDB_Frequency getiaorigfreq(WFDB_Annotator a);
extern void iannclose(WFDB_Annotator a);
extern void oannclose(WFDB_Annotator a);
extern int wfdb_isann(int code);
extern int wfdb_isqrs(int code);
extern int wfdb_setisqrs(int code, int newval);
extern int wfdb_map1(int code);
extern int wfdb_setmap1(int code, int newval);
extern int wfdb_map2(int code);
extern int wfdb_setmap2(int code, int newval);
extern int wfdb_ammap(int code);
extern int wfdb_mamap(int code, int subtype);
extern int wfdb_annpos(int code);
extern int wfdb_setannpos(int code, int newval);
extern char *timstr(WFDB_Time t);
extern char *mstimstr(WFDB_Time t);
extern WFDB_Time strtim(const char *time_string);
extern char *datstr(WFDB_Date d);
extern WFDB_Date strdat(const char *date_string);
extern int adumuv(WFDB_Signal s, WFDB_Sample a);
extern WFDB_Sample muvadu(WFDB_Signal s, int microvolts);
extern double aduphys(WFDB_Signal s, WFDB_Sample a);
extern WFDB_Sample physadu(WFDB_Signal s, double v);
extern WFDB_Sample sample(WFDB_Signal s, WFDB_Time t);
extern int sample_valid(void);
extern int calopen(const char *calibration_filename);
extern int getcal(const char *description, const char *units,
		   WFDB_Calinfo *cal);
extern int putcal(const WFDB_Calinfo *cal);
extern int newcal(const char *calibration_filename);
extern void flushcal(void);
extern char *getinfo(char *record);
extern int putinfo(const char *info);
extern int setinfo(char *record);
extern void wfdb_freeinfo(void);
extern int newheader(char *record);
extern int setheader(char *record, const WFDB_Siginfo *siarray,
		      unsigned int nsig);
extern int setmsheader(char *record, char **segnames, unsigned int nsegments);
extern int getseginfo(WFDB_Seginfo **segments);
extern int wfdbgetskew(WFDB_Signal s);
extern void wfdbsetiskew(WFDB_Signal s, int skew);
extern void wfdbsetskew(WFDB_Signal s, int skew);
extern long wfdbgetstart(WFDB_Signal s);
extern void wfdbsetstart(WFDB_Signal s, long bytes);
extern int wfdbputprolog(const char *prolog, long bytes, WFDB_Signal s);
extern void wfdbquit(void);
extern WFDB_Frequency sampfreq(char *record);
extern int setsampfreq(WFDB_Frequency sampling_frequency);
extern WFDB_Frequency getcfreq(void);
extern void setcfreq(WFDB_Frequency counter_frequency);
extern double getbasecount(void);
extern void setbasecount(double count);
extern int setbasetime(char *time_string);
extern void wfdbquiet(void);
extern void wfdbverbose(void);
extern char *wfdberror(void);
extern void setwfdb(const char *database_path_string);
extern char *getwfdb(void);
extern void resetwfdb(void);
extern int setibsize(int input_buffer_size);
extern int setobsize(int output_buffer_size);
extern char *wfdbfile(const char *file_type, char *record);
extern void wfdbflush(void);
extern void wfdbmemerr(int exit_on_error);
#if __GNUC__ >= 3
__attribute__((__format__(__printf__, 1, 2)))
#endif
extern void wfdb_error(const char *format_string, ...);
extern int wfdb_me_fatal(void);
extern const char *wfdbversion(void);
extern const char *wfdbldflags(void);
extern const char *wfdbcflags(void);
extern const char *wfdbdefwfdb(void);
extern const char *wfdbdefwfdbcal(void);

/* Context lifecycle */
extern WFDB_Context *wfdb_context_create(void);
extern void wfdb_context_destroy(WFDB_Context *ctx);

/* Context-aware variants of WFDB library functions.
   These take an explicit WFDB_Context* as their first argument, allowing
   multiple independent library sessions to coexist (e.g. in separate
   threads).  The original functions above are thin wrappers that call
   these with the default (global) context. */

/* Signal I/O (signal.c) */
extern int isigopen_ctx(WFDB_Context *ctx, char *record,
			WFDB_Siginfo *siarray, int nsig);
extern int osigopen_ctx(WFDB_Context *ctx, char *record,
			WFDB_Siginfo *siarray, unsigned int nsig);
extern int osigfopen_ctx(WFDB_Context *ctx, const WFDB_Siginfo *siarray,
			  unsigned int nsig);
extern int findsig_ctx(WFDB_Context *ctx, const char *signame);
extern int getspf_ctx(WFDB_Context *ctx);
extern void setgvmode_ctx(WFDB_Context *ctx, int mode);
extern int getgvmode_ctx(WFDB_Context *ctx);
extern int setifreq_ctx(WFDB_Context *ctx, WFDB_Frequency freq);
extern WFDB_Frequency getifreq_ctx(WFDB_Context *ctx);
extern int getvec_ctx(WFDB_Context *ctx, WFDB_Sample *vector);
extern int getframe_ctx(WFDB_Context *ctx, WFDB_Sample *vector);
extern int putvec_ctx(WFDB_Context *ctx, const WFDB_Sample *vector);
extern int isigsettime_ctx(WFDB_Context *ctx, WFDB_Time t);
extern int isgsettime_ctx(WFDB_Context *ctx, WFDB_Group g, WFDB_Time t);
extern WFDB_Time tnextvec_ctx(WFDB_Context *ctx, WFDB_Signal s, WFDB_Time t);
extern int setibsize_ctx(WFDB_Context *ctx, int input_buffer_size);
extern int setobsize_ctx(WFDB_Context *ctx, int output_buffer_size);
extern WFDB_Sample sample_ctx(WFDB_Context *ctx, WFDB_Signal s, WFDB_Time t);
extern int sample_valid_ctx(WFDB_Context *ctx);

/* Header/info (signal.c) */
extern int newheader_ctx(WFDB_Context *ctx, char *record);
extern int setheader_ctx(WFDB_Context *ctx, char *record,
			  const WFDB_Siginfo *siarray, unsigned int nsig);
extern int setmsheader_ctx(WFDB_Context *ctx, char *record,
			    char **segnames, unsigned int nsegments);
extern int getseginfo_ctx(WFDB_Context *ctx, WFDB_Seginfo **segments);
extern int wfdbgetskew_ctx(WFDB_Context *ctx, WFDB_Signal s);
extern void wfdbsetiskew_ctx(WFDB_Context *ctx, WFDB_Signal s, int skew);
extern void wfdbsetskew_ctx(WFDB_Context *ctx, WFDB_Signal s, int skew);
extern long wfdbgetstart_ctx(WFDB_Context *ctx, WFDB_Signal s);
extern void wfdbsetstart_ctx(WFDB_Context *ctx, WFDB_Signal s, long bytes);
extern int wfdbputprolog_ctx(WFDB_Context *ctx, const char *prolog,
			      long bytes, WFDB_Signal s);
extern int setinfo_ctx(WFDB_Context *ctx, char *record);
extern int putinfo_ctx(WFDB_Context *ctx, const char *info);
extern char *getinfo_ctx(WFDB_Context *ctx, char *record);
extern void wfdb_freeinfo_ctx(WFDB_Context *ctx);

/* Annotation I/O (annot.c) */
extern int annopen_ctx(WFDB_Context *ctx, char *record,
			const WFDB_Anninfo *aiarray, unsigned int nann);
extern int getann_ctx(WFDB_Context *ctx, WFDB_Annotator a,
		       WFDB_Annotation *annot);
extern int ungetann_ctx(WFDB_Context *ctx, WFDB_Annotator a,
			 const WFDB_Annotation *annot);
extern int putann_ctx(WFDB_Context *ctx, WFDB_Annotator a,
		       const WFDB_Annotation *annot);
extern int iannsettime_ctx(WFDB_Context *ctx, WFDB_Time t);
extern void setafreq_ctx(WFDB_Context *ctx, WFDB_Frequency f);
extern WFDB_Frequency getafreq_ctx(WFDB_Context *ctx);
extern void setiafreq_ctx(WFDB_Context *ctx, WFDB_Annotator a,
			    WFDB_Frequency f);
extern WFDB_Frequency getiafreq_ctx(WFDB_Context *ctx, WFDB_Annotator a);
extern WFDB_Frequency getiaorigfreq_ctx(WFDB_Context *ctx,
					  WFDB_Annotator a);
extern void iannclose_ctx(WFDB_Context *ctx, WFDB_Annotator a);
extern void oannclose_ctx(WFDB_Context *ctx, WFDB_Annotator a);
extern char *ecgstr_ctx(WFDB_Context *ctx, int annotation_code);
extern int strecg_ctx(WFDB_Context *ctx,
		       const char *annotation_mnemonic_string);
extern int setecgstr_ctx(WFDB_Context *ctx, int annotation_code,
			  const char *annotation_mnemonic_string);
extern char *annstr_ctx(WFDB_Context *ctx, int annotation_code);
extern int strann_ctx(WFDB_Context *ctx,
		       const char *annotation_mnemonic_string);
extern int setannstr_ctx(WFDB_Context *ctx, int annotation_code,
			  const char *annotation_mnemonic_string);
extern char *anndesc_ctx(WFDB_Context *ctx, int annotation_code);
extern int setanndesc_ctx(WFDB_Context *ctx, int annotation_code,
			   const char *annotation_description);

/* Calibration (calib.c) */
extern int calopen_ctx(WFDB_Context *ctx, const char *calibration_filename);
extern int getcal_ctx(WFDB_Context *ctx, const char *description,
		       const char *units, WFDB_Calinfo *cal);
extern int putcal_ctx(WFDB_Context *ctx, const WFDB_Calinfo *cal);
extern int newcal_ctx(WFDB_Context *ctx, const char *calibration_filename);
extern void flushcal_ctx(WFDB_Context *ctx);

/* Time/frequency conversion (timeconv.c) */
extern WFDB_Frequency sampfreq_ctx(WFDB_Context *ctx, char *record);
extern int setsampfreq_ctx(WFDB_Context *ctx,
			    WFDB_Frequency sampling_frequency);
extern int setbasetime_ctx(WFDB_Context *ctx, char *time_string);
extern char *timstr_ctx(WFDB_Context *ctx, WFDB_Time t);
extern char *mstimstr_ctx(WFDB_Context *ctx, WFDB_Time t);
extern WFDB_Time strtim_ctx(WFDB_Context *ctx, const char *time_string);
extern char *datstr_ctx(WFDB_Context *ctx, WFDB_Date d);
extern WFDB_Date strdat_ctx(WFDB_Context *ctx, const char *date_string);
extern WFDB_Frequency getcfreq_ctx(WFDB_Context *ctx);
extern void setcfreq_ctx(WFDB_Context *ctx, WFDB_Frequency counter_frequency);
extern double getbasecount_ctx(WFDB_Context *ctx);
extern void setbasecount_ctx(WFDB_Context *ctx, double count);

/* Unit conversion (timeconv.c) */
extern int adumuv_ctx(WFDB_Context *ctx, WFDB_Signal s, WFDB_Sample a);
extern WFDB_Sample muvadu_ctx(WFDB_Context *ctx, WFDB_Signal s,
			       int microvolts);
extern double aduphys_ctx(WFDB_Context *ctx, WFDB_Signal s, WFDB_Sample a);
extern WFDB_Sample physadu_ctx(WFDB_Context *ctx, WFDB_Signal s, double v);

/* Library control (wfdbio.c) */
extern void setwfdb_ctx(WFDB_Context *ctx, const char *database_path_string);
extern char *getwfdb_ctx(WFDB_Context *ctx);
extern void resetwfdb_ctx(WFDB_Context *ctx);
extern void wfdbquiet_ctx(WFDB_Context *ctx);
extern void wfdbverbose_ctx(WFDB_Context *ctx);
extern char *wfdberror_ctx(WFDB_Context *ctx);
extern char *wfdbfile_ctx(WFDB_Context *ctx, const char *file_type,
			   char *record);
extern void wfdbmemerr_ctx(WFDB_Context *ctx, int exit_on_error);

/* Init/quit (wfdbinit.c) */
extern int wfdbinit_ctx(WFDB_Context *ctx, char *record,
			 const WFDB_Anninfo *aiarray, unsigned int nann,
			 WFDB_Siginfo *siarray, unsigned int nsig);
extern void wfdbquit_ctx(WFDB_Context *ctx);
extern void wfdbflush_ctx(WFDB_Context *ctx);

#ifdef __cplusplus
}
#endif

#include <stdlib.h>
#include <string.h>

#endif

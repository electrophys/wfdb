/* file: wfdb_context.h	2026
   WFDB library context structure.

   This header defines the WFDB_Context structure that encapsulates all
   library state, enabling multiple simultaneous record access and
   thread safety.  Each WFDB_Context holds the complete state that was
   previously stored in file-scope static and global variables.

   Phase 3 of WFDB modernization.  The context is populated module by
   module; fields are added as each module is converted.
*/

#ifndef WFDB_CONTEXT_H
#define WFDB_CONTEXT_H

#include "wfdblib.h"
#include "ecgcodes.h"

/* ---- Calibration list entry ---- */
struct cle {
    double low;
    double high;
    double scale;
    char *sigtype;
    char *units;
    int caltype;
    struct cle *next;
};

/* ---- Annotation data structures (from annot.c) ---- */

#define AUXBUFLEN 771

struct iadata {
    WFDB_FILE *file;		/* file pointer for input annotation file */
    WFDB_Anninfo info;		/* input annotator information */
    WFDB_Annotation ann;	/* next annotation to be returned by getann */
    WFDB_Annotation pann;	/* pushed-back annotation from ungetann */
    WFDB_Frequency afreq;	/* time resolution, in ticks/second */
    unsigned word;		/* next word from the input file */
    int ateof;			/* EOF-reached indicator */
    unsigned char auxstr[AUXBUFLEN]; /* aux string buffer */
    unsigned index;		/* next available position in auxstr */
    double tmul;		/* tmul * annotation time = sample count */
    double tt;			/* annotation time (MIT format only) */
    double ann_tt;		/* unscaled annotation time of 'ann' */
    double pann_tt;		/* unscaled annotation time of 'pann' */
    double prev_tt;		/* unscaled time of last annotation returned */
    WFDB_Time prev_time;	/* sample number of last annotation returned */
};

struct oadata {
    WFDB_FILE *file;		/* file pointer for output annotation file */
    WFDB_Anninfo info;		/* output annotator information */
    WFDB_Annotation ann;	/* most recent annotation written by putann */
    WFDB_Frequency afreq;	/* time resolution, in ticks/second */
    int seqno;			/* annotation serial number (AHA format only) */
    char *rname;		/* record with which annotator is associated */
    char out_of_order;		/* if >0, annotations not in canonical order */
    char table_written;		/* if >0, table has been written */
};

/* ---- WFDB path component (from wfdbio.c) ---- */
struct wfdb_path_component {
    char *prefix;
    struct wfdb_path_component *next, *prev;
    int type;		/* WFDB_LOCAL or WFDB_NET */
};

/* ---- WFDB_Context ---- */
struct WFDB_Context {

    /* Calibration state (from calib.c) */
    struct cle *first_cle;
    struct cle *this_cle;
    struct cle *prev_cle;

    /* Annotation state (from annot.c) */
    char *cstring[ACMAX+1];	/* ECG mnemonic strings */
    char *astring[ACMAX+1];	/* annotation mnemonic strings */
    char *tstring[ACMAX+1];	/* annotation descriptive strings */
    char wfdb_qrs[ACMAX+1];	/* QRS classification table */
    char wfdb_mp1[ACMAX+1];	/* map1 classification table */
    char wfdb_mp2[ACMAX+1];	/* map2 classification table */
    char wfdb_annp[ACMAX+1];	/* annotation position table */
    char ecgstr_buf[14];	/* ecgstr return buffer */
    char annstr_buf[14];	/* annstr return buffer */
    int wfdb_mt;		/* ecgmap.h macro temporary */
    unsigned maxiann;		/* max allowed number of input annotators */
    unsigned niaf;		/* number of open input annotators */
    struct iadata **iad;	/* input annotator data */
    unsigned maxoann;		/* max allowed number of output annotators */
    unsigned noaf;		/* number of open output annotators */
    struct oadata **oad;	/* output annotator data */
    WFDB_Frequency oafreq;	/* time resolution for output annotation files */
    int annclose_error;		/* error flag for annotation close */
    char modified[ACMAX+1];	/* modified annotation type flags */

    /* I/O state (from wfdbio.c) */
    char *wfdbpath;		/* WFDB database path string */
    char *wfdbpath_init;	/* initial WFDB path (for resetwfdb) */
    int error_print;		/* if nonzero, print error messages */
    char *wfdb_filename;	/* last resolved filename */
    int wfdb_mem_behavior;	/* memory error behavior (1=fatal) */
    int error_flag;		/* set after first call to wfdb_error */
    char *error_message;	/* most recent error message */
    struct wfdb_path_component *wfdb_path_list; /* parsed path */
    char irec[WFDB_MAXRNL+1];	/* current record name */
    /* putenv strings (must remain valid for process lifetime) */
    char *p_wfdb;
    char *p_wfdbcal;
    char *p_wfdbannsort;
    char *p_wfdbgvmode;
    int wfdbpath_parsed;	/* nonzero after first getwfdb call */

    /* Signal state (from signal.c / signal_internal.h) */

    /* Header data */
    unsigned maxhsig;
    WFDB_FILE *hheader;
    char *linebuf;
    size_t linebufsize;
    struct hsdata **hsd;

    /* Time/frequency/conversion */
    WFDB_Frequency ffreq;
    WFDB_Frequency ifreq;
    WFDB_Frequency sfreq;
    WFDB_Frequency cfreq;
    int spfmax;
    long btime;
    WFDB_Date bdate;
    WFDB_Time nsamples;
    double bcount;
    long prolog_bytes;

    /* Multi-segment record */
    int segments;
    int in_msrec;
    long msbtime;
    WFDB_Date msbdate;
    WFDB_Time msnsamples;
    WFDB_Seginfo *segarray, *segp, *segend;

    /* Input signals */
    unsigned maxisig;
    unsigned maxigroup;
    unsigned nisig;
    unsigned nigroup;
    unsigned ispfmax;
    struct isdata **isd;
    struct igdata **igd;
    WFDB_Sample *tvector;
    WFDB_Sample *uvector;
    WFDB_Sample *vvector;
    int tuvlen;
    WFDB_Time istime;
    int ibsize;
    unsigned skewmax;
    WFDB_Sample *dsbuf;
    int dsbi;
    unsigned dsblen;
    unsigned framelen;
    int gvmode;
    int gvc;
    int isedf;
    WFDB_Sample *sbuf;
    int sample_vflag;

    /* Output signals */
    unsigned maxosig;
    unsigned maxogroup;
    unsigned nosig;
    unsigned nogroup;
    WFDB_FILE *oheader;
    WFDB_FILE *outinfo;
    struct osdata **osd;
    struct ogdata **ogd;
    WFDB_Time ostime;
    int obsize;

    /* Info strings */
    char **pinfo;
    int nimax;
    int ninfo;

    /* Sigmap */
    int need_sigmap, maxvsig, nvsig, tspf, vspfmax;
    struct isdata **vsd;
    WFDB_Sample *ovec;
    struct sigmapinfo *smi;

    /* Resampling state (from signal.c) */
    long mticks, nticks, mnticks;
    int rgvstat;
    WFDB_Time rgvtime, gvtime;
    WFDB_Sample *gv0, *gv1;

    /* getinfo iteration index */
    int getinfo_index;

    /* sample() function cache */
    WFDB_Time sample_tt;

    /* Format I/O helpers */
    int _l;		/* macro temp for low byte of word */
    int _lw;		/* macro temp for low 16 bits of int */
    int _n;		/* macro temp for byte count */

    /* Time conversion (from timeconv.c) */
    char date_string[37];
    char time_string[62];
    WFDB_Date pdays;

#if WFDB_NETFILES
    /* NETFILES state (from wfdbio.c) */
    int nf_open_files;		/* number of open netfiles */
    long nf_page_size;		/* bytes per http range request */
    int www_done_init;		/* TRUE once libcurl is initialized */
    CURL *curl_ua;		/* libcurl easy handle */
    char curl_error_buf[CURL_ERROR_SIZE]; /* curl error message buffer */
    char **www_passwords;	/* parsed WFDBPASSWORD credentials */
    char *curl_ua_string;	/* cached User-Agent string */
#endif

    /* Initialization flag */
    int initialized;

};

/* Typedef for convenience (also declared as opaque type in wfdb.h). */
typedef struct WFDB_Context WFDB_Context;

/* Get the default (global) context. */
WFDB_Context *wfdb_get_default_context(void);

/* Create a new context with default initialization. */
WFDB_Context *wfdb_context_new(void);

/* Free a context and all associated resources. */
void wfdb_context_free(WFDB_Context *ctx);

#endif /* WFDB_CONTEXT_H */

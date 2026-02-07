/* file: header.c	2026  Header reading/parsing for WFDB library. */

#include "signal_internal.h"

#define strtotime strtoll

/* Read a fixed-size character field and remove trailing spaces. */
void read_edf_str(char *buf, int size, WFDB_FILE *ifile)
{
    if (wfdb_fread(buf, 1, size, ifile) != size) {
	*buf = 0;
    }
    else {
	while (size > 0 && buf[size - 1] == ' ')
	    size--;
	buf[size] = 0;
    }
}

/* get header information from an EDF file */
int edfparse(WFDB_FILE *ifile)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    char buf[81], *edf_fname, *p;
    double *pmax, *pmin, spr, baseline;
    int format, i, s, nsig, offset, day, month, year, hour, minute, second;
    long adcrange, *dmax, *dmin, nframes;

    edf_fname = wfdbfile(NULL, NULL);

    /* Read the first 8 bytes and check for the magic string.  (This might
       accept some non-EDF files.) */
    wfdb_fread(buf, 1, 8, ifile);
    if (strncmp(buf, "0       ", 8) == 0)
	format = 16;	/* EDF or EDF+ */
    else if (strncmp(buf+1, "BIOSEMI", 7) == 0)
	format = 24;	/* BDF */
    else {
	wfdb_error("init: '%s' is not EDF or EDF+\n", edf_fname);
	return (-2);
    }

    /* Read the remainder of the fixed-size section of the header. */
    read_edf_str(buf, 80, ifile);	/* patient ID (ignored) */
    read_edf_str(buf, 80, ifile);	/* recording ID (ignored) */
    read_edf_str(buf, 8, ifile);	/* recording date */
    sscanf(buf, "%d%*c%d%*c%d", &day, &month, &year);
    year += 1900;			/* EDF has only two-digit years */
    if (year < 1985) year += 100;	/* fix this before 1/1/2085! */
    read_edf_str(buf, 8, ifile);	/* recording time */
    sscanf(buf, "%d%*c%d%*c%d", &hour, &minute, &second);
    read_edf_str(buf, 8, ifile);	/* number of bytes in header */
    sscanf(buf, "%d", &offset);
    read_edf_str(buf, 44, ifile);	/* free space (ignored) */
    read_edf_str(buf, 8, ifile);	/* number of frames (EDF blocks) */
    sscanf(buf, "%ld", &nframes);
    if (nframes < 0) nframes = 0;
    nsamples = nframes;
    read_edf_str(buf, 8, ifile);	/* data record duration (seconds) */
    sscanf(buf, "%lf", &spr);
    if (spr <= 0.0) spr = 1.0;
    wfdb_fread(buf+4, 1, 4, ifile);	/* number of signals */
    sscanf(buf+4, "%d", &nsig);

    if (nsig < 1 || (nsig + 1)*256 != offset) {
	wfdb_error("init: '%s' is not EDF or EDF+\n", edf_fname);
	return (-2);
    }

    /* Allocate workspace. */
    if (maxhsig < nsig) {
	unsigned m = maxhsig;

	SREALLOC(hsd, nsig, sizeof(struct hsdata *));
	while (m < nsig) {
	    SUALLOC(hsd[m], 1, sizeof(struct hsdata));
	    m++;
	}
	maxhsig = nsig;
    }
    SUALLOC(dmax, nsig, sizeof(long));
    SUALLOC(dmin, nsig, sizeof(long));
    SUALLOC(pmax, nsig, sizeof(double));
    SUALLOC(pmin, nsig, sizeof(double));

    /* Strip off any path info from the EDF file name. */
    p = edf_fname + strlen(edf_fname) - 4;
    while (--p > edf_fname)
	if (*p == '/') edf_fname = p+1;

    /* Read the variable-size section of the header. */
    for (s = 0; s < nsig; s++) {
	hsd[s]->start = offset;
	hsd[s]->skew = 0;
	SSTRCPY(hsd[s]->info.fname, edf_fname);
	hsd[s]->info.group = hsd[s]->info.bsize = hsd[s]->info.cksum = 0;
	hsd[s]->info.fmt = format;
	hsd[s]->info.nsamp = nframes;

	read_edf_str(buf, 16, ifile);	/* signal type */
	SSTRCPY(hsd[s]->info.desc, buf);
    }

    for (s = 0; s < nsig; s++)
	read_edf_str(buf, 80, ifile); /* transducer type (ignored) */

    for (s = 0; s < nsig; s++) {
	read_edf_str(buf, 8, ifile);	/* signal units */
	SSTRCPY(hsd[s]->info.units, buf);
    }

    for (s = 0; s < nsig; s++) {
	read_edf_str(buf, 8, ifile);	/* physical minimum */
	sscanf(buf, "%lf", &pmin[s]);
    }

    for (s = 0; s < nsig; s++) {
	read_edf_str(buf, 8, ifile);	/* physical maximum */
	sscanf(buf, "%lf", &pmax[s]);
    }

    for (s = 0; s < nsig; s++) {
	read_edf_str(buf, 8, ifile);	/* digital minimum */
	sscanf(buf, "%ld", &dmin[s]);
    }

    for (s = 0; s < nsig; s++) {
	read_edf_str(buf, 8, ifile);	/* digital maximum */
	sscanf(buf, "%ld", &dmax[s]);
	hsd[s]->info.initval = hsd[s]->info.adczero = (dmax[s]+1 + dmin[s])/2;
	adcrange = dmax[s] - dmin[s];
	for (i = 0; adcrange > 0; i++)
	    adcrange /= 2;
	hsd[s]->info.adcres = i;
	if (pmax[s] != pmin[s]) {
	    hsd[s]->info.gain = (dmax[s] - dmin[s])/(pmax[s] - pmin[s]);
	    baseline = dmax[s] - pmax[s] * hsd[s]->info.gain;
	    if (baseline >= 0.0)
		hsd[s]->info.baseline = baseline + 0.5;
	    else
		hsd[s]->info.baseline = baseline - 0.5;
	}
	else			/* gain is undefined */
	    hsd[s]->info.gain = hsd[s]->info.baseline = 0;
    }

    for (s = 0; s < nsig; s++)
	read_edf_str(buf, 80, ifile);	/* filtering information (ignored) */

    for (s = 0; s < nsig; s++) {
	int n;

	read_edf_str(buf, 8, ifile);	/* samples per frame (EDF block) */
	sscanf(buf, "%d", &n);
	if ((hsd[s]->info.spf = n) > spfmax) spfmax = n;
    }

    (void)wfdb_fclose(ifile);	/* (don't bother reading nsig*32 bytes of free
				   space) */
    hheader = NULL;	/* make sure getinfo doesn't try to read the EDF file */

    ffreq = 1.0 / spr;	/* frame frequency = 1/(seconds per EDF block) */
    cfreq = ffreq; /* set sampling and counter frequencies to match */
    sfreq = ffreq * spfmax;
    if (getafreq() == 0.0) setafreq(sfreq);
    gvmode |= WFDB_HIGHRES;
    sprintf(buf, "%02d:%02d:%02d %02d/%02d/%04d",
	    hour, minute, second, day, month, year);
    setbasetime(buf);

    SFREE(pmin);
    SFREE(pmax);
    SFREE(dmin);
    SFREE(dmax);
    isedf = 1;
    return (nsig);
}

int readheader(const char *record)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    char *p, *q;
    WFDB_Frequency f;
    WFDB_Signal s;
    WFDB_Time ns;
    unsigned int i, nsig;
    static char sep[] = " \t\n\r";

    /* If another input header file was opened, close it. */
    if (hheader) {
	(void)wfdb_fclose(hheader);
	hheader = NULL;
    }

    spfmax = 1;
    sfreq = ffreq;
    isedf = 0;
    if (strcmp(record, "~") == 0) {
	if (in_msrec && vsd) {
	    char *p;

	    SALLOC(hsd, 1, sizeof(struct hsdata *));
	    SALLOC(hsd[0], 1, sizeof(struct hsdata));
	    SSTRCPY(hsd[0]->info.desc, "~");
	    hsd[0]->info.spf = 1;
	    hsd[0]->info.fmt = 0;
	    hsd[0]->info.nsamp = nsamples = segp->nsamp;
	    return (maxhsig = 1);
	}
	return (0);
    }

    /* If the final component of the record name includes a '.', assume it is a
       file name. */
    q = (char *)record + strlen(record) - 1;
    while (q > record && *q != '.' && *q != '/' && *q != ':' && *q != '\\')
	q--;
    if (*q == '.') {
	if ((hheader = wfdb_open(NULL, record, WFDB_READ)) == NULL) {
	    wfdb_error("init: can't open %s\n", record);
	    return (-1);
	}
	else if (strcmp(q+1, "hea"))	/* assume EDF if suffix is not '.hea' */
	    return (edfparse(hheader));
    }

    /* Otherwise, assume the file name is record.hea. */
    else if ((hheader = wfdb_open("hea", record, WFDB_READ)) == NULL) {
	wfdb_error("init: can't open header for record %s\n", record);
	return (-1);
    }

    /* Read the first line and check for a magic string. */
    if (wfdb_getline(&linebuf, &linebufsize, hheader) == 0) {
        wfdb_error("init: record %s header is empty\n", record);
	    return (-2);
    }
    if (strncmp("#wfdb", linebuf, 5) == 0) { /* found the magic string */
	int i, major, minor = 0, release = 0;

	i = sscanf(linebuf+5, "%d.%d.%d", &major, &minor, &release);
	if ((i > 0 && major > WFDB_MAJOR) ||
	    (i > 1 && major == WFDB_MAJOR && minor > WFDB_MINOR) ||
	    (i > 2 && major == WFDB_MAJOR && minor == WFDB_MINOR &&
	     release > WFDB_RELEASE)) {
	    wfdb_error("init: reading record %s requires WFDB library "
		       "version %d.%d.%d or later\n"
"  (the most recent version is always available from http://physionet.org)\n",
		       record, major, minor, release);
	    return (-1);
	}
    }

    /* Get the first token (the record name) from the first non-empty,
       non-comment line. */
    while ((p = strtok(linebuf, sep)) == NULL || *p == '#') {
	if (wfdb_getline(&linebuf, &linebufsize, hheader) == 0) {
	    wfdb_error("init: can't find record name in record %s header\n",
		     record);
	    return (-2);
	}
    }

    for (q = p+1; *q && *q != '/'; q++)
	;
    if (*q == '/') {
	if (in_msrec) {
	    wfdb_error(
	  "init: record %s cannot be nested in another multi-segment record\n",
		     record);
	    return (-2);
	}
	segments = strtol(q+1, NULL, 10);
	*q = '\0';
    }

    /* For local files, be sure that the name (p) within the header file
       matches the name (record) provided as an argument to this function --
       if not, the header file may have been renamed in error or its contents
       may be corrupted.  The requirement for a match is waived for remote
       files since the user may not be able to make any corrections to them. */
    if (hheader->type == WFDB_LOCAL &&
	hheader->fp != stdin && strncmp(p, record, strlen(p)) != 0) {
	/* If there is a mismatch, check to see if the record argument includes
	   a directory separator (whether valid or not for this OS);  if so,
	   compare only the final portion of the argument against the name in
	   the header file. */
	const char *q, *r, *s;

	for (r = record, q = s = r + strlen(r) - 1; r != s; s--)
	    if (*s == '/' || *s == '\\' || *s == ':')
		break;

	if (q > s && (r > s || strcmp(p, s+1) != 0)) {
	    wfdb_error("init: record name in record %s header is incorrect\n",
		       record);
	    return (-2);
	}
    }

    /* Identify which type of header file is being read by trying to get
       another token from the line which contains the record name.  (Old-style
       headers have only one token on the first line, but new-style headers
       have two or more.) */
    if ((p = strtok((char *)NULL, sep)) == NULL) {
	/* The file appears to be an old-style header file. */
	wfdb_error("init: obsolete format in record %s header\n", record);
	return (-2);
    }

    /* The file appears to be a new-style header file.  The second token
       specifies the number of signals. */
    nsig = (unsigned)strtol(p, NULL, 10);

    /* Determine the frame rate, if present and not set already. */
    if (p = strtok((char *)NULL, sep)) {
	if ((f = (WFDB_Frequency)strtod(p, NULL)) <= (WFDB_Frequency)0.) {
	    wfdb_error(
		 "init: sampling frequency in record %s header is incorrect\n",
		 record);
	    return (-2);
	}
	if (ffreq > (WFDB_Frequency)0. && f != ffreq) {
	    wfdb_error("warning (init):\n");
	    wfdb_error(" record %s sampling frequency differs", record);
	    wfdb_error(" from that of previously opened record\n");
	}
	else
	    ffreq = f;
    }
    else if (ffreq == (WFDB_Frequency)0.)
	ffreq = WFDB_DEFFREQ;

    /* Set the sampling rate to the frame rate for now.  This may be
       changed later by isigopen or by setgvmode, if this is a multi-
       frequency record and WFDB_HIGHRES mode is in effect. */
    sfreq = ffreq;

    /* Determine the counter frequency and the base counter value. */
    cfreq = bcount = 0.0;
    if (p) {
	for ( ; *p && *p != '/'; p++)
	    ;
	if (*p == '/') {
	    cfreq = strtod(++p, NULL);
	    for ( ; *p && *p != '('; p++)
		;
	    if (*p == '(')
		bcount = strtod(++p, NULL);
	}
    }
    if (cfreq <= 0.0) cfreq = ffreq;

    /* Determine the number of samples per signal, if present and not
       set already. */
    if (p = strtok((char *)NULL, sep)) {
	if ((ns = strtotime(p, NULL, 10)) < 0L) {
	    wfdb_error(
		"init: number of samples in record %s header is incorrect\n",
		record);
	    return (-2);
	}
	if (nsamples == (WFDB_Time)0L)
	    nsamples = ns;
	else if (ns > (WFDB_Time)0L && ns != nsamples && !in_msrec) {
	    wfdb_error("warning (init):\n");
	    wfdb_error(" record %s duration differs", record);
	    wfdb_error(" from that of previously opened record\n");
	    /* nsamples must match the shortest record duration. */
	    if (nsamples > ns)
		nsamples = ns;
	}
    }
    else
	ns = (WFDB_Time)0L;

    /* Determine the base time and date, if present and not set already. */
    if ((p = strtok((char *)NULL,"\n\r")) != NULL &&
	btime == 0L && setbasetime(p) < 0)
	return (-2);	/* error message will come from setbasetime */

    /* Special processing for master header of a multi-segment record. */
    if (segments && !in_msrec) {
	msbtime = btime;
	msbdate = bdate;
	msnsamples = nsamples;
	/* Read the names and lengths of the segment records. */
	SALLOC(segarray, segments, sizeof(WFDB_Seginfo));
	segp = segarray;
	for (i = 0, ns = (WFDB_Time)0L; i < segments; i++, segp++) {
	    /* Get next segment spec, skip empty lines and comments. */
	    do {
		if (wfdb_getline(&linebuf, &linebufsize, hheader) == 0) {
		    wfdb_error(
			"init: unexpected EOF in header file for record %s\n",
			record);
		    SFREE(segarray);
		    segments = 0;
		    return (-2);
		}
	    } while ((p = strtok(linebuf, sep)) == NULL || *p == '#');
	    if (*p == '+') {
		wfdb_error(
		    "init: `%s' is not a valid segment name in record %s\n",
		    p, record);
		SFREE(segarray);
		segments = 0;
		return (-2);
	    }
	    if (strlen(p) > WFDB_MAXRNL) {
		wfdb_error(
		    "init: `%s' is too long for a segment name in record %s\n",
		    p, record);
		SFREE(segarray);
		segments = 0;
		return (-2);
	    }
	    (void)strcpy(segp->recname, p);
	    if ((p = strtok((char *)NULL, sep)) == NULL ||
		(segp->nsamp = strtotime(p, NULL, 10)) < 0L) {
		wfdb_error(
		"init: length must be specified for segment %s in record %s\n",
		           segp->recname, record);
		SFREE(segarray);
		segments = 0;
		return (-2);
	    }
	    segp->samp0 = ns;
	    ns += segp->nsamp;
	}
	segend = --segp;
	segp = segarray;
	if (msnsamples == 0L)
	    msnsamples = ns;
	else if (ns != msnsamples) {
	    wfdb_error("warning (init): in record %s, "
		       "stated record length (%"WFDB_Pd_TIME")\n",
		       record, msnsamples);
	    wfdb_error(" does not match sum of segment lengths "
		       "(%"WFDB_Pd_TIME")\n", ns);
	}
	return (0);
    }

    /* Allocate workspace. */
    if (maxhsig < nsig) {
	unsigned m = maxhsig;

	SREALLOC(hsd, nsig, sizeof(struct hsdata *));
	while (m < nsig) {
	    SUALLOC(hsd[m], 1, sizeof(struct hsdata));
	    m++;
	}
	maxhsig = nsig;
    }

    /* Now get information for each signal. */
    for (s = 0; s < nsig; s++) {
	struct hsdata *hp, *hs;
	int nobaseline;

	hs = hsd[s];
	hp = (s ? hsd[s-1] : NULL);
	/* Get the first token (the signal file name) from the next
	   non-empty, non-comment line. */
	do {
	    if (wfdb_getline(&linebuf, &linebufsize, hheader) == 0) {
		wfdb_error(
			"init: unexpected EOF in header file for record %s\n",
			record);
		return (-2);
	    }
	} while ((p = strtok(linebuf, sep)) == NULL || *p == '#');

	/* Determine the signal group number.  The group number for signal
	   0 is zero.  For subsequent signals, if the file name does not
	   match that of the previous signal, the group number is one
	   greater than that of the previous signal. */
	if (s == 0 || strcmp(p, hp->info.fname)) {
	    hs->info.group = (s == 0) ? 0 : hp->info.group + 1;
	    SSTRCPY(hs->info.fname, p);
	}
	/* If the file names of the current and previous signals match,
	   they are assigned the same group number and share a copy of the
	   file name.  All signals associated with a given file must be
	   listed together in the header in order to be identified as
	   belonging to the same group;  readheader does not check that
	   this has been done. */
	else {
	    hs->info.group = hp->info.group;
	    SSTRCPY(hs->info.fname, hp->info.fname);
	}

	/* Determine the signal format. */
	if ((p = strtok((char *)NULL, sep)) == NULL ||
	    !isfmt(hs->info.fmt = strtol(p, NULL, 10))) {
	    wfdb_error("init: illegal format for signal %d, record %s\n",
		       s, record);
	    return (-2);
	}
	hs->info.spf = 1;
	hs->skew = 0;
	hs->start = 0L;
	while (*(++p)) {
	    if (*p == 'x' && *(++p))
		if ((hs->info.spf = strtol(p, NULL, 10)) < 1) hs->info.spf = 1;
	    if (*p == ':' && *(++p))
		if ((hs->skew = strtol(p, NULL, 10)) < 0) hs->skew = 0;
	    if (*p == '+' && *(++p))
		if ((hs->start = strtol(p, NULL, 10)) < 0L) hs->start = 0L;
	}
	if (hs->info.spf > spfmax) spfmax = hs->info.spf;
	/* The resolution for deskewing is one frame.  The skew in samples
	   (given in the header) is converted to skew in frames here. */
	hs->skew = (int)(((double)hs->skew)/hs->info.spf + 0.5);

	/* Determine the gain in ADC units per physical unit.  This number
	   may be zero or missing;  if so, the signal is uncalibrated. */
	if (p = strtok((char *)NULL, sep))
	    hs->info.gain = (WFDB_Gain)strtod(p, NULL);
	else
	    hs->info.gain = (WFDB_Gain)0.;

	/* Determine the baseline if specified, and the physical units
	   (assumed to be millivolts unless otherwise specified). */
	nobaseline = 1;
	if (p) {
	    for ( ; *p && *p != '(' && *p != '/'; p++)
		;
	    if (*p == '(') {
		hs->info.baseline = strtol(++p, NULL, 10);
		nobaseline = 0;
	    }
	    while (*p)
		if (*p++ == '/' && *p)
		    break;
	}
	if (p && *p) {
	    SALLOC(hs->info.units, WFDB_MAXUSL+1, 1);
	    (void)strncpy(hs->info.units, p, WFDB_MAXUSL);
	}
	else
	    hs->info.units = NULL;

	/* Determine the ADC resolution in bits.  If this number is
	   missing and cannot be inferred from the format, the default
	   value (from wfdb.h) is filled in. */
	if (p = strtok((char *)NULL, sep))
	    i = (unsigned)strtol(p, NULL, 10);
	else switch (hs->info.fmt) {
	  case 80: i = 8; break;
	  case 160: i = 16; break;
	  case 212: i = 12; break;
	  case 310: i = 10; break;
	  case 311: i = 10; break;
	  default: i = WFDB_DEFRES; break;
	}
	hs->info.adcres = i;

	/* Determine the ADC zero (assumed to be zero if missing). */
	hs->info.adczero = (p=strtok((char *)NULL,sep)) ? strtol(p,NULL,10) : 0;

	/* Set the baseline to adczero if no baseline field was found. */
	if (nobaseline) hs->info.baseline = hs->info.adczero;

	/* Determine the initial value (assumed to be equal to the ADC
	   zero if missing). */
	hs->info.initval = (p = strtok((char *)NULL, sep)) ?
	    strtol(p, NULL, 10) : hs->info.adczero;

	/* Determine the checksum (assumed to be zero if missing). */
	if (p = strtok((char *)NULL, sep)) {
	    hs->info.cksum = strtol(p, NULL, 10);
	    hs->info.nsamp = (ns > LONG_MAX ? 0 : ns);
	}
	else {
	    hs->info.cksum = 0;
	    hs->info.nsamp = (WFDB_Time)0L;
	}

	/* Determine the block size (assumed to be zero if missing). */
	hs->info.bsize = (p = strtok((char *)NULL,sep)) ? strtol(p,NULL,10) : 0;

	/* Check that formats and block sizes match for signals belonging
	   to the same group. */
	if (s && (hp == NULL || (hs->info.group == hp->info.group &&
	    (hs->info.fmt != hp->info.fmt ||
	     hs->info.bsize != hp->info.bsize)))) {
	    wfdb_error("init: error in specification of signal %d or %d\n",
		       s-1, s);
	    return (-2);
	}

	/* Get the signal description.  If missing, a description of
	   the form "record xx, signal n" is filled in. */
	SALLOC(hs->info.desc, 1, WFDB_MAXDSL+1);
	if (p = strtok((char *)NULL, "\n\r"))
	    (void)strncpy(hs->info.desc, p, WFDB_MAXDSL);
	else
	    (void)sprintf(hs->info.desc,
			  "record %s, signal %d", record, s);
    }
    setgvmode(gvmode);		/* Reset sfreq if appropriate. */
    return (s);			/* return number of available signals */
}

void hsdfree(void)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    struct hsdata *hs;

    if (hsd) {
	while (maxhsig)
	    if (hs = hsd[--maxhsig]) {
		SFREE(hs->info.fname);
		SFREE(hs->info.units);
		SFREE(hs->info.desc);
		SFREE(hs);
	    }
	SFREE(hsd);
    }
    maxhsig = 0;
}

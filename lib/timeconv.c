/* file: timeconv.c	G. Moody	13 April 1989
			Last revised:    18 May 2022		wfdblib 10.7.0
WFDB library functions for time/frequency/unit conversion

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

This file contains definitions of the following WFDB library functions:
 sampfreq	(returns the sampling frequency of the specified record)
 setsampfreq	(sets the putvec sampling frequency)
 setbasetime	(sets the base time and date)
 ftimstr	(converts sample intervals to time strings)
 timstr		(converts sample intervals to time strings)
 fmstimstr	(converts sample intervals to time strings with milliseconds)
 mstimstr	(converts sample intervals to time strings with milliseconds)
 getcfreq	(gets the counter frequency)
 setcfreq	(sets the counter frequency)
 getbasecount	(gets the base counter value)
 setbasecount	(sets the base counter value)
 fstrtim	(converts time strings to sample intervals)
 strtim		(converts time strings to sample intervals)
 datstr		(converts Julian dates to date strings)
 strdat		(converts date strings to Julian dates)
 adumuv		(converts ADC units to microvolts)
 muvadu		(converts microvolts to ADC units)
 aduphys	(converts ADC units to physical units)
 physadu	(converts physical units to ADC units)

These functions were previously part of signal.c.
*/

#include "signal_internal.h"

#include <time.h>

WFDB_Frequency sampfreq_ctx(WFDB_Context *ctx, char *record)
{
    int n;

    /* Remove trailing .hea, if any, from record name. */
    wfdb_striphea(record);

    if (record != NULL) {
	/* Save the current record name. */
	wfdb_setirec(record);
	/* Don't require the sampling frequency of this record to match that
	   of the previously opened record, if any.  (readheader will
	   complain if the previously defined sampling frequency was > 0.) */
	setsampfreq(0.);
	/* readheader sets sfreq if successful. */
	if ((n = readheader(record)) < 0)
	    /* error message will come from readheader */
	    return ((WFDB_Frequency)n);
    }
    return (sfreq);
}

WFDB_Frequency sampfreq(char *record)
{
    return sampfreq_ctx(wfdb_get_default_context(), record);
}

int setsampfreq_ctx(WFDB_Context *ctx, WFDB_Frequency freq)
{
    if (freq >= 0.) {
	sfreq = ffreq = freq;
	if (spfmax == 0) spfmax = 1;
	if ((gvmode & WFDB_HIGHRES) == WFDB_HIGHRES) sfreq *= spfmax;
	return (0);
    }
    wfdb_error("setsampfreq: sampling frequency must not be negative\n");
    return (-1);
}

int setsampfreq(WFDB_Frequency freq)
{
    return setsampfreq_ctx(wfdb_get_default_context(), freq);
}

int setbasetime_ctx(WFDB_Context *ctx, char *string)
{
    char *p;

    pdays = -1;
    if (string == NULL || *string == '\0') {
	struct tm *now;
	time_t t;

	t = time((time_t *)NULL);    /* get current time from system clock */
	now = localtime(&t);
	(void)snprintf(date_string, sizeof(date_string), "%02d/%02d/%d",
		now->tm_mday, now->tm_mon+1, now->tm_year+1900);
	bdate = strdat(date_string);
	(void)snprintf(time_string, sizeof(time_string), "%d:%d:%d",
		now->tm_hour, now->tm_min, now->tm_sec);
	btime = fstrtim(time_string, 1000.0);
	return (0);
    }
    while (*string == ' ') string++;
    if (p = strchr(string, ' '))
        *p++ = '\0';	/* split time and date components */
    btime = fstrtim(string, 1000.0);
    bdate = p ? strdat(p) : (WFDB_Date)0;
    if (btime == 0L && bdate == (WFDB_Date)0 && *string != '[') {
	if (p) *(--p) = ' ';
	wfdb_error("setbasetime: incorrect time format, '%s'\n", string);
	return (-1);
    }
    return (0);
}

int setbasetime(char *string)
{
    return setbasetime_ctx(wfdb_get_default_context(), string);
}

/* Convert sample number to string, using the given sampling
   frequency */
char *ftimstr(WFDB_Time t, WFDB_Frequency f)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    char *p;

    p = strtok(fmstimstr(t, f), ".");		 /* discard msec field */
    if (t <= 0L && (btime != 0L || bdate != (WFDB_Date)0)) { /* time of day */
	(void)strcat(p, date_string);		  /* append dd/mm/yyyy */
	(void)strcat(p, "]");
    }
    return (p);
}

char *timstr_ctx(WFDB_Context *ctx, WFDB_Time t)
{
    double f;

    if (ifreq > 0.) f = ifreq;
    else if (sfreq > 0.) f = sfreq;
    else f = 1.0;

    return ftimstr(t, f);
}

char *timstr(WFDB_Time t)
{
    return timstr_ctx(wfdb_get_default_context(), t);
}

/* Convert sample number to string, using the given sampling
   frequency */
char *fmstimstr(WFDB_Time t, WFDB_Frequency f)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    int hours, minutes, seconds, msec;
    WFDB_Date days;
    double tms;
    WFDB_Time s;

    if (t > 0L || (btime == 0L && bdate == (WFDB_Date)0)) { /* time interval */
	if (t < 0L) t = -t;
	/* Convert from sample intervals to seconds. */
	s = (WFDB_Time)(t / f);
	msec = (int)((t - s*f)*1000/f + 0.5);
	if (msec == 1000) { msec = 0; s++; }
	t = s;
	seconds = t % 60;
	t /= 60;
	minutes = t % 60;
	hours = t / 60;
	if (hours > 0)
	    (void)snprintf(time_string, sizeof(time_string), "%2d:%02d:%02d.%03d",
			  hours, minutes, seconds, msec);
	else
	    (void)snprintf(time_string, sizeof(time_string), "   %2d:%02d.%03d",
			  minutes, seconds, msec);
    }
    else {			/* time of day */
	/* Convert to milliseconds since midnight. */
	tms = btime - (t * 1000.0 / f);
	/* Convert to seconds. */
	s = (WFDB_Time)(tms / 1000.0);
	msec = (int)((tms - s*1000.0) + 0.5);
	if (msec == 1000) { msec = 0; s++; }
	t = s;
	seconds = t % 60;
	t /= 60;
	minutes = t % 60;
	t /= 60;
	hours = t % 24;
	days = t / 24;
	if (days != pdays) {
	    if (bdate > 0)
		(void)datstr(days + bdate);
	    else if (days == 0)
		date_string[0] = '\0';
	    else
		(void)snprintf(date_string, sizeof(date_string), " %ld", days);
	    pdays = days;
	}
	(void)snprintf(time_string, sizeof(time_string), "[%02d:%02d:%02d.%03d%s]",
		      hours, minutes, seconds, msec, date_string);
    }
    return (time_string);
}

char *mstimstr_ctx(WFDB_Context *ctx, WFDB_Time t)
{
    double f;

    if (ifreq > 0.) f = ifreq;
    else if (sfreq > 0.) f = sfreq;
    else f = 1.0;

    return fmstimstr(t, f);
}

char *mstimstr(WFDB_Time t)
{
    return mstimstr_ctx(wfdb_get_default_context(), t);
}

WFDB_Frequency getcfreq_ctx(WFDB_Context *ctx)
{
    return (cfreq > 0. ? cfreq : ffreq);
}

WFDB_Frequency getcfreq(void)
{
    return getcfreq_ctx(wfdb_get_default_context());
}

void setcfreq_ctx(WFDB_Context *ctx, WFDB_Frequency freq)
{
    cfreq = freq;
}

void setcfreq(WFDB_Frequency freq)
{
    setcfreq_ctx(wfdb_get_default_context(), freq);
}

double getbasecount_ctx(WFDB_Context *ctx)
{
    return (bcount);
}

double getbasecount(void)
{
    return getbasecount_ctx(wfdb_get_default_context());
}

void setbasecount_ctx(WFDB_Context *ctx, double counter)
{
    bcount = counter;
}

void setbasecount(double counter)
{
    setbasecount_ctx(wfdb_get_default_context(), counter);
}

/* Convert string to sample number, using the given sampling
   frequency */
WFDB_Time fstrtim(const char *string, WFDB_Frequency f)
{
    WFDB_Context *ctx = wfdb_get_default_context();
    const char *p, *q, *r;
    double x, y, z;
    WFDB_Date days;
    WFDB_Time t;

    while (*string==' ' || *string=='\t' || *string=='\n' || *string=='\r')
	string++;
    switch (*string) {
      case 'c': return (cfreq > 0. ?
			(WFDB_Time)((strtod(string+1, NULL)-bcount)*f/cfreq) :
			strtoll(string+1, NULL, 10));
      case 'e':	return ((in_msrec ? msnsamples : nsamples) *
		        (((gvmode&WFDB_HIGHRES) == WFDB_HIGHRES) ? spfmax: 1));
      case 'f': return (WFDB_Time)(strtoll(string+1, NULL, 10)*f/ffreq);
      case 'i':	return (WFDB_Time)(istime *
			(ifreq > 0.0 ? (ifreq/sfreq) : 1.0) *
			(((gvmode&WFDB_HIGHRES) == WFDB_HIGHRES) ? ispfmax: 1));
      case 'o':	return (ostime);
      case 's':	return (strtoll(string+1, NULL, 10));
      case '[':	  /* time of day, possibly with date or days since start */
	if ((q = strchr(++string, ']')) == NULL)
	    return ((WFDB_Time)0);	/* '[...': malformed time string */
	if ((p = strchr(string, ' ')) == NULL || p > q)
	    days = (WFDB_Date)0;/* '[hh:mm:ss.sss]': time since midnight only */
	else if ((r = strchr(p+1, '/')) == NULL || r > q)
	    days = (WFDB_Date)strtol(p+1, NULL, 10); /* '[hh:mm:ss.sss d]' */
	else
	    days = strdat(p+1) - bdate; /* '[hh:mm:ss.sss dd/mm/yyyy]' */
        x = fstrtim(string, 1000.0) - btime;
        if (days > 0L) x += (days*(24*60*60*1000.0));
        t = (WFDB_Time)(x * f / 1000.0 + 0.5);
	return (-t);
      default:
	x = strtod(string, NULL);
	if ((p = strchr(string, ':')) == NULL) return ((WFDB_Time)(x*f + 0.5));
	y = strtod(++p, NULL);
	if ((p = strchr(p, ':')) == NULL) return ((WFDB_Time)((60.*x + y)*f + 0.5));
	z = strtod(++p, NULL);
	return ((WFDB_Time)((3600.*x + 60.*y + z)*f + 0.5));
    }
}

WFDB_Time strtim_ctx(WFDB_Context *ctx, const char *string)
{
    double f;

    if (ifreq > 0.) f = ifreq;
    else if (sfreq > 0.) f = sfreq;
    else f = 1.0;

    return fstrtim(string, f);
}

WFDB_Time strtim(const char *string)
{
    return strtim_ctx(wfdb_get_default_context(), string);
}

/* The functions datstr and strdat convert between Julian dates (used
   internally) and dd/mm/yyyy format dates.  (Yes, this is overkill for this
   application.  For the astronomically-minded, Julian dates are supposed
   to begin at noon GMT, but these begin at midnight local time.)  They are
   based on similar functions in chapter 1 of "Numerical Recipes", by Press,
   Flannery, Teukolsky, and Vetterling (Cambridge U. Press, 1986). */

char *datstr_ctx(WFDB_Context *ctx, WFDB_Date date)
{
    int d, m, y, gcorr, jm, jy;
    WFDB_Date jd;

    if (date >= 2299161L) {	/* Gregorian calendar correction */
	gcorr = (int)(((date - 1867216L) - 0.25)/36524.25);
	date += 1 + gcorr - (long)(0.25*gcorr);
    }
    date += 1524;
    jy = (int)(6680 + ((date - 2439870L) - 122.1)/365.25);
    jd = (WFDB_Date)(365L*jy + (0.25*jy));
    jm = (int)((date - jd)/30.6001);
    d = date - jd - (int)(30.6001*jm);
    if ((m = jm - 1) > 12) m -= 12;
    y = jy - 4715;
    if (m > 2) y--;
    if (y <= 0) y--;
    (void)snprintf(date_string, sizeof(date_string), " %02d/%02d/%d", d, m, y);
    pdays = -1;
    return (date_string);
}

char *datstr(WFDB_Date date)
{
    return datstr_ctx(wfdb_get_default_context(), date);
}

WFDB_Date strdat(const char *string)
{
    const char *mp, *yp;
    int d, m, y, gcorr, jm, jy;
    WFDB_Date date;

    if ((mp = strchr(string,'/')) == NULL || (yp = strchr(mp+1,'/')) == NULL ||
	(d = strtol(string, NULL, 10)) < 1 || d > 31 ||
	(m = strtol(mp+1, NULL, 10)) < 1 || m > 12 ||
	(y = strtol(yp+1, NULL, 10)) == 0)
	return (0L);
    if (m > 2) { jy = y; jm = m + 1; }
    else { jy = y - 1; 	jm = m + 13; }
    if (jy > 0) date = (WFDB_Date)(365.25*jy);
    else date = -(long)(-365.25 * (jy + 0.25));
    date += (int)(30.6001*jm) + d + 1720995L;
    if (d + 31L*(m + 12L*y) >= (15 + 31L*(10 + 12L*1582))) { /* 15/10/1582 */
	gcorr = (int)(0.01*jy);		/* Gregorian calendar correction */
	date += 2 - gcorr + (int)(0.25*gcorr);
    }
    return (date);
}

WFDB_Date strdat_ctx(WFDB_Context *ctx, const char *string)
{
    return strdat(string);
}

int adumuv_ctx(WFDB_Context *ctx, WFDB_Signal s, WFDB_Sample a)
{
    double x;
    WFDB_Gain g = (s < nvsig) ? vsd[s]->info.gain : WFDB_DEFGAIN;

    if (g == 0.) g = WFDB_DEFGAIN;
    x = a*1000./g;
    if (x >= 0.0)
	return ((int)(x + 0.5));
    else
	return ((int)(x - 0.5));
}

int adumuv(WFDB_Signal s, WFDB_Sample a)
{
    return adumuv_ctx(wfdb_get_default_context(), s, a);
}

WFDB_Sample muvadu_ctx(WFDB_Context *ctx, WFDB_Signal s, int v)
{
    double x;
    WFDB_Gain g = (s < nvsig) ? vsd[s]->info.gain : WFDB_DEFGAIN;

    if (g == 0.) g = WFDB_DEFGAIN;
    x = g*v*0.001;
    if (x >= 0.0)
	return ((int)(x + 0.5));
    else
	return ((int)(x - 0.5));
}

WFDB_Sample muvadu(WFDB_Signal s, int v)
{
    return muvadu_ctx(wfdb_get_default_context(), s, v);
}

double aduphys_ctx(WFDB_Context *ctx, WFDB_Signal s, WFDB_Sample a)
{
    double b;
    WFDB_Gain g;

    if (s < nvsig) {
	b = vsd[s]->info.baseline;
	g = vsd[s]->info.gain;
	if (g == 0.) g = WFDB_DEFGAIN;
    }
    else {
	b = 0;
	g = WFDB_DEFGAIN;
    }
    return ((a - b)/g);
}

double aduphys(WFDB_Signal s, WFDB_Sample a)
{
    return aduphys_ctx(wfdb_get_default_context(), s, a);
}

WFDB_Sample physadu_ctx(WFDB_Context *ctx, WFDB_Signal s, double v)
{
    int b;
    WFDB_Gain g;

    if (s < nvsig) {
	b = vsd[s]->info.baseline;
	g = vsd[s]->info.gain;
	if (g == 0.) g = WFDB_DEFGAIN;
    }
    else {
	b = 0;
	g = WFDB_DEFGAIN;
    }
    v *= g;
    if (v >= 0)
	return ((int)(v + 0.5) + b);
    else
	return ((int)(v - 0.5) + b);
}

WFDB_Sample physadu(WFDB_Signal s, double v)
{
    return physadu_ctx(wfdb_get_default_context(), s, v);
}

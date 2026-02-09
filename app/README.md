# WFDB Applications

**File:** README
**Author:** G. Moody
**Date:** 1 June 1989
**Last revised:** 8 June 2005

---

## License

WFDB applications: programs for working with annotated signals
Copyright (C) 1989-2024 George B. Moody and contributors

These programs are free software; you can redistribute them and/or modify
them under the terms of the GNU General Public License (GPL) as published by
the Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

These programs are distributed in the hope that they will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License along with
these programs; if not, see <https://www.gnu.org/licenses/>.

For updates to this software, please visit PhysioNet at <https://physionet.org/>.

---

## Overview

This directory contains sources for the standard applications which come
with the WFDB software package. Before attempting to compile these
programs, build and install the WFDB library (look in the `lib` directory
on the same level as this one).

The WFDB software package uses the Meson build system. See the top-level
README.md or CLAUDE.md for build instructions.

You may wish to set the WFDB environment variable (the database path) before
using these applications. Select and customize the appropriate script for
doing so from among the templates listed below.

For information about using these applications, look in the `doc` directory on
the same level as this one. You can also obtain a brief usage summary for each
application by running it without any command-line arguments.

Before using any of the applications, you will need to have database files
for input. The contents of the `data` directory, on the same level as this
one, will get you started if no others are available.

## Files in This Directory

The following files will be found in this directory:

| File | Description |
|------|-------------|
| `12lead.pro` | Alternate prolog for printing 12-lead ECGs using `pschart` |
| `README.md` | this file |
| `ann2rr.c` | Converts an annotation file to an RR interval series |
| `bxb.c` | AAMI-standard beat-by-beat annotation comparator |
| `calsig.c` | Calibrates signals of a database record |
| `cshsetwfdb` | Template for C-shell WFDB path initialization script |
| `ecgeval.c` | Generates and runs ECG analyzer evaluation script |
| `epicmp.c` | ANSI/AAMI-standard episode-by-episode annotation comparator |
| `fir.c` | General-purpose FIR filter |
| `gqfuse.c` | Combine QRS annotation files |
| `gqpost.c` | Post-processor for GQRS detector |
| `gqrs.c` | QRS detector and post-processor |
| `gqrs.conf` | Configuration file for `gqrs` and `gqpost` |
| `hrstats.c` | Collect and summarize heart rate statistics from an annotation file |
| `ihr.c` | Generates a non-uniformly sampled heart rate signal from an annotation file |
| `map-record` | Shell script to make a map of a WFDB record |
| `meson.build` | Meson build description file |
| `mfilt.c` | General-purpose median filter |
| `mrgann.c` | Merges a pair of annotation files, generating a third one |
| `mxm.c` | AAMI-standard measurement-by-measurement annotation comparator |
| `nguess.c` | Guess the times of missing normal beats in an annotation file |
| `nst.c` | Mixes noise with ECGs for noise stress tests |
| `plotstm.c` | Produces a PostScript scatter plot of ST measurement errors |
| `pnwlogin` | Shell script to log in to PhysioNetWorks |
| `pscgen.c` | Generate a `pschart` script from an annotation file |
| `pschart.c` | Makes annotated 'chart recordings' on PostScript devices |
| `pschart.pro` | PostScript prolog file for use with `pschart` |
| `psfd.c` | Makes annotated 'full-disclosure' plots on PostScript devices |
| `psfd.pro` | PostScript prolog file for use with `psfd` |
| `rdann.c` | Reads annotations and prints them |
| `rdsamp.c` | Reads signals and prints them |
| `rr2ann.c` | Converts an RR interval series into an annotation file |
| `rxr.c` | AAMI-standard run-by-run annotation comparator |
| `sampfreq.c` | Prints the sampling frequency of a record |
| `setwfdb` | Template for Bourne/bash shell WFDB path initialization script |
| `sigamp.c` | Measures signal amplitudes |
| `sigavg.c` | Calculate averages of annotated waveforms |
| `signal-colors.h` | Signal color definitions for visualization |
| `signame.c` | Print names of signals of a WFDB record |
| `signum.c` | Print signal numbers of a WFDB record having specified names |
| `skewedit.c` | Rewrites header files to correct for measured inter-signal skew |
| `snip.c` | Copies an excerpt of a database record |
| `sortann.c` | Rearranges annotations in canonical order |
| `sqrs.c` | Single-channel QRS detector (optimized for 250 samples/sec) |
| `sqrs125.c` | Variant of sqrs optimized for signals sampled at 125 Hz |
| `stepdet.c` | Single-channel step change detector |
| `sumann.c` | Summarizes the contents of an annotation file |
| `sumstats.c` | Derives aggregate statistics from `bxb`, `rxr`, etc. output |
| `tach.c` | Generates a uniformly sampled heart rate signal from an annotation file |
| `time2sec.c` | Convert WFDB standard time format into seconds |
| `wabp.c` | Arterial blood pressure (ABP) pulse detector |
| `wfdb-config.c` | Print WFDB library version and configuration info |
| `wfdbcat.c` | Copies a WFDB file to standard output |
| `wfdbcollate.c` | Collates multiple WFDB records into a multi-segment record |
| `wfdbdesc.c` | Describes signals based on header file contents |
| `wfdbmap.c` | Make a synoptic map of a WFDB record |
| `wfdbsignals.c` | List signal specifications of a WFDB record |
| `wfdbtime.c` | Convert time to sample number, elapsed, and absolute time |
| `wfdbwhich.c` | Finds a WFDB file and prints its pathname |
| `wqrs.c` | Single-channel QRS detector based on length transform |
| `wrann.c` | Creates an annotation file from `rdann` output |
| `wrsamp.c` | Creates signal and header files by converting text input |
| `xform.c` | Sampling frequency, amplitude, and format converter |

## What Next?

### Power Spectral Density Estimation

If you are interested in power spectral density estimation (e.g., for studying
heart rate variability), you may wish to compile and install the applications
in the `psd` directory at the same level as this one. The `psd` applications
can process the text output produced by several of the applications in this
directory, including `rdsamp`, `tach`, and `ihr`.

### Format Conversion

If you have database records in a format that is not directly supported by the
WFDB library, or if you need to produce records in such formats, you may wish
to look for format-conversion programs in the `convert` directory on the same
level as this one.

### Example Programs

Finally, you may also wish to compile the example programs from chapter 6 of
the WFDB Programmer's Guide, which can be found in the `examples` directory on
the same level as this one.

See the `README` files in the `psd`, `convert`, and `examples` directories for
further information.

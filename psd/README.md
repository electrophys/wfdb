# Power Spectral Density Estimation Tools

**File:** README
**Author:** G. Moody
**Date:** 14 June 1995
**Last revised:** 9 February 2026

---

## License

Power spectral density estimation and related applications
Copyright (C) 1995-2024 George B. Moody and contributors

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

This directory contains sources for applications for power spectral density
(PSD) estimation, and sample Unix shell scripts for using these applications to
obtain heart rate power spectra based upon beat annotation files. The scripts
require `ihr` and `tach` from the `../app` directory, and also require either
`plt` or `gnuplot` for output. (See <http://www.physionet.org/physiotools/plt/>
or <http://www.gnuplot.info/>.) The programs to be compiled within this
directory are independent of the remainder of the WFDB Software Package, and
require only the standard C math library. The directory `../doc/wag-src`
contains documentation for these programs and scripts in the form of Unix man
pages.

These programs were written for SunOS, Solaris, and GNU/Linux; they are also
portable to and have been tested with other versions of Unix and macOS. On
MS-Windows, use WSL (Windows Subsystem for Linux).

The WFDB software package uses the Meson build system. See the top-level
README.md or CLAUDE.md for build instructions.

## Files in This Directory

| File | Description |
|------|-------------|
| `README.md` | this file |
| `meson.build` | Meson build description file |
| `coherence.c` | Calculate coherence and cross-spectrum of two time series |
| `fft.c` | Calculate fast Fourier transform (FFT), inverse FFT, etc. |
| `hrfft` | Plot heart rate power spectrum (using tach and fft) |
| `hrlomb` | Plot heart rate power spectrum (using ihr and lomb) |
| `hrmem` | Plot heart rate power spectrum (using tach and memse) |
| `hrplot` | Plot heart rate time series (using ihr) |
| `log10.c` | Utility for calculating common logarithms of two data columns |
| `lomb.c` | Calculate Lomb periodogram from an irregularly-sampled series |
| `memse.c` | Calculate PSD estimate using maximum entropy (all poles) method (also known as autoregressive, or AR, PSD estimation) |
| `plot2d` | A script that drives `gnuplot`, using a few `plt` options |
| `plot3d` | Another `gnuplot` driver, for 3-D plots |

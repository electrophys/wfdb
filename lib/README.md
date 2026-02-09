# WFDB Library

**File:** README
**Author:** G. Moody
**Date:** 1 July 1989
**Last revised:** 6 February 2026

---

## License

wfdb: a library for reading and writing annotated waveforms (time series data)
Copyright (C) 1989-2024 George B. Moody and contributors

This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License (LGPL) as published by the
Free Software Foundation; either version 2.1 of the License, or (at your option)
any later version.

This library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this library; if not, see <https://www.gnu.org/licenses/>.

For updates to this software, please visit PhysioNet at <https://physionet.org/>.

---

## Overview

This directory contains sources for the WFDB library. The library is built
using Meson; see the top-level README for build instructions.

## Files in This Directory

| File | Description |
|------|-------------|
| `COPYING.LIB` | the GNU Lesser General Public License (LGPL) |
| `README.md` | this file |
| `meson.build` | Meson build description file |
| `annot.c` | annotation reading/writing |
| `calib.c` | signal calibration |
| `ecgcodes.h` | ECG annotation codes |
| `ecgmap.h` | ECG annotation code mapping macros |
| `flac.c` | FLAC signal encoder/decoder |
| `header.c` | header file parsing |
| `signal.c` | signal I/O (public API, core read/write) |
| `signal_internal.h` | shared internal structs, macros, and function declarations |
| `sigformat.c` | format-specific signal read/write/flush |
| `sigmap.c` | variable-layout multi-segment signal mapping |
| `timeconv.c` | time/date/unit conversion |
| `wfdb.h` | public header (types, constants, function prototypes) |
| `wfdb_context.c` | WFDB_Context lifecycle (new, free, get_default) |
| `wfdb_context.h` | WFDB_Context struct definition |
| `wfdbinit.c` | library initialization (wfdbinit, wfdbquit, wfdbflush) |
| `wfdbio.c` | low-level I/O abstraction (local files, HTTP/FTP via libcurl) |
| `wfdblib.h` | internal library definitions and private function prototypes |

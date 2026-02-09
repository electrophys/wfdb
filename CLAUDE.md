# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

WFDB (WaveForm Database) is a C library and application suite for reading, writing, and processing annotated time-series signals, primarily ECG and biomedical waveform data. Maintained by PhysioNet. Licensed under LGPL (library) and GPL (applications).

## Build Commands

```bash
# Prerequisites (Debian/Ubuntu)
sudo apt-get install gcc meson ninja-build libflac-dev libcurl4-gnutls-dev

# Prerequisites (Fedora/RHEL)
sudo dnf install gcc meson ninja-build flac-devel libcurl-devel

# Configure and build
meson setup build
ninja -C build

# Install (to /usr/local by default)
ninja -C build install

# Configure for local testing (no root needed)
meson setup build --prefix=$HOME/wfdb-test
ninja -C build install

# Run the full test suite (after install)
meson test -C build

# Clean build artifacts
ninja -C build clean

# Reconfigure (e.g. change prefix)
meson setup build --reconfigure --prefix=/new/path
```

### Meson Options

- `--prefix=DIR` - installation directory (default: `/usr/local`)
- `-Dnetfiles=enabled/disabled/auto` - network file access via libcurl
- `-Dflac=enabled/disabled/auto` - FLAC compressed signal support
- `-Ddbdir=PATH` - database directory (default: `<datadir>/wfdb`)

## Test Suite

Tests live in `checkpkg/`. `meson test -C build` runs both:

- **libcheck** / **lcheck.c** - Library unit tests. Validates signal I/O, annotation I/O, calibration, sampling frequency, format conversion. Run with `-v` for verbose, `-V` to list untested functions.
- **appcheck** - Shell script testing ~25 applications. Generates output and diffs against `checkpkg/expected/`. Uses test record `100s` from `data/`.

Set `WFDB_NO_NET_CHECK=1` to skip network-dependent tests.

## Architecture

### Library (`lib/`)

The core WFDB library.

- **wfdb.h** - Public API header (types, constants, function prototypes, `_ctx` variants)
- **wfdb_config.h** - Generated build configuration (version, NETFILES, DBDIR)
- **wfdblib.h** - Internal library definitions (DEFWFDB, private function prototypes)
- **signal.c** - Signal I/O: reading/writing in all formats (8/16/24/32-bit, 212, 310/311 packed, FLAC 508/516/524), multi-frequency/multi-segment records
- **header.c** - Header parsing (readheader, edfparse)
- **sigformat.c** - Format-specific read/write/flush functions
- **flac.c** - FLAC encoder/decoder
- **sigmap.c** - Variable-layout multi-segment signal mapping
- **timeconv.c** - Time/date/unit conversion functions
- **annot.c** - Annotation reading/writing and AHA format conversion
- **wfdbio.c** - Low-level I/O abstraction: local files, HTTP/FTP via libcurl (NETFILES), path resolution, buffering
- **calib.c** - Calibration database
- **wfdbinit.c** - Library initialization (wfdbinit, wfdbquit, wfdbflush)
- **wfdb_context.h/c** - WFDB_Context struct and lifecycle (thread-safe state management)
- **signal_internal.h** - Shared internal structs, macros, function declarations

Key types (defined in `wfdb.h`): `WFDB_Sample`, `WFDB_Time`, `WFDB_Frequency`, `WFDB_Siginfo`, `WFDB_Annotation`, `WFDB_Seginfo`, `WFDB_Context`. Memory management uses `SALLOC`/`SREALLOC`/`SFREE` macros.

### Applications (`app/`)

~40 command-line tools built on the library:
- **I/O**: `rdsamp`, `rdann`, `wrsamp`, `wrann` - read/write samples and annotations
- **Detectors**: `sqrs`, `gqrs`, `wqrs` - QRS detection algorithms
- **Analysis**: `bxb` (beat-by-beat comparison), `rxr` (run comparison), `epic`, `mxm`
- **Processing**: `xform` (format/frequency converter), `fir`, `mfilt`, `sigamp`
- **Visualization**: `pschart`, `psfd` - PostScript output

### Format Converters (`convert/`)

Bidirectional converters: `edf2mit`/`mit2edf`, `wav2mit`/`mit2wav`, `wfdb2mat`, `ahaconvert`, etc.

### Other Directories

- **psd/** - Power spectral density tools (`fft`, `lomb`, `memse`)
- **data/** - Test records (notably `100s.*`) and calibration file `wfdbcal`
- **fortran/** - Fortran wrapper (`wfdbf.c`)
- **doc/** - Man pages and documentation sources (Applications Guide, Programmer's Guide, WAVE User Guide)

## Build System

Meson-based. The root `meson.build` includes subdirectories: `lib` → `app` → `convert` → `psd` → `data` → `doc` → `checkpkg`.

Build configuration values (version, NETFILES, DBDIR) are passed as `-D` compiler flags to the library build. For installed headers, `wfdb_config.h` is generated at configure time with `#ifndef` guards so the `-D` flags take precedence during library compilation.

## Code Conventions

- C11 (gnu11) with POSIX extensions
- Use `WFDB_Pd_TIME`, `WFDB_Pd_SAMP`, etc. format macros for portable printf/scanf of WFDB types
- `WFDB_Time` is unconditionally `long long`
- WFDB library functions have `_ctx` variants that take an explicit `WFDB_Context*` for thread-safe usage; the original functions are thin wrappers using the default global context
- `wfdbquit()` resets all state

### File Header Maintenance

Many source files (.c, .h) contain a "Last revised" date in their header comments. **When modifying a file:**

- **DO** update the "Last revised" date to the current date if you make substantive changes to the file
- **DO** update the version number (e.g., `wfdblib 11.0.0`) in library files when bumping the version
- **DO NOT** update dates on files you haven't actually modified - historical dates should be preserved for files that genuinely haven't been touched

README.md files follow a similar convention with "**Last revised:**" in their frontmatter. Update these when making changes to the documentation.

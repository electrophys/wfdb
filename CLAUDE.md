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

## Version Bump and Release Process

### Version Bump Checklist

When bumping the version number (e.g., from 10.7.0 to 11.0.0), update the following files:

**Core version files:**
- `meson.build` - `version:` field in `project()` call

**Release notes and documentation:**
- `NEWS.md` - Add new release section at the top with date and changelog
- `README.md` - Update version in example commands (e.g., RPM tarball name on line ~170)
- All `**/README.md` files - Update "**Last revised:**" date to current date

**Library source files** (update version and date in file headers):
- `lib/wfdb.h` - Header comment: `Last revised:` date and `wfdblib X.Y.Z`
- `lib/wfdbio.c` - Header comment
- `lib/wfdblib.h` - Header comment
- `lib/wfdbinit.c` - Header comment
- `lib/annot.c` - Header comment
- `lib/signal.c` - Header comment
- `lib/timeconv.c` - Header comment
- `lib/calib.c` - Header comment

**Application files:**
- `app/ecgeval.c` - Header comment: `wfdb X.Y.Z`

**Package metadata files:**
- `wfdb.rb` - Homebrew formula: `version` field (line ~5) and test assertion (line ~36)
- `wfdb.spec` - RPM spec: `Version:` field and add new `%changelog` entry
- `debian/changelog` - Add new version entry at top

**Man pages:**
- `doc/wag-src/wave.1` - `.TH` line: update date and version

### Pre-Release Testing

Before creating a release:

1. **Run the test suite:**
   ```bash
   meson test -C build -v
   ```

2. **Test with network disabled:**
   ```bash
   WFDB_NO_NET_CHECK=1 meson test -C build
   ```

3. **Verify package builds:**
   - Test Debian package build: `dpkg-buildpackage -us -uc`
   - Test RPM spec syntax: `rpmlint wfdb.spec`
   - Verify Homebrew formula syntax

4. **Check documentation builds:**
   ```bash
   # Verify man pages install
   ninja -C build install
   man wfdb

   # If asciidoctor available, check HTML docs build
   ls build/doc/
   ```

## Package Metadata

WFDB supports three packaging systems:

### Homebrew (macOS and Linux)

- **File:** `wfdb.rb`
- **Location:** Installable via `brew tap electrophys/wfdb`
- **Installation:** `brew install electrophys/wfdb/wfdb`
- **Contents:** Library, command-line tools, XML converters (WAVE excluded)
- **Dependencies:** Automatically handles meson, ninja, curl, flac, expat

### RPM (Fedora/RHEL)

- **File:** `wfdb.spec`
- **Build command:** `rpmbuild -ta wfdb-X.Y.Z.tar.gz`
- **Packages produced:**
  - `wfdb` - Command-line applications and data files
  - `wfdb-libs` - Shared library
  - `wfdb-devel` - Headers, static library, pkg-config
  - `wave` - WAVE viewer (if GTK3/VTE available)

### Debian/Ubuntu

- **Directory:** `debian/`
- **Build command:** `dpkg-buildpackage -us -uc`
- **Key files:**
  - `debian/changelog` - Version history (must be updated for each release)
  - `debian/control` - Package metadata and dependencies
  - `debian/rules` - Build instructions (Meson-based)
- **Packages produced:**
  - `libwfdb10` - Shared library
  - `libwfdb-dev` - Development files
  - `wfdb-tools` - Command-line applications
  - `wave` - WAVE viewer

## Documentation Structure

The project uses three different documentation formats:

### Man Pages (troff format)

- **Location:** `doc/wag-src/*.1`, `*.3`, `*.5`
- **Format:** Traditional Unix man page format (troff/groff)
- **Sections:**
  - Section 1: Command-line applications (e.g., `rdsamp.1`, `wave.1`)
  - Section 3: Library functions (e.g., `wfdb.3`)
  - Section 5: File formats (e.g., `header.5`, `signal.5`)
- **Installation:** Automatically installed by `ninja -C build install`
- **Viewing:** `man rdsamp`, `man 3 wfdb`, etc.

### AsciiDoc (.adoc files)

- **Location:** `doc/wag-src/*.adoc`, `doc/wpg-src/wpg.adoc`, `doc/wug-src/wug.adoc`
- **Purpose:** Generate HTML versions of comprehensive guides
- **Guides:**
  - **WFDB Applications Guide** (WAG) - `doc/wag-src/` - User guide for command-line tools
  - **WFDB Programmer's Guide** (WPG) - `doc/wpg-src/wpg.adoc` - Library API documentation
  - **WAVE User's Guide** (WUG) - `doc/wug-src/wug.adoc` - Guide for the WAVE viewer
- **Build requirement:** `asciidoctor` (optional, auto-detected by Meson)
- **Output:** HTML files in `build/doc/`

### Markdown (.md files)

- **Location:** Throughout the repository (README.md files, LICENSE.md, NEWS.md, etc.)
- **Purpose:** Modern, readable documentation for GitHub and development
- **Key files:**
  - Root: `README.md`, `LICENSE.md`, `NEWS.md`, `CLAUDE.md`
  - Per-directory: `lib/README.md`, `app/README.md`, `convert/README.md`, etc.
- **Convention:** Include file metadata header with Author, Date, and "Last revised" fields

## Git Commit Conventions

### Co-Authorship Attribution

When Claude Code assists with commits, use co-authorship attribution:

```bash
git commit -m "$(cat <<'EOF'
Brief summary of changes in imperative mood

Optional detailed explanation of what changed and why,
wrapped at 72 characters.

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>
EOF
)"
```

### Commit Message Style

- **First line:** Brief summary (50-72 chars) in imperative mood ("Add feature" not "Added feature")
- **Body:** Optional detailed explanation, wrapped at 72 characters
- **Examples from this project:**
  - "Add acknowledgment of George B. Moody and clarify current maintainer"
  - "Bump version to 11.0.0 and update documentation dates"
  - "Convert documentation to Markdown format"

### Commit Organization

- Group related changes into single commits (e.g., all version bump changes together)
- Separate concerns: documentation updates separate from code changes
- Test before committing: ensure builds and tests pass

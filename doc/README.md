# WFDB Documentation

**File:** README
**Author:** G. Moody
**Date:** 7 September 1989
**Last revised:** 9 February 2026

## Overview

This directory contains documentation for the WFDB Software Package.

Man pages in troff source format are in `wag-src/`. The Meson build system
installs them automatically:

```bash
meson setup build --prefix=/usr/local
ninja -C build install
```

After installation, you can read any man page with:

```bash
man rdsamp
```

## Directory Structure

The following directories are present:

| Directory | Contents |
|-----------|----------|
| `wag-src/` | Man pages (sections 1, 3, 5) and AsciiDoc appendices for the WFDB Applications Guide |
| `wpg-src/` | Source for the WFDB Programmer's Guide |
| `wug-src/` | Source for the WAVE User's Guide |

## HTML Documentation

Optional HTML documentation can be generated if `mandoc` and/or `asciidoctor`
are installed. See `doc/meson.build` for details.

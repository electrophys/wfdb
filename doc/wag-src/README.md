# WFDB Applications Guide Source

**File:** README
**Author:** G. Moody
**Date:** 7 September 1989
**Last revised:** 7 February 2026

## Overview

This directory contains the source files for the WFDB Applications Guide.

## Files in This Directory

| File Pattern | Description |
|--------------|-------------|
| `*.1` | Man pages for WFDB applications (section 1) |
| `*.3` | Man pages for WFDB library functions (section 3) |
| `*.5` | Man pages for WFDB file format specifications (section 5) |
| `intro.adoc` | Introduction to the guide (AsciiDoc) |
| `faq.adoc` | Frequently asked questions (AsciiDoc) |
| `install.adoc` | Installation instructions (AsciiDoc) |
| `eval.adoc` | Evaluating ECG analyzers (AsciiDoc) |
| `index.adoc` | Table of contents / index page (AsciiDoc) |

## Building

Man pages are installed by the Meson build system. If `asciidoctor` and/or
`mandoc` are available, HTML versions of the guide can also be generated.

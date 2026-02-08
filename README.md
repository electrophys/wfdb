# WFDB Software Package

Software for working with annotated physiologic signals.

The WFDB (WaveForm Database) library supports creating, reading, and
annotating digitized signals in a wide variety of formats.  Input can be
from local files or directly from web servers via libcurl.  WFDB
applications need not be aware of the source or format of their input,
since input files are located by searching a configurable path and all
data are transparently converted on-the-fly into a common format.

## Prerequisites

The following are required for a complete build.  All are free, open-source
software available for all popular platforms:

- GCC (or another C11 compiler), Meson, and Ninja
- [libcurl](https://curl.se/) (including development headers)
- [libFLAC](https://xiph.org/flac/) (including development headers)
- [expat](https://libexpat.github.io/) (including development headers) --
  needed for the XML tools (`xmlann`, `xmlhea`); `annxml` and `heaxml`
  are built without it

**Debian / Ubuntu:**

```sh
sudo apt-get install gcc meson ninja-build \
  libcurl4-gnutls-dev libflac-dev libexpat1-dev
```

**Fedora / RHEL:**

```sh
sudo dnf install gcc meson ninja-build \
  libcurl-devel flac-devel expat-devel
```

**macOS** (via [Homebrew](https://brew.sh/)):

```sh
brew install meson flac curl expat
```

**Windows:** Install [WSL](https://learn.microsoft.com/en-us/windows/wsl/install)
(`wsl --install` in an administrator PowerShell), then install the
Debian/Ubuntu prerequisites inside the WSL distribution.

Meson and Ninja are also available via pip (`pip install meson ninja`) if
your system packages are too old.

## Building and Installing

```sh
meson setup build
ninja -C build install
```

To install to a non-default location (no root needed):

```sh
meson setup build --prefix=$HOME/wfdb-local
ninja -C build install
```

Run the test suite:

```sh
meson test -C build
```

Uninstall:

```sh
ninja -C build uninstall
```

## Build Options

Customize the build with `-Doption=value` at configure time:

| Option | Values | Description |
|--------|--------|-------------|
| `netfiles` | `auto` / `enabled` / `disabled` | HTTP/FTP support via libcurl |
| `flac` | `auto` / `enabled` / `disabled` | FLAC signal compression |
| `expat` | `auto` / `enabled` / `disabled` | XML tools (requires expat) |
| `wave` | `auto` / `enabled` / `disabled` | WAVE viewer (requires GTK 3 and VTE) |
| `docs` | `auto` / `enabled` / `disabled` | HTML documentation (requires mandoc / asciidoctor) |
| `dbdir` | path | Database directory (default: `datadir/wfdb`) |

Options set to `auto` (the default) are enabled when the required libraries
are found and silently disabled otherwise.  Set to `enabled` to make a
missing library a hard error.

Example:

```sh
meson setup build -Dnetfiles=enabled -Dflac=enabled -Dwave=disabled
```

## Building Packages

The source tree includes packaging metadata for Debian/Ubuntu, Fedora/RHEL,
and Homebrew (macOS and Linux).

### Debian / Ubuntu (.deb)

Install the build dependencies:

```sh
sudo apt-get install build-essential debhelper-compat meson ninja-build \
  pkg-config libcurl4-gnutls-dev libexpat1-dev libflac-dev \
  libgtk-3-dev libvte-2.91-dev
```

Build the packages:

```sh
dpkg-buildpackage -us -uc
```

The `.deb` files are created in the parent directory:

| Package | Contents |
|---------|----------|
| `libwfdb10` | Shared library |
| `libwfdb-dev` | Headers, static library, pkg-config file |
| `wfdb-tools` | Command-line applications |
| `wave` | WAVE viewer and companion tools |

Install them:

```sh
sudo dpkg -i ../libwfdb10_*.deb ../libwfdb-dev_*.deb \
  ../wfdb-tools_*.deb ../wave_*.deb
```

### Fedora / RHEL (.rpm)

Install the build dependencies:

```sh
sudo dnf install gcc meson ninja-build \
  libcurl-devel expat-devel flac-devel \
  gtk3-devel vte291-devel rpm-build
```

Create a source tarball and build RPMs:

```sh
tar czf wfdb-10.7.0.tar.gz --transform='s,^,wfdb-10.7.0/,' \
  --exclude=build --exclude=.git .
rpmbuild -ta wfdb-10.7.0.tar.gz
```

The RPMs are placed under `~/rpmbuild/RPMS/`:

| Package | Contents |
|---------|----------|
| `wfdb` | Command-line applications and data files |
| `wfdb-libs` | Shared library |
| `wfdb-devel` | Headers, static library, pkg-config file |
| `wave` | WAVE viewer and companion tools |

### Homebrew (macOS and Linux)

```sh
brew install --formula wfdb.rb
```

The formula works on both macOS and Linux.  It builds WAVE (via GTK 3
and VTE), and enables libcurl, FLAC, and expat support.

## License

The WFDB library (`lib/`) is free software under the
[GNU Lesser General Public License v2](lib/COPYING.LIB) (or later).
You may link the library into your own programs (free or commercial,
open-source or proprietary) without restriction.  If you modify the
library itself, some restrictions apply to how you may distribute your
modified versions; see the LGPL for details.

The applications and tools in the other subdirectories are free software
under the [GNU General Public License v2](COPYING) (or later).

## Further Reading

- [PhysioNet](https://physionet.org/) -- home of the WFDB project
- [WFDB manuals](https://physionet.org/physiotools/manuals.shtml)
- [PhysioNet FAQ](https://physionet.org/faq.shtml)

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

## Development Container (Optional)

The repository includes a [devcontainer](.devcontainer/) configuration for
VS Code and GitHub Codespaces that provides a pre-configured development
environment with all prerequisites installed.

**VS Code with Docker:**

1. Install the [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)
2. Open the repository in VS Code
3. Click "Reopen in Container" when prompted (or use Command Palette → "Dev Containers: Reopen in Container")

**GitHub Codespaces:**

1. Click "Code" → "Codespaces" → "Create codespace on master" in the GitHub web interface
2. Wait for the container to build and start

The devcontainer automatically runs the build and install process on creation.
The built binaries are available immediately in the container environment.

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
brew install electrophys/wfdb/wfdb
```

Or equivalently:

```sh
brew tap electrophys/wfdb
brew install wfdb
```

This installs the library, command-line tools, and XML converters.
WAVE is not included; on Linux, use the deb or rpm packages for WAVE
support.

## License

The WFDB Software Package is dual-licensed:

**WFDB Library (`lib/`):** Licensed under the
[GNU Lesser General Public License (LGPL) v2.1+](lib/COPYING.LIB).
This allows the library to be linked with proprietary software without restriction.

**Applications and Tools (`app/`, `convert/`, `psd/`, `wave/`, `xml/`):** Licensed under the
[GNU General Public License (GPL) v2+](COPYING).

Both licenses give you the freedom to use, study, share, and modify the software.
See [LICENSE.md](LICENSE.md) for complete details.

## Acknowledgments

The WFDB Software Package was created and maintained by **George B. Moody** (1947-2020), whose pioneering work in biomedical signal processing and open-source software has benefited researchers and clinicians worldwide for over four decades. His legacy continues through this software and the PhysioNet resource he helped create.

The project is currently maintained by Benjamin Moody and the PhysioNet team at the MIT Laboratory for Computational Physiology.

## Further Reading

- [PhysioNet](https://physionet.org/) -- home of the WFDB project
- [WFDB manuals](https://physionet.org/physiotools/manuals.shtml)
- [PhysioNet FAQ](https://physionet.org/faq.shtml)

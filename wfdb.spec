Name:           wfdb
Version:        10.7.0
Release:        1%{?dist}
Summary:        WaveForm Database library and tools for physiologic signals
License:        LGPL-2.0-or-later AND GPL-2.0-or-later
URL:            https://physionet.org/
Source0:        https://physionet.org/static/published-projects/wfdb/wfdb-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  meson >= 0.61
BuildRequires:  ninja-build
BuildRequires:  pkgconfig(libcurl)
BuildRequires:  pkgconfig(expat)
BuildRequires:  pkgconfig(flac)
BuildRequires:  pkgconfig(gtk+-3.0)
BuildRequires:  pkgconfig(vte-2.91)

%description
The WFDB (WaveForm Database) library supports creating, reading, and
annotating digitized signals in a wide variety of formats. Input can be
from local files or directly from web servers via libcurl. WFDB
applications need not be aware of the source or format of their input,
since input files are located by searching a configurable path and all
data are transparently converted on-the-fly into a common format.

# ---- wfdb-libs subpackage ---------------------------------------------------

%package libs
Summary:        WaveForm Database shared library
License:        LGPL-2.0-or-later

%description libs
The WFDB shared library for reading and writing physiologic signal data.

%ldconfig_scriptlets libs

# ---- wfdb-devel subpackage ---------------------------------------------------

%package devel
Summary:        WaveForm Database library - development files
License:        LGPL-2.0-or-later
Requires:       %{name}-libs%{?_isa} = %{version}-%{release}

%description devel
Header files, static library, and pkg-config file for developing
applications with the WFDB library.

# ---- wave subpackage ---------------------------------------------------------

%package -n wave
Summary:        Waveform Analyzer, Viewer, and Editor
License:        GPL-2.0-or-later
Requires:       %{name} = %{version}-%{release}

%description -n wave
WAVE provides a graphical environment for exploring digitized signals
and time series. It supports fast, high-quality views of data stored
locally or on remote servers, flexible control of analysis modules,
efficient interactive annotation editing, and multiple simultaneous views.

# ---- prep/build/install/check ------------------------------------------------

%prep
%autosetup

%build
%meson -Dnetfiles=enabled -Dflac=enabled -Dexpat=enabled -Dwave=auto -Ddocs=disabled
%meson_build

%install
%meson_install

%check
WFDB_NO_NET_CHECK=1 %meson_test

# ---- file lists --------------------------------------------------------------

%files
%license COPYING
%doc README.md
%{_bindir}/a2m
%{_bindir}/ad2m
%{_bindir}/ahaconvert
%{_bindir}/ahaecg2mit
%{_bindir}/ann2rr
%{_bindir}/annxml
%{_bindir}/bxb
%{_bindir}/calsig
%{_bindir}/coherence
%{_bindir}/cshsetwfdb
%{_bindir}/ecgeval
%{_bindir}/edf2mit
%{_bindir}/epicmp
%{_bindir}/fft
%{_bindir}/fir
%{_bindir}/heaxml
%{_bindir}/gqfuse
%{_bindir}/gqpost
%{_bindir}/gqrs
%{_bindir}/hrfft
%{_bindir}/hrlomb
%{_bindir}/hrmem
%{_bindir}/hrplot
%{_bindir}/hrstats
%{_bindir}/ihr
%{_bindir}/log10
%{_bindir}/lomb
%{_bindir}/m2a
%{_bindir}/makeid
%{_bindir}/md2a
%{_bindir}/memse
%{_bindir}/mfilt
%{_bindir}/mit2edf
%{_bindir}/mit2wav
%{_bindir}/mrgann
%{_bindir}/mxm
%{_bindir}/nguess
%{_bindir}/nst
%{_bindir}/parsescp
%{_bindir}/plot2d
%{_bindir}/plot3d
%{_bindir}/plotstm
%{_bindir}/pnwlogin
%{_bindir}/pscgen
%{_bindir}/pschart
%{_bindir}/psfd
%{_bindir}/rdann
%{_bindir}/rdedfann
%{_bindir}/rdsamp
%{_bindir}/readid
%{_bindir}/revise
%{_bindir}/rr2ann
%{_bindir}/rxr
%{_bindir}/sampfreq
%{_bindir}/setwfdb
%{_bindir}/sigamp
%{_bindir}/sigavg
%{_bindir}/signame
%{_bindir}/signum
%{_bindir}/skewedit
%{_bindir}/snip
%{_bindir}/sortann
%{_bindir}/sqrs
%{_bindir}/sqrs125
%{_bindir}/stepdet
%{_bindir}/sumann
%{_bindir}/sumstats
%{_bindir}/tach
%{_bindir}/time2sec
%{_bindir}/wabp
%{_bindir}/wav2mit
%{_bindir}/wfdb-config
%{_bindir}/wfdb2mat
%{_bindir}/wfdbcat
%{_bindir}/wfdbcollate
%{_bindir}/wfdbdesc
%{_bindir}/wfdbmap
%{_bindir}/wfdbsignals
%{_bindir}/wfdbtime
%{_bindir}/wfdbwhich
%{_bindir}/wqrs
%{_bindir}/wrann
%{_bindir}/wrsamp
%{_bindir}/xform
%{_bindir}/xmlann
%{_bindir}/xmlhea
%{_datadir}/wfdb/
%exclude %{_mandir}/man1/wave.1*
%{_mandir}/man1/

%files libs
%license lib/COPYING.LIB
%{_libdir}/libwfdb.so.10{,.*}

%files devel
%{_includedir}/wfdb/
%{_libdir}/libwfdb.a
%{_libdir}/libwfdb.so
%{_libdir}/pkgconfig/wfdb.pc
%{_mandir}/man3/
%{_mandir}/man5/

%files -n wave
%{_bindir}/url_view
%{_bindir}/wave
%{_bindir}/wave-remote
%{_bindir}/wavescript
%{_mandir}/man1/wave.1*

%changelog
* Fri Feb 07 2026 PhysioNet <wfdb@physionet.org> - 10.7.0-1
- Rewrite spec for Meson build system
- Split into wfdb, wfdb-libs, wfdb-devel, and wave subpackages
- Add pkg-config support

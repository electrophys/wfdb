class Wfdb < Formula
  desc "WaveForm Database library and tools for physiologic signals"
  homepage "https://physionet.org/"
  url "https://github.com/electrophys/wfdb/archive/refs/heads/master.tar.gz"
  version "10.7.0"
  sha256 "4428a73392896f5e8abe6c2c9124181795b80ed3ba84acea513110bc8ee07050"
  license "LGPL-2.0-or-later"

  depends_on "meson" => :build
  depends_on "ninja" => :build
  depends_on "pkg-config" => :build
  depends_on "gcc" => :build unless OS.mac?
  depends_on "curl"
  depends_on "expat"
  depends_on "flac"
  depends_on "gtk+3"
  depends_on "vte3"

  def install
    system "meson", "setup", "build", *std_meson_args,
           "-Dnetfiles=enabled", "-Dflac=enabled",
           "-Dexpat=enabled", "-Dwave=enabled", "-Ddocs=disabled"
    system "meson", "compile", "-C", "build"
    system "meson", "install", "-C", "build"
  end

  test do
    (testpath/"test.c").write <<~C
      #include <wfdb/wfdb.h>
      #include <stdio.h>
      int main(void) {
        printf("WFDB %d.%d.%d\\n", WFDB_MAJOR, WFDB_MINOR, WFDB_RELEASE);
        return 0;
      }
    C
    system ENV.cc, "test.c", "-o", "test",
           *shell_output("pkg-config --cflags --libs wfdb").chomp.split
    assert_match "WFDB 10.7.0", shell_output("./test")
  end
end

class Wfdb < Formula
  desc "WaveForm Database library and tools for physiologic signals"
  homepage "https://physionet.org/"
  url "https://physionet.org/static/published-projects/wfdb/wfdb-10.7.0.tar.gz"
  sha256 "PLACEHOLDER"
  license "LGPL-2.0-or-later"

  depends_on "meson" => :build
  depends_on "ninja" => :build
  depends_on "pkg-config" => :build
  depends_on "curl"
  depends_on "flac"

  def install
    system "meson", "setup", "build", *std_meson_args,
           "-Dnetfiles=enabled", "-Dflac=enabled",
           "-Dwave=disabled", "-Ddocs=disabled"
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

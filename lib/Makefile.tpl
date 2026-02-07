# file: Makefile.tpl		G. Moody	  24 May 2000
#				Last revised:   20 December 2021
# This section of the Makefile should not need to be changed.

INCLUDES = $(DESTDIR)$(INCDIR)/wfdb/wfdb.h \
           $(DESTDIR)$(INCDIR)/wfdb/wfdblib.h \
           $(DESTDIR)$(INCDIR)/wfdb/ecgcodes.h \
           $(DESTDIR)$(INCDIR)/wfdb/ecgmap.h
HFILES = wfdb.h ecgcodes.h ecgmap.h wfdblib.h
CFILES = wfdbinit.c annot.c signal.c header.c sigformat.c flac.c sigmap.c timeconv.c calib.c wfdbio.c wfdb_context.c
OFILES = wfdbinit.o annot.o signal.o header.o sigformat.o flac.o sigmap.o timeconv.o calib.o wfdbio.o wfdb_context.o
MFILES = Makefile Makefile.dos

# `make' or `make all':  build the WFDB library
all:
	$(MAKE) setup
	$(MAKE) $(OFILES)
	$(BUILDLIB) $(OFILES) $(BUILDLIB_LDFLAGS)

# `make install':  install the WFDB library and headers
install:
	$(MAKE) clean	    # force recompilation since config may have changed
	$(MAKE) all
	$(MAKE) $(INCLUDES) $(DESTDIR)$(LIBDIR)
	../install.sh $(DESTDIR)$(LIBDIR) $(WFDBLIB)
	$(SETLPERMISSIONS) $(DESTDIR)$(LIBDIR)/$(WFDBLIB)
	$(MAKE) lib-post-install 2>/dev/null

# 'make collect':  retrieve the installed WFDB library and headers
collect:
	../conf/collect.sh $(INCDIR)/wfdb wfdb.h ecgcodes.h ecgmap.h
	../conf/collect.sh $(LIBDIR) $(WFDBLIB) $(WFDBLIB_DLLNAME)

uninstall:
	../uninstall.sh $(DESTDIR)$(INCDIR)/wfdb $(HFILES)
	../uninstall.sh $(DESTDIR)$(INCDIR)
	../uninstall.sh $(DESTDIR)$(LIBDIR) $(WFDBLIB)
	$(MAKE) lib-post-uninstall
	../uninstall.sh $(DESTDIR)$(LIBDIR)

setup:
	sed "s+@DBDIR@+$(DBDIR)+" <wfdblib.h0 >wfdblib.h

# `make clean': remove binaries and backup files
clean:
	rm -f $(OFILES) libwfdb.* *.dll *~

# `make TAGS':  make an `emacs' TAGS file
TAGS:		$(HFILES) $(CFILES)
	@etags $(HFILES) $(CFILES)

# `make listing':  print a listing of WFDB library sources
listing:
	$(PRINT) README $(MFILES) $(HFILES) $(CFILES)

# Rule for creating installation directories
$(DESTDIR)$(INCDIR) $(DESTDIR)$(INCDIR)/wfdb $(DESTDIR)$(INCDIR)/ecg $(DESTDIR)$(LIBDIR):
	mkdir -p $@; $(SETDPERMISSIONS) $@

# Rules for installing the include files
$(DESTDIR)$(INCDIR)/wfdb/wfdb.h:	$(DESTDIR)$(INCDIR)/wfdb wfdb.h
	../install.sh $(DESTDIR)$(INCDIR)/wfdb wfdb.h
	$(SETPERMISSIONS) $(DESTDIR)$(INCDIR)/wfdb/wfdb.h
$(DESTDIR)$(INCDIR)/wfdb/wfdblib.h:	$(DESTDIR)$(INCDIR)/wfdb wfdblib.h
	../install.sh $(DESTDIR)$(INCDIR)/wfdb wfdblib.h
	$(SETPERMISSIONS) $(DESTDIR)$(INCDIR)/wfdb/wfdblib.h
$(DESTDIR)$(INCDIR)/wfdb/ecgcodes.h:	$(DESTDIR)$(INCDIR)/wfdb ecgcodes.h
	../install.sh $(DESTDIR)$(INCDIR)/wfdb ecgcodes.h
	$(SETPERMISSIONS) $(DESTDIR)$(INCDIR)/wfdb/ecgcodes.h
$(DESTDIR)$(INCDIR)/wfdb/ecgmap.h:	$(DESTDIR)$(INCDIR)/wfdb ecgmap.h
	../install.sh $(DESTDIR)$(INCDIR)/wfdb ecgmap.h
	$(SETPERMISSIONS) $(DESTDIR)$(INCDIR)/wfdb/ecgmap.h

# Prerequisites for the library modules
wfdbinit.o:	wfdb.h wfdblib.h wfdbinit.c
annot.o:	wfdb.h ecgcodes.h ecgmap.h wfdblib.h wfdb_context.h annot.c
signal.o:	wfdb.h wfdblib.h signal_internal.h signal.c
header.o:	wfdb.h wfdblib.h signal_internal.h header.c
sigformat.o:	wfdb.h wfdblib.h signal_internal.h sigformat.c
flac.o:		wfdb.h wfdblib.h signal_internal.h flac.c
sigmap.o:	wfdb.h wfdblib.h signal_internal.h sigmap.c
timeconv.o:	wfdb.h wfdblib.h signal_internal.h timeconv.c
calib.o:	wfdb.h wfdblib.h wfdb_context.h calib.c
wfdb_context.o:	wfdb.h wfdblib.h wfdb_context.h wfdb_context.c
wfdbio.o:	wfdb.h wfdblib.h wfdb_context.h wfdbio.c
	lf='"$(LDFLAGS)"' ; \
	lf=`echo "$$lf" | sed 's|$(DESTDIR)$(LIBDIR)|$(LIBDIR)|g'` ; \
	$(CC) $(CFLAGS) -DVERSION='"$(VERSION)"' -DCFLAGS='"-I$(INCDIR)"' \
	  -DLDFLAGS="$$lf" $(BUILD_DATE_FLAGS) -c wfdbio.c

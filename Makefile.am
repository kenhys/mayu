ACLOCAL_AMFLAGS = -I m4

DEBUG_CXXFLAGS = -g -D_DEBUG -DDEBUG

bin_PROGRAMS = mayu

INCLUDE_FILES = \
		_tchar.h \
		array.h \
		compiler_specific.h \
		compiler_specific_func.h \
		common.h \
		gcc_main.h \
		misc.h \
		stringtool.h \
		wintypes.h \
		engine.h \
		errormessage.h \
		function.h \
		functions.h \
		driver.h \
		keymap.h \
		keyboard.h \
		parser.h \
		mayu.h \
		msgstream.h \
		multithread.h \
		setting.h \
		vkeytable.h \
		d/ioctl.h \
		d/linux/cpu_dependency.h \
		d/linux/keydriver.h

mayu_SOURCES = \
		$(INCLUDE_FILES) \
		compiler_specific_func.cpp \
		engine.cpp \
		function.cpp \
		keyboard.cpp \
		keymap.cpp \
		vkeytable.cpp \
		mayu.cpp \
		parser.cpp \
		setting.cpp \
		stringtool.cpp \
		gcc_main.cpp

mayu_CXXFLAGS = -Wall

mayu_CPPFLAGS = \
		$(DEBUG_CPPFLAGS) \
		-DVERSION=""""$(VERSION)"""" \
		-DLOGNAME=""""$(LOGNAME)"""" \
		-DCOMPUTERNAME=""""$(COMPUTERNAME)"""" \
		-DPKGDATADIR="\"$(pkgdatadir)\"" \
		@BOOST_CPPFLAGS@

nobase_dist_pkgdata_DATA= \
		109.mayu \
		104.mayu \
		mac109.mayu \
		mac104.mayu \
		windows109.mayu \
		windows104.mayu \
		linux109.mayu \
		linux104.mayu \
		104on109.mayu \
		109on104.mayu \
		default.mayu \
		dot.mayu \
		contrib/DVORAKon109.mayu \
		contrib/109onAX.mayu \
		contrib/keitai.mayu \
		contrib/dvorak.mayu \
		contrib/dvorak109.mayu \
		emacsedit.mayu

mayu_LDFLAGS = @BOOST_LDFLAGS@
mayu_LDADD = @BOOST_REGEX_LIB@

if OS_LINUX
mayu_SOURCES += d/linux/keydriver.cpp
else
if OS_DARWIN
SUBDIRS = d/darwin
endif
endif

debug :
	$(MAKE) CFLAGS="$(DEBUG_CXXFLAGS)" CXXFLAGS="$(DEBUG_CXXFLAGS)"

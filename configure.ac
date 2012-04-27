#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([Mayu], [0.12.1], [http://hibari.2ch.net/test/read.cgi/software/1280042042/])
AC_CONFIG_SRCDIR([mayu.cpp])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE

# check os type
AC_CANONICAL_HOST

AM_CONDITIONAL(OS_LINUX, false)
AM_CONDITIONAL(OS_DARWIN, false)
AM_CONDITIONAL(OS_TIGER, false)
AM_CONDITIONAL(OS_LEOPARD, false)
AM_CONDITIONAL(OS_SNOW_LEOPARD, false)

case "$host_os" in
linux*)
	AM_CONDITIONAL(OS_LINUX, true)
	os_type=linux
	;;
darwin*)
	AM_CONDITIONAL(OS_DARWIN, true)
	os_type=darwin
	case "$host_os" in
	darwin8.*)
		AM_CONDITIONAL(OS_TIGER, true)
		;;
	darwin9.*)
		AM_CONDITIONAL(OS_LEOPARD, true)
		;;
	darwin10.*)
		AM_CONDITIONAL(OS_SNOW_LEOPARD, true)
		;;
	esac
	;;
esac

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_MAKE_SET
AM_PROG_CC_C_O

CFLAGS="-O2 -DNDEBUG"
CXXFLAGS=$CFLAGS

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lcc_kext':
dnl AC_CHECK_LIB([cc_kext], [main])
dnl # FIXME: Replace `main' with a function in `-lcpp_kext':
dnl AC_CHECK_LIB([cpp_kext], [main])
dnl # FIXME: Replace `main' with a function in `-lkmod':
dnl AC_CHECK_LIB([kmod], [main])
case "$os_type" in
linux)
	AC_CHECK_LIB([udev], [udev_new],,AC_MSG_ERROR([libudev not found]))
	AC_CHECK_LIB([usb-1.0], [libusb_init],,AC_MSG_ERROR([libusb-1.0 not found]))
	;;
darwin)
	;;
esac

# Checks boost
AX_BOOST_BASE([1.33])
AX_BOOST_REGEX

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h inttypes.h locale.h malloc.h stdlib.h string.h sys/ioctl.h sys/param.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_TYPE_UINT8_T
AC_C_VOLATILE

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_HEADER_MAJOR
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_CHECK_FUNCS([gettimeofday memset select strchr])

# PS/2 (Darwin only)
AM_CONDITIONAL(SUPPORT_PS2,false)
if test "$os_type" = "darwin" ; then
	AC_ARG_ENABLE(ps2, [  --enable-ps2           Enalbe PS/2 Keyboard support.(Mac/Darwin only) [default=no]],[],
		AC_MSG_CHECKING([for PS/2 Keyboard support])
		if test -n "`kextstat|grep ApplePS2Controller`" ; then
			AC_MSG_RESULT(yes)
			enable_ps2=yes
		else  
			AC_MSG_RESULT(no)
			enable_ps2=no
		fi
	)
	if test "$enable_ps2" = "yes" ; then
		AM_CONDITIONAL(SUPPORT_PS2,true)
	else
		AM_CONDITIONAL(SUPPORT_PS2,false)
	fi
fi

AC_CONFIG_FILES([Makefile
                 d/darwin/Makefile])
AC_OUTPUT

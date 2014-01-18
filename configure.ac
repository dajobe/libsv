dnl -*- Mode: autoconf -*-
dnl
dnl configure.ac - autoconf file for libsv
dnl (Process this file with autoconf to produce a configure script.)
dnl
dnl Copyright (C) 2014 David Beckett http://www.dajobe.org/
dnl
dnl This package is Free Software.
dnl
dnl It is licensed under the following three licenses as alternatives:
dnl   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
dnl   2. GNU General Public License (GPL) V2 or any newer version
dnl   3. Apache License, V2.0 or any newer version
dnl
dnl You may not use this file except in compliance with at least one of
dnl the above three licenses.
dnl
dnl See LICENSE.html or LICENSE.txt at the top of this package for the
dnl complete terms and further detail along with the license texts for
dnl the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
dnl
dnl


AC_PREREQ([2.62])
AC_INIT([libsv], [1.0.0], [https://github.com/dajobe/libsv/issues], [libsv])
AC_CONFIG_SRCDIR([sv.c])
AC_CONFIG_HEADERS([sv_config.h])
AC_CONFIG_AUX_DIR(build)
AC_CONFIG_MACRO_DIR(build)
AM_INIT_AUTOMAKE([1.11 check-news std-options -Wobsolete -Wportability -Wsyntax -Wunsupported -Wextra-portability])

AM_MAINTAINER_MODE

dnl Checks for programs.
AC_CANONICAL_HOST
AM_SANITY_CHECK
AM_PROG_AR
AC_PROG_CC
AM_PROG_CC_C_O
AC_MSG_CHECKING(whether $CC is clang)
CC_IS_CLANG=no
if $CC 2>&1 | grep clang >/dev/null 2>&1; then
  CC_IS_CLANG=yes
else
  :
fi
AC_MSG_RESULT($CC_IS_CLANG)

dnl Initialize libtool
LT_INIT

AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Find a tar command for 'make dist'
AC_CHECK_PROGS(TAR, gnutar gtar tar)
AC_CHECK_PROGS(PERL, perl)

AM_MISSING_PROG(ACLOCAL, aclocal, $missing_dir)
AM_MISSING_PROG(AUTOCONF, autoconf, $missing_dir)
AM_MISSING_PROG(AUTOMAKE, automake, $missing_dir)
AM_MISSING_PROG(AUTOHEADER, autoheader, $missing_dir)

AC_CHECK_PROGS(RECHO, echo)
RECHO_C=
RECHO_N=
case `$RECHO -n x` in
  -n*)
    case `$RECHO 'xy\c'` in
      *c*)
         ;;
      xy)
         RECHO_C='\c'
         ;;
    esac;;
  *)
    RECHO_N='-n'
    ;;
esac



dnl compiler checks

AC_DEFUN([REDLAND_CC_TRY_FLAG], [
  AC_MSG_CHECKING([whether $CC supports $1])
  redland_save_CFLAGS="$CFLAGS"
  if test $CC_IS_CLANG = yes; then
    # Make unknown warnings into errors for clang so the test below works
    CFLAGS="-Werror $CFLAGS"
  fi
  CFLAGS="$CFLAGS $1"
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([ ])], [redland_cc_flag=yes], [redland_cc_flag=no])
  CFLAGS="$redland_save_CFLAGS"
  if test "X$redland_cc_flag" = "Xyes"; then
    ifelse([$2], , :, [$2])
  else
    ifelse([$3], , :, [$3])
  fi
  AC_MSG_RESULT($redland_cc_flag)
])

# GCC warning options
# http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
#
# Too noisy:
#  -Wconversion
#  -Wformat-nonliteral : variables for format strings
#  -Wmissing-field-initializers : too noisy in libsv_rss_common.c
#  -Wsystem-headers   : not debugging system
#  -Wsign-conversion: : many int / unsigned int / size_t
#  -Wunused-parameter : variables can be marked  __attribute__('unused')
#
# Apple gcc specific (probably): -Wshorten-64-to-32
possible_warnings="\
-Wall \
-Wc++-compat \
-Wextra \
-Wunused \
\
-Waggregate-return \
-Wbad-function-cast \
-Wcast-align \
-Wdeclaration-after-statement \
-Wdisabled-optimization \
-Wdiv-by-zero \
-Wendif-labels \
-Werror-implicit-function-declaration \
-Wfloat-equal \
-Wformat-security \
-Wframe-larger-than=4096 \
-Winit-self \
-Winline \
-Wmissing-declarations \
-Wmissing-format-attribute \
-Wmissing-noreturn \
-Wmissing-prototypes \
-Wnested-externs \
-Wold-style-definition \
-Wpacked \
-Wpointer-arith \
-Wredundant-decls \
-Wshadow \
-Wsign-compare \
-Wstrict-prototypes \
-Wswitch-enum \
-Wundef \
-Wunsafe-loop-optimizations \
-Wwrite-strings \
\
-Wshorten-64-to-32
\
-Wno-conversion \
-Wno-format-nonliteral \
-Wno-missing-field-initializers \
-Wno-sign-conversion \
-Wno-system-headers \
-Wno-unused-parameter \
"

warning_cflags=
if test "$USE_MAINTAINER_MODE" = yes; then
  AC_MSG_CHECKING(for supported $CC warning flags)
  AC_MSG_RESULT($warning_cflags)
  for warning in $possible_warnings; do
    REDLAND_CC_TRY_FLAG([$warning], [warning_cflags="$warning_cflags $warning"])
  done
  AC_MSG_CHECKING($CC supports warning flags)
  AC_MSG_RESULT($warning_cflags)
fi

MAINTAINER_CPPFLAGS="$warning_cflags"


dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(errno.h stdlib.h unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE

dnl need to change quotes to allow square brackets
changequote(<<, >>)dnl
version_major=`echo $VERSION | sed -e 's/^\([^\.]*\)\.\([^\.]*\)\.\(.*\)$/\1/'`
version_minor=`echo $VERSION | sed -e 's/^\([^\.]*\)\.\([^\.]*\)\.\(.*\)$/\2/'`
version_release=`echo $VERSION | sed -e 's/^\([^\.]*\)\.\([^\.]*\)\.\(.*\)$/\3/'`
changequote([, ])dnl

version_decimal=`expr $version_major \* 10000 + $version_minor \* 100 + $version_release`

AC_DEFINE_UNQUOTED(LIBSV_VERSION_MAJOR, $version_major, [Major version number])
AC_DEFINE_UNQUOTED(LIBSV_VERSION_MINOR, $version_minor, [Minor version number])
AC_DEFINE_UNQUOTED(LIBSV_VERSION_RELEASE, $version_release, [Release version number])
AC_DEFINE_UNQUOTED(LIBSV_VERSION_DECIMAL, $version_decimal, [Release version as a decimal])

# for libsv-config.in
LIBSV_VERSION_MAJOR=$version_major
LIBSV_VERSION_MINOR=$version_minor
LIBSV_VERSION_RELEASE=$version_release
LIBSV_VERSION_DECIMAL=$version_decimal
AC_SUBST(LIBSV_VERSION)
AC_SUBST(LIBSV_VERSION_MAJOR)
AC_SUBST(LIBSV_VERSION_MINOR)
AC_SUBST(LIBSV_VERSION_RELEASE)
AC_SUBST(LIBSV_VERSION_DECIMAL)

# Libtool versioning
#
# CURRENT
#      The most recent interface number that this library implements.
#
# REVISION
#      The implementation number of the CURRENT interface.
#
# AGE
#      The difference between the newest and oldest interfaces that this
#      library implements.  In other words, the library implements all the
#      interface numbers in the range from number `CURRENT - AGE' to
#      `CURRENT'.
#
# Rules:
#  1. Start with version information of `0:0:0' for each libtool library.
#
#  2. Update the version information only immediately before a public
#     release of your software.  More frequent updates are unnecessary,
#     and only guarantee that the current interface number gets larger
#     faster.
#
#  3. If the library source code has changed at all since the last
#     update, then increment REVISION (`C:R:A' becomes `C:r+1:A').
#
#  4. If any interfaces have been added, removed, or changed since the
#     last update, increment CURRENT, and set REVISION to 0.
#
#  5. If any interfaces have been added since the last public release,
#     then increment AGE.
#
#  6. If any interfaces have been removed since the last public release,
#     then set AGE to 0.
#
# syntax: CURRENT[:REVISION[:AGE]]
LIBSV_LIBTOOL_VERSION=0:0:0
AC_SUBST(LIBSV_LIBTOOL_VERSION)

dnl Checks for library functions.


# Make final changes to cflags
MEM=
MEM_LIBS=
CPPFLAGS="-DLIBSV_INTERNAL=1 $CPPFLAGS"

debug_messages=no

AC_ARG_ENABLE(debug, [  --enable-debug          Enable debug messages (default no).  ], debug_messages=$enableval)	
if test "$debug_messages" = "yes"; then
  AC_DEFINE([LIBSV_DEBUG], [1], [Define to 1 if debug messages are enabled.])
fi

if test "$USE_MAINTAINER_MODE" = yes; then
  AC_DEFINE([MAINTAINER_MODE], [1], [Define to 1 if maintainer mode is enabled.])
  CPPFLAGS="$MAINTAINER_CPPFLAGS $CPPFLAGS"
fi


AC_SUBST(LIBSV_LDFLAGS)

AC_SUBST(RECHO)
AC_SUBST(RECHO_N)
AC_SUBST(RECHO_C)

abs_top_srcdir=`cd $srcdir; pwd`
AC_SUBST(abs_top_srcdir)
abs_top_builddir=`pwd`
AC_SUBST(abs_top_builddir)

dnl automake 1.11
AM_SILENT_RULES([no])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT

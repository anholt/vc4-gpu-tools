# Copyright © 2015 Broadcom
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

AC_PREREQ([2.60])
AC_INIT([vc4-gpu-tools],
	[1.6],
	[https://bugs.freedesktop.org/enter_bug.cgi?product=xorg],
	[vc4-gpu-tools])

AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AC_GNU_SOURCE
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([foreign dist-bzip2 subdir-objects])

AC_PROG_CC

# Checks for functions, headers, structures, etc.
AC_HEADER_STDC
AC_CHECK_FUNCS([asprintf])

# Initialize libtool
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

# Require X.Org macros 1.16 or later for XORG_TESTSET_CFLAG
m4_ifndef([XORG_MACROS_VERSION],
	  [m4_fatal([must install xorg-macros 1.16 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.16)
XORG_DEFAULT_OPTIONS

if  test "x$GCC" = xyes ; then
    CWARNFLAGS="$CWARNFLAGS -fno-strict-aliasing -Wno-pointer-arith -Wno-declaration-after-statement"
fi

PKG_CHECK_MODULES(LIBDRM, [libdrm])

PKG_CHECK_MODULES([SIMPENROSE], [simpenrose],
                  [HAVE_SIMPENROSE=yes], [HAVE_SIMPENROSE=no])
AM_CONDITIONAL(HAVE_SIMPENROSE, test x$HAVE_SIMPENROSE = xyes)

AC_CONFIG_FILES([
		 Makefile
		 tests/Makefile
		 tools/Makefile
		 ])
AC_OUTPUT

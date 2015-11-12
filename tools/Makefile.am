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

AM_CPPFLAGS = -I$(top_srcdir)/include/drm -I$(top_srcdir)/include
AM_CFLAGS = $(LIBDRM_CFLAGS) $(CWARNFLAGS)

if HAVE_SIMPENROSE
SIMPENROSE_PROGS = \
	vc4_dump_to_clif \
	$()
endif

bin_PROGRAMS = \
	$(SIMPENROSE_PROGS) \
	vc4_dump_hang_state \
	vc4_dump_parse \
	$()

vc4_dump_hang_state_LDADD = $(LIBDRM_LIBS)
vc4_dump_to_clif_LDFLAGS = $(SIMPENROSE_LIBS)

vc4_dump_parse_SOURCES = \
	vc4_dump_parse.c \
	vc4_dump_parse.h \
	vc4_dump_parse_cl.c \
	$()

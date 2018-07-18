# SPDX-License-Identifier: LGPL-2.1-or-later
# Copyright © 2007-2018 ANSSI. All Rights Reserved.
clipincludedir = $(includedir)/clip
clipinclude_HEADERS = \
	clip.h

lib_LTLIBRARIES = libclip.la

libclip_la_SOURCES = \
	clip-unistd.c \
	clip-socket.c \
	clip-passwd.c \
	clip-capability.c 

libclip_la_LDFLAGS = -version-info @VERSION_INFO@

INCLUDES = -I$(srcdir) ${CLIP_CFLAGS}


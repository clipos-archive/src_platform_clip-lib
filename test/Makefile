# SPDX-License-Identifier: LGPL-2.1-or-later
# Copyright © 2007-2018 ANSSI. All Rights Reserved.
TESTS_SRC := test_cap.c test_priv.c test_sock_read.c
LIBS := clip

###############################################################################

ifndef VERBOSE
MAKEFLAGS += -s --no-print-directory
ECHO = echo
else
ECHO = echo > /dev/null
endif

LIBFLAGS := ${foreach lib, ${LIBS}, -l${lib}}
CFLAGS := -Wall -I../src
LDFLAGS := -L../src/.libs -Wl,-rpath,${PWD}/../src/.libs ${LIBFLAGS}

TESTS := ${patsubst %.c,%.test, ${TESTS_SRC}}

all: ${TESTS}

%.test:%.c Makefile
	@${ECHO} "        CC                $<"
	$(CC) -o $@ $< ${CFLAGS} ${LDFLAGS}

clean: 
	@${ECHO} "        CLEAN"
	rm -f ${TESTS}


// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/**
 *  test_cap.c - test for clip_reducecaps
 *  Copyright (C) 2007 SGDN/DCSSI
 *  Author: Vincent Strubel <clipos@ssi.gouv.fr>
 * 
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License 
 *  version 2 as published by the Free Software Foundation.
 **/

#include "test_prelude.h"


#include <linux/capability.h>
#include <clip.h>

#define ARGS_TYPE uint32_t mask
#define ARGS mask
#define TEST_FUN clip_reducecaps

static inline void 
print_args(ARGS_TYPE)
{
	printf("\nTesting mask = 0x%x\nResults:\n", mask);
}


static char *status_matches[] = { "Cap", NULL };

static int call_test(ARGS_TYPE);


static int
show_results(void)
{
	if (_read_file("/proc/self/status", status_matches))
		return -1;
	return 0;
}

int 
main(void)
{
	if (call_test(1<<CAP_DAC_OVERRIDE)) 
		return EXIT_FAILURE;
	if (call_test((1<<CAP_DAC_OVERRIDE) | (1<<CAP_NET_BIND_SERVICE)))
		return EXIT_FAILURE;
	if (call_test(0xffffffff))
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

#include "test_common.h"

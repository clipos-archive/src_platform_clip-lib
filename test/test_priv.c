// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/**
 *  test_priv.c - test for clip_revokeprivs
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

#define ARGS_TYPE uid_t uid, gid_t gid, gid_t *grps, size_t size, uint32_t mask
#define ARGS uid, gid, grps, size, mask
#define TEST_FUN clip_revokeprivs

static inline void 
print_args(ARGS_TYPE)
{
	int i;
	printf("\nTesting uid=%d, gid=%d, grps = { ", uid, gid);
	for (i = 0; i < size; i++)
		printf("%d, ", grps[i]);
	printf("}, mask = 0x%x\nResults:\n", mask); 
}

static char *status_matches[] = { "Uid", "Gid", "Groups", "Cap", NULL };

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
	gid_t grp1[2] = {300, 301};
	gid_t grp2[1] = {300};

	if (call_test(300, 300, grp1, 2, 1<<CAP_DAC_OVERRIDE)) 
		return EXIT_FAILURE;
	if (call_test(300, 300, grp2, 1, 1<<CAP_DAC_OVERRIDE)) 
		return EXIT_FAILURE;
	if (call_test(300, 0, NULL, 0, 1<<CAP_DAC_OVERRIDE)) 
		return EXIT_FAILURE;
	if (call_test(0, 300, NULL, 0, 1<<CAP_DAC_OVERRIDE)) 
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

#include "test_common.h"

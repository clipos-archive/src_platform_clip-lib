// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/**
 *  @file clip-capability.c 
 *  Capability functions for clip-lib
 *  @author Vincent Strubel <clipos@ssi.gouv.fr>
 * 
 *  Copyright (C) 2007 SGDN/DCSSI
 *  @n
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License 
 *  version 2 as published by the Free Software Foundation.
 */

#define _GNU_SOURCE
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <grp.h>
#include <linux/capability.h>
#include <sys/syscall.h>
#include <sys/prctl.h>

#include "clip.h"

/**
 * @internal Helper: get current capabilities masks.
 */
static inline long int
capget(cap_user_header_t hdr, cap_user_data_t data)
{
	return syscall(SYS_capget, hdr, data);
}

/**
 * @internal Helper: set capabilities masks.
 */
static inline long int
capset(cap_user_header_t hdr, cap_user_data_t data)
{
	return syscall(SYS_capset, hdr, data);
}

/**
 * @internal Capabilities reduction helper.
 * Reduces the permitted and inherited masks by intersecting them with an 
 * authorized mask. The effective mask is reduced in the same way if @a
 * set_eff is zero, otherwise the effective mask is set to the (reduced)
 * permitted mask (to restore caps after a setuid).
 */
static long int
_clip_reducecaps(uint32_t mask, int set_eff)
{
	cap_user_header_t hdr;
	cap_user_data_t data;
	long int ret, err;
	
	err = -ENOMEM;
	ret = -1;
	hdr = malloc(sizeof(*hdr));
	if (!hdr)
		goto out;
	data = malloc(sizeof(*data));
	if (!data)
		goto out_freehdr;

	hdr->pid = 0;
	hdr->version = _LINUX_CAPABILITY_VERSION;

	err = capget(hdr, data);
	if (err)
		goto out_freedata;

	data->inheritable &= mask;
	data->permitted &= mask;
	if (set_eff) {
		data->effective = data->permitted;
	} else {
		data->effective &= mask;
	}

	err = capset(hdr, data);
	if (err)
		goto out_freedata;

	ret = 0;
	
	/* fall through */
out_freedata:
	free(data);
	/* fall through */
out_freehdr:
	free(hdr);
	/* fall through */
out:
	if (ret) {
		errno = (int) err;
		perror("clip_reducecaps");
	}
	return ret;
}

long int
clip_reducecaps(uint32_t mask)
{
	return _clip_reducecaps(mask, 0);
}

int
clip_revokeprivs(uid_t uid, gid_t gid, const gid_t *grouplist, 
				size_t groupsize, uint32_t keepcaps)
{
	if (prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0)) {
		perror("prctl");
		return -1;
	}
	if (groupsize && setgroups(groupsize, grouplist)) {
		perror("setgroups");
		return -1;
	}
	if (gid && setresgid(gid, gid, gid)) {
		perror("setresgid");
		return -1;
	}
	if (uid && setresuid(uid, uid, uid)) {
		perror("setresuid");
		return -1;
	}
	if (_clip_reducecaps(keepcaps, 1))
		return -1;

	if (prctl(PR_SET_KEEPCAPS, 0, 0, 0, 0)) {
		perror("prctl");
		return -1;
	}

	return 0;
}

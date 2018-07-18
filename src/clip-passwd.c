// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/**
 *  @file clip-passwd.c 
 *  Passwd / group management functions for clip-lib
 *  @author Vincent Strubel <clipos@ssi.gouv.fr>
 * 
 *  Copyright (C) 2006 SGDN/DCSSI
 *  @n
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License 
 *  version 2 as published by the Free Software Foundation.
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>

#include "clip.h"

int
clip_checkgroup(const struct passwd *pwd, const char *grname)
{
	gid_t *groups;
	struct group *grp;
	int i;

	int ngroups = 0;

	grp = getgrnam(grname);
	if (!grp) {
		errno = ENOENT;
		return -1;
	}
	/* Get actual number of groups for this user. >=0 means none */
	/* WARNING : this is broken on glibc-2.3.2, see getgrouplist(3) */
	if (getgrouplist(pwd->pw_name, pwd->pw_gid, NULL, &ngroups) >= 0) 
		return 0;
	
	groups = malloc((unsigned)ngroups * sizeof(*groups));
	if (!groups) {
		fprintf(stderr, "%s: out of memory allocating "
				"%d groups\n", __FUNCTION__, ngroups);
		errno = ENOMEM;
		return -1;
	}
	if (getgrouplist(pwd->pw_name, pwd->pw_gid, groups, &ngroups) < 0) {
		fprintf(stderr, "%s: group number changed under our feet!\n",
						__FUNCTION__);
		free(groups);
		errno = EFAULT;
		return -1;
	}

	for (i = 0; i < ngroups; ++i)
		if (groups[i] == grp->gr_gid) {
			free(groups);
			return 1;
		}
	
	free(groups);
	return 0;
}

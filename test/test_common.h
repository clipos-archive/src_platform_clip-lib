// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/**
 *  test_common.h - common test code
 *  Copyright (C) 2007 SGDN/DCSSI
 *  Author: Vincent Strubel <clipos@ssi.gouv.fr>
 * 
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License 
 *  version 2 as published by the Free Software Foundation.
 **/

#ifndef _TEST_COMMON_H
#define _TEST_COMMON_H

#define READLEN 256

/* strcmp, limited to the length of the first string */
static inline int
_strcmp(const char *pattern, const char *str)
{
	const char *ptr1 = pattern;
	const char *ptr2 = str;

	while (*ptr1 && *ptr2 && (*ptr1 == *ptr2)) {
		ptr1++;
		ptr2++;
	}
	return (*ptr1) ? 1 : 0;
}

static inline int
_match(const char *buf, char **matches)
{
	char **ptr = matches;

	while (*ptr) {
		if (!_strcmp(*ptr, buf)) 
			return 1;
		ptr++;
	}
	return 0;
}

static int
_read_file(const char *fname, char **matches)
{
	char buf[READLEN];
	FILE *f;
	
	f = fopen(fname, "r");
	if (!f) {
		perror("fopen");
		return -1;
	}

	for(;;) {
		if (!fgets(buf, READLEN, f)) {
			if (feof(f))
				break;
			perror("fgets");
			return -1;
		}
		if (!_match(buf, matches))
			continue;
		printf("\t%s", buf);;
	} 

	fclose(f);
	return 0;
}

static int
do_test(ARGS_TYPE)
{
	int ret;
	
	ret = TEST_FUN(ARGS);
	if (ret) {
		fputs("Call failed\n", stderr);
		return -1;
	}

	return show_results();
}

static int
call_test(ARGS_TYPE)
{
	int status;
	pid_t pid;
	int ret;

	print_args(ARGS);
	
	pid = fork();
	switch (pid) {
		case -1:
			perror("fork");
			return -1;
		case 0:
			exit(do_test(ARGS));
		default:
			if (waitpid(pid, &status, 0) != pid) {
				fputs("WTF?\n", stderr);
				return -1;
			}
			if (!WIFEXITED(status))
				return -1;
			ret = WEXITSTATUS(status);
#ifdef parent_hook
			parent_hook();
#endif
			return ret;
	}
}

#endif /* _TEST_COMMON */

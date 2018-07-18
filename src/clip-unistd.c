// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/**
 *  @file clip-unistd.c 
 *  Filesystem-related functions for clip-lib.
 *  @author Vincent Strubel <clipos@ssi.gouv.fr>
 * 
 *  Copyright (C) 2006 SGDN/DCSSI
 *  @n
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License 
 *  version 2 as published by the Free Software Foundation.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#define __USE_GNU
#include <fcntl.h>
#undef __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "clip.h"

inline int 
clip_closeall(int std_p)
{
	int ret, fd, nofiles;

	/* There is no error return code per se on
	 * getdtablesize() : it returns OPEN_MAX -
	 * which is OK if not optimal for us - when
	 * the rlimit syscal fails.
	 */
	nofiles = getdtablesize();

	/* close std{in,out,err} only if std_p is set */
	fd = (std_p) ? 0 : STDERR_FILENO+1;

	for (; fd < nofiles; ++fd) {
		ret = close(fd);
		if (ret == -1 && errno != EBADF && errno != ENODEV) {
			if (!std_p)
				perror("clip_closeall");
			return -1;
		}
	}

	return 0;
}

int
clip_reset_fds(const char *stdin_path, const char *stdout_path)
{
	int fd;
	if (chdir("/") < 0) {
		perror("clip_reset_fd: chdir");
		return -1;
	}
	
	if (clip_closeall(1))
		return -1;

	if (stdin_path) {
		fd = open(stdin_path, O_RDONLY|O_NONBLOCK|O_NOATIME|O_NOFOLLOW);
		if (fd == -1)
			return -1;
		if (dup2(fd, STDIN_FILENO) < 0) {
			close(fd);
			return -1;
		}
		close(fd);
	}

	if (stdout_path) {
		fd = open(stdout_path, O_WRONLY|O_NOFOLLOW);
		if (fd == -1)
			return -1;
		if (dup2(fd, STDOUT_FILENO) < 0) {
			close(fd);
			return -1;
		}
		if (dup2(fd, STDERR_FILENO) < 0) {
			close(fd);
			return -1;
		}
		close(fd);
	}

	return 0;
}

static inline int 
__clip_fork(void)
{
	
	pid_t ret;

	ret = fork();

	switch(ret) {
		case -1: 
			 perror("__clip_fork");
			 return -1;
			 break;
		case 0: 
			 break;
		default: 
			 _exit(0); /* no atexit here... */
			 break;
	}

	return 0;
}

int
clip_fork(void)
{
	int fd0;

	if (chdir("/") < 0) {
		perror("clip_daemonize: chdir");
		return -1;
	}
	
	if (__clip_fork() < 0) {
		perror("clip_daemonize: __clip_fork");
		return -1;
	}

	if (clip_closeall(1) < 0) 
		return -1;

	(void)umask(0077);

	/* /dev/null as stdin */
	fd0 = open("/dev/null", O_RDONLY|O_NONBLOCK);
	if (fd0 == -1) 
		return -1;
	if (dup2(fd0, STDIN_FILENO) < 0) { 
		close(fd0);
		return -1;
	}

	/* TODO /XXXXlog as stdout, stderr ? */
	if (dup2(fd0, STDOUT_FILENO) < 0) {
		close(fd0);
		return -1;
	}
	if (dup2(fd0, STDERR_FILENO) < 0) {
		close(fd0);
		return -1;
	}
	
	close(fd0);
	if (setsid() < 0)  
		return -1;

	return 0;
}

int
clip_daemonize(void)
{

	if (clip_fork() < 0)
		return -1;

	/* Refork, let group leader exit 
	 * -> we can't get a new controlling terminal */
	if (__clip_fork() < 0)
		return -1;
	
	return 0;
}

int
clip_chroot(const char *path)
{
	if (chdir(path)) {
		perror("clip_daemonize: chdir");
		return -1;
	}
	if (chroot(".")) {
		perror("clip_daemonize: chroot");
		return -1;
	}
	return 0;
}

// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/**
 *  test_sock_read.c - test for clip_sock_read
 *  Copyright (C) 2007 SGDN/DCSSI
 *  Author: Vincent Strubel <clipos@ssi.gouv.fr>
 * 
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License 
 *  version 2 as published by the Free Software Foundation.
 **/

#include "test_prelude.h"


#include <clip.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/socket.h>

#define ARGS_TYPE int delay, int tries
#define ARGS delay, tries
#define TEST_FUN spawn_child

static inline void 
print_args(ARGS_TYPE)
{
	printf("\nTesting sock_read with delay %d and %d tries\n", delay, tries);
}

static int call_test(ARGS_TYPE);

static pid_t child_pid;
static int father_sock;
static int father_pipe;

static void
sighandle(int sig)
{
	printf("\t\t%d got signal\n", getpid());
	exit((sig == SIGTERM) ? 0 : -1);
}

#define wait4ack() do {\
	if (read(pipe, &c, 1) != 1) { \
		perror("Child read"); \
		return -1; \
	} \
} while (0)

static int
do_child(int pipe, int s_data, int delay, int tries)
{
	char c;
	char buf[32];
	ssize_t readlen;
	
	/* start */
	wait4ack();

	puts("Child trying to read");

	readlen = clip_sock_read(s_data, buf, 31, delay, tries);
	if (readlen >= 0) {
		buf[readlen] = '\0';
		printf("Child read %d bytes : %s\n", readlen, buf);
	} else {
		puts("Child read error");
	}

	close (s_data);
	return 0;
}

#undef wait4ack

static int
spawn_child(ARGS_TYPE)
{
	pid_t pid;
	int socks[2];
	int fds[2];

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, socks) < 0) {
		perror("socketpair");
		return -1;
	}
	if (pipe(fds) == -1) {
		perror("pipe");
		return -1;
	}	
	pid = fork();
	switch (pid) {
		case -1: 
			perror("fork");
			return -1;
		case 0: 
			(void)close(socks[0]);
			(void)close(fds[1]);
			if (signal(SIGTERM, sighandle) == SIG_ERR) {
				perror("signal");
				exit(-1);
			}
			exit(do_child(fds[0], socks[1], ARGS));
		default:
			child_pid = pid;
			(void)close(socks[1]);
			(void)close(fds[0]);
			father_sock = socks[0];
			father_pipe = fds[1];
			return 0;
	}
}
			
#define do_ack() do {\
	if (write(father_pipe, "a", 1) != 1) { \
		perror("Father write"); \
		goto error; \
	} \
} while (0)

static int
show_results(void)
{
	int status;
	char buf[256];
	ssize_t len;

	puts("please enter a string");
	do_ack();
	if ((len = read(STDIN_FILENO, buf, 255)) < 0) {
		perror("read");
		goto error;
	}
	if (write(father_sock, buf, len) < 0) {
		perror("write");
		goto error;
	}
	
	
	if (waitpid(child_pid, &status, 0) != child_pid) {
		fputs("WTF???\n", stderr);
		goto error;
	}
	if (!WIFEXITED(status))
		return -1;
	return WEXITSTATUS(status);
error:
	if (kill(child_pid, SIGTERM))
		perror("kill");
	return -1;
}

int 
main(void)
{
	if (call_test(-1, 0))
		return EXIT_FAILURE;
	if (call_test(2000, 3))
		return EXIT_FAILURE;
	if (call_test(3000, 5))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

#include "test_common.h"

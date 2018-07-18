// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/**
 *  @file clip-socket.c 
 *  Socket functions for libclip
 *  @author Vincent Strubel <clipos@ssi.gouv.fr>
 * 
 *  Copyright (C) 2006-2010 SGDN/DCSSI
 *  Copyright (C) 2011 SGDSN/ANSSI
 *  @n
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License 
 *  version 2 as published by the Free Software Foundation.
 */

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>

#include "clip.h"

int 
clip_getpeereid(int sock, uid_t *euid, gid_t *egid)
{
	struct ucred creds;
	socklen_t len = sizeof(creds);

	if (getsockopt(sock, SOL_SOCKET, SO_PEERCRED, &creds, &len)) {
		perror("clip_getpeereid: getsockopt");
		return -1;
	}

	*euid = creds.uid;
	*egid = creds.gid;

	return 0;
}
	
inline int
clip_sock_listen(const char *path, struct sockaddr_un *addr, mode_t mode)
{
	int s, retval;
	mode_t omode;

	memset(addr, 0, sizeof(struct sockaddr_un));
	addr->sun_family = AF_UNIX;

	if (strlen(path) > sizeof(addr->sun_path) - 1) {
		fprintf(stderr, "Path too long: %s\n", path);
		return -1;
	}
	(void) strcpy(addr->sun_path, path);

	s = socket(PF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		perror("clip_listenOnSock: socket");
		return -1;
	}
	
	(void) unlink(path);
	
	omode = umask(mode);
	retval = bind(s, (struct sockaddr *)addr, sizeof(struct sockaddr_un));
	if (retval < 0) {
		perror("clip_listenOnSock: bind");
		(void) umask(omode);
		close(s);
		return -1;
	}
	(void) umask(omode);

	retval = listen(s,0);
	if (retval < 0) {
		perror("clip_listenOnSock: listen");
		close(s);
		return -1;
	}

	return s;
}

int
clip_listenOnSock(const char *path, struct sockaddr_un *addr, mode_t mode)
{
	return clip_sock_listen(path, addr, mode);
}

/** _do_one() return code : End of file */
#define IO_EOF		 0
/** _do_one() return code : Error */
#define IO_ERROR	-1
/** _do_one() return code : Time out before polling return */
#define IO_TIMEOUT	-2

/** @internal I/O function type : read() or write() */
typedef ssize_t (*io_fun_t)(int, void *, size_t);
/** @internal _do_one() direction argument : read() */
#define DIR_IN	0
/** @internal _do_one() direction argument : write() */
#define DIR_OUT	1

/**
 * @internal Do one poll step, either read or write 
 * This calls poll once on @a s, with an optional time-out @a tmout, to
 * try to either:
 * @li read at most @a len @bytes into @a buf, if @a dir is DIR_IN 
 * @li write at most @a len @bytes from @a buf, if @a dir is DIR_OUT
 */
static inline ssize_t
_do_one(int s, char *buf, size_t len, int tmout, int dir)
{
	ssize_t slen;
	int ret;
	io_fun_t fun;
	
	struct pollfd spoll = {
		.fd = s,
	};

	if (dir == DIR_IN) {
		spoll.events = POLLIN;
		fun = read;
	} else {
		spoll.events = POLLOUT;
		fun = (io_fun_t)write;
	}
	
	ret = poll(&spoll, 1, tmout);
	if (!ret)
		return IO_TIMEOUT;

	if (ret < 0) {
		perror("poll");
		return IO_ERROR;
	}

	if (ret != 1 || (spoll.revents != spoll.events 
			&& spoll.revents != (spoll.events | POLLHUP))) {
		fputs("poll error\n", stderr);
		return IO_ERROR;
	}

	slen = fun(s, buf, len);
	if (slen < 0) {
		perror("io");
		return IO_ERROR;
	}
		
	return slen;
}

/**
 * @internal Polling loop.
 * Loop calling _do_one() until an error or EOF is encountered, or, 
 * optionnally, the maximum number of tries is reached.
 */
static inline ssize_t
_do_some(int s, char *buf, size_t len, int delay, int tries, int dir)
{
	ssize_t slen;
	int count = tries;
	size_t remaining = len;
	char *ptr = buf;

	while (remaining) {
		slen = _do_one(s, ptr, remaining, delay, dir);

		switch (slen) {
			case IO_EOF:
				return (ssize_t)(len - remaining);
				break;
			case IO_TIMEOUT:
				break;
			case IO_ERROR:
				return -1;
			default:  /* > 0 */
				ptr += slen;
				remaining -= (size_t)slen;
				break;
		}

		if (tries && --count == 0)
			break;
	}

	return (ssize_t)(len - remaining);
}

ssize_t 
clip_sock_read(int s, char *buf, size_t len, int msecs, int tries)
{
	if (msecs > 0 && !tries) {
		fprintf(stderr, "%s needs a number of tries when a timeout "
				"is specified\n", __FUNCTION__);
		return -1;
	}

	if (msecs < 0)
		return _do_some(s, buf, len, -1, tries, DIR_IN);
	if (!tries) 
		return _do_some(s, buf, len, msecs, 0, DIR_IN);

	return _do_some(s, buf, len, msecs / tries, tries, DIR_IN);
}

ssize_t
clip_sock_write(int s, char *buf, size_t len, int msecs, int tries)
{
	if (msecs > 0 && !tries) {
		fprintf(stderr, "%s needs a number of tries when a timeout "
				"is specified\n", __FUNCTION__);
		return -1;
	}

	if (msecs < 0)
		return _do_some(s, buf, len, -1, tries, DIR_OUT);
	if (!tries) 
		return _do_some(s, buf, len, 0, 0, DIR_OUT);

	return _do_some(s, buf, len, msecs / tries, tries, DIR_OUT);
}

/** 
 * @internal Helper: handle a new connection on a socket, by accepting it,
 * and calling the handler code on the connected socket.
 */
static int
_handle_connect(clip_sock_t *sock)
{
	int s_com, ret;
	socklen_t len = sizeof(sock->sau);

	s_com = accept(sock->sock, (struct sockaddr *)&(sock->sau), &len);
	if (s_com < 0) {
		fprintf(stderr, "accept error on %s: %s\n", 
					sock->name, strerror(errno));
		return -1;
	}
	
	ret = sock->handler(s_com, sock);

	(void)close(s_com);
	return ret;
}

int 
clip_accept_one(clip_sock_t *socks, size_t num, int restart)
{
	struct pollfd fds[num];
	unsigned int i;
	int ret;
	
restart:
	for (i = 0; i < num; i++) {
		fds[i].fd = socks[i].sock;
		fds[i].events = POLLIN;
		fds[i].revents = 0;
	}
	
	ret = poll(fds, num, -1);
	if (ret < 0) {
		if (errno == EINTR && restart)
			goto restart;
		perror("poll");
		return ret;
	}
	if (!ret) {
		fputs("poll returned 0??\n", stderr);
		return -1;
	}

	for (i = 0; i < num; i++) {
		if (fds[i].revents == POLLIN) {
			(void)_handle_connect(socks+i);
			continue;
		}
		if (fds[i].revents) {
			fprintf(stderr, "revents %d on desc %d\n", 
							fds[i].revents, i);
		}
	}
	
	return 0;
}

int
clip_send_fd(int s, int fd)
{
	ssize_t sret;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	struct iovec iov;
	char ctrl[CMSG_SPACE(sizeof(int))];
	char buf[] = "fd";

	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = ctrl;
	msg.msg_controllen = sizeof(ctrl);
	
	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));

	memcpy(CMSG_DATA(cmsg), &fd, sizeof(fd));

	for (;;) {
		sret = sendmsg(s, &msg, 0);
		if (sret == -1) {
			if (errno == EINTR)
				continue;
			fprintf(stderr, "sendmsg failed: %s\n", 
							strerror(errno));
			return -1;
		}
		return 0;
	}
}

int
clip_recv_fd(int s, int *fd)
{
	ssize_t sret;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	struct iovec iov;
	char ctrl[CMSG_SPACE(sizeof(int))];
	char buf[sizeof("fd")];

	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = ctrl;
	msg.msg_controllen = sizeof(ctrl);
	
	for (;;) {
		sret = recvmsg(s, &msg, 0);
		if (sret == -1) {
			if (errno == EINTR)
				continue;
			fprintf(stderr, "recvmsg failed: %s\n", 
							strerror(errno));
			return -1;
		}
		cmsg = CMSG_FIRSTHDR(&msg);
		if (cmsg) {
			memcpy(fd, CMSG_DATA(cmsg), sizeof(int));
			return 0;
		}
		return -1;
	}
}

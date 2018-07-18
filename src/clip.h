// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/**
 *  @file clip.h 
 *  Main header for libclip
 *  @author Vincent Strubel <clipos@ssi.gouv.fr>
 * 
 *  Copyright (C) 2006-2010 SGDN/DCSSI
 *  Copyright (C) 2011 SGDSN/ANSSI
 *
 *  @n
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License 
 *  version 2 as published by the Free Software Foundation.
 */


#ifndef _CLIP_H
#define _CLIP_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/un.h>

/**
 * @mainpage clip-lib documentation
 * This is the inline documentation for the clip-lib package.
 * @n
 * Public interface documentation is available directly through the 
 * clip.h reference.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 */
			/* Socket */

struct sockaddr_un;
struct clip_sock_t;

/**
 * Socket connection handler type for clip_accept_one(). This takes a 
 * socket file descriptor as argument, and returns an error code, 0
 * on success or -1 on error.
 */
typedef int (*conn_handler_t)(int, struct clip_sock_t *);

/**
 * Argument type for clip_accept_one().
 */
typedef struct clip_sock_t {
	int sock; /**< Listening socket file descriptor */
	char *name; /**< Arbitrary socket name */
	char *path; /**< Socket path */
	struct sockaddr_un sau; /**< Socket address storage */
	conn_handler_t handler; /**< Socket connection handler */
	void *private;	/**< Other uses I didn't think about */
} clip_sock_t;

/** @name Socket functions */
/*@{*/

/**
 * Get peer credentials on a UNIX socket.
 * This retrieves the peer effective uid and gid on a connected
 * UNIX socket.
 * @param s Socket descriptor.
 * @param euid Pointer to returned effective uid.
 * @param egid Pointer to returned effective gid.
 * @return 0 on success, -1 on error.
 */
extern int clip_getpeereid(int s, uid_t *euid, gid_t *egid);

/**
 * Create a file-bound UNIX socket and listen on it.
 * This creates a UNIX socket, binds it to @a path, and starts listening
 * on it.
 * @param path Path to bind the socket to.
 * @param addr Sockaddr structure to store the socket address. This must be
 * allocated by the caller, and will be set up with the actual address on 
 * successful return.
 * @param mode Umask to apply when creating the file the socket is bound to.
 * @return Positive file descriptor for the socket on success, -1 on error.
 */
extern int clip_sock_listen(const char *path, 
			struct sockaddr_un *addr, mode_t mode);
/**
 * Create a file-bound UNIX socket and listen on it.
 * This is the same function as clip_sock_listen().
 * @deprecated This function is deprecated, since it does not follow the 
 * same naming conventions as the other functions in clip-lib. Use 
 * clip_sock_listen() instead.
 */
extern int clip_listenOnSock(const char *path, 
			struct sockaddr_un *addr, mode_t mode);

/**
 * Read data on a non-blocking socket, with an optional time-out.
 * This function reads some data (at most @a len bytes) on a non-blocking
 * socket, in a certain number of tries, and with an optional time-out. 
 * It polls the socket until some data is available to read (or, optionally,
 * until a time-out occurs), then reads it into @a buf. This operation is 
 * repeated until @a len bytes have been read, or the maximum of @a tries 
 * read attempts is reached (if @a tries is non-zero).
 * @param s Socket to read on, which must be configured as non-blocking.
 * @param buf Buffer to store the read data to. This must be allocated to at 
 * least @a len bytes.
 * @param len Number of bytes to read at the most.
 * @param msecs Global timeout in milliseconds. A negative number means no 
 * time-out. If @a msecs is not negative, @a tries must be one or more, since 
 * each poll step will time-out after @a msecs / @a tries milliseconds.
 * @param tries Maximum number of poll steps. This must be one or more if a 
 * finite time-out is passed as @a msecs.
 * @return Actual number of bytes read before time-out (between 0 and len, 
 * included) on success, -1 on error.
 */
extern ssize_t clip_sock_read(int s, char *buf, size_t len, 
					int msecs, int tries);

/**
 * Write data on a non-blocking socket, with an optional time-out.
 * This function writes some data (at most @a len bytes) on a non-blocking
 * socket, in a certain number of tries, and with an optional time-out. 
 * It polls the socket until writing to it is possible (or, optionally,
 * until a time-out occurs), then writes on it from @a buf. This operation is 
 * repeated until @a len bytes have been written, or the maximum of @a tries 
 * write attempts is reached (if @a tries is non-zero).
 * @param s Socket to write to, which must be configured as non-blocking.
 * @param buf Buffer to read the data to be written from. This must contain at 
 * least @a len bytes of valid data.
 * @param len Number of bytes to write at the most.
 * @param msecs Global timeout in milliseconds. A negative number means no 
 * time-out. If @a msecs is not negative, @a tries must be one or more, since 
 * each poll step will time-out after @a msecs / @a tries milliseconds.
 * @param tries Maximum number of poll steps. This must be one or more if a 
 * finite time-out is passed as @a msecs.
 * @return Actual number of bytes written before time-out (between 0 and len, 
 * included) on success, -1 on error.
 */
extern ssize_t clip_sock_write(int s, char *buf, size_t len, 
					int msecs, int tries);

/**
 * Wait for at least one connection on a set of sockets, and handle those
 * connections.
 * This function polls a set of listening sockets, waiting for them to 
 * receive an incoming connection. As soon as it receives at least one
 * such connection, it accepts the connection, and handles it by calling the 
 * corresponding clip_sock_t socket handler on the connected socket.
 * then returns. If connection attempts are received on several sockets
 * at the same time, all are accepted and handled by their respective 
 * connection handlers. Only the first connection attempt is handled on 
 * each socket.
 * @param socks Array of clip_sock_t socket description. All must be 
 * completely defined, with their @a sock descriptor associated to a
 * listening socket.
 * @param num Number of valid clip_sock_t entries in the @a socks array.
 * @return 0 on success, -1 on error. Note that, for the time being, 
 * the errors returned by the individual conn_handler_t calls are ignored.
 */
extern int clip_accept_one(clip_sock_t *socks, size_t num, int restart);

/**
 * Send a file descriptor over a UNIX socket.
 * @param s Socket to send over.
 * @param fd File descriptor to send.
 * @return 0 on success, -1 on error.
 */
extern int clip_send_fd(int s, int fd);

/**
 * Receive a file descriptor over a UNIX socket.
 * @param s Socket to receive over.
 * @param fd Where to store the received file descriptor.
 * @return 0 on success, -1 on error.
 */
extern int clip_recv_fd(int s, int *fd);

/*@}*/


			/* Passwd */
/** @name User management functions */
/*@{*/

struct passwd;
/**
 * Check if a user is a member of a group.
 * This function tests if a user is a member of the @a grp group,
 * either through her main group or her supplementary groups.
 * @param pwd The struct passwd for the user to test.
 * @param grp The name of the group to test.
 * @return 1 if the user is a member of @a grp, 0 if she is not, and
 * -1 if an error was encountered.
 */
extern int clip_checkgroup(const struct passwd *pwd, const char *grp);

/*@}*/

			/* Unistd */

/** @name File system related functions */
/*@{*/

/**
 * Close all open file descriptors.
 * This tries to close all file descriptors in the calling process file
 * descriptor table, except @a stdin, @a stdout and @a stderr unless
 * @a std_p is non-zero. This is typically called before chroot()ing
 * or daemonizing a process.
 * @param std_p If non-zero, @a stdin, @a stdout and @a stderr will
 * be closed as well.
 * @return 0 on success, -1 on error (a file descriptor that was actually
 * open could not be closed).
 */
extern int clip_closeall(int std_p);

/** 
 * Close all open file descriptors, and re-open standard input and outputs.
 * This function closes all open file descriptors for the calling process, then
 * optionally reopens a file as standard input and / or a file as standard
 * output and error output. It also chdir()'s to the root directory, to avoid
 * keeping an invalid CWD.
 * @param stdin_path Path to the file to open as @a stdin. If left NULL, no 
 * such file will be opened. Note that symbolic links will not be followed.
 * @param stdout_path Path to the file to open as both @a stdout and @a stderr.
 * If left NULL, no such file will be opened. Note that symbolic will not be
 * followed.
 * @return 0 on success, -1 on error.
 */
extern int clip_reset_fds(const char *stdin_path, const char *stdout_path);

/**
 * Close all file descriptors and execute in a new session.
 * This function forks a new process in which all file descriptors are
 * closed, including standard input and outputs, and a new process session 
 * created. On success, the function returns in this new process, with a
 * umask set to 0077 and CWD to '/', and @a /dev/null re-opened as standard
 * input and outputs. The original process exits before returning,
 * and the child process is reparented to init.
 * @return 0 on success, -1 on error. This error can be returned either in 
 * the original process, if the failure was encountered before or at the fork()
 * call, or in the new process if the failure was after the fork. In the latter
 * case, the original process still exits normally before the function returns.
 */
extern int clip_fork(void);

/**
 * Daemonize a process. 
 * This function closes all file descriptor and creates a new session, just
 * like clip_fork(), then re-forks the process and lets the new session leader
 * process exit, to detach the child process from any controlling terminal.
 * @return 0 on sucess, in the detached child process, -1 on error. This error
 * can be returned in either the original process, or the intermediate one 
 * (new session leader), in which case the original process exits normally 
 * before returning.
 */
extern int clip_daemonize(void);

/**
 * Chroot wrapper.
 * This calls chdir(@a path), then chroot("."), in that order.
 * @param path Path to chroot to.
 * @return 0 on success, -1 on error (possibly with a CWD changed to 
 * @a path).
 */
extern int clip_chroot(const char* path);

/*@}*/

			/* Capabilities */
/** @name Privilege management functions */
/*@{*/

/** 
 * Reduce current capabilities to a maximal mask
 * This intersects all three POSIX capabilities masks (effective, permitted,
 * inherited) of the calling process with a common authorized mask.
 * @param mask Authorized capabilities mask. All capabilities not present
 * in this mask are removed from the calling process's masks.
 * @return 0 on success, -1 on error.
 */
extern long int clip_reducecaps(uint32_t mask);

/**
 * Change uid, gid, and supplementary groups, while keeping some capabilities.
 * This function changes the calling process's uids (uid, euid, suid), and / or
 * gids (gid, egid, sgid) and / or supplementary groups, while keeping some
 * POSIX capabilities.
 * @param uid New uids (uid, euid, suid) for the process, if non-zero. If zero,
 * the caller's uids are not changed.
 * @param gid New gids (gid, egid, sgid) for the process, if non-zero. If zero,
 * the caller's gids are not changed.
 * @param grouplist New supplementary groups array for the process, if not NULL.
 * If NULL, the caller's supplementary groups are not changed.
 * @groupsize Number of significant entries in @a grouplist.
 * @param keepcaps Mask of capabilities to keep through ID changes.
 * @return 0 on success, -1 on error.
 */
extern int clip_revokeprivs(uid_t uid, gid_t gid, const gid_t *grouplist, 
					size_t groupsize, uint32_t keepcaps);

/*@}*/

#endif /* _CLIP_H */

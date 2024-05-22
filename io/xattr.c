// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Red Hat, Inc.
 * All Rights Reserved.
 */

#include "xattr.h"
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define SPECIAL_FILE(x) \
	   (S_ISCHR((x)) \
	|| S_ISBLK((x)) \
	|| S_ISFIFO((x)) \
	|| S_ISLNK((x)) \
	|| S_ISSOCK((x)))

int
get_fsxattrat(
	const int		dfd,
	const char		*path,
	const struct stat	*stat,
	struct fsxattr		*fsx)
{
	int			error;
	int			fd;
	struct fsxattrat	xreq = {
		.fsx = { 0 },
		.dfd = dfd,
		.atfd = 0,
	};

	if (SPECIAL_FILE(stat->st_mode)) {
		xreq.atfd = open(path, O_PATH | O_NOFOLLOW);
		if (xreq.atfd == -1)
			return errno;

		error = ioctl(dfd, FS_IOC_FSGETXATTRAT, &xreq);
		if (error)
			return error;

		memcpy(fsx, &xreq.fsx, sizeof(struct fsxattr));
		return error;
	}

	fd = open(path, O_RDONLY|O_NOCTTY);
	if (fd == -1)
		return errno;

	error = ioctl(fd, FS_IOC_FSGETXATTR, fsx);
	close(fd);

	return error;
}

int
set_fsxattrat(
	const int		dfd,
	const char		*path,
	const struct stat	*stat,
	struct fsxattr		*fsx)
{
	int			error;
	int			fd;
	struct fsxattrat	xreq = {
		.fsx = *fsx, /* struct copy */
		.dfd = dfd,
		.atfd = 0,
	};

	if (SPECIAL_FILE(stat->st_mode)) {
		xreq.atfd = open(path, O_PATH | O_NOFOLLOW);
		if (xreq.atfd == -1)
			return errno;
		return ioctl(dfd, FS_IOC_FSSETXATTRAT, &xreq);
	}

	fd = open(path, O_RDONLY|O_NOCTTY);
	if (fd == -1)
		return errno;

	error = ioctl(fd, FS_IOC_FSSETXATTR, fsx);
	close(fd);

	return error;
}

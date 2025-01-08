// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Red Hat, Inc.
 * All Rights Reserved.
 */

#include "ixattr.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/syscall.h>
#include <asm/types.h>
#include <fcntl.h>

#define SPECIAL_FILE(x) \
	   (S_ISCHR((x)) \
	|| S_ISBLK((x)) \
	|| S_ISFIFO((x)) \
	|| S_ISLNK((x)) \
	|| S_ISSOCK((x)))

int
getfsxattrat(
	const int		dfd,
	const char		*path,
	const struct stat	*stat,
	struct fsxattr		*fsx,
	const unsigned int	at_flags)
{
	int			error;
	int			fd;

	if (SPECIAL_FILE(stat->st_mode)) {
#ifdef HAVE_FSXATTRAT
		return syscall(__NR_getfsxattrat, dfd, path, fsx,
				sizeof(struct fsxattr), at_flags);
#else
		return 0;
#endif
	}

	fd = open(path, O_RDONLY|O_NOCTTY);
	if (fd == -1)
		return errno;

	error = ioctl(fd, FS_IOC_FSGETXATTR, fsx);
	close(fd);

	return error;
}

int
setfsxattrat(
	const int		dfd,
	const char		*path,
	const struct stat	*stat,
	struct fsxattr		*fsx,
	const unsigned int	at_flags)
{
	int			error;
	int			fd;

	if (SPECIAL_FILE(stat->st_mode)) {
#ifdef HAVE_FSXATTRAT
		/* For special files we can only use FS_XFLAG_PROJINHERIT */
		fsx->fsx_xflags &= FS_XFLAG_PROJINHERIT;
		return syscall(__NR_setfsxattrat, dfd, path, fsx,
				sizeof(struct fsxattr), at_flags);
#else
		return 0;
#endif
	}

	fd = open(path, O_RDONLY|O_NOCTTY);
	if (fd == -1)
		return errno;

	error = ioctl(fd, FS_IOC_FSSETXATTR, fsx);
	close(fd);

	return error;
}

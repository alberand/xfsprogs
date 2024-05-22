// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Red Hat, Inc.
 * All Rights Reserved.
 */

#include "linux.h"
#include <asm/types.h>
#include <fcntl.h>

#ifndef FS_IOC_FSGETXATTRAT
/*
 * Structure passed to FS_IOC_FSGETXATTRAT/FS_IOC_FSSETXATTRAT
 */
struct fsxattrat {
	struct fsxattr	fsx;		/* XATTR to get/set */
	__u32		dfd;		/* parent dir */
	__u32		atfd;
};

#define FS_IOC_FSGETXATTRAT   _IOR ('X', 33, struct fsxattrat)
#define FS_IOC_FSSETXATTRAT   _IOW ('X', 34, struct fsxattrat)
#endif

int
get_fsxattrat(const int dfd, const char *path, const struct stat *stat,
		struct fsxattr *fsx);

int
set_fsxattrat(const int dfd, const char *path, const struct stat *stat,
		struct fsxattr *fsx);

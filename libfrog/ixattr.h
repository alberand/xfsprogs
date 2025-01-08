// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Red Hat, Inc.
 * All Rights Reserved.
 */
#ifndef __LIBFROG_IXATTR_H__
#define __LIBFROG_IXATTR_H__

#include "linux.h"
#include <sys/stat.h>

int
getfsxattrat(
	const int		dfd,
	const char		*path,
	const struct stat	*stat,
	struct fsxattr		*fsx,
	const unsigned int	at_flags);

int
setfsxattrat(
	const int		dfd,
	const char		*path,
	const struct stat	*stat,
	struct fsxattr		*fsx,
	const unsigned int	at_flags);

#endif /* __LIBFROG_IXATTR_H__ */

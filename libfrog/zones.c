// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025, Western Digital Corporation or its affiliates.
 */
#include "platform_defs.h"
#include "libfrog/zones.h"
#include <sys/ioctl.h>

/* random size that allows efficient processing */
#define ZONES_PER_REPORT		16384

struct xfrog_zone_report *
xfrog_report_zones(
	int			fd,
	uint64_t		sector,
	struct xfrog_zone_report *rep)
{
	if (!rep) {
		rep = calloc(1, struct_size(rep, zones, ZONES_PER_REPORT));
		if (!rep) {
			fprintf(stderr, "%s\n",
_("Failed to allocate memory for reporting zones."));
			return NULL;
		}
	}

	rep->rep.sector = sector;
	rep->rep.nr_zones = ZONES_PER_REPORT;

	if (ioctl(fd, BLKREPORTZONE, &rep->rep)) {
		fprintf(stderr, "%s %s\n",
_("ioctl(BLKREPORTZONE) failed:\n"),
			strerror(-errno));
		free(rep);
		return NULL;
	}

	return rep;
}

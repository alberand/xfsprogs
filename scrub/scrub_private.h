// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2021-2023 Oracle.  All Rights Reserved.
 * Author: Darrick J. Wong <darrick.wong@oracle.com>
 */
#ifndef XFS_SCRUB_SCRUB_PRIVATE_H_
#define XFS_SCRUB_SCRUB_PRIVATE_H_

/* Shared code between scrub.c and repair.c. */

int format_scrub_descr(struct scrub_ctx *ctx, char *buf, size_t buflen,
		void *where);

/* Predicates for scrub flag state. */

static inline bool is_corrupt(struct xfs_scrub_metadata *sm)
{
	return sm->sm_flags & XFS_SCRUB_OFLAG_CORRUPT;
}

static inline bool is_unoptimized(struct xfs_scrub_metadata *sm)
{
	return sm->sm_flags & XFS_SCRUB_OFLAG_PREEN;
}

static inline bool xref_failed(struct xfs_scrub_metadata *sm)
{
	return sm->sm_flags & XFS_SCRUB_OFLAG_XFAIL;
}

static inline bool xref_disagrees(struct xfs_scrub_metadata *sm)
{
	return sm->sm_flags & XFS_SCRUB_OFLAG_XCORRUPT;
}

static inline bool is_incomplete(struct xfs_scrub_metadata *sm)
{
	return sm->sm_flags & XFS_SCRUB_OFLAG_INCOMPLETE;
}

static inline bool is_suspicious(struct xfs_scrub_metadata *sm)
{
	return sm->sm_flags & XFS_SCRUB_OFLAG_WARNING;
}

/* Should we fix it? */
static inline bool needs_repair(struct xfs_scrub_metadata *sm)
{
	return is_corrupt(sm) || xref_disagrees(sm);
}

void scrub_warn_incomplete_scrub(struct scrub_ctx *ctx, struct descr *dsc,
		struct xfs_scrub_metadata *meta);

/* Scrub item functions */

static inline void
scrub_item_save_state(
	struct scrub_item		*sri,
	unsigned  int			scrub_type,
	unsigned  int			scrub_flags)
{
	sri->sri_state[scrub_type] = scrub_flags & SCRUB_ITEM_REPAIR_ANY;
}

static inline void
scrub_item_clean_state(
	struct scrub_item		*sri,
	unsigned  int			scrub_type)
{
	sri->sri_state[scrub_type] = 0;
}

static inline bool
scrub_item_type_boosted(
	struct scrub_item		*sri,
	unsigned  int			scrub_type)
{
	return sri->sri_state[scrub_type] & SCRUB_ITEM_BOOST_REPAIR;
}

#endif /* XFS_SCRUB_SCRUB_PRIVATE_H_ */

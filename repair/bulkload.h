/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020 Oracle.  All Rights Reserved.
 * Author: Darrick J. Wong <darrick.wong@oracle.com>
 */
#ifndef __XFS_REPAIR_BULKLOAD_H__
#define __XFS_REPAIR_BULKLOAD_H__

extern int bload_leaf_slack;
extern int bload_node_slack;

struct repair_ctx {
	struct xfs_mount	*mp;
	struct xfs_inode	*ip;
	struct xfs_trans	*tp;
};

struct bulkload_resv {
	/* Link to list of extents that we've reserved. */
	struct list_head	list;

	struct xfs_perag	*pag;

	/* FSB of the block we reserved. */
	xfs_fsblock_t		fsbno;

	/* Length of the reservation. */
	xfs_extlen_t		len;

	/* How much of this reservation we've used. */
	xfs_extlen_t		used;
};

struct bulkload {
	struct repair_ctx	*sc;

	/* List of extents that we've reserved. */
	struct list_head	resv_list;

	/* Fake root for new btree. */
	union {
		struct xbtree_afakeroot	afake;
		struct xbtree_ifakeroot	ifake;
	};

	/* rmap owner of these blocks */
	struct xfs_owner_info	oinfo;

	/* The last reservation we allocated from. */
	struct bulkload_resv	*last_resv;

	/* Hint as to where we should allocate blocks. */
	xfs_fsblock_t		alloc_hint;

	/* Number of blocks reserved via resv_list. */
	unsigned int		nr_reserved;
};

#define for_each_bulkload_reservation(bkl, resv, n)	\
	list_for_each_entry_safe((resv), (n), &(bkl)->resv_list, list)

void bulkload_init_ag(struct bulkload *bkl, struct repair_ctx *sc,
		const struct xfs_owner_info *oinfo, xfs_fsblock_t alloc_hint);
void bulkload_init_inode(struct bulkload *bkl, struct repair_ctx *sc,
		int whichfork, const struct xfs_owner_info *oinfo);
int bulkload_add_blocks(struct bulkload *bkl, xfs_fsblock_t fsbno,
		xfs_extlen_t len);
int bulkload_alloc_blocks(struct bulkload *bkl, uint64_t nr_blocks);
void bulkload_destroy(struct bulkload *bkl, int error);
int bulkload_claim_block(struct xfs_btree_cur *cur, struct bulkload *bkl,
		union xfs_btree_ptr *ptr);
void bulkload_estimate_ag_slack(struct repair_ctx *sc,
		struct xfs_btree_bload *bload, unsigned int free);

#endif /* __XFS_REPAIR_BULKLOAD_H__ */

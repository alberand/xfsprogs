// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022-2023 Oracle, Inc.
 * All rights reserved.
 */
#include "libxfs_priv.h"
#include "xfs_shared.h"
#include "xfs_format.h"
#include "xfs_log_format.h"
#include "xfs_trans_resv.h"
#include "xfs_mount.h"
#include "xfs_inode.h"
#include "xfs_trans_resv.h"
#include "xfs_mount.h"
#include "xfs_trace.h"
#include "xfs.h"
#include "xfs_fs.h"
#include "xfs_da_format.h"
#include "xfs_bmap_btree.h"
#include "xfs_trans.h"
#include "xfs_da_btree.h"
#include "xfs_attr.h"
#include "xfs_dir2.h"
#include "xfs_dir2_priv.h"
#include "xfs_attr_sf.h"
#include "xfs_bmap.h"
#include "xfs_parent.h"
#include "xfs_da_format.h"
#include "xfs_format.h"
#include "xfs_trans_space.h"
#include "xfs_health.h"

struct kmem_cache		*xfs_parent_intent_cache;

/*
 * Parent pointer attribute handling.
 *
 * Because the attribute value is a filename component, it will never be longer
 * than 255 bytes. This means the attribute will always be a local format
 * attribute as it is xfs_attr_leaf_entsize_local_max() for v5 filesystems will
 * always be larger than this (max is 75% of block size).
 *
 * Creating a new parent attribute will always create a new attribute - there
 * should never, ever be an existing attribute in the tree for a new inode.
 * ENOSPC behavior is problematic - creating the inode without the parent
 * pointer is effectively a corruption, so we allow parent attribute creation
 * to dip into the reserve block pool to avoid unexpected ENOSPC errors from
 * occurring.
 */

/* Return true if parent pointer EA name is valid. */
bool
xfs_parent_namecheck(
	struct xfs_mount			*mp,
	const struct xfs_parent_name_rec	*rec,
	size_t					reclen,
	unsigned int				attr_flags)
{
	xfs_ino_t				p_ino;

	if (!(attr_flags & XFS_ATTR_PARENT))
		return false;

	/* pptr updates use logged xattrs, so we should never see this flag */
	if (attr_flags & XFS_ATTR_INCOMPLETE)
		return false;

	if (reclen != sizeof(struct xfs_parent_name_rec))
		return false;

	/* Only one namespace bit allowed. */
	if (hweight32(attr_flags & XFS_ATTR_NSP_ONDISK_MASK) > 1)
		return false;

	p_ino = be64_to_cpu(rec->p_ino);
	if (!xfs_verify_ino(mp, p_ino))
		return false;

	return true;
}

/* Return true if parent pointer EA value is valid. */
bool
xfs_parent_valuecheck(
	struct xfs_mount		*mp,
	const void			*value,
	size_t				valuelen)
{
	if (valuelen == 0 || valuelen > XFS_PARENT_DIRENT_NAME_MAX_SIZE)
		return false;

	if (value == NULL)
		return false;

	/* Valid dirent name? */
	if (!xfs_dir2_namecheck(value, valuelen))
		return false;

	return true;
}

/* Initializes a xfs_parent_name_rec to be stored as an attribute name. */
static inline void
xfs_init_parent_name_rec(
	struct xfs_parent_name_rec	*rec,
	const struct xfs_inode		*dp,
	const struct xfs_name		*name,
	struct xfs_inode		*ip)
{
	rec->p_ino = cpu_to_be64(dp->i_ino);
	rec->p_gen = cpu_to_be32(VFS_IC(dp)->i_generation);
	rec->p_namehash = cpu_to_be32(xfs_dir2_hashname(dp->i_mount, name));
}

/* Point the da args value fields at the non-key parts of a parent pointer. */
static inline void
xfs_init_parent_davalue(
	struct xfs_da_args		*args,
	const struct xfs_name		*name)
{
	args->valuelen = name->len;
	args->value = (void *)name->name;
}

/*
 * Allocate memory to control a logged parent pointer update as part of a
 * dirent operation.
 */
int
__xfs_parent_init(
	struct xfs_mount		*mp,
	struct xfs_parent_defer		**parentp)
{
	struct xfs_parent_defer		*parent;

	parent = kmem_cache_zalloc(xfs_parent_intent_cache, GFP_KERNEL);
	if (!parent)
		return -ENOMEM;

	/* init parent da_args */
	parent->args.geo = mp->m_attr_geo;
	parent->args.whichfork = XFS_ATTR_FORK;
	parent->args.attr_filter = XFS_ATTR_PARENT;
	parent->args.op_flags = XFS_DA_OP_OKNOENT | XFS_DA_OP_LOGGED |
				XFS_DA_OP_NVLOOKUP;
	parent->args.name = (const uint8_t *)&parent->rec;
	parent->args.namelen = sizeof(struct xfs_parent_name_rec);

	*parentp = parent;
	return 0;
}

static inline xfs_dahash_t
xfs_parent_hashname(
	struct xfs_inode		*ip,
	const struct xfs_parent_defer	*parent)
{
	return xfs_da_hashname((const void *)&parent->rec,
			sizeof(struct xfs_parent_name_rec));
}

/* Add a parent pointer to reflect a dirent addition. */
int
xfs_parent_add(
	struct xfs_trans	*tp,
	struct xfs_parent_defer	*parent,
	struct xfs_inode	*dp,
	const struct xfs_name	*parent_name,
	struct xfs_inode	*child)
{
	struct xfs_da_args	*args = &parent->args;

	xfs_init_parent_name_rec(&parent->rec, dp, parent_name, child);
	args->hashval = xfs_parent_hashname(dp, parent);

	args->trans = tp;
	args->dp = child;

	xfs_init_parent_davalue(&parent->args, parent_name);

	return xfs_attr_defer_add(args);
}

/* Remove a parent pointer to reflect a dirent removal. */
int
xfs_parent_remove(
	struct xfs_trans	*tp,
	struct xfs_parent_defer	*parent,
	struct xfs_inode	*dp,
	const struct xfs_name	*parent_name,
	struct xfs_inode	*child)
{
	struct xfs_da_args	*args = &parent->args;

	/*
	 * For regular attrs, removing an attr from a !hasattr inode is a nop.
	 * For parent pointers, we require that the pointer must exist if the
	 * caller wants us to remove the pointer.
	 */
	if (XFS_IS_CORRUPT(child->i_mount, !xfs_inode_hasattr(child))) {
		xfs_inode_mark_sick(child, XFS_SICK_INO_PARENT);
		return -EFSCORRUPTED;
	}

	xfs_init_parent_name_rec(&parent->rec, dp, parent_name, child);
	args->hashval = xfs_parent_hashname(dp, parent);

	args->trans = tp;
	args->dp = child;

	xfs_init_parent_davalue(&parent->args, parent_name);

	return xfs_attr_defer_remove(args);
}

/* Cancel a parent pointer operation. */
void
__xfs_parent_cancel(
	struct xfs_mount	*mp,
	struct xfs_parent_defer	*parent)
{
	kmem_cache_free(xfs_parent_intent_cache, parent);
}

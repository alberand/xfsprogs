/*
 * Copyright (c) 2012 Red Hat, Inc. All rights reserved.
 */
#ifndef __XFS_SYMLINK_H
#define __XFS_SYMLINK_H 1

#define XFS_SYMLINK_MAGIC	0x58534c4d	/* XSLM */

struct xfs_dsymlink_hdr {
	__be32	sl_magic;
	__be32	sl_offset;
	__be32	sl_bytes;
	__be32	sl_crc;
	uuid_t	sl_uuid;
	__be64	sl_owner;
	__be64	sl_blkno;
	__be64	sl_lsn;
};

/*
 * The maximum pathlen is 1024 bytes. Since the minimum file system
 * blocksize is 512 bytes, we can get a max of 3 extents back from
 * bmapi when crc headers are taken into account.
 */
#define XFS_SYMLINK_MAPS 3

#define XFS_SYMLINK_BUF_SPACE(mp, bufsize)	\
	((bufsize) - (xfs_sb_version_hascrc(&(mp)->m_sb) ? \
			sizeof(struct xfs_dsymlink_hdr) : 0))

int xfs_symlink_blocks(struct xfs_mount *mp, int pathlen);

extern const struct xfs_buf_ops xfs_symlink_buf_ops;

#ifdef __KERNEL__

int xfs_symlink(struct xfs_inode *dp, struct xfs_name *link_name,
		const char *target_path, umode_t mode, struct xfs_inode **ipp);
int xfs_readlink(struct xfs_inode *ip, char *link);
int xfs_inactive_symlink_rmt(struct xfs_inode *ip, struct xfs_trans **tpp);

#endif /* __KERNEL__ */
#endif /* __XFS_SYMLINK_H */

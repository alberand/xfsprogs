/*
 * Copyright (c) 2005 Silicon Graphics, Inc.  All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Further, this software is distributed without any warranty that it is
 * free of the rightful claim of any third person regarding infringement
 * or the like.  Any license provided herein, whether implied or
 * otherwise, applies only to this software file.  Patent licenses, if
 * any, provided herein do not apply to combinations of this program with
 * other software, or any other product whatsoever.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * Contact information: Silicon Graphics, Inc., 1600 Amphitheatre Pkwy,
 * Mountain View, CA  94043, or:
 *
 * http://www.sgi.com
 *
 * For further information regarding this notice, see:
 *
 * http://oss.sgi.com/projects/GenInfo/SGIGPLNoticeExplan/
 */

#include <xfs/command.h>
#include "init.h"
#include "quota.h"

static cmdinfo_t free_cmd;

static void
free_help(void)
{
	printf(_(
"\n"
" reports the number of free disk blocks and inodes\n"
"\n"
" This command reports the number of total, used, and available disk blocks.\n"
" It can optionally report the same set of numbers for inodes and realtime\n"
" disk blocks, and will report on all known XFS filesystem mount points and\n"
" project quota paths by default (see 'print' command for a list).\n"
" -b -- report the block count values\n"
" -i -- report the inode count values\n"
" -r -- report the realtime block count values\n"
" -h -- report in a human-readable format\n"
" -N -- suppress the header from the output\n"
"\n"));
}

static int
mount_free_space_data(
	struct fs_path		*mount,
	__uint64_t		*bcount,
	__uint64_t		*bused,
	__uint64_t		*bfree,
	__uint64_t		*icount,
	__uint64_t		*iused,
	__uint64_t		*ifree,
	__uint64_t		*rcount,
	__uint64_t		*rused,
	__uint64_t		*rfree)
{
	struct xfs_fsop_counts	fscounts;
	struct xfs_fsop_geom	fsgeo;
	struct statfs		st;
	__uint64_t		logsize, count, free;
	int			fd;

	if ((fd = open(mount->fs_dir, O_RDONLY)) < 0) {
		fprintf(stderr, "%s: cannot open %s: %s\n",
			progname, mount->fs_dir, strerror(errno));
		return 0;
	}

	if (platform_fstatfs(fd, &st) < 0) {
		perror("fstatfs");
		close(fd);
		return 0;
	}
	if ((xfsctl(mount->fs_dir, fd, XFS_IOC_FSGEOMETRY_V1, &fsgeo)) < 0) {
		perror("XFS_IOC_FSGEOMETRY_V1");
		close(fd);
		return 0;
	}
	if ((xfsctl(mount->fs_dir, fd, XFS_IOC_FSCOUNTS, &fscounts)) < 0) {
		perror("XFS_IOC_FSCOUNTS");
		close(fd);
		return 0;
	}

	logsize = fsgeo.logstart ? fsgeo.logblocks : 0;
	count = (fsgeo.datablocks - logsize) * fsgeo.blocksize;
	free  = fscounts.freedata * fsgeo.blocksize;
	*bcount = BTOBB(count);
	*bfree  = BTOBB(free);
	*bused  = BTOBB(count - free);

	*icount = st.f_files;
	*ifree  = st.f_ffree;
	*iused  = st.f_files - st.f_ffree;

	count = fsgeo.rtextents * fsgeo.rtextsize;
	free  = fscounts.freertx * fsgeo.rtextsize;
	*rcount = BTOBB(count);
	*rfree  = BTOBB(free);
	*rused  = BTOBB(count - free);

	close(fd);
	return 1;
}

static int
projects_free_space_data(
	struct fs_path		*path,
	__uint64_t		*bcount,
	__uint64_t		*bused,
	__uint64_t		*bfree,
	__uint64_t		*icount,
	__uint64_t		*iused,
	__uint64_t		*ifree,
	__uint64_t		*rcount,
	__uint64_t		*rused,
	__uint64_t		*rfree)
{
	fs_disk_quota_t		d;
	struct fsxattr		fsx;
	__uint32_t		projid;
	uint			type = XFS_PROJ_QUOTA;
	char			*dev = path->fs_name;
	int			fd;

	if ((fd = open(path->fs_dir, O_RDONLY)) < 0) {
		fprintf(stderr, "%s: cannot open %s: %s\n",
			progname, path->fs_dir, strerror(errno));
		return 0;
	}

	if ((xfsctl(path->fs_dir, fd, XFS_IOC_FSGETXATTR, &fsx)) < 0) {
		perror("XFS_IOC_FSGETXATTR");
		close(fd);
		return 0;
	}
	if (!(fsx.fsx_xflags & XFS_XFLAG_PROJINHERIT)) {
		fprintf(stderr, _("%s: project quota flag not set on %s\n"),
			progname, path->fs_dir);
		close(fd);
		return 0;
	}

	if ((getprojid(path->fs_dir, fd, &projid)) < 0) {
		close(fd);
		return 0;
	}
	if (path->fs_prid != projid) {
		fprintf(stderr,
			_("%s: project ID %u (%s) doesn't match ID %u (%s)\n"),
			progname, path->fs_prid, projects_file,
			projid, path->fs_dir);
		close(fd);
		return 0;
	}

	if (xfsquotactl(XFS_GETQUOTA, dev, type, projid, (void *)&d) < 0) {
		perror("XFS_GETQUOTA");
		close(fd);
		return 0;
	}

	/* If no softlimit is set for any of blk/ino/rt, get actual usage */
	if (!d.d_blk_softlimit || !d.d_ino_softlimit || !d.d_rtb_softlimit) {
		mount_free_space_data(path, bcount, bused, bfree,
				icount, iused, ifree, rcount, rused, rfree);
	}

	if (d.d_blk_softlimit) {
		*bcount = d.d_blk_softlimit << 1;
		*bfree = (d.d_blk_softlimit - d.d_bcount) << 1;
	}
	*bused = d.d_bcount << 1;
	if (d.d_ino_softlimit) {
		*icount = d.d_ino_softlimit;
		*ifree = (d.d_ino_softlimit - d.d_icount);
	}
	*iused = d.d_icount;
	if (d.d_rtb_softlimit) {
		*rcount = d.d_rtb_softlimit << 1;
		*rfree = (d.d_rtb_softlimit - d.d_rtbcount) << 1;
	}
	*rcount = d.d_rtbcount << 1;

	close(fd);
	return 1;
}

static int
free_space(
	FILE		*fp,
	uint		form,
	fs_path_t	*path,
	uint		flags)
{
	__uint64_t	bcount, bused, bfree;
	__uint64_t	icount, iused, ifree;
	__uint64_t	rcount, rused, rfree;
	char		a[8], s[8], u[8], p[8];
	int		count;

	count = (path->fs_flags & FS_PROJECT_PATH) ?
		projects_free_space_data(path, &bcount, &bused, &bfree,
						&icount, &iused, &ifree,
						&rcount, &rused, &rfree) :
		mount_free_space_data(path, &bcount, &bused, &bfree,
						&icount, &iused, &ifree,
						&rcount, &rused, &rfree);
	if (!count)
		return 0;

	if (!(flags & NO_HEADER_FLAG)) {
		fprintf(fp, (flags & HUMAN_FLAG) ?
			_("Filesystem  ") : _("Filesystem          "));
		if (form & (XFS_BLOCK_QUOTA|XFS_RTBLOCK_QUOTA))
			fprintf(fp, (flags & HUMAN_FLAG) ?
				_("   Size   Used  Avail Use%%") :
				_(" 1K-blocks       Used  Available  Use%%"));
		else if (form & XFS_INODE_QUOTA)
			fprintf(fp, (flags & HUMAN_FLAG) ?
				_(" Inodes   Used   Free Use%%") :
				_("    Inodes      IUsed      IFree IUse%%"));
		fprintf(fp, _(" Pathname\n"));
	}

	if (flags & HUMAN_FLAG) {
		count = fprintf(fp, "%-12s", path->fs_name);
		if (count > 13)
			fprintf(fp, "\n%12s", " ");
	} else {
		count = fprintf(fp, "%-19s", path->fs_name);
		if (count > 20)
			fprintf(fp, "\n%19s", " ");
	}

	if (form & XFS_BLOCK_QUOTA) {
		if (flags & HUMAN_FLAG)
			fprintf(fp, " %6s %6s %6s %3s%%",
				bbs_to_string(bcount, s, sizeof(s)),
				bbs_to_string(bused, u, sizeof(u)),
				bbs_to_string(bfree, a, sizeof(a)),
				pct_to_string(bused, bcount, p, sizeof(p)));
		else
			fprintf(fp, " %10llu %10llu %10llu  %3s%%",
				(unsigned long long)bcount >> 1,
				(unsigned long long)bused >> 1,
				(unsigned long long)bfree >> 1,
				pct_to_string(bused, bcount, p, sizeof(p)));
	} else if (form & XFS_INODE_QUOTA) {
		if (flags & HUMAN_FLAG)
			fprintf(fp, " %6s %6s %6s %3s%%",
				num_to_string(icount, s, sizeof(s)),
				num_to_string(iused, u, sizeof(u)),
				num_to_string(ifree, a, sizeof(a)),
				pct_to_string(iused, icount, p, sizeof(p)));
		else
			fprintf(fp, " %10llu %10llu %10llu  %3s%%",
				(unsigned long long)icount,
				(unsigned long long)iused,
				(unsigned long long)ifree,
				pct_to_string(iused, icount, p, sizeof(p)));
	} else if (form & XFS_RTBLOCK_QUOTA) {
		if (flags & HUMAN_FLAG)
			fprintf(fp, " %6s %6s %6s %3s%%",
				bbs_to_string(rcount, s, sizeof(s)),
				bbs_to_string(rused, u, sizeof(u)),
				bbs_to_string(rfree, a, sizeof(a)),
				pct_to_string(rused, rcount, p, sizeof(p)));
		else
			fprintf(fp, " %10llu %10llu %10llu  %3s%%",
				(unsigned long long)rcount >> 1,
				(unsigned long long)rused >> 1,
				(unsigned long long)rfree >> 1,
				pct_to_string(rused, rcount, p, sizeof(p)));
	}
	fprintf(fp, " %s\n", path->fs_dir);
	return 1;
}

static void
free_space_list(
	FILE			*fp,
	uint			form,
	uint			type,
	char			*dir,
	uint			flags)
{
	fs_cursor_t		cursor;
	fs_path_t		*path;

	fs_cursor_initialise(dir, type, &cursor);
	while ((path = fs_cursor_next_entry(&cursor))) {
		if (free_space(fp, form, path, flags))
			flags |= NO_HEADER_FLAG;
	}
}

static int
free_f(
	int		argc,
	char		**argv)
{
	FILE		*fp = NULL;
	char		*fname = NULL;
	int		c, flags = 0, form = 0, type = 0;

	while ((c = getopt(argc, argv, "bf:hNir")) != EOF) {
		switch (c) {
		case 'f':
			fname = optarg;
			break;
		case 'b':
			form |= XFS_BLOCK_QUOTA;
			break;
		case 'i':
			form |= XFS_INODE_QUOTA;
			break;
		case 'r':
			form |= XFS_RTBLOCK_QUOTA;
			break;
		case 'h':
			flags |= HUMAN_FLAG;
			break;
		case 'N':
			flags |= NO_HEADER_FLAG;
			break;
		default:
			return command_usage(&free_cmd);
		}
	}

	if (!form)
		form = XFS_BLOCK_QUOTA;

	if (!type)
		type = FS_MOUNT_POINT|FS_PROJECT_PATH;

	if ((fp = fopen_write_secure(fname)) == NULL)
		return 0;

	if (argc == optind)
		free_space_list(fp, form, type, NULL, flags);
	else while (argc > optind)
		free_space_list(fp, form, type, argv[optind++], flags);

	if (fname)
		fclose(fp);
	return 0;
}

void
free_init(void)
{
	free_cmd.name = _("df");
	free_cmd.altname = _("free");
	free_cmd.cfunc = free_f;
	free_cmd.argmin = 0;
	free_cmd.argmax = -1;
	free_cmd.args = _("[-bir] [-hn] [-f file]");
	free_cmd.oneline = _("show free and used counts for blocks and inodes");
	free_cmd.help = free_help;

	add_command(&free_cmd);
}

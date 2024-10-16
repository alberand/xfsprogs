// SPDX-License-Identifier: LGPL-2.1
/*
 * Copyright (c) 2000-2005 Silicon Graphics, Inc.
 * All Rights Reserved.
 */
#ifndef __XFS_PLATFORM_DEFS_H__
#define __XFS_PLATFORM_DEFS_H__

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/param.h>
#include <limits.h>
#include <stdbool.h>
#include <libgen.h>
#include <urcu.h>

/* long and pointer must be either 32 bit or 64 bit */
#define BITS_PER_LONG (sizeof(long) * CHAR_BIT)

typedef unsigned short umode_t;

/* Define if you want gettext (I18N) support */
#ifdef ENABLE_GETTEXT
# include <libintl.h>
# define _(x)                   gettext(x)
# define N_(x)			 x
#else
# define _(x)                   (x)
# define N_(x)			 x
# define textdomain(d)          do { } while (0)
# define bindtextdomain(d,dir)  do { } while (0)
#endif
#include <locale.h>

#define IRIX_DEV_BITSMAJOR      14
#define IRIX_DEV_BITSMINOR      18
#define IRIX_DEV_MAXMAJ         0x1ff
#define IRIX_DEV_MAXMIN         0x3ffff
#define IRIX_DEV_MAJOR(dev)	((int)(((unsigned)(dev) >> IRIX_DEV_BITSMINOR) \
					& IRIX_DEV_MAXMAJ))
#define IRIX_DEV_MINOR(dev)	((int)((dev) & IRIX_DEV_MAXMIN))
#define IRIX_MKDEV(major,minor)	((xfs_dev_t)(((major) << IRIX_DEV_BITSMINOR) \
					| (minor&IRIX_DEV_MAXMIN)))
#define IRIX_DEV_TO_KDEVT(dev)	makedev(IRIX_DEV_MAJOR(dev),IRIX_DEV_MINOR(dev))

#ifndef min
#define min(a,b)	(((a)<(b))?(a):(b))
#define max(a,b)	(((a)>(b))?(a):(b))
#endif
#define max3(a,b,c)	max(max(a, b), c)

/* If param.h doesn't provide it, i.e. for Android */
#ifndef NBBY
#define NBBY 8
#endif

#ifdef DEBUG
# define ASSERT(EX)	assert(EX)
#else
# define ASSERT(EX)	((void) 0)
#endif

extern int	platform_nproc(void);

#define NSEC_PER_SEC	(1000000000ULL)
#define NSEC_PER_USEC	(1000ULL)

/* Simplified from version in include/linux/overflow.h */

/*
 * Compute a*b+c, returning SIZE_MAX on overflow. Internal helper for
 * struct_size() below.
 */
static inline size_t __ab_c_size(size_t a, size_t b, size_t c)
{
	return (a * b) + c;
}

#define __must_be_array(a) (0)

/**
 * struct_size() - Calculate size of structure with trailing array.
 * @p: Pointer to the structure.
 * @member: Name of the array member.
 * @count: Number of elements in the array.
 *
 * Calculates size of memory needed for structure @p followed by an
 * array of @count number of @member elements.
 *
 * Return: number of bytes needed or SIZE_MAX on overflow.
 */
#define struct_size(p, member, count)					\
	__ab_c_size(count,						\
		sizeof(*(p)->member) + __must_be_array((p)->member),	\
		sizeof(*(p)))

/**
 * struct_size_t() - Calculate size of structure with trailing flexible array
 * @type: structure type name.
 * @member: Name of the array member.
 * @count: Number of elements in the array.
 *
 * Calculates size of memory needed for structure @type followed by an
 * array of @count number of @member elements. Prefer using struct_size()
 * when possible instead, to keep calculations associated with a specific
 * instance variable of type @type.
 *
 * Return: number of bytes needed or SIZE_MAX on overflow.
 */
#define struct_size_t(type, member, count)					\
	struct_size((type *)NULL, member, count)

/*
 * Add the pseudo keyword 'fallthrough' so case statement blocks
 * must end with any of these keywords:
 *   break;
 *   fallthrough;
 *   continue;
 *   goto <label>;
 *   return [expression];
 *
 *  gcc: https://gcc.gnu.org/onlinedocs/gcc/Statement-Attributes.html#Statement-Attributes
 */
#if defined __has_attribute
#  if __has_attribute(__fallthrough__)
#    define fallthrough                    __attribute__((__fallthrough__))
#  else
#    define fallthrough                    do {} while (0)  /* fallthrough */
#  endif
#else
#    define fallthrough                    do {} while (0)  /* fallthrough */
#endif

/* Only needed for the kernel. */
#define __init

#ifdef __GNUC__
#define __return_address	__builtin_return_address(0)

/*
 * Return the address of a label.  Use barrier() so that the optimizer
 * won't reorder code to refactor the error jumpouts into a single
 * return, which throws off the reported address.
 */
#define __this_address	({ __label__ __here; __here: barrier(); &&__here; })
/* Optimization barrier */

/* The "volatile" is due to gcc bugs */
#define barrier() __asm__ __volatile__("": : :"memory")
#endif

/* Optimization barrier */
#ifndef barrier
# define barrier() __memory_barrier()
#endif

#endif	/* __XFS_PLATFORM_DEFS_H__ */

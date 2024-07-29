// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2000-2001,2005 Silicon Graphics, Inc.
 * Copyright (c) 2012 Red Hat, Inc.
 * Copyright (c) 2017-2024 Oracle.
 * All Rights Reserved.
 */
#ifndef __LIBFROG_HISTOGRAM_H__
#define __LIBFROG_HISTOGRAM_H__

struct histbucket {
	/* Low and high size of this bucket */
	long long		low;
	long long		high;

	/* Count of observations recorded */
	long long		nr_obs;

	/* Sum of values recorded */
	long long		sum;
};

struct histogram {
	/* Sum of all values recorded */
	long long		tot_sum;

	/* Count of all observations recorded */
	long long		tot_obs;

	struct histbucket	*buckets;

	/* Number of buckets */
	unsigned int		nr_buckets;
};

int hist_add_bucket(struct histogram *hs, long long bucket_low);
void hist_add(struct histogram *hs, long long value);
void hist_init(struct histogram *hs);
void hist_prepare(struct histogram *hs, long long maxvalue);
void hist_free(struct histogram *hs);

struct histogram_strings {
	/* What does each sum represent? ("free blocks") */
	const char		*sum;

	/* What does each observation represent? ("free extents") */
	const char		*observations;

	/* What does sum / observation represent? ("average extent length") */
	const char		*averages;
};

void hist_print(const struct histogram *hs,
		const struct histogram_strings *hstr);
void hist_summarize(const struct histogram *hs,
		const struct histogram_strings *hstr);

static inline unsigned int hist_buckets(const struct histogram *hs)
{
	return hs->nr_buckets;
}

#endif /* __LIBFROG_HISTOGRAM_H__ */

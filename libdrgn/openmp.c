// Copyright (c) Meta Platforms, Inc. and affiliates.
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifdef _OPENMP

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "cleanup.h"
#include "hash_table.h"
#include "openmp.h"
#include "util.h"

DEFINE_HASH_SET(int_set, int, int_key_hash_pair, scalar_key_eq);

int drgn_num_threads;

struct cpu_list_state {
	int current;
	int end;
};

static int cpu_list_next(FILE *file, struct cpu_list_state *state)
{
	if (state->current >= state->end) {
		if (fscanf(file, "%d", &state->current) < 1)
			return -1;
		if (fscanf(file, "-%d", &state->end) >= 1)
			state->end++;
		else
			state->end = state->current + 1;
		fgetc(file);
	}
	return state->current++;
}

__attribute__((__constructor__))
static void drgn_init_num_threads(void)
{
	// If the number of threads was set explicitly or anything below fails,
	// use the current OpenMP setting.
	drgn_num_threads = omp_get_max_threads();
	if (getenv("OMP_NUM_THREADS"))
		return;

	// Simultaneous multithreading rarely helps, and often slows down, our
	// parallel indexing. Limit the number of threads to the number of
	// online CPU cores, ignoring hardware threads.

	#define SIBLINGS_FORMAT "/sys/devices/system/cpu/cpu%d/topology/thread_siblings_list"
	char siblings_path[sizeof(SIBLINGS_FORMAT)
			   - sizeof("%d")
			   + max_decimal_length(int)
			   + 1];

	int num_threads = 0;
	_cleanup_(int_set_deinit) struct int_set cpus_seen = HASH_TABLE_INIT;

	_cleanup_fclose_ FILE *online = fopen("/sys/devices/system/cpu/online", "r");
	if (!online)
		return;
	struct cpu_list_state online_state = {};
	int cpu;
	while ((cpu = cpu_list_next(online, &online_state)) >= 0) {
		if (int_set_search(&cpus_seen, &cpu).entry)
			continue;

		num_threads++;

		snprintf(siblings_path, sizeof(siblings_path), SIBLINGS_FORMAT,
			 cpu);
		_cleanup_fclose_ FILE *siblings = fopen(siblings_path, "r");
		if (!siblings)
			continue;
		struct cpu_list_state siblings_state = {};
		int sibling_cpu;
		while ((sibling_cpu = cpu_list_next(siblings, &siblings_state))
		       >= 0) {
			if (int_set_insert(&cpus_seen, &sibling_cpu, NULL) < 0)
				return;
		}
	}

	if (num_threads > 0 && num_threads < drgn_num_threads)
		drgn_num_threads = num_threads;
}

#endif /* _OPENMP */

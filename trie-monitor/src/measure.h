/* SPDX-License-Identifier: AGPL-3.0-only
 *
 * Copyright (C) 2025-2026 IKERLAN, S.Coop
 *
 * Author: Charles-Alexis Lefebvre
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* measure.h

Measurement utilities shared by both trie implementations:
  - loading the trace files into memory
  - timing helpers
  - the search-time benchmark harness
  - the headline comparison summary
*/

#ifndef MONITOR_MEASURE_H
#define MONITOR_MEASURE_H

#include <stddef.h>
#include <stdio.h>
#include <time.h>

/* Headline numbers used to compare the two structures. */
struct TrieSummary {
    const char   *label;
    unsigned int  nodes;         /* number of allocated nodes                 */
    unsigned int  stored_calls;  /* number of call names stored in the tree   */
    size_t        actual_bytes;  /* real heap usage (with allocator padding)  */
};

/*
 * Signature every trie exposes so the benchmark can drive it generically.
 * `root` is the (opaque) tree root, cast back inside each implementation.
 */
typedef int (*trace_test_fn)(void *root, char **trace, unsigned int trace_length,
                             long double *average_t, long *max_t);

/* Read every regular file in `dir_path` into a matrix of trace lines. */
char ***read_files_into_matrix(const char *dir_path,
                               unsigned int *file_count_out,
                               unsigned int **line_counts_out);

/* Free a matrix returned by read_files_into_matrix. */
void free_traces_matrix(char ***matrix);

/*
 * Run `runs` randomly-chosen traces through `test_fn` and write, for each run,
 * one line "<kind>,<average_ns>,<max_ns>" to `out`: the average node-search time
 * over the nodes of the trace and the largest single node-search time of the
 * trace. Raw values only; the statistics are computed by scripts/trie.Rmd.
 *
 * With `modified` == 0 the traces are known (they are in the trie); with
 * `modified` != 0 one random line of each trace is replaced by "MODIFICATION",
 * so the trace is unknown and the search stops at the first mismatch.
 * The traces are drawn with srand(seed): the same seed gives the same traces.
 */
void benchmark_search(trace_test_fn test_fn, void *root,
                      char ***traces, unsigned int file_count,
                      unsigned int *line_counts, int modified,
                      unsigned int seed, unsigned long runs, FILE *out);

/* Print a side-by-side comparison of the two structures. */
void print_summary_comparison(const struct TrieSummary *plain,
                              const struct TrieSummary *radix);

#endif /* MONITOR_MEASURE_H */

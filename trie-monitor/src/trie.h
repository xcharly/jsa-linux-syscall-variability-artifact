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

/* trie.h

Plain (uncompressed) trie: one call name per node.
*/

#ifndef MONITOR_TRIE_H
#define MONITOR_TRIE_H

#include "measure.h"
#include <stddef.h>

struct Trie {
    char *name;
    struct Trie *children;
    unsigned int children_num;
};

/*
 * Build the plain trie from the traces, print its build + memory report and
 * fill `summary` with the headline numbers (label is left untouched).
 */
int trie_build(struct Trie *root, char ***traces, unsigned int traces_num,
               unsigned int *traces_length, struct TrieSummary *summary);

/*
 * Run a single trace through the trie, measuring per-node search time.
 * Matches trace_test_fn so it can be driven by the benchmark harness.
 */
int trie_test_trace(void *root, char **trace, unsigned int trace_length,
                    long double *average_t, long *max_t);

/* Recursively free the trie contents (not the root struct itself). */
void trie_free(struct Trie *root);

#endif /* MONITOR_TRIE_H */

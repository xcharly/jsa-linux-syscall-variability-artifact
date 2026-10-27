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

/* trie_radix.h

Radix trie: consecutive single-child calls are compressed into one node whose
`values` array holds the whole chain. Same node layout as the str-radix project.
*/

#ifndef MONITOR_TRIE_RADIX_H
#define MONITOR_TRIE_RADIX_H

#include "measure.h"
#include <stddef.h>

struct TrieRadix {
    /* Compressed chain of call names stored in this node. A maximal run of
     * single-child (non-branching) nodes from the uncompressed trie is merged
     * here, so `values[0..values_len-1]` are the consecutive calls that must be
     * matched, in order, to traverse this node. Each entry is a heap-allocated
     * NUL-terminated string. */
    char **values;

    /* Array of child nodes reached after the chain in `values` ends. Each child
     * begins an alternative branch (a different continuation of the sequence).
     * Heap-allocated array of `children_num` elements, or NULL for a leaf. */
    struct TrieRadix *children;

    /* Number of call names stored in `values` (length of the compressed chain,
     * always >= 1). */
    unsigned int values_len;

    /* Number of child nodes in `children` (0 for a leaf node). */
    unsigned int children_num;
};

/*
 * Build the radix trie from the traces, print its build + memory report and
 * fill `summary` with the headline numbers (label is left untouched).
 */
int trie_radix_build(struct TrieRadix *root, char ***traces, unsigned int traces_num,
                     unsigned int *traces_length, struct TrieSummary *summary);

/*
 * Run a single trace through the radix trie, measuring per-node search time.
 * Matches trace_test_fn so it can be driven by the benchmark harness.
 */
int trie_radix_test_trace(void *root, char **trace, unsigned int trace_length,
                          long double *average_t, long *max_t);

/*
 * Write the structure of the trie to a CSV file, one line per identifier (that
 * is, per node of the plain trie, which the radix trie stores in the chains of
 * its nodes), in the order of a depth-first walk: the root is node 1.
 *   node    number of the node (the root is 1)
 *   parent  number of the parent node (0 for the root)
 *   depth   number of function calls from the root (0 for the root)
 *   rnode   number of the radix node whose chain holds the identifier
 *   offset  position of the identifier inside that chain (0 = first)
 *   label   the function-call identifier
 * Returns 0 on success.
 */
int trie_radix_export(const struct TrieRadix *root, const char *path);

/* Recursively free the radix trie contents (not the root struct itself). */
void trie_radix_free(struct TrieRadix *root);

#endif /* MONITOR_TRIE_RADIX_H */

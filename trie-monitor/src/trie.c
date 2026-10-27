/* SPDX-License-Identifier: AGPL-3.0-only
 *
 * Copyright (C) 2025-2026 IKERLAN, S.Coop
 *
 * Authors: Markel Galarraga (original author), Charles-Alexis Lefebvre
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

/* trie.c

Plain (uncompressed) trie implementation, based on Trie trees.
*/

#include "trie.h"
#include "common.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <malloc.h>

static void sizeof_trie(struct Trie, size_t *, size_t *, size_t *);
static unsigned int count_trie(struct Trie);
static size_t theoretical_sizeof_trie(struct Trie);
static void name_stats_trie(struct Trie, size_t *, double *);
static void sizeof_trie_breakdown(struct Trie);

static struct Trie make_trie(char *name) {
    struct Trie trie;
    trie.name = malloc(strlen(name) + 1); // +1 for \0
    strcpy(trie.name, name);
    trie.children = NULL; // a leaf has no children array (malloc_usable_size(NULL) is 0)
    trie.children_num = 0;

    return trie;
}

int trie_build(struct Trie *root, char ***traces, unsigned int traces_num,
               unsigned int *traces_length, struct TrieSummary *summary) {

    unsigned int i, j, k;
    struct Trie *node;
    bool already_inserted;

    *root = make_trie("root");
    for (i = 0; i < traces_num; i++) {
        node = root;
        for (j = 0; j < traces_length[i]; j++) {
            already_inserted = false; // re-initialize
            for (k = 0; k < node->children_num; k++) { // Check if already inserted
                if ((strcmp(node->children[k].name, traces[i][j]) == 0)) {
                    already_inserted = true;
                    node = &node->children[k]; // Go to children and repeat
                    break; // We do not need to continue checking children
                }
            }

            if (!already_inserted) { // Add the new child
                if (node->children_num == 0) {
                    node->children = malloc(sizeof(struct Trie));
                }
                else {
                    node->children = realloc(node->children, sizeof(struct Trie) * (node->children_num + 1));
                }

                node->children[node->children_num] = make_trie(traces[i][j]);

                node->children_num++;
                node = &node->children[node->children_num - 1]; // Go to children and repeat
            }
        }
    }
    printf("Trie built\n");
    printf("Nodes:\t%u\n", count_trie(*root));
    printf("Size:\t");

    size_t size_b, size_kb, size_mb;
    sizeof_trie(*root, &size_b, &size_kb, &size_mb);
    printf("%zu MB, %zu KB, %zu B\n", size_mb, size_kb, size_b);

    size_t th_size = theoretical_sizeof_trie(*root);
    printf("Theoretical size (N*%zu + sum_name_lens):\t%zu B  [N=%u, sizeof(struct Trie)=%zu]\n",
           sizeof(struct Trie), th_size, count_trie(*root), sizeof(struct Trie));

    size_t max_name_len;
    double avg_name_len;
    name_stats_trie(*root, &max_name_len, &avg_name_len);
    printf("Name length: max=%zu, avg=%.2f\n", max_name_len, avg_name_len);

    sizeof_trie_breakdown(*root);

    if (summary) {
        summary->nodes        = count_trie(*root);
        summary->stored_calls = count_trie(*root);
        summary->actual_bytes = size_b;
    }
    return 0;
}

int trie_test_trace(void *root_arg, char **trace, unsigned int trace_length,
                    long double *average_t, long *max_t) {

    struct Trie *node = (struct Trie *)root_arg;
    unsigned int i = 0, j = 0;
    bool unknown = false;

    struct timespec start_t, end_t;
    long total_t = 0, tmp_t;
    *average_t = 0; *max_t = 0;

    while (!unknown && i < trace_length) {

        clock_gettime(CLOCK_MONOTONIC, &start_t);                           // START MEASURING

        unknown = true; // Assume we will not find it
        for (j = 0; j < node->children_num; j++) {
            if (strcmp(node->children[j].name, trace[i]) == 0) {
                unknown = false; // Found it
                node = &node->children[j]; // Go to children and repeat
                break;
            }
        }
        i++;

        clock_gettime(CLOCK_MONOTONIC, &end_t);                             // FINISH MEASURING

        tmp_t = (end_t.tv_sec - start_t.tv_sec) * S_TO_NS + end_t.tv_nsec - start_t.tv_nsec;
        total_t += tmp_t;
        if (tmp_t > *max_t) {
            *max_t = tmp_t;
        }
    }

    *average_t = ((long double)total_t) / i;

    return 0;
}

static void _sizeof_trie(struct Trie trie, size_t *size, size_t *size_kb, size_t *size_mb) {
    unsigned int i;

    *size += sizeof(trie);
    *size += malloc_usable_size(trie.name);
    *size += malloc_usable_size(trie.children) - trie.children_num * sizeof(trie);
    for (i = 0; i < trie.children_num; i++) {
        _sizeof_trie(trie.children[i], size, size_kb, size_mb);
    }
}

static void sizeof_trie(struct Trie trie, size_t *out_size, size_t *out_size_kb, size_t *out_size_mb) {
    *out_size = 0;
    *out_size_kb = 0;
    *out_size_mb = 0;
    _sizeof_trie(trie, out_size, out_size_kb, out_size_mb);
}

/**
 * Recursively count all nodes in the trie (including root).
 */
static unsigned int count_trie(struct Trie trie) {
    unsigned int i, count = 1; /* count this node */
    for (i = 0; i < trie.children_num; i++) {
        count += count_trie(trie.children[i]);
    }
    return count;
}

/**
 * Theoretical memory: N * sizeof(struct Trie) + sum(strlen(name)+1) for all nodes.
 * Does not account for allocator padding or overhead.
 */
static size_t theoretical_sizeof_trie(struct Trie trie) {
    unsigned int i;
    /* sizeof(struct Trie): node struct; strlen+1: heap-allocated name string */
    size_t size = sizeof(struct Trie) + strlen(trie.name) + 1;
    for (i = 0; i < trie.children_num; i++) {
        size += theoretical_sizeof_trie(trie.children[i]);
    }
    return size;
}

/**
 * Recursively compute max and cumulative sum of strlen(name) across all nodes.
 * Call with *max=0, *sum=0 initially.
 */
static void _name_stats_trie(struct Trie trie, size_t *max, size_t *sum, unsigned int *n) {
    unsigned int i;
    size_t len = strlen(trie.name);
    if (len > *max) *max = len;
    *sum += len;
    (*n)++;
    for (i = 0; i < trie.children_num; i++) {
        _name_stats_trie(trie.children[i], max, sum, n);
    }
}

/**
 * Returns max name length and average name length across all nodes.
 */
static void name_stats_trie(struct Trie trie, size_t *out_max, double *out_avg) {
    size_t sum = 0;
    unsigned int n = 0;
    *out_max = 0;
    _name_stats_trie(trie, out_max, &sum, &n);
    *out_avg = (n > 0) ? (double)sum / n : 0.0;
}

/* Effective (padding-free) size of the node struct: the bytes that carry the
 * information strictly required by the abstract trie node (call-name pointer,
 * children pointer and children count). The remaining bytes up to
 * sizeof(struct Trie) are C alignment padding. */
#define TRIE_STRUCT_EFF (sizeof(char *) + sizeof(struct Trie *) + sizeof(unsigned int))

/**
 * Recursively accumulate the memory components following the paper's
 * decomposition into effective bytes and C-implementation overhead:
 *
 *   Function-call identifier (heap-allocated name string):
 *     name_eff   : sum(strlen(name))                                 [effective]
 *     name_null  : sum(1) == N                (null terminator)      [overhead]
 *     name_pad   : sum(malloc_usable_size(name) - (strlen(name)+1))  [overhead]
 *
 *   Node structure (struct itself + children array):
 *     struct_eff : N * TRIE_STRUCT_EFF        (effective fields)     [effective]
 *     struct_pad : N * (sizeof(struct Trie) - TRIE_STRUCT_EFF)       [overhead]
 *     child_pad  : sum(malloc_usable_size(children)
 *                        - children_num * sizeof(struct Trie))       [overhead]
 *
 * Invariant: (name_eff + name_null + name_pad)
 *          + (struct_eff + struct_pad + child_pad) == sizeof_trie() (bytes)
 */
static void _sizeof_trie_breakdown(struct Trie trie,
                                   size_t *name_eff, size_t *name_null, size_t *name_pad,
                                   size_t *struct_eff, size_t *struct_pad, size_t *child_pad) {
    unsigned int i;
    size_t name_len   = strlen(trie.name);
    size_t name_alloc = malloc_usable_size(trie.name);
    size_t ch_alloc   = malloc_usable_size(trie.children);
    size_t ch_needed  = (size_t)trie.children_num * sizeof(struct Trie);

    *name_eff   += name_len;
    *name_null  += 1;                              /* string null terminator */
    *name_pad   += name_alloc - (name_len + 1);

    *struct_eff += TRIE_STRUCT_EFF;
    *struct_pad += sizeof(struct Trie) - TRIE_STRUCT_EFF;
    *child_pad  += ch_alloc - ch_needed;           /* 0 when no children */

    for (i = 0; i < trie.children_num; i++) {
        _sizeof_trie_breakdown(trie.children[i], name_eff, name_null, name_pad,
                               struct_eff, struct_pad, child_pad);
    }
}

/**
 * Print the effective-vs-overhead memory breakdown described in the paper,
 * verifying that effective + padding equals the actual measured size.
 */
static void sizeof_trie_breakdown(struct Trie trie) {
    size_t name_eff = 0, name_null = 0, name_pad = 0;
    size_t struct_eff = 0, struct_pad = 0, child_pad = 0;
    _sizeof_trie_breakdown(trie, &name_eff, &name_null, &name_pad,
                           &struct_eff, &struct_pad, &child_pad);

    size_t effective = name_eff + struct_eff;
    size_t padding   = name_null + name_pad + struct_pad + child_pad;
    size_t total     = effective + padding;

    printf("Memory breakdown (bytes):\n");
    printf("  Function-call identifier:\n");
    printf("    [F ] Effective   sum(strlen)             : %zu B\n", name_eff);
    printf("    [pn] Null terminators   sum(1) = N       : %zu B\n", name_null);
    printf("    [pN] Name buffer padding                 : %zu B\n", name_pad);
    printf("  Node structure (struct + children array):\n");
    printf("    [S ] Effective   N * %zu                  : %zu B\n", (size_t)TRIE_STRUCT_EFF, struct_eff);
    printf("    [pS] Struct padding   N * %zu             : %zu B\n", sizeof(struct Trie) - TRIE_STRUCT_EFF, struct_pad);
    printf("    [pC] Children array padding              : %zu B\n", child_pad);
    printf("  Effective  [F]+[S]                         : %zu B  (%.1f%% of actual)\n",
           effective, total ? 100.0 * effective / total : 0.0);
    printf("  Padding    [pn]+[pN]+[pS]+[pC]             : %zu B  (%.1f%% of actual)\n",
           padding, total ? 100.0 * padding / total : 0.0);
    printf("  Actual (== Size above)                     : %zu B\n", total);
}

void trie_free(struct Trie *root) {
    unsigned int i;
    for (i = 0; i < root->children_num; i++) {
        trie_free(&root->children[i]);
    }
    if (root->children_num > 0) {
        free(root->children);
    }
    free(root->name);
}

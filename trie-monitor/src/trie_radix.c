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

/* trie_radix.c

Radix trie implementation. The tree is first built as an uncompressed trie
(one call per node) and then compressed: maximal chains of single-child nodes
are merged into one node whose `values` array holds the whole chain.
*/

#include "trie_radix.h"
#include "common.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <malloc.h>

/*
 * Temporary (uncompressed) trie node, used only while building the tree from
 * the traces. Each node holds a single call name and its children.
 */
struct TrieRadixTmp {
    char *name;
    struct TrieRadixTmp *children;
    unsigned int children_num;
};

static void sizeof_trie(struct TrieRadix, size_t *, size_t *, size_t *);
static unsigned int count_trie(struct TrieRadix);
static unsigned int count_values_trie(struct TrieRadix);
static size_t theoretical_sizeof_trie(struct TrieRadix);
static void name_stats_trie(struct TrieRadix, size_t *, double *);
static void sizeof_trie_breakdown(struct TrieRadix);

static struct TrieRadixTmp make_tmp_trie(char *name) {
    struct TrieRadixTmp trie;
    trie.name = malloc(strlen(name) + 1); // +1 for \0
    strcpy(trie.name, name);
    trie.children = NULL;
    trie.children_num = 0;

    return trie;
}

/*
 * Recursively free a temporary (uncompressed) trie.
 */
static void free_tmp_trie(struct TrieRadixTmp *node) {
    unsigned int i;
    for (i = 0; i < node->children_num; i++) {
        free_tmp_trie(&node->children[i]);
    }
    if (node->children_num > 0) {
        free(node->children);
    }
    free(node->name);
}

/*
 * Compress a temporary trie (one call per node) into a radix trie: a maximal
 * chain of single-child nodes is merged into one node whose `values` array
 * holds every call name of the chain. Branching or leaf nodes terminate the
 * chain; their children are compressed recursively.
 */
static void compress_trie(const struct TrieRadixTmp *node, struct TrieRadix *out) {
    unsigned int i, len = 1;
    const struct TrieRadixTmp *cur = node;
    const struct TrieRadixTmp *walk;

    /* Extend the chain while there is exactly one child (no branching) */
    while (cur->children_num == 1) {
        cur = &cur->children[0];
        len++;
    }

    /* Copy the chain of names into the radix node's values array */
    out->values = malloc(sizeof(char *) * len);
    walk = node;
    for (i = 0; i < len; i++) {
        out->values[i] = malloc(strlen(walk->name) + 1);
        strcpy(out->values[i], walk->name);
        if (i < len - 1) {
            walk = &walk->children[0];
        }
    }
    out->values_len = len;

    /* `cur` is the end of the chain: its children are the branches */
    out->children_num = cur->children_num;
    if (cur->children_num > 0) {
        out->children = malloc(sizeof(struct TrieRadix) * cur->children_num);
        for (i = 0; i < cur->children_num; i++) {
            compress_trie(&cur->children[i], &out->children[i]);
        }
    } else {
        out->children = NULL;
    }
}

int trie_radix_build(struct TrieRadix *root, char ***traces, unsigned int traces_num,
                     unsigned int *traces_length, struct TrieSummary *summary) {

    unsigned int i, j, k;
    struct TrieRadixTmp tmp_root, *node;
    bool already_inserted;

    tmp_root = make_tmp_trie("root");
    for (i = 0; i < traces_num; i++) {
        node = &tmp_root;
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
                    node->children = malloc(sizeof(struct TrieRadixTmp));
                }
                else {
                    node->children = realloc(node->children, sizeof(struct TrieRadixTmp) * (node->children_num + 1));
                }

                node->children[node->children_num] = make_tmp_trie(traces[i][j]);

                node->children_num++;
                node = &node->children[node->children_num - 1]; // Go to children and repeat
            }
        }
    }

    /* Compress the uncompressed trie into a radix trie */
    compress_trie(&tmp_root, root);
    free_tmp_trie(&tmp_root);

    printf("Radix trie built\n");
    printf("Nodes:\t%u\n", count_trie(*root));
    printf("Stored calls (sum values_len):\t%u\n", count_values_trie(*root));
    printf("Size:\t");

    size_t size_b, size_kb, size_mb;
    sizeof_trie(*root, &size_b, &size_kb, &size_mb);
    printf("%zu MB, %zu KB, %zu B\n", size_mb, size_kb, size_b);

    size_t th_size = theoretical_sizeof_trie(*root);
    printf("Theoretical size (N*%zu + values_ptrs + sum_name_lens):\t%zu B  [N=%u, sizeof(struct TrieRadix)=%zu]\n",
           sizeof(struct TrieRadix), th_size, count_trie(*root), sizeof(struct TrieRadix));

    size_t max_name_len;
    double avg_name_len;
    name_stats_trie(*root, &max_name_len, &avg_name_len);
    printf("Name length: max=%zu, avg=%.2f\n", max_name_len, avg_name_len);

    sizeof_trie_breakdown(*root);

    if (summary) {
        summary->nodes        = count_trie(*root);
        summary->stored_calls = count_values_trie(*root);
        summary->actual_bytes = size_b;
    }
    return 0;
}

int trie_radix_test_trace(void *root_arg, char **trace, unsigned int trace_length,
                          long double *average_t, long *max_t) {

    struct TrieRadix *node = (struct TrieRadix *)root_arg;
    unsigned int value_index = 1; // values[0] is the anchor already consumed
    unsigned int i = 0, j = 0;
    bool unknown = false;

    struct timespec start_t, end_t;
    long total_t = 0, tmp_t;
    *average_t = 0; *max_t = 0;

    while (!unknown && i < trace_length) {

        clock_gettime(CLOCK_MONOTONIC, &start_t);                           // START MEASURING

        unknown = true; // Assume we will not find it
        if (value_index < node->values_len) {
            // Still inside the current node's compressed chain
            if (strcmp(node->values[value_index], trace[i]) == 0) {
                value_index++;
                unknown = false; // Found it
            }
        } else {
            // Chain consumed: look for a matching child branch
            for (j = 0; j < node->children_num; j++) {
                if (strcmp(node->children[j].values[0], trace[i]) == 0) {
                    unknown = false; // Found it
                    node = &node->children[j]; // Go to children and repeat
                    value_index = 1;
                    break;
                }
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

static void _sizeof_trie(struct TrieRadix trie, size_t *size, size_t *size_kb, size_t *size_mb) {
    unsigned int i;

    *size += sizeof(trie);
    *size += malloc_usable_size(trie.values);
    for (i = 0; i < trie.values_len; i++) {
        *size += malloc_usable_size(trie.values[i]);
    }
    *size += malloc_usable_size(trie.children) - trie.children_num * sizeof(trie);
    for (i = 0; i < trie.children_num; i++) {
        _sizeof_trie(trie.children[i], size, size_kb, size_mb);
    }
}

static void sizeof_trie(struct TrieRadix trie, size_t *out_size, size_t *out_size_kb, size_t *out_size_mb) {
    *out_size = 0;
    *out_size_kb = 0;
    *out_size_mb = 0;
    _sizeof_trie(trie, out_size, out_size_kb, out_size_mb);
}

/**
 * Recursively count all nodes in the radix trie (including root).
 */
static unsigned int count_trie(struct TrieRadix trie) {
    unsigned int i, count = 1; /* count this node */
    for (i = 0; i < trie.children_num; i++) {
        count += count_trie(trie.children[i]);
    }
    return count;
}

/**
 * Recursively count all stored calls (sum of values_len across all nodes).
 * Equivalent to the number of nodes an uncompressed trie would have.
 */
static unsigned int count_values_trie(struct TrieRadix trie) {
    unsigned int i, count = trie.values_len;
    for (i = 0; i < trie.children_num; i++) {
        count += count_values_trie(trie.children[i]);
    }
    return count;
}

/**
 * Theoretical memory: N * sizeof(struct TrieRadix) + values pointer arrays +
 * sum(strlen(name)+1) for all stored names. No allocator padding.
 */
static size_t theoretical_sizeof_trie(struct TrieRadix trie) {
    unsigned int i;
    /* sizeof(struct TrieRadix): node struct; values pointer array + each name string */
    size_t size = sizeof(struct TrieRadix) + trie.values_len * sizeof(char *);
    for (i = 0; i < trie.values_len; i++) {
        size += strlen(trie.values[i]) + 1;
    }
    for (i = 0; i < trie.children_num; i++) {
        size += theoretical_sizeof_trie(trie.children[i]);
    }
    return size;
}

/**
 * Recursively compute max and cumulative sum of strlen(value) across all values.
 * Call with *max=0, *sum=0 initially.
 */
static void _name_stats_trie(struct TrieRadix trie, size_t *max, size_t *sum, unsigned int *n) {
    unsigned int i;
    for (i = 0; i < trie.values_len; i++) {
        size_t len = strlen(trie.values[i]);
        if (len > *max) *max = len;
        *sum += len;
        (*n)++;
    }
    for (i = 0; i < trie.children_num; i++) {
        _name_stats_trie(trie.children[i], max, sum, n);
    }
}

/**
 * Returns max name length and average name length across all stored values.
 */
static void name_stats_trie(struct TrieRadix trie, size_t *out_max, double *out_avg) {
    size_t sum = 0;
    unsigned int n = 0;
    *out_max = 0;
    _name_stats_trie(trie, out_max, &sum, &n);
    *out_avg = (n > 0) ? (double)sum / n : 0.0;
}

/* Effective (padding-free) size of the radix node struct: values pointer,
 * children pointer and the two counts. sizeof(struct TrieRadix) is already
 * 8-aligned, so on common platforms there is no struct padding. */
#define TRIE_RADIX_STRUCT_EFF \
    (sizeof(char **) + sizeof(struct TrieRadix *) + 2 * sizeof(unsigned int))

/**
 * Recursively accumulate the memory components following the paper's
 * decomposition into effective bytes and C-implementation overhead. The radix
 * node adds one category (the values pointer array) with respect to the plain
 * trie, because each node stores a compressed chain of identifiers:
 *
 *   Function-call identifiers (heap-allocated name strings, V = sum values_len):
 *     name_eff   : sum(strlen(value))                                [effective]
 *     name_null  : sum(1) == V                 (null terminator)     [overhead]
 *     name_pad   : sum(malloc_usable_size(value) - (strlen+1))       [overhead]
 *
 *   Values pointer array (the compressed chain of identifiers):
 *     values_eff : sum(values_len) * sizeof(char *)                  [effective]
 *     values_pad : sum(malloc_usable_size(values)
 *                        - values_len * sizeof(char *))              [overhead]
 *
 *   Node structure (struct itself + children array):
 *     struct_eff : N * TRIE_RADIX_STRUCT_EFF   (effective fields)    [effective]
 *     struct_pad : N * (sizeof(struct TrieRadix) - TRIE_RADIX_STRUCT_EFF) [overhead]
 *     child_pad  : sum(malloc_usable_size(children)
 *                        - children_num * sizeof(struct TrieRadix))  [overhead]
 *
 * Invariant: effective + padding == sizeof_trie() (bytes)
 */
static void _sizeof_trie_breakdown(struct TrieRadix trie,
                                   size_t *name_eff, size_t *name_null, size_t *name_pad,
                                   size_t *values_eff, size_t *values_pad,
                                   size_t *struct_eff, size_t *struct_pad, size_t *child_pad) {
    unsigned int i;
    size_t vp_alloc  = malloc_usable_size(trie.values);
    size_t vp_needed = (size_t)trie.values_len * sizeof(char *);
    size_t ch_alloc  = malloc_usable_size(trie.children);
    size_t ch_needed = (size_t)trie.children_num * sizeof(struct TrieRadix);

    for (i = 0; i < trie.values_len; i++) {
        size_t name_len = strlen(trie.values[i]);
        *name_eff  += name_len;
        *name_null += 1;                              /* string null terminator */
        *name_pad  += malloc_usable_size(trie.values[i]) - (name_len + 1);
    }

    *values_eff += vp_needed;
    *values_pad += vp_alloc - vp_needed;

    *struct_eff += TRIE_RADIX_STRUCT_EFF;
    *struct_pad += sizeof(struct TrieRadix) - TRIE_RADIX_STRUCT_EFF;
    *child_pad  += ch_alloc - ch_needed;              /* 0 when no children */

    for (i = 0; i < trie.children_num; i++) {
        _sizeof_trie_breakdown(trie.children[i], name_eff, name_null, name_pad,
                               values_eff, values_pad, struct_eff, struct_pad, child_pad);
    }
}

/**
 * Print the effective-vs-overhead memory breakdown described in the paper,
 * verifying that effective + padding equals the actual measured size.
 */
static void sizeof_trie_breakdown(struct TrieRadix trie) {
    size_t name_eff = 0, name_null = 0, name_pad = 0;
    size_t values_eff = 0, values_pad = 0;
    size_t struct_eff = 0, struct_pad = 0, child_pad = 0;
    _sizeof_trie_breakdown(trie, &name_eff, &name_null, &name_pad,
                           &values_eff, &values_pad,
                           &struct_eff, &struct_pad, &child_pad);

    size_t effective = name_eff + values_eff + struct_eff;
    size_t padding   = name_null + name_pad + values_pad + struct_pad + child_pad;
    size_t total     = effective + padding;

    printf("Memory breakdown (bytes):\n");
    printf("  Function-call identifiers (V = sum values_len):\n");
    printf("    [F ] Effective   sum(strlen)             : %zu B\n", name_eff);
    printf("    [pn] Null terminators   sum(1) = V       : %zu B\n", name_null);
    printf("    [pN] Name buffer padding                 : %zu B\n", name_pad);
    printf("  Values pointer array (compressed chain):\n");
    printf("    [P ] Effective   V * %zu                  : %zu B\n", sizeof(char *), values_eff);
    printf("    [pP] Values array padding                : %zu B\n", values_pad);
    printf("  Node structure (struct + children array):\n");
    printf("    [S ] Effective   N * %zu                  : %zu B\n", (size_t)TRIE_RADIX_STRUCT_EFF, struct_eff);
    printf("    [pS] Struct padding   N * %zu              : %zu B\n", sizeof(struct TrieRadix) - TRIE_RADIX_STRUCT_EFF, struct_pad);
    printf("    [pC] Children array padding              : %zu B\n", child_pad);
    printf("  Effective  [F]+[P]+[S]                     : %zu B  (%.1f%% of actual)\n",
           effective, total ? 100.0 * effective / total : 0.0);
    printf("  Padding    [pn]+[pN]+[pP]+[pS]+[pC]        : %zu B  (%.1f%% of actual)\n",
           padding, total ? 100.0 * padding / total : 0.0);
    printf("  Actual (== Size above)                     : %zu B\n", total);
}

void trie_radix_free(struct TrieRadix *root) {
    unsigned int i;
    for (i = 0; i < root->children_num; i++) {
        trie_radix_free(&root->children[i]);
    }
    if (root->children_num > 0) {
        free(root->children);
    }
    for (i = 0; i < root->values_len; i++) {
        free(root->values[i]);
    }
    free(root->values);
}

/*
 * Write the identifiers of one radix node and, after them, those of its children
 * (a depth-first walk). Each identifier of the chain is a node of the plain trie
 * whose parent is the previous identifier of the chain; the first identifier of
 * a radix node hangs from the last identifier of its parent radix node.
 */
static void export_radix_node(FILE *f, const struct TrieRadix *r, unsigned int parent,
                              int parent_depth, unsigned int *next_node,
                              unsigned int *next_rnode) {
    unsigned int i;
    unsigned int rnode = ++(*next_rnode);

    for (i = 0; i < r->values_len; i++) {
        unsigned int node = ++(*next_node);
        int depth = parent_depth + 1;
        fprintf(f, "%u,%u,%d,%u,%u,%s\n", node, parent, depth, rnode, i, r->values[i]);
        parent = node;
        parent_depth = depth;
    }
    for (i = 0; i < r->children_num; i++) {
        export_radix_node(f, &r->children[i], parent, parent_depth, next_node, next_rnode);
    }
}

int trie_radix_export(const struct TrieRadix *root, const char *path) {
    unsigned int next_node = 0, next_rnode = 0;
    FILE *f = fopen(path, "w");

    if (f == NULL) {
        perror(path);
        return -1;
    }
    fprintf(f, "node,parent,depth,rnode,offset,label\n");
    /* The root is the first identifier of the first radix node: parent 0, depth 0 */
    export_radix_node(f, root, 0, -1, &next_node, &next_rnode);
    fclose(f);
    return 0;
}

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

/* monitor.c

Driver comparing two implementations of the trace monitor:
  - a plain trie          (trie.c)
  - a radix trie          (trie_radix.c)

Both are built from the same traces so their memory figures can be compared
side by side; the search time of either one can be measured and written as raw
values. Shared measurement code lives in measure.c.
*/

#include "common.h"
#include "measure.h"
#include "trie.h"
#include "trie_radix.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_RUNS 1000000UL

static void usage(FILE *to, const char *prog) {
    fprintf(to,
        "Usage: %s -d DIR [options]\n"
        "\n"
        "Builds a plain trie and a radix trie from the traces in DIR (one trace per\n"
        "file, one function call per line) and prints their memory report.\n"
        "\n"
        "  -d, --traces DIR         directory with the traces (required)\n"
        "  -t, --trie FILE          write the structure of the trie, one line per node\n"
        "  -p, --plain-times FILE   time the plain trie and write the raw search times\n"
        "  -r, --radix-times FILE   time the radix trie and write the raw search times\n"
        "  -n, --runs N             searches per kind (known, unknown) and per trie\n"
        "                           (default %lu)\n"
        "  -s, --seed N             seed of the random choice of traces (default 1)\n"
        "  -h, --help               show this help\n"
        "\n"
        "The timing files have one line \"kind,average_ns,max_ns\" per search. Without\n"
        "-p and -r the traces are not searched and only the memory report is printed.\n"
        "\n"
        "Examples:\n"
        "  %s -d data/traces_rt_openat\n"
        "      memory report only\n"
        "  %s -d data/traces_rt_openat -t build/trie_nodes.csv -p build/search_times_raw.csv\n"
        "      memory report, trie structure, and search times of the plain trie\n"
        "  %s -d data/traces_rt_openat -p plain.csv -r radix.csv -n 100000 -s 7\n"
        "      search times of both tries, 100000 searches per kind, seed 7\n",
        prog, DEFAULT_RUNS, prog, prog, prog);
}

static void heading(const char *title) {
    printf("\n== %s ==\n\n", title);
}

/* Run the known and unknown searches of one trie and write the raw times to `path`. */
static int time_trie(const char *label, trace_test_fn test_fn, void *root,
                     char ***traces, unsigned int file_count, unsigned int *line_counts,
                     unsigned long runs, unsigned int seed, const char *path) {
    FILE *out = fopen(path, "w");
    if (out == NULL) {
        perror(path);
        return -1;
    }
    fprintf(out, "kind,average_ns,max_ns\n");
    benchmark_search(test_fn, root, traces, file_count, line_counts, 0, seed, runs, out);
    benchmark_search(test_fn, root, traces, file_count, line_counts, 1, seed, runs, out);
    fclose(out);
    printf("%s: raw search times written to %s\n", label, path);
    return 0;
}

int main(int argc, char **argv) {

    static const struct option long_options[] = {
        {"traces",      required_argument, NULL, 'd'},
        {"trie",        required_argument, NULL, 't'},
        {"plain-times", required_argument, NULL, 'p'},
        {"radix-times", required_argument, NULL, 'r'},
        {"runs",        required_argument, NULL, 'n'},
        {"seed",        required_argument, NULL, 's'},
        {"help",        no_argument,       NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    const char *dir = NULL, *trie_path = NULL, *plain_path = NULL, *radix_path = NULL;
    unsigned long runs = DEFAULT_RUNS;
    unsigned int seed = 1;
    char *end;
    int opt;

    while ((opt = getopt_long(argc, argv, "d:t:p:r:n:s:h", long_options, NULL)) != -1) {
        switch (opt) {
        case 'd': dir = optarg; break;
        case 't': trie_path = optarg; break;
        case 'p': plain_path = optarg; break;
        case 'r': radix_path = optarg; break;
        case 'n':
            runs = strtoul(optarg, &end, 10);
            if (*optarg == '\0' || *end != '\0' || runs == 0) {
                fprintf(stderr, "%s: --runs needs a positive integer, got '%s'\n", argv[0], optarg);
                return EXIT_FAILURE;
            }
            break;
        case 's':
            seed = (unsigned int)strtoul(optarg, &end, 10);
            if (*optarg == '\0' || *end != '\0') {
                fprintf(stderr, "%s: --seed needs a non-negative integer, got '%s'\n", argv[0], optarg);
                return EXIT_FAILURE;
            }
            break;
        case 'h': usage(stdout, argv[0]); return EXIT_SUCCESS;
        default:  usage(stderr, argv[0]); return EXIT_FAILURE;
        }
    }
    if (dir == NULL || optind != argc) {
        usage(stderr, argv[0]);
        return EXIT_FAILURE;
    }

    struct Trie      plain_trie;
    struct TrieRadix radix_trie;
    struct TrieSummary plain_summary = { .label = "plain trie" };
    struct TrieSummary radix_summary = { .label = "radix trie" };

    char ***traces;
    unsigned int file_count = 0, longest = 0, i;
    unsigned int *line_counts = NULL;

    traces = read_files_into_matrix(dir, &file_count, &line_counts);
    for (i = 0; i < file_count; i++) {
        if (line_counts[i] > longest) longest = line_counts[i];
    }
    printf("Traces: %s (%u files, the longest has %u lines)\n", dir, file_count, longest);

    heading("Plain trie");
    trie_build(&plain_trie, traces, file_count, line_counts, &plain_summary);

    heading("Radix trie");
    trie_radix_build(&radix_trie, traces, file_count, line_counts, &radix_summary);

    heading("Comparison");
    print_summary_comparison(&plain_summary, &radix_summary);

    /* Structure of the trie (the nodes of the plain trie, with the radix node
     * that holds each of them), for the on-chip memory evaluation of trie.Rmd */
    if (trie_path != NULL) {
        if (trie_radix_export(&radix_trie, trie_path) != 0) {
            return EXIT_FAILURE;
        }
        printf("\nTrie structure written to %s\n", trie_path);
    }

    /* Search times: only raw values are written; scripts/trie.Rmd computes the statistics. */
    if (plain_path != NULL || radix_path != NULL) {
        heading("Search times");
        printf("Seed %u, %lu searches per kind and per trie\n", seed, runs);
        if (plain_path != NULL &&
            time_trie("Plain trie", trie_test_trace, &plain_trie, traces, file_count,
                      line_counts, runs, seed, plain_path) != 0) {
            return EXIT_FAILURE;
        }
        if (radix_path != NULL &&
            time_trie("Radix trie", trie_radix_test_trace, &radix_trie, traces, file_count,
                      line_counts, runs, seed, radix_path) != 0) {
            return EXIT_FAILURE;
        }
    }

    /* Free it all */
    trie_free(&plain_trie);
    trie_radix_free(&radix_trie);
    free_traces_matrix(traces);
    free(line_counts);

    return 0;
}

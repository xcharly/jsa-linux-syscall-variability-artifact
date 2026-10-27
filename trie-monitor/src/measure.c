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

/* measure.c

Implementation of the shared measurement utilities.
*/

#include "measure.h"
#include "common.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

char ***read_files_into_matrix(const char *dir_path, unsigned int *file_count_out,
                               unsigned int **line_counts_out) {
    int i, j;
    DIR *dir;
    struct dirent *file;
    char full_path[1024];
    struct stat path_stat;
    FILE *f;
    char buffer[MAX_LINE_LEN];
    char *buffer_p;
    unsigned int i_file = 0, i_line = 0;
    unsigned int *line_counts = malloc(sizeof(unsigned int) * MAX_FILES);

    char ***matrix = malloc(sizeof(char **) * MAX_FILES);
    for (i = 0; i < MAX_FILES; i++) {
        matrix[i] = malloc(sizeof(char *) * MAX_LINES);
        for (j = 0; j < MAX_LINES; j++) {
            matrix[i][j] = malloc(MAX_LINE_LEN);
        }
    }

    dir = opendir(dir_path);
    if (dir == NULL) {
        perror(dir_path);
        fprintf(stderr, "The traces of the artifact must be extracted first (see README.md).\n");
        exit(EXIT_FAILURE);
    }
    while ((file = readdir(dir))) {
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, file->d_name);

        stat(full_path, &path_stat);
        if S_ISREG(path_stat.st_mode) { // We read regular files only
            f = fopen(full_path, "r");
            while (fgets(buffer, MAX_LINE_LEN, f)) {
                buffer[strcspn(buffer, "\n")] = '\0';   // Remove \n
                buffer_p = buffer;
                while (*buffer_p == ' ') buffer_p++;    // Remove leading spaces
                strcpy(matrix[i_file][i_line], buffer_p);
                i_line++;
            }
            line_counts[i_file] = i_line;
            i_line = 0;
            i_file++;
            fclose(f);
        }
    }
    closedir(dir);
    *file_count_out = i_file;
    *line_counts_out = line_counts;
    return matrix;
}

void free_traces_matrix(char ***matrix) {
    int i, j;
    for (i = 0; i < MAX_FILES; i++) {
        for (j = 0; j < MAX_LINES; j++) {
            free(matrix[i][j]);
        }
        free(matrix[i]);
    }
    free(matrix);
}

void benchmark_search(trace_test_fn test_fn, void *root,
                      char ***traces, unsigned int file_count,
                      unsigned int *line_counts, int modified,
                      unsigned int seed, unsigned long runs, FILE *out) {
    unsigned long i;
    unsigned int j, rand_i, rand_j;
    long double average_time;
    long max_time;
    char **single_trace;
    const char *kind = modified ? "unknown" : "known";

    srand(seed);

    for (i = 0; i < runs; i++) {
        rand_i = (unsigned int)(rand() % file_count);

        single_trace = malloc(line_counts[rand_i] * sizeof(char *));
        for (j = 0; j < line_counts[rand_i]; j++) {
            single_trace[j] = malloc(strlen(traces[rand_i][j]) + 1); // +1 for \0
            strcpy(single_trace[j], traces[rand_i][j]);
        }

        if (modified) {
            // Replace one random line: the trace is no longer in the trie
            rand_j = (unsigned int)(rand() % line_counts[rand_i]);
            free(single_trace[rand_j]);
            single_trace[rand_j] = malloc(strlen("MODIFICATION") + 1); // +1 for \0
            strcpy(single_trace[rand_j], "MODIFICATION");
        }

        test_fn(root, single_trace, line_counts[rand_i], &average_time, &max_time);

        for (j = 0; j < line_counts[rand_i]; j++) {
            free(single_trace[j]);
        }
        free(single_trace);

        fprintf(out, "%s,%.5Lf,%ld\n", kind, average_time, max_time);
    }
}

void print_summary_comparison(const struct TrieSummary *plain,
                              const struct TrieSummary *radix) {
    double node_ratio = plain->nodes ? 100.0 * radix->nodes / plain->nodes : 0.0;
    double mem_ratio  = plain->actual_bytes
                            ? 100.0 * radix->actual_bytes / plain->actual_bytes
                            : 0.0;

    printf("%-14s %14s %14s\n", "", plain->label, radix->label);
    printf("%-14s %14u %14u\n", "Nodes",        plain->nodes,        radix->nodes);
    printf("%-14s %14u %14u\n", "Stored calls", plain->stored_calls, radix->stored_calls);
    printf("%-14s %14zu %14zu\n", "Actual bytes", plain->actual_bytes, radix->actual_bytes);
    printf("\n%s uses %.1f%% of the nodes and %.1f%% of the memory of %s\n",
           radix->label, node_ratio, mem_ratio, plain->label);
}

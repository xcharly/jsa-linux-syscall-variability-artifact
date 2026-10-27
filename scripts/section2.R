# SPDX-License-Identifier: AGPL-3.0-only
#
# Copyright (C) 2025-2026 IKERLAN, S.Coop
#
# Author: Charles-Alexis Lefebvre
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Execution path and execution time variability of openat: statistics and Figs. 1-2 (Sections 3.3-3.4).
#
# Reads data/traces/<scenario>/runs/<campaign>/SyS_openat.csv for the first 100 campaigns of each
# scenario (regular and real-time Linux, in chronological order). Each file has one line of
# comma-separated entries "<md5 of the execution path>$$<execution time in us>", one entry per openat
# call (two calls per run of randbyte).
#
# It prints the statistics of Sections 3.3-3.4 (number of execution paths, new paths per campaign,
# share of the most common paths, paths needed to cover 99% and 99.9% of the executions, execution
# time range, and the campaign-level means with their 95% confidence intervals); the output must
# match scripts/section2_expected_output.txt.
# It also writes four PDF files to the current folder:
#   new_ep.pdf, new_ep_rt.pdf        Fig. 1: new execution paths found in each campaign
#   distribution.pdf, distribution_rt.pdf
#                                    Fig. 2: execution time of all the executions and of the twenty
#                                    most common execution paths, limited to 1,500 us
#
# Usage, from the artifact root, after extracting the data (tar xzf data/traces.tar.gz -C data):
#   Rscript scripts/section2.R

n_campaigns <- 100
n_common <- 20
y_limit <- 1500
data_dir <- file.path("data", "traces")

read_campaign <- function(file) {
  x <- scan(file, what = "", sep = ",", quiet = TRUE, strip.white = TRUE)
  x <- x[grepl("$$", x, fixed = TRUE)]
  parts <- strsplit(x, "$$", fixed = TRUE)
  data.frame(path = vapply(parts, `[`, "", 1),
             time = as.numeric(vapply(parts, `[`, "", 2)),
             stringsAsFactors = FALSE)
}

load_scenario <- function(scenario) {
  runs <- file.path(data_dir, scenario, "runs")
  if (!dir.exists(runs)) {
    stop("'", runs, "' not found. Run from the artifact root, after extracting the data: ",
         "tar xzf data/traces.tar.gz -C data", call. = FALSE)
  }
  campaigns <- head(sort(list.files(runs), method = "radix"), n_campaigns)
  data <- lapply(campaigns, function(c) read_campaign(file.path(runs, c, "SyS_openat.csv")))
  list(campaigns = campaigns, data = data)
}

# Number of paths of each campaign that no previous campaign had observed
new_paths <- function(data) {
  seen <- character(0)
  new <- integer(length(data))
  for (i in seq_along(data)) {
    paths <- unique(data[[i]]$path)
    new[i] <- sum(!(paths %in% seen))
    seen <- union(seen, paths)
  }
  new
}

# Smallest number of paths (most common first) that cover a share of the executions
paths_for <- function(counts, share) {
  which(cumsum(counts) >= share * sum(counts))[1]
}

# Mean and 95% confidence interval of the mean across campaigns (t distribution)
ci95 <- function(x) {
  h <- qt(0.975, length(x) - 1) * sd(x) / sqrt(length(x))
  sprintf("%.1f [%.1f, %.1f]", mean(x), mean(x) - h, mean(x) + h)
}

thousands <- function(x) format(x, big.mark = ",", scientific = FALSE, trim = TRUE)

campaign_level <- function(data, top) {
  path_sets <- lapply(data, function(d) unique(d$path))
  occurrences <- table(unlist(path_sets))
  only <- vapply(path_sets, function(p) sum(occurrences[p] == 1), numeric(1))
  cat("campaign level (mean and 95% CI across campaigns):\n")
  cat("  paths seen in no other campaign:            ", ci95(only), "\n", sep = "")
  cat("  share of the most common path (%):         ",
      ci95(vapply(data, function(d) 100 * mean(d$path == top), numeric(1))), "\n", sep = "")
  cat("  median execution time (us):                ",
      ci95(vapply(data, function(d) median(d$time), numeric(1))), "\n", sep = "")
  cat("  99th percentile of the execution time (us): ",
      ci95(vapply(data, function(d) unname(quantile(d$time, 0.99)), numeric(1))), "\n", sep = "")
  cat("  distinct paths per campaign:               ",
      ci95(vapply(data, function(d) length(unique(d$path)), numeric(1))), "\n", sep = "")
}

report <- function(name, scenario) {
  campaigns <- scenario$campaigns
  data <- scenario$data
  executions <- do.call(rbind, data)
  counts <- sort(table(executions$path), decreasing = TRUE)
  total <- sum(counts)
  new <- new_paths(data)
  cat(sprintf("=== %s: %d campaigns (%s .. %s)\n", name, length(campaigns), campaigns[1], tail(campaigns, 1)))
  cat(sprintf("openat executions: %s; unique execution paths: %s\n", thousands(total), thousands(length(counts))))
  cat(sprintf("new paths per campaign: first 5 [%s], mean of the last 20 campaigns %.1f\n",
              paste(head(new, 5), collapse = ", "), mean(tail(new, 20))))
  cat("most common paths: ", paste(sprintf("%.1f%%", 100 * head(counts, 3) / total), collapse = ", "), "\n", sep = "")
  cat(sprintf("paths needed to cover 99%% / 99.9%% of the executions: %d / %d\n",
              paths_for(counts, 0.99), paths_for(counts, 0.999)))
  cat(sprintf("execution time: min %.1f us, median %.1f us, max %s us\n",
              min(executions$time), median(executions$time), thousands(round(max(executions$time)))))
  campaign_level(data, names(counts)[1])
  list(new = new, executions = executions, counts = counts)
}

pdf_size <- list(width = 490 / 72, height = 340 / 72)

# Fig. 1
plot_new_paths <- function(new, file) {
  do.call(pdf, c(list(file), pdf_size))
  par(mar = c(5, 5, 2, 1) + 0.1)
  bars <- barplot(new, ylim = c(0, 150), col = "gray", axes = FALSE, ann = FALSE)
  abline(h = c(50, 100), col = "grey", lty = "longdash")
  barplot(new, ylim = c(0, 150), col = "gray", axes = FALSE, ann = FALSE, add = TRUE)
  axis(1, at = bars[seq(10, 100, 10)], labels = seq(10, 100, 10), cex.axis = 1.2)
  axis(2, at = seq(0, 150, 50), cex.axis = 1.2)
  box()
  title(xlab = "Campaigns", line = 3, cex.lab = 1.5)
  title(ylab = "New execution paths", line = 3, cex.lab = 1.5)
  invisible(dev.off())
}

# Fig. 2
plot_distribution <- function(executions, counts, file) {
  common <- names(counts)[order(-as.integer(counts), names(counts))][seq_len(n_common)]
  groups <- c(list(executions$time), lapply(common, function(p) executions$time[executions$path == p]))
  labels <- c("All", seq_len(n_common))

  do.call(pdf, c(list(file), pdf_size))
  par(mar = c(5, 5, 2, 1) + 0.1)
  # Empty frame with the coordinates of the box plot, so that the grid lines are behind the boxes
  plot(NA, xlim = c(0.5, length(groups) + 0.5), ylim = c(0, y_limit), xaxt = "n", yaxt = "n", ann = FALSE)
  abline(h = seq(0, y_limit, 500), col = "grey", lty = "longdash")
  boxplot(groups, ylim = c(0, y_limit), col = "gray", names = labels, xaxt = "n", yaxt = "n", ann = FALSE, add = TRUE)
  axis(1, at = seq_along(groups), labels = labels, las = 2, cex.axis = 1.2)
  axis(2, at = seq(0, y_limit, 500), cex.axis = 1.2)
  box()
  title(xlab = "Execution path", line = 4, cex.lab = 1.5)
  title(ylab = "Execution time (µs)", line = 3, cex.lab = 1.5)
  invisible(dev.off())
}

scenarios <- list(list(name = "regular", dir = "regular", new = "new_ep.pdf", dist = "distribution.pdf"),
                  list(name = "real-time", dir = "real-time", new = "new_ep_rt.pdf", dist = "distribution_rt.pdf"))
for (s in scenarios) {
  result <- report(s$name, load_scenario(s$dir))
  plot_new_paths(result$new, s$new)
  plot_distribution(result$executions, result$counts, s$dist)
}

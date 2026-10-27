# Reproducibility artifact

Artifact for the paper *Characterizing and Monitoring Linux System Call Variability for Safety-Critical Systems*.

It contains the configuration of the platform, the tracing tool, the program under test, the raw data of the comparison between the regular and real-time scenarios (Section 3), the trie-based monitor prototype with its execution-path traces (Sections 4.3-4.5), and the scripts that reproduce the statistics of Section 3, the node search times of Section 4.4 and the on-chip memory example (Section 4.5).

## Contents

| Folder | Content | Paper |
|---|---|---|
| `platform/` | Configuration of the Nvidia Jetson Nano used in both scenarios, extracted (read-only) from the SD-card image of the board, except the kernel symbol table (see `platform/kernel/`); `platform_info.txt` gathers the Jetson Linux release, the power mode, the kernel version strings and the enabled services | Section 3.2 |
| `platform/kernel/` | Complete kernel configurations of the regular (`config-4.9.253-tegra`) and real-time (`config-4.9.253-rt168-tegra`) kernels and the kernel symbol table of the real-time kernel (`kallsyms-4.9.253-rt168-tegra.txt.gz`, `/proc/kallsyms` read as root on the Jetson Nano) | Sections 3.2, 4.4 |
| `platform/boot/` | Kernel command line (`extlinux.conf`; the part added by the bootloader, `${cbootargs}`, is not captured), the boot service (`monitor-startup.service`) and the script it runs (`startup_script.sh`), which mounts tracefs and starts the DB4SIL2 client | Sections 3.1-3.2 |
| `db4sil2/` | DB4SIL2 tracing tool: archive of the DB4SIL2 repository (`DB4SIL2-git-2024-11-26.tar.gz`, with its history and its AGPLv3 license), from the repository gitlab.opentech.at/public_ot/db4sil2, which was publicly available on the date of its last commit (2024-11-26), and the patch with our modifications to commit 15c2c2b (`patches/`); how to obtain the version used: `db4sil2/README.txt` | Section 3.1 |
| `program/` | Source of the program under test, `randbyte`, which enables ftrace by itself | Section 3.2 |
| `data/traces.tar.gz` | Raw DB4SIL2 output of the regular and real-time campaigns (extracts to `data/traces/`): every execution with its execution path (MD5 identifier) and its duration (`runs/`) | Sections 3.3-3.4, Figs. 1-2 |
| `data/trie-monitor_run/` | Output of one run of the prototype on the Jetson Nano: the raw node search times (`search_times_raw.csv.gz`) and the structure of the trie (`trie_nodes.csv`) | Sections 4.4-4.5, Fig. 7 |
| `trie-monitor/` | Snapshot of the trie-based monitor prototype (C) and the 1,053 execution paths of `openat` from which it builds the trie (`trie-monitor/data/traces_rt_openat.tar.gz`, which extracts to `trie-monitor/data/traces_rt_openat/`) | Sections 4.3-4.5 |
| `scripts/` | `section2.R` (statistics of Section 3 from the raw data, and Figs. 1-2), `trie.Rmd` (node search times of Section 4.4 from the raw output of the prototype, Fig. 7, and the on-chip memory example of Section 4.5), with the expected output of `section2.R` | Sections 3, 4.4, 4.5 |

## Platform

| Item | Configuration | Source in this artifact |
|---|---|---|
| Board | Nvidia Jetson Nano, Jetson Linux R32.7.1, power mode 0 (`nvpmodel`) | `platform/platform_info.txt` |
| Regular kernel | Linux 4.9.253-tegra, preemptible kernel (`CONFIG_PREEMPT=y`), 250 Hz tick, tickless idle | `platform/kernel/config-4.9.253-tegra` |
| Real-time kernel | Linux 4.9.253-rt168-tegra: the same kernel patched with PREEMPT_RT (patch 4.9.253-rt168, maintained by the Real-Time Linux Collaborative Project), fully preemptible (`CONFIG_PREEMPT_RT_FULL=y`), 250 Hz tick, tickless idle | `platform/kernel/config-4.9.253-rt168-tegra` |
| Scheduling | Real-time scenario: SCHED_FIFO, priority 98 (`chrt -f 98` on the DB4SIL2 client, inherited by the program under test) | `platform/boot/startup_script.sh` |
| CPU frequency | `performance` governor (kernel default, `CONFIG_CPU_FREQ_DEFAULT_GOV_PERFORMANCE=y`) | kernel configurations |
| CPU affinity | No isolated CPUs (no `isolcpus`), no CPU pinning in the DB4SIL2 client | `platform/boot/extlinux.conf`, `db4sil2/src/test_client.py` |
| Interrupt affinity | Default (no `irqaffinity` parameter) | `platform/boot/extlinux.conf` |
| Background services | Default services of the Jetson Linux image | `platform/platform_info.txt` |
| Thermal conditions | Not controlled (laboratory ambient) | - |
| KASLR | Not active: `CONFIG_RANDOMIZE_BASE=y` in both configurations, but the symbol table of the real-time kernel has `_text` at the default address (`ffffff8008080000`), i.e., the kernel ran without address randomization | `platform/kernel/` |
| Tracing | `ftrace` function-graph tracer, options `funcgraph-proc`, `irq-info` and `funcgraph-irqs`, filtered on the PID of the program under test; trace buffer of CPU 1 set to 1,500,000 KB | `db4sil2/src/test_client.py`, `program/randbyte.c`, `platform/boot/startup_script.sh` |
| Reboot | The DB4SIL2 client reboots the board at the end of each campaign; the boot service starts the next campaign | `db4sil2/src/test_client.py`, `platform/boot/` |

Both scenarios use the same Jetson Linux image. The real-time scenario differs in two respects: the kernel is patched with PREEMPT_RT, and the program under test is scheduled with SCHED_FIFO at priority 98. Besides the preemption model, the two kernel configurations differ in about thirty other options, most of which come with the real-time build (for example, the `interactive` and `schedutil` CPU-frequency governors are only available in the regular kernel); the default governor is `performance` in both.

## Data

The traces are stored as two archives.

```bash
tar xzf data/traces.tar.gz -C data
tar xzf trie-monitor/data/traces_rt_openat.tar.gz -C trie-monitor/data
```

After extracting, `data/traces/regular/` and `data/traces/real-time/` hold the raw output for the 100 campaigns of each scenario used in the paper (1,000 runs each), one folder per campaign, named after its start time.

- `runs/<campaign>/SyS_<system call>.csv` (for example `SyS_openat.csv`): every execution of the system call in the campaign, as comma-separated entries `<md5 of its execution path>$$<execution time in microseconds>` on one line.

The execution paths from which the prototype builds the trie (lists of kernel functions) are in `trie-monitor/data/traces_rt_openat/`, one file per path.

`data/trie-monitor_run/search_times_raw.csv.gz` holds the raw output of the trie prototype (1,000,000 runs on known paths and 1,000,000 on unknown paths, one line `kind,average_ns,max_ns` per run) for a run on the Jetson Nano with the real-time kernel, built with `OPT=O2`, started with `sudo chrt -f 98`, seed 1, on a board with the graphical session stopped (`systemctl isolate multi-user.target`), power mode 0 (`nvpmodel -m 0`) and the clocks at maximum (`jetson_clocks`). Its statistics, computed by `scripts/trie.Rmd` with the outlier fences at 1.5 times the IQR.

## Collecting the traces

1. Prepare the board with Jetson Linux R32.7.1 and build its kernel with `platform/kernel/config-4.9.253-tegra` for the regular scenario; for the real-time scenario, patch it with PREEMPT_RT 4.9.253-rt168 and build it with `platform/kernel/config-4.9.253-rt168-tegra`.
2. Obtain DB4SIL2 as described in `db4sil2/README.txt` (extract the archive, check out commit 15c2c2b, apply the patch); the files cited in this README as `db4sil2/src/...` are in `db4sil2/DB4SIL2/src/` once these steps are done (run them on Linux: on Windows, the checkout fails because `tar` changes file modes and symbolic links). Install the DB4SIL2 client on the board and the server on a PC connected through Ethernet (`README` and `src/db4sil2.cfg` of the patched repository: 1,000 runs per campaign, server at 10.0.0.1:4713). The server identifies the execution paths by their MD5 hash and records the execution times.
3. Install `platform/boot/monitor-startup.service` and `platform/boot/startup_script.sh` on the board. At every boot, the script mounts tracefs, mounts the NFS export `10.0.0.1:/srv/nano2` of the server on `/nfsclient`, sets the trace buffer of CPU 1 and starts the client from `/nfsclient/src`; the service runs the script as `/home/nano1/startup_script.sh` (`ExecStart` of the service), which must be executable; in the real-time scenario the client is started with `chrt -f 98` (line 13 of the script; for the regular scenario, line 12 is used instead). Compile the program under test, `program/randbyte.c` (it sets `set_ftrace_pid` to its own PID and enables `tracing_on` itself; the client does not start tracing), and list it in `db4sil2/src/tests_list` (the patch lists `/home/nano1/randbyte`).
4. Each boot runs one campaign of 1,000 runs, after which the client reboots the board. The paper uses 100 campaigns per scenario.

## Reproducing the results

- **Comparison of the scenarios and Figs. 1-2 (Section 3).**

  ```bash
  Rscript scripts/section2.R
  ```

  It needs the extracted `data/traces/` (see "Data"). From these data, and with base R only, it recomputes the number of execution paths of `openat` (2,737 and 1,053), the new paths per campaign, the share of the most common paths, the paths needed to cover 99% and 99.9% of the executions, the execution-time range, and the campaign-level means with their 95% confidence intervals reported in Section 3.4; the printed output must match `scripts/section2_expected_output.txt` (on Windows, R writes CRLF line endings). It also writes, to the current folder, the new execution paths found in each campaign (`new_ep.pdf`, `new_ep_rt.pdf`: Fig. 1) and the execution time of all the executions and of the twenty most common execution paths (`distribution.pdf`, `distribution_rt.pdf`: Fig. 2) for regular and real-time Linux.
- **Trie prototype, memory (Sections 4.4-4.5).** The prototype needs Linux with glibc: the padding terms of its memory report come from `malloc_usable_size()`, a glibc extension.

  ```bash
  cd trie-monitor && make
  ./build/monitor --traces data/traces_rt_openat
  ./build/monitor --help
  cd ..
  ```

  The prototype (`--traces DIR` is the only required option; `--help` lists the others) builds the plain trie and the radix trie from the 1,053 paths of `trie-monitor/data/traces_rt_openat/` (extract the archive first, see "Data") and prints their memory report. The padding terms depend on the machine, by a few bytes: the files are read in the order returned by `readdir`, which changes the order in which the children arrays grow, and the allocator can round the name buffers differently. On the Jetson Nano the report is exactly the one above (children padding 6,448 B, name-buffer padding 366,579 B, 2,341,232 B in total, the values of the paper); on x86 machines we measured 2,341,248 B in total, with the 16 extra bytes in the children-array padding (6,464 B, ext4 file system) or in the name-buffer padding (366,595 B, WSL). The effective sizes do not depend on it. The memory of the two tries is given only by this report.
- **Structure of the trie.** Besides the report, the prototype writes, with the option `--trie FILE`, the structure of the trie to `FILE` (about 1.8 MB): one line per node of the plain trie (`node,parent,depth,rnode,offset,label`), taken from the radix trie that it builds, with the radix node that holds each node and its position in the chain. The file of the paper is `data/trie-monitor_run/trie_nodes.csv`, written by the prototype on the Jetson Nano; `scripts/trie.Rmd` reads it. On another machine, the numbering of the nodes can differ, because the files are read in the order returned by `readdir`, but the structure of the trie and every result of `scripts/trie.Rmd` are the same.
- **Trie prototype, search times (Section 4.4).** With `--plain-times FILE` the prototype runs `--runs` searches (default 1,000,000) of known paths and as many of unknown paths (a path of the trie with one random line replaced) on the plain trie, and writes the raw times of each run, one line `kind,average_ns,max_ns`, to `FILE` (about 40 MB for 1,000,000 runs; compress it with `gzip`). With `--radix-times FILE` it does the same, with the same traces, on the radix trie (the paper evaluates the search time of the plain trie only). The seed (`--seed`, default 1) selects the traces; the times themselves depend on the platform, so the paper's values are those of a run on the Jetson Nano (real-time kernel) built with `OPT=O2` (the Makefile default) and started, from `trie-monitor/`, with `sudo chrt -f 98 ./build/monitor --traces data/traces_rt_openat --trie build/trie_nodes.csv --plain-times build/search_times_raw.csv` (SCHED_FIFO, priority 98); with `OPT=O0` the board gives about 135-140 ns instead of 110-115 ns, and without `chrt` the outliers are much larger. 
  
- **Plain trie (Section 4.4).** It is the first of the trie parts of `scripts/trie.Rmd`, rendered by the command above. It reads the structure of the trie written by the prototype (`data/trie-monitor_run/trie_nodes.csv`) and shows the plain trie: 47,771 nodes (with the root), with a depth of 547 function calls, the same node count as the memory report of the prototype.
- **Radix trie (Section 4.5).** It is the radix trie of the same file, as two tables (one row per radix node with the length of its chain, and for every identifier the radix node that contains it and its position in it): 1,643 radix nodes with 47,771 stored calls, as in the report of the prototype. The Rmd does not build the tries and does not compute their memory: both come from the C prototype.
- **On-chip memory organization (Section 4.5).** It is the last part of `scripts/trie.Rmd`, and it is independent of the memory of the trie. It follows the paper, with its notation.

- **Number of system calls (Section 4.4).**

  ```bash
  gzip -dc platform/kernel/kallsyms-4.9.253-rt168-tegra.txt.gz | grep -c -E ' (compat_)?SyS_'
  ```

  It counts the system-call entries of the kernel symbol table (317 `SyS_` and 116 `compat_SyS_`) and prints 433.

The trie prototype reads the execution paths from the folder given with `--traces` (`data/traces_rt_openat` when it is run from `trie-monitor/`); the scripts do not read this folder: `section2.R` reads `data/traces/`, and `trie.Rmd` reads `data/trie-monitor_run/`.

## Licenses

- `trie-monitor/` (source code) and `scripts/`: GNU Affero General Public License v3 (`trie-monitor/LICENSE`, `scripts/LICENSE`). Copyright IKERLAN, S.Coop.
- `data/` and `trie-monitor/data/`: Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (`data/LICENSE`, `trie-monitor/data/LICENSE`). Copyright IKERLAN, S.Coop.
- `db4sil2/`: AGPL-3.0 (`LICENSE` in `DB4SIL2-git-2024-11-26.tar.gz`), the license of the current state of DB4SIL2, with our modifications in `db4sil2/patches/` (author: Markel Galarraga).

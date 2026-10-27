DB4SIL2 and the patch used in the paper

DB4SIL2/
    DB4SIL2 from the repository https://gitlab.opentech.at/public_ot/db4sil2, which
    was publicly available on the date of its last commit (2024-11-26), branch
    master, with its full history. Tip of master:
    27db9ee3dc2255a5cbc37ca81a5736b6073fa585. It contains LICENSE (GNU
    Affero General Public License v3), which is the license of the current state
    of DB4SIL2.

patches/15c2c2b_read_execution_times.patch
    Patch by Markel Galarraga. It modifies eight files in src/ (db4sil2.cfg,
    ftracelexer.py, process_trace.py, test_client.py, test_server.py,
    test_server.sh, test_worker.py, tests_list) to read the execution time of
    each system call and to set up measurement campaigns. It is written against
    commit 15c2c2bd7a0d8cbc04de331b018796c89f383a78 (2020-03-02, "data-set0 formed
    by different systemcalls results") and does not apply to the tip of master.

Obtaining the version used in the paper

    tar xzf DB4SIL2-git-2024-11-26.tar.gz
    cd DB4SIL2
    git checkout 15c2c2bd7a0d8cbc04de331b018796c89f383a78
    git apply ../patches/15c2c2b_read_execution_times.patch


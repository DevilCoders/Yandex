"""
WAL coverage test data checker
"""

import argparse
import logging
import os
import subprocess
import sys

logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)-8s: %(message)s')

LOG = logging.getLogger('main')

# We are skipping some hard to generate rmgr/info combinations:
# Btree/DELETE
# CLOG/TRUNCATE
# Gist/DELETE
# Hash/VACUUM_ONE_PAGE
# Heap/CONFIRM
# Heap2/LOCK_UPDATED
# Heap2/REWRITE
# MultiXact/TRUNCATE_ID
# RelMap/UPDATE

COMBOS = {
    'XLOG': set(
        [
            'NEXTOID',
            'CHECKPOINT_SHUTDOWN',
            'CHECKPOINT_ONLINE',
            'SWITCH',
            'FPI',
            'FPI_FOR_HINT',
            'PARAMETER_CHANGE',
        ],
    ),
    'Database': set(['CREATE_FILE_COPY', 'DROP']),
    'Sequence': set(['LOG']),
    'Tablespace': set(['CREATE', 'DROP']),
    'MultiXact': set(['CREATE_ID', 'ZERO_MEM_PAGE', 'ZERO_OFF_PAGE']),
    'BRIN': set(
        [
            'CREATE_INDEX',
            'INSERT',
            'INSERT+INIT',
            'REVMAP_EXTEND',
            'SAMEPAGE_UPDATE',
            'REVMAP_EXTEND',
            'DESUMMARIZE',
        ]
    ),
    'CLOG': set(['ZEROPAGE']),
    'Hash': set(
        [
            'INIT_META_PAGE',
            'INIT_BITMAP_PAGE',
            'INSERT',
            'ADD_OVFL_PAGE',
            'SPLIT_ALLOCATE_PAGE',
            'SPLIT_PAGE',
            'SPLIT_COMPLETE',
            'MOVE_PAGE_CONTENTS',
            'SQUEEZE_PAGE',
            'DELETE',
            'SPLIT_CLEANUP',
            'UPDATE_META_PAGE',
        ]
    ),
    'Storage': set(['CREATE', 'TRUNCATE']),
    'Generic': set(['Generic']),
    'Gin': set(
        [
            'INSERT',
            'CREATE_PTREE',
            'SPLIT',
            'VACUUM_PAGE',
            'VACUUM_DATA_LEAF_PAGE',
            'DELETE_PAGE',
            'UPDATE_META_PAGE',
            'INSERT_LISTPAGE',
            'DELETE_LISTPAGE',
        ]
    ),
    'SPGist': set(
        [
            'ADD_LEAF',
            'MOVE_LEAFS',
            'ADD_NODE',
            'SPLIT_TUPLE',
            'PICKSPLIT',
            'VACUUM_LEAF',
            'VACUUM_ROOT',
            'VACUUM_REDIRECT',
        ]
    ),
    'Gist': set(['PAGE_UPDATE', 'PAGE_REUSE', 'PAGE_SPLIT', 'PAGE_DELETE']),
    'Standby': set(['LOCK', 'RUNNING_XACTS', 'INVALIDATIONS']),
    'Transaction': set(
        ['ASSIGNMENT', 'PREPARE', 'ABORT_PREPARED', 'ABORT', 'COMMIT', 'COMMIT_PREPARED', 'INVALIDATION']
    ),
    'Heap': set(
        [
            'INSERT',
            'INSERT+INIT',
            'DELETE',
            'UPDATE',
            'UPDATE+INIT',
            'TRUNCATE',
            'HOT_UPDATE',
            'LOCK',
            'INPLACE',
        ]
    ),
    'Heap2': set(
        [
            'PRUNE',
            'VACUUM',
            'FREEZE_PAGE',
            'VISIBLE',
            'MULTI_INSERT',
            'MULTI_INSERT+INIT',
            'NEW_CID',
            'VISIBLE',
        ]
    ),
    'Btree': set(
        [
            'DEDUP',
            'INSERT_LEAF',
            'INSERT_UPPER',
            'INSERT_POST',
            'INSERT_META',
            'SPLIT_L',
            'SPLIT_R',
            'VACUUM',
            'MARK_PAGE_HALFDEAD',
            'META_CLEANUP',
            'UNLINK_PAGE',
            'UNLINK_PAGE_META',
            'NEWROOT',
            'REUSE_PAGE',
        ]
    ),
}


def report_missing(seen):
    """
    Report rmgr/info combinations that are missing from checked WAL
    """
    failed = False
    for rmgr in sorted(COMBOS):
        missing_infos = COMBOS[rmgr].difference(seen.get(rmgr, set()))
        unexpected_infos = seen.get(rmgr, set()).difference(COMBOS[rmgr])
        if missing_infos or unexpected_infos:
            failed = True
        print(
            f'Resource manager: {rmgr}\n\n'
            f'\tSeen: {", ".join(sorted(seen.get(rmgr, set())))}\n'
            f'\tMissing: {", ".join(sorted(missing_infos))}\n'
            f'\tUnexpected: {", ".join(sorted(unexpected_infos))}\n'
        )

    if failed:
        sys.exit(1)


def get_last_checkpoint_info(bin_dir, data_dir):
    """
    Get last checkpoint wal file and lsn from controlfile
    """
    cmd = [os.path.join(bin_dir, 'bin', 'pg_controldata'), '-D', data_dir]
    out = subprocess.check_output(cmd).decode('utf-8')
    lsn = None
    wal_file = None
    for line in out.splitlines():
        if line.startswith('Latest checkpoint location:'):
            lsn = line.split()[-1].strip()
        elif line.startswith('Latest checkpoint\'s REDO WAL file:'):
            wal_file = line.split()[-1].strip()

    if not (lsn and wal_file):
        raise RuntimeError(f'Unexpected pg_controldata output format (parsed lsn: {lsn} and wal file: {wal_file})')

    return wal_file, lsn


def get_waldump_out(bin_dir, data_dir, start_wal_file_name, start_lsn, end_wal_file_name, end_lsn):
    """
    Get waldump output splitted by lines
    """
    cmd = [
        os.path.join(bin_dir, 'bin', 'pg_waldump'),
        '-p',
        data_dir,
        '-s',
        start_lsn,
        '-e',
        end_lsn,
        start_wal_file_name,
    ]
    if start_wal_file_name != end_wal_file_name:
        cmd.append(end_wal_file_name)
    LOG.info('Starting pg_waldump: %s', ' '.join(cmd))
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (waldump_out, _) = proc.communicate()
    return waldump_out.decode('utf-8').splitlines()


def validate_coverage(waldump_out):
    """
    Check that all expected combinations of rmgr/info found in WAL
    """
    seen = {}
    for line in waldump_out:
        rmgr = None
        info = None
        expect_next = None
        for token in line.split():
            if token == 'rmgr:':
                expect_next = 'rmgr'
            elif token == 'desc:':
                expect_next = 'info'
            elif expect_next == 'rmgr':
                rmgr = token
                expect_next = None
            elif expect_next == 'info':
                info = token
                expect_next = None
        if info and rmgr not in seen:
            seen[rmgr] = set()
        seen[rmgr].add(info)

    report_missing(seen)


def main():
    """
    Console entry point
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('--bin-dir', type=str, required=True)
    parser.add_argument('--init-data-dir', type=str, default='data_init')
    parser.add_argument('--result-data-dir', type=str, default='data_result')

    args = parser.parse_args()

    start_wal_file_name, start_lsn = get_last_checkpoint_info(args.bin_dir, args.init_data_dir)
    end_wal_file_name, end_lsn = get_last_checkpoint_info(args.bin_dir, args.result_data_dir)

    out = get_waldump_out(
        args.bin_dir, args.result_data_dir, start_wal_file_name, start_lsn, end_wal_file_name, end_lsn
    )
    validate_coverage(out)

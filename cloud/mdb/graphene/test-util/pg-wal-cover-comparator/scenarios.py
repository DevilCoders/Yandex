"""
WAL coverage test data comparator scenarios
"""

import os
import subprocess
import tempfile


def get_end_lsn(waldump_out):
    """
    Get end lsn from waldump output
    """
    rec_len = None
    start_lsn = None
    for line in reversed(waldump_out):
        if not line:
            continue
        tokens = line.split()
        rec_len = int(tokens[5].replace(',', ''))
        start_lsn = tokens[9].replace(',', '')
        break
    if rec_len is None or start_lsn is None:
        raise RuntimeError('Unable to get end lsn from waldump output')
    if (rec_len % 8) != 0:
        rec_len += 8 - (rec_len % 8)
    return '0/{lsn:08X}'.format(lsn=int(start_lsn.split("/")[1], 16) + rec_len)


def sequence_redo(waldump_out, pg_data_dir, storage_bin_dir, logger):
    """
    Compare storage-generated sequence with postgres-generated
    """
    logger.info('Getting position from waldump')
    for line in reversed(waldump_out):
        if not line:
            continue
        tokens = line.split()
        if tokens[1] != 'Sequence':
            continue
        rel = tokens[19]
        break
    logger.info('Will operate on rel %s', rel)
    with tempfile.TemporaryDirectory(prefix='sequence_redo') as work_dir:
        end_lsn = get_end_lsn(waldump_out)
        logger.info('Exporting page at %s', end_lsn)
        subprocess.check_call(
            [
                os.path.join(storage_bin_dir, 'storage_cli'),
                'page-export',
                '--coords',
                f'{rel}/0/0',
                '--lsn',
                end_lsn,
                '--file',
                os.path.join(work_dir, 'generated'),
            ],
            env={'TSAN_OPTIONS': 'report_bugs=0'},
        )

        original_inspect_out = subprocess.check_output(
            [
                os.path.join(storage_bin_dir, 'page_inspect'),
                '--mask',
                '--path',
                os.path.join(pg_data_dir, f'base/{"/".join(rel.split("/")[1:])}'),
                '--type',
                'seq',
            ]
        ).decode('utf-8')

        generated_inspect_out = subprocess.check_output(
            [
                os.path.join(storage_bin_dir, 'page_inspect'),
                '--mask',
                '--path',
                os.path.join(work_dir, 'generated'),
                '--type',
                'seq',
            ]
        ).decode('utf-8')

        if original_inspect_out != generated_inspect_out:
            raise RuntimeError(
                'Original and generated pages are not equal:\n'
                f'original:\n{original_inspect_out}\n'
                f'generated:\n{generated_inspect_out}'
            )

        logger.info('OK')
        return {'matched': 1, 'skipped': 0}


def heap_parse(waldump_out):
    """
    Heap wal parse helper
    """
    coords = set()
    # We skip coords w/o init because dbase redo is hard to test with current initdb out of storage
    coords_with_init = set()
    for line in waldump_out:
        if not line:
            continue
        tokens = line.split()
        if tokens[1] not in ('Heap', 'Heap2'):
            continue
        refs = {}
        cur_ref = None
        state = None
        rel = None
        for token in tokens:
            if token == 'blkref':
                state = 'blkref'
            elif state == 'blkref' and token.startswith('#') and token.endswith(':'):
                cur_ref = int(token.replace('#', '').replace(':', ''))
                state = 'rel'
            elif state == 'rel' and token != 'rel' and '/' in token:
                rel = token
                state = 'blk'
            elif state == 'blk' and token == 'fork':
                state = None
                rel = None
                cur_ref = None
            elif state == 'blk' and token != 'blk':
                refs[cur_ref] = f'{rel}/{token.replace(",", "")}'
                state = None
                rel = None
                cur_ref = None
        if not refs:
            continue

        for ref, coord in refs.items():
            if '+INIT' in tokens[13] and ref == 0 and coord not in coords:
                coords_with_init.add(coord)
            coords.add(coord)

    return coords.intersection(coords_with_init)


def heap_redo(waldump_out, pg_data_dir, storage_bin_dir, logger):
    """
    Add all heap records and compare all pages
    """
    res = {'matched': 0, 'skipped': 0}
    coords = heap_parse(waldump_out)
    end_lsn = get_end_lsn(waldump_out)
    diffs = []
    with tempfile.TemporaryDirectory(prefix='heap_redo') as work_dir:
        for coord in coords:
            page_path = os.path.join(work_dir, 'generated')
            if os.path.exists(page_path):
                os.unlink(page_path)
            try:
                original_inspect_out = subprocess.check_output(
                    [
                        os.path.join(storage_bin_dir, 'page_inspect'),
                        '--path',
                        os.path.join(pg_data_dir, f'base/{"/".join(coord.split("/")[1:3])}'),
                        '--mask',
                        '--type',
                        'heap',
                        '--offset',
                        coord.split('/')[-1],
                    ]
                ).decode('utf-8')
            except subprocess.CalledProcessError:
                logger.info('Skipping %s (unable to inspect original)', coord)
                res['skipped'] += 1
                continue
            logger.info('Exporting page %s at %s', coord, end_lsn)
            full_coord = coord.split('/')
            full_coord.insert(3, '0')
            subprocess.check_call(
                [
                    os.path.join(storage_bin_dir, 'storage_cli'),
                    'page-export',
                    '--coords',
                    '/'.join(full_coord),
                    '--lsn',
                    end_lsn,
                    '--file',
                    page_path,
                ],
                env={'TSAN_OPTIONS': 'report_bugs=0'},
            )
            generated_inspect_out = subprocess.check_output(
                [os.path.join(storage_bin_dir, 'page_inspect'), '--mask', '--type', 'heap', '--path', page_path]
            ).decode('utf-8')

            if original_inspect_out != generated_inspect_out:
                diffs.append(
                    f'Original and generated pages for {coord} are not equal:\n'
                    f'original:\n{original_inspect_out}\n'
                    f'generated:\n{generated_inspect_out}'
                )
            else:
                res['matched'] += 1
    if diffs:
        raise RuntimeError('\n'.join(diffs))

    logger.info('OK')
    return res


def btree_parse(waldump_out):
    """
    Btree wal parse helper
    """
    coords = set()
    for line in waldump_out:
        if not line:
            continue
        tokens = line.split()
        if tokens[1] != 'Btree':
            continue
        refs = {}
        cur_ref = None
        state = None
        rel = None
        for token in tokens:
            if token == 'blkref':
                state = 'blkref'
            elif state == 'blkref' and token.startswith('#') and token.endswith(':'):
                cur_ref = int(token.replace('#', '').replace(':', ''))
                state = 'rel'
            elif state == 'rel' and token != 'rel' and '/' in token:
                rel = token
                state = 'blk'
            elif state == 'blk' and token != 'blk':
                refs[cur_ref] = f'{rel}/{token.replace(",", "")}'
                state = None
                rel = None
                cur_ref = None
        if not refs:
            continue

        for coord in refs.values():
            coords.add(coord)

    return coords


def btree_redo(waldump_out, pg_data_dir, storage_bin_dir, logger):
    """
    Add all btree records and compare all pages
    """
    res = {'matched': 0, 'skipped': 0}
    coords = btree_parse(waldump_out)
    end_lsn = get_end_lsn(waldump_out)
    diffs = []
    with tempfile.TemporaryDirectory(prefix='btree_redo') as work_dir:
        for coord in coords:
            page_path = os.path.join(work_dir, 'generated')
            if os.path.exists(page_path):
                os.unlink(page_path)
            try:
                original_inspect_out = subprocess.check_output(
                    [
                        os.path.join(storage_bin_dir, 'page_inspect'),
                        '--path',
                        os.path.join(pg_data_dir, f'base/{"/".join(coord.split("/")[1:3])}'),
                        '--mask',
                        '--type',
                        'btree',
                        '--offset',
                        coord.split('/')[-1],
                    ]
                ).decode('utf-8')
            except subprocess.CalledProcessError:
                logger.info('Skipping %s (unable to inspect original)', coord)
                res['skipped'] += 1
                continue
            logger.info('Exporting page %s at %s', coord, end_lsn)
            full_coord = coord.split('/')
            full_coord.insert(3, '0')
            subprocess.check_call(
                [
                    os.path.join(storage_bin_dir, 'storage_cli'),
                    'page-export',
                    '--coords',
                    '/'.join(full_coord),
                    '--lsn',
                    end_lsn,
                    '--file',
                    page_path,
                ],
                env={'TSAN_OPTIONS': 'report_bugs=0'},
            )
            generated_inspect_out = subprocess.check_output(
                [os.path.join(storage_bin_dir, 'page_inspect'), '--mask', '--type', 'btree', '--path', page_path]
            ).decode('utf-8')

            if original_inspect_out != generated_inspect_out:
                diffs.append(
                    f'Original and generated pages for {coord} are not equal:\n'
                    f'original:\n{original_inspect_out}\n'
                    f'generated:\n{generated_inspect_out}'
                )
            else:
                res['matched'] += 1

    if diffs:
        raise RuntimeError('\n'.join(diffs))

    logger.info('OK')
    return res


def xact_parse(waldump_out):
    """
    Xact wal parse helper
    """
    coords = set()
    for line in waldump_out:
        if not line:
            continue
        tokens = line.split()
        if tokens[1] != 'CLOG':
            continue

        page = tokens[-1]
        coords.add(f'clog/{page}')

    return coords


def xact_redo(waldump_out, pg_data_dir, storage_bin_dir, logger):
    """
    Add all xact/clog records and compare result clog pages
    """
    res = {'matched': 0, 'skipped': 0}
    coords = xact_parse(waldump_out)
    end_lsn = get_end_lsn(waldump_out)
    diffs = []
    with tempfile.TemporaryDirectory(prefix='xact_redo') as work_dir:
        for coord in coords:
            page_path = os.path.join(work_dir, 'generated')
            if os.path.exists(page_path):
                os.unlink(page_path)
            orig_path = os.path.join(work_dir, 'original')
            if os.path.exists(orig_path):
                os.unlink(orig_path)
            subprocess.check_call(
                [
                    'dd',
                    f'if={os.path.join(pg_data_dir, "pg_xact/0000")}',
                    f'of={orig_path}',
                    'bs=8192',
                    'count=1',
                    f'skip={coord.split("/")[-1]}',
                ]
            )
            original_hexdump_out = subprocess.check_output(['hexdump', orig_path]).decode('utf-8')
            logger.info('Exporting page %s at %s', coord, end_lsn)
            subprocess.check_call(
                [
                    os.path.join(storage_bin_dir, 'storage_cli'),
                    'slru-page-export',
                    '--coords',
                    coord,
                    '--lsn',
                    end_lsn,
                    '--file',
                    page_path,
                ],
                env={'TSAN_OPTIONS': 'report_bugs=0'},
            )
            generated_hexdump_out = subprocess.check_output(['hexdump', orig_path]).decode('utf-8')

            if original_hexdump_out != generated_hexdump_out:
                diffs.append(
                    f'Original and generated pages for {coord} are not equal:\n'
                    f'original:\n{original_hexdump_out}\n'
                    f'generated:\n{generated_hexdump_out}'
                )
            else:
                res['matched'] += 1

    if diffs:
        raise RuntimeError('\n'.join(diffs))

    logger.info('OK')
    return res


TEST_SCENARIOS = [
    sequence_redo,
    heap_redo,
    btree_redo,
    xact_redo,
]

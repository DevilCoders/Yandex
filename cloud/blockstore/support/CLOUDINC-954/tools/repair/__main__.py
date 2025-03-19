import argparse
import concurrent.futures
import logging
import os
import requests
import sys
import time

from cloud.blockstore.public.sdk.python.client import CreateClient
from cloud.blockstore.public.sdk.python.client.error import ClientError

import cloud.blockstore.public.sdk.python.protos as protos
import cloud.blockstore.tools.nbsapi as nbs
import cloud.blockstore.tools.util as util


def align_ranges(ranges):
    if not ranges:
        return ranges

    aligned = []
    for s, n in ranges:
        l = (s // 1024) * 1024
        r = ((s + max(1, n) - 1) // 1024 + 1) * 1024
        aligned.append((l, r - l))

    aligned.sort(key=lambda x: x[0])

    result = [aligned[0]]

    for s in aligned:
        r = result[-1]

        l1 = r[0]
        r1 = l1 + r[1]

        l2 = s[0]
        r2 = l2 + s[1]

        if r1 < l2:     # separate
            result.append(s)
        elif r1 < r2:   # extend
            result[-1] = (l1, r2 - l1)

    return result


def mount_disk(client, disk_id):
    try:
        logging.info("mount disk %s" % disk_id)
        client.start_endpoint(
            unix_socket_path='/var/tmp/nbs-endpoint-%s' % disk_id,
            disk_id=disk_id,
            ipc_type=protos.EClientIpcType.Value("IPC_GRPC"),
            client_id='blockstore-repair',
            access_mode=protos.EVolumeAccessMode.Value("VOLUME_ACCESS_REPAIR"))

        return True
    except Exception as e:
        logging.exception("Exception at mount disk %s, e: %s. Skip" % (disk_id, e))
        return False


def unmount_disk(client, disk_id):
    try:
        logging.info("unmount disk %s" % disk_id)
        client.stop_endpoint(unix_socket_path='/var/tmp/nbs-endpoint-%s' % disk_id)
    except Exception as e:
        logging.exception("Exception at unmount disk %s, e: %s. Skip" % (disk_id, e))


def fire_compaction_http(tablet_id, blockIndex, blockCount):
    r = requests.post('http://localhost:8766/tablets/app?TabletID=%s' % tablet_id,
                      data={
                          'TabletID': tablet_id,
                          'BlockIndex': blockIndex,
                          'BlocksCount': blockCount,
                          'action': 'compact'
                      })

    r.raise_for_status()

    ok = r.text.find('Compaction has been started') != -1

    if not ok:
        i = r.text.find('alert-success')
        if i == -1:
            logging.info("Compaction response %s" % r.text)
        else:
            logging.info("Compaction response %s" % r.text[i:])

    return ok


def fire_compaction(client, disk_id, start_index, blocks_count):
    response = nbs.compact_disk(client, disk_id, start_index, blocks_count)

    logging.info("Start compaction %s" % response)

    op = response.get("OperationId")

    return op


def get_compaction_status(client, disk_id, operation_id):
    response = nbs.get_compaction_status(client, disk_id, operation_id)

    logging.info('Compaction %s progress: %s' % (operation_id, response))

    if response.get('Error'):
        raise Exception(response.get('Error'))

    return response.get('IsCompleted', False)


def rescan_disk(client, disk_id, prev_blocks, describe_parallel):
    logging.info("rescan disk %s, blocks: %d" % (disk_id, len(prev_blocks)))

    blocks = []
    error_ranges = []

    progress = 0

    cache = util.BlobIdCache()

    checkpoint_id = "blockstore-repair"

    try:
        logging.info("creating checkpoint %s" % checkpoint_id)
        client.create_checkpoint(disk_id, checkpoint_id)

        with concurrent.futures.ThreadPoolExecutor(max_workers=describe_parallel) as executor:
            future_to_range = dict()

            for r in prev_blocks:
                future_to_range[
                    executor.submit(
                        util.find_no_data_ranges,
                        client,
                        disk_id,
                        r[0],
                        r[1],
                        checkpoint_id,
                        cache=cache
                    )
                ] = r

            for future in concurrent.futures.as_completed(future_to_range):
                r = future_to_range[future]

                progress += 1
                logging.info("rescan progress: %d/%d" % (progress, len(future_to_range)))

                try:
                    for n in future.result():
                        blocks.append(n)
                except Exception as e:
                    logging.exception("Exception while handling range: %s, e: %s" % (r, e))
                    error_ranges.append(r)

    except Exception as e:
        logging.exception("Exception while scanning disk %s, e: %s" % (disk_id, e))
        raise e
    finally:
        logging.info("deleting checkpoint %s" % checkpoint_id)
        client.delete_checkpoint(disk_id, checkpoint_id)

    logging.info("cache hits: %d/%d" % (cache.hits, cache.reqs))
    logging.info("found blocks: %d error ranges: %d" % (len(blocks), len(error_ranges)))
    logging.debug("found blocks: %s" % blocks)

    if len(error_ranges) == len(future_to_range):
        raise Exception('bad rescan')

    return (blocks, error_ranges)


def scan_disk(client, disk_id, block_count, describe_parallel):
    logging.info("scan disk %s" % disk_id)

    blocks = []
    error_ranges = []
    i = 0
    batch = 100000

    cache = util.BlobIdCache()

    progress = 0

    checkpoint_id = "blockstore-repair"

    try:
        logging.info("creating checkpoint %s" % checkpoint_id)
        client.create_checkpoint(disk_id, checkpoint_id)

        with concurrent.futures.ThreadPoolExecutor(max_workers=describe_parallel) as executor:
            future_to_range = dict()

            while i < block_count:
                r = (i, min(batch, block_count - i))

                future_to_range[
                    executor.submit(
                        util.find_no_data_ranges,
                        client,
                        disk_id,
                        r[0],
                        r[1],
                        checkpoint_id,
                        cache=cache
                    )
                ] = r

                i += batch

            for future in concurrent.futures.as_completed(future_to_range):
                r = future_to_range[future]

                progress += 1
                logging.info("scan progress: %d/%d" % (progress, len(future_to_range)))

                try:
                    for n in future.result():
                        blocks.append(n)

                except Exception as e:
                    logging.exception("Exception while handling range: %s, e: %s" % (r, e))
                    error_ranges.append(r)

    except Exception as e:
        logging.exception("Exception while rescanning disk %s, e: %s" % (disk_id, e))
        raise e
    finally:
        logging.info("deleting checkpoint %s" % checkpoint_id)
        client.delete_checkpoint(disk_id, checkpoint_id)

    logging.info("cache hits: %d/%d" % (cache.hits, cache.reqs))
    logging.info("found blocks: %d error ranges: %d" % (len(blocks), len(error_ranges)))
    logging.debug("found blocks: %s" % blocks)

    if len(error_ranges) == len(future_to_range):
        raise Exception('bad scan')

    return (blocks, error_ranges)


def compact(client, disk_id, blocks):
    logging.info("compact disk %s, block count: %d" % (disk_id, len(blocks)))

    i = 0

    for start_index, n in blocks:
        logging.info("compact range %s : %s" % (start_index, n))

        op = None

        attempts = 0
        while True:
            logging.info("total compaction progress: %d/%d" % (i, len(blocks)))

            if attempts > 100:
                logging.warn('too many attempts at compaction. Skip')
                return False

            attempts += 1

            if not mount_disk(client, disk_id):
                logging.warn("can't mount disk at compaction: %s. Skip" % disk_id)
                time.sleep(10.0)
                continue
            try:
                if op is None:
                    op = fire_compaction(client, disk_id, start_index, n)

                if op is None:
                    time.sleep(1.0)
                    continue

                if get_compaction_status(client, disk_id, op):
                    logging.info('compaction %s done' % op)
                    op = None
                    break
                time.sleep(1.0)
            except Exception as e:
                logging.exception('Exception at compaction, e: %s' % e)
                time.sleep(10.0)
                op = None

        i += 1

    return True


def load_ranges(disk_id):
    ranges = []

    try:
        filename = disk_id + '.ranges'
        if not os.path.exists(filename):
            return []

        with open(filename) as f:
            for line in f.readlines():
                if not line:
                    continue
                s, n = line.split()
                ranges.append((int(s), int(n)))
    except Exception as e:
        logging.exception('Cannot restore ranges for disk %s, e: %s' % (disk_id, e))

    return ranges


def dump_ranges(disk_id, ranges):
    try:
        with open(disk_id + '.ranges', 'w') as f:
            for r in ranges:
                f.write("%s %s\n" % r)
    except Exception as e:
        logging.exception('cannot dump ranges for disk %s, e: %s' % (disk_id, e))


def repair(
        client,
        disk_id,
        parallel,
        scan_only,
        skip_scan,
        save_ranges,
        block_count):

    logging.info('disk id: %s block count: %s' % (disk_id, block_count))

    scan_errors = 0
    attempts = 0

    blocks = []
    error_ranges = []

    no_progress = 0

    if save_ranges:
        blocks = load_ranges(disk_id)
        logging.info('loaded ranges: %d' % len(blocks))

    prev_block_count = len(blocks)

    while True:
        if scan_errors > 3:
            logging.warn('too many errors at scan. Skip disk')
            return False

        if attempts > 30:
            logging.warn('too many attempts. Skip disk')
            return False

        if not mount_disk(client, disk_id):
            logging.warn("can't mount disk: %s. Skip" % disk_id)
            return False

        attempts += 1

        try:
            if not skip_scan:
                if len(blocks) == 0 and len(error_ranges) == 0:
                    blocks, error_ranges = scan_disk(client, disk_id, block_count, parallel)
                else:
                    blocks, error_ranges = rescan_disk(client, disk_id, blocks + error_ranges, parallel)

            no_bad_blocks = len(blocks) == 0 and len(error_ranges) == 0

            if no_bad_blocks and prev_block_count == 0:
                print('%s - OK' % disk_id)
                break

            if save_ranges:
                dump_ranges(disk_id, blocks + error_ranges)

            if scan_only:
                break

            if no_bad_blocks:
                logging.info('final scan for %s' % disk_id)
                prev_block_count = 0
                attempts -= 1
                continue

            if len(blocks) == 0:
                logging.warn("no blocks to compaction. Rescan")
                continue

            if prev_block_count:
                logging.info("disk %s, progress: %d -> %d" % (disk_id, prev_block_count, len(blocks)))

                if prev_block_count <= len(blocks) and len(error_ranges) == 0:
                    no_progress += 1

            if no_progress > 5:
                logging.warn("no progress for disk %s. Skip" % disk_id)
                return False

            scan_errors = 0

            aligned = align_ranges(blocks)

            logging.info('align blocks: %d -> %d' % (len(blocks), len(aligned)))

            blocks = aligned
            prev_block_count = len(blocks)

            if not compact(client, disk_id, aligned):
                return False

            if not save_ranges:
                blocks = []
                error_ranges = []

        except ClientError as e:
            logging.exception('ClientError at scan')
            scan_errors += 1

            if not e.is_retriable:
                return False
            time.sleep(1.0)
        except Exception as e:
            logging.exception('Exception at scan')
            time.sleep(1.0)
            scan_errors += 1

        finally:
            unmount_disk(client, disk_id)

    return True


def load_list(name):
    r = set()
    with open(name) as f:
        r = set([l.rstrip() for l in f.readlines()])
    return r


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', help='server', default='localhost')
    parser.add_argument('--port', help='nbs grpc port', type=int, default=9766)
    parser.add_argument('--disk-id', help='volume id', type=str, required=True)
    parser.add_argument('--white-list', help='volume ids', type=str)
    parser.add_argument('--dead-tablets', help='dead tablet ids', type=str)
    # parser.add_argument('--sort-by-size', help='sort volumes by size before repair', action='store_true')
    parser.add_argument('--parallel', help='parallel describe blocks count', type=int, default=1000)
    parser.add_argument('--scan-only', help='scan only', action='store_true')
    parser.add_argument('--skip-scan', help='skip scan', action='store_true')
    parser.add_argument('--save-ranges', help='save scan results to files', action='store_true')
    parser.add_argument('-v', '--verbose', help='verbose mode', default=0, action='count')

    args = parser.parse_args()

    if args.verbose:
        log_level = max(0, logging.ERROR - 10 * int(args.verbose))
    else:
        log_level = logging.INFO

    logging.basicConfig(stream=sys.stderr, level=log_level, format='[%(levelname)s] [%(asctime)s] %(message)s')

    client = CreateClient('%s:%d' % (args.host, args.port), log=logging)

    disk_ids = set([args.disk_id])
    white_list = set()
    dead_tablets = set()

    if args.white_list:
        white_list = load_list(args.white_list)

    if os.path.exists(args.disk_id):
        disk_ids = load_list(args.disk_id)

    if args.dead_tablets:
        dead_tablets = load_list(args.dead_tablets)

    disk_ids = disk_ids - white_list

    errors = []
    attempts = 0

    while True:
        for disk_id in disk_ids:
            try:
                d = nbs.describe_volume(client, disk_id)

                tablet_id = d['Partitions'][0]['TabletId']
                block_count = int(d['VolumeConfig']['Partitions'][0]['BlockCount'])

                if tablet_id in dead_tablets:
                    print('%s - dead tablet %s' % (disk_id, tablet_id))
                    continue

                if not repair(
                        client,
                        disk_id,
                        args.parallel,
                        args.scan_only,
                        args.skip_scan,
                        args.save_ranges,
                        block_count):
                    errors.append(disk_id)
            except ClientError as e:
                logging.exception('ClientError at repair')
                if e.message == 'Path not found':
                    print('%s - deleted' % disk_id)
                else:
                    errors.append(disk_id)
            except Exception as e:
                logging.exception('Exception at repair')
                errors.append(disk_id)

        disk_ids = errors
        errors = []

        if len(disk_ids) == 0:
            break

        if attempts > 30:
            logging.warn('too many attempts. Errors: %s' % disk_ids)
            break
        logging.info('retry repair for disks: %s' % disk_ids)
        time.sleep(1.0)

    for disk_id in errors:
        print('%s - fail' % disk_id)

    print('---')


if __name__ == '__main__':
    sys.exit(main())

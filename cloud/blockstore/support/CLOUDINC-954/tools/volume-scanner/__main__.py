import argparse
import concurrent.futures
import logging
import os
import sys

import cloud.blockstore.tools.util as util

from cloud.blockstore.public.sdk.python.client import CreateClient


def scan_disk(disk_id, client, index_only, describe_parallel, batch):
    filename = "{}.log".format(disk_id)
    if os.path.isfile(filename):
        return

    logging.info("{} started".format(disk_id))
    outstr = ""

    checkpoint_id = "blockstore-volume-scanner"

    try:
        logging.info("creating checkpoint %s" % checkpoint_id)
        client.create_checkpoint(disk_id, checkpoint_id)

        d = client.describe_volume(disk_id)
        blocks_count = d.BlocksCount

        with concurrent.futures.ThreadPoolExecutor(max_workers=describe_parallel) as executor:
            future_to_range = dict()

            i = 0
            while i < blocks_count:
                r = (i, min(batch, blocks_count - i))

                future_to_range[
                    executor.submit(
                        util.find_no_data_ranges,
                        client,
                        disk_id,
                        r[0],
                        r[1],
                        checkpoint_id,
                        index_only=index_only)
                ] = r

                i += batch

            for future in concurrent.futures.as_completed(future_to_range):
                r = future_to_range[future]

                try:
                    for n in future.result():
                        outstr += ("%s %s" % (n[0], n[1])) + "\n"

                except Exception as e:
                    logging.error("Exception while handling range: %s %s, e: %s" % (r[0], r[1], e))
                    raise e

            with open(filename, "w") as f:
                if outstr != "":
                    f.write("{}".format(outstr))
                    logging.info("{}\n{}".format(disk_id, outstr))

    except Exception as e:
        with open(filename, "w") as f:
            f.write("FAILED to scan %s, error: %s" % (disk_id, e))
    finally:
        logging.info("deleting checkpoint %s" % checkpoint_id)
        client.delete_checkpoint(disk_id, checkpoint_id)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', help='server', default='localhost')
    parser.add_argument('--port', help='nbs grpc port', type=int, default=9766)
    parser.add_argument('--disk-id', help='disk id', type=str)
    parser.add_argument('--disk-ids', help='disk ids file', type=str)
    parser.add_argument('--index-only', help='IndexOnly BlobStorage get request flag', type=bool, default=True)
    parser.add_argument('--batch', help='describe blocks request blocks count', type=int, default=100000)
    parser.add_argument('--parallel', help='parallel describe blocks count', type=int, default=1000)
    parser.add_argument('-v', '--verbose', help='verbose mode', default=0, action='count')
    parser.add_argument('--disk-parallel', help='number of disks to check in parallel', type=int, default=1)

    args = parser.parse_args()

    if args.verbose:
        log_level = max(0, logging.ERROR - 10 * int(args.verbose))
    else:
        log_level = logging.INFO

    logging.basicConfig(stream=sys.stderr, level=log_level, format='[%(levelname)s] [%(asctime)s] %(message)s')

    client = CreateClient(
        '{}:{}'.format(args.host, args.port),
        request_timeout=100,
        retry_timeout=100,
        retry_timeout_increment=1,
        log=logging
    )

    if args.disk_ids is not None:
        with open(args.disk_ids) as f:
            disk_ids = [l.rstrip() for l in f.readlines()]
    else:
        if args.disk_id is None:
            raise Exception('one of --disk-id --disk-ids required')
        disk_ids = [args.disk_id]

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.disk_parallel) as executor:
        future_to_disk = dict()

        for disk in disk_ids:
            future_to_disk[
                executor.submit(
                    scan_disk,
                    disk,
                    client,
                    args.index_only,
                    args.parallel,
                    args.batch)
            ] = disk

        for future in concurrent.futures.as_completed(future_to_disk):
            disk_id = future_to_disk[future]
            future.result()
            logging.info("{} finished".format(disk_id))


if __name__ == '__main__':
    sys.exit(main())

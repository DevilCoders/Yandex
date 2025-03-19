import argparse
import concurrent.futures
import logging

from cloud.blockstore.tools.kikimrapi import find_tablet, fetch_groups, fetch_blobs, add_garbage

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

formatter = logging.Formatter(fmt="%(message)s", datefmt='%m/%d/%Y %I:%M:%S')
ch = logging.StreamHandler()
ch.setLevel(logging.INFO)
ch.setFormatter(formatter)
logger.addHandler(ch)


HOST = "kikimr0430.search.yandex.net"
PORT = 8765
HIVE = 72057594037968897
MAX_BLOBS = 100


def prune_tablet_garbage(host, port, tablet, hive, max_blobs):

    node, gen = find_tablet(host, port, tablet)
    if node == 0:
        raise Exception("tablet not found", tablet)
    groups = fetch_groups(host, port, hive, tablet)
    for channel, group in groups:

        if channel < 3:
            continue

        blobs = fetch_blobs(
            host,
            port,
            tablet,
            channel,
            group,
            gen - 1)

        offset = 0
        while offset < len(blobs):
            request = blobs[offset:offset + max_blobs]
            offset += len(request)

            add_garbage(
                host,
                port,
                tablet,
                ";".join(request))

    return True


def run():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--host", help="host name", default=HOST)
    parser.add_argument(
        "--port", help="monitoring port", type=int, default=PORT)
    parser.add_argument(
        "--hive", help="hive tablet id", type=int, default=HIVE)
    parser.add_argument(
        "--max-blobs", help="max request size", type=int, default=MAX_BLOBS)
    parser.add_argument(
        "--input-file", help="file with list of tablets", required=True)
    parser.add_argument(
        "--workers", help="number of parallel workers", type=int, default=1)

    args = parser.parse_args()

    tablets = []
    host = args.host
    port = args.port
    hive = args.hive
    max_blobs = args.max_blobs

    with open(args.input_file, "r") as f:
        tablets = [line.strip() for line in f]

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as executor:
        futures_to_tablet = {
            executor.submit(prune_tablet_garbage, host, port, t, hive, max_blobs): t for t in tablets
        }
        for future in concurrent.futures.as_completed(futures_to_tablet):
            tablet = futures_to_tablet[future]
            try:
                future.result()
            except Exception as exc:
                print("{} generated an exception: {}".format(tablet, exc))
            else:
                print("{} is done".format(tablet))


run()

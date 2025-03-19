import argparse
import logging

from cloud.blockstore.tools.kikimrapi import fetch, find_tablet, fetch_groups, fetch_blobs, add_garbage

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

formatter = logging.Formatter(fmt="%(message)s", datefmt='%m/%d/%Y %I:%M:%S')
ch = logging.StreamHandler()
ch.setLevel(logging.INFO)
ch.setFormatter(formatter)
logger.addHandler(ch)


HOST = "localhost"
PORT = 8765
HIVE = 72057594037968897
MAX_BLOBS = 100


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
        "--hard", help="use hard barriers", action="store_true", default=False)
    parser.add_argument(
        "--tablet", help="tablet to proceed", type=int, required=True)

    args = parser.parse_args()

    logger.info("tablet: {}".format(args.tablet))

    if (args.hard is True):
        url = "http://{}:{}/tablets/app?TabletID={}&action=collectGarbage&type=hard".format(
            args.host,
            args.port,
            args.tablet)
        fetch(url)
        return

    node, gen = find_tablet(args.host, args.port, args.tablet)
    if node == 0:
        raise Exception("tablet not found", args.tablet)

    logger.info("node: {}".format(node))
    logger.info("generation: {}".format(gen))

    groups = fetch_groups(args.host, args.port, args.hive, args.tablet)
    for channel, group in groups:
        logger.info("channel: {} group: {}".format(channel, group))

        if channel < 3:
            continue

        blobs = fetch_blobs(
            args.host,
            args.port,
            args.tablet,
            channel,
            group,
            gen - 1)

        offset = 0
        while offset < len(blobs):
            request = blobs[offset:offset + args.max_blobs]
            offset += len(request)

            logger.info("blobs: {}".format(len(request)))

            add_garbage(
                args.host,
                args.port,
                args.tablet,
                ";".join(request))


run()

import argparse
import logging
import sys

import cloud.blockstore.tools.nbsapi as nbs

from cloud.blockstore.public.sdk.python.client import CreateClient

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

formatter = logging.Formatter(fmt="%(message)s", datefmt='%m/%d/%Y %I:%M:%S')
ch = logging.StreamHandler()
ch.setLevel(logging.INFO)
ch.setFormatter(formatter)
logger.addHandler(ch)


def run():
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', help='nbs server endpoint', default='localhost')
    parser.add_argument('--port', help='nbs grpc port', default=9766)
    parser.add_argument('--disk-id', help='disk id of volume to describe', default='')
    parser.add_argument('--disk-ids-file', help='path to disk ids file', default='')

    args = parser.parse_args()

    if len(args.disk_ids_file) == 0:
        if len(args.disk_id) == 0:
            logger.info('No disk ids to describe')
            sys.exit(0)

        disk_ids = [args.disk_id]
    else:
        with open(args.disk_ids_file, "r") as f:
            disk_ids = f.read().splitlines()

    client = CreateClient('{}:{}'.format(args.host, args.port), log=logging)

    for disk_id in disk_ids:
        try:
            description = nbs.describe_volume(client, disk_id)
        except Exception as e:
            logging.exception('Got exception in nbs.describe_volume: {}'.format(str(e)))
        else:
            print(description)


run()

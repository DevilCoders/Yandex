import argparse
import json
import logging
import os
import requests
import sys

from cloud.blockstore.public.sdk.python.client import CreateClient
from cloud.blockstore.public.sdk.python.client import ClientCredentials

DEFAULT_VOLUME_TABLES = 6
DEFAULT_PARTITION_TABLES = 11


def compact_localdb(endpoint, port, id, num_tables):
    url = 'http://{}:{}/tablets/executorInternals?TabletID={}&force_compaction={}'
    for i in range(1, num_tables + 1):
        try:
            response = requests.get(url.format(endpoint, port, id, i))
            response.raise_for_status()
        except Exception as e:
            raise Exception('HTTP GET request failed for tablet [id={}]: {}'.format(id, e))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('endpoint', help='server endpoint')
    parser.add_argument('--http-endpoint', help='server http endpoint', default='localhost')
    parser.add_argument('--http-port', help='server port', default=8766)
    parser.add_argument('--port', help='server port', default=9766)
    parser.add_argument('--secure-port', help='server secure port', default=0)
    parser.add_argument('--iam-token-file', help='file with IAM token', default=os.path.expanduser('~') + "/.nbs-client/iam-token")
    parser.add_argument('--dry', help='dry run', action="store_true")
    parser.add_argument('-v', '--verbose', help='verbose mode', default=0, action='count')
    parser.add_argument('--tablet-type', help='tablet to compact [volume, partition]', choices=['volume', 'partition'], default='volume')

    args = parser.parse_args()

    if args.verbose:
        log_level = max(0, logging.ERROR - 10 * int(args.verbose))
    else:
        log_level = logging.ERROR

    logger = logging.getLogger(__name__)
    logger.setLevel(logging.INFO)

    formatter = logging.Formatter(fmt="%(message)s", datefmt='%m/%d/%Y %I:%M:%S')
    ch = logging.StreamHandler()
    ch.setLevel(log_level)
    ch.setFormatter(formatter)
    logger.addHandler(ch)

    port = args.port
    credentials = None
    if args.secure_port != 0:
        port = args.secure_port
        with open(args.iam_token_file, "rb") as f:
            token = f.readline().strip().decode()
            credentials = ClientCredentials(auth_token=token)

    client = CreateClient('{}:{}'.format(args.endpoint, port), credentials, log=logger)

    for line in sys.stdin:
        volume_id = line.strip()

        try:
            query = {"DiskId": volume_id}
            volume = client.execute_action('DescribeVolume', json.dumps(query).encode())
            if args.dry:
                logger.info(volume)
        except Exception as e:
            logger.info("execute action request failed: {}, {}".format(
                volume_id, e
            ))
            continue

        volume_info = json.loads(volume)
        tablet_ids = [volume_info["VolumeTabletId"]]
        num_tables = DEFAULT_VOLUME_TABLES

        if args.tablet_type == 'partition':
            tablet_ids = [p["TabletId"] for p in volume_info["Partitions"]]
            num_tables = DEFAULT_PARTITION_TABLES

        for t in tablet_ids:
            try:
                compact_localdb(args.http_endpoint, args.http_port, t, num_tables)
            except Exception as e:
                logger.info("http request failed for volume: {}, {}".format(
                    volume_id, e
                ))
                continue
            logger.info("compacted local db for volume {}".format(volume_id))


if __name__ == '__main__':
    main()

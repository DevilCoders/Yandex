import argparse
import json
import logging
import os
import sys

from cloud.blockstore.public.sdk.python.client import CreateClient
from cloud.blockstore.public.sdk.python.client import ClientCredentials


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('endpoint', help='server endpoint')
    parser.add_argument('--port', help='server port', default=9766)
    parser.add_argument('--secure-port', help='server secure port', default=0)
    parser.add_argument('--iam-token-file', help='file with IAM token', default=os.path.expanduser('~') + "/.nbs-client/iam-token")
    parser.add_argument('--dry', help='dry run', action="store_true")
    parser.add_argument('-v', '--verbose', help='verbose mode', default=0, action='count')
    parser.add_argument('--tablet-type', help='tablet to kill [volume, partition]', choices=['volume', 'partition'], default='volume')

    args = parser.parse_args()

    if args.verbose:
        log_level = max(0, logging.ERROR - 10 * int(args.verbose))
    else:
        log_level = logging.INFO

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

        if args.tablet_type == 'partition':
            tablet_ids = [p["TabletId"] for p in volume_info["Partitions"]]

        if args.dry is True:
            return
        for t in tablet_ids:
            try:
                query = {"TabletId": int(t)}
                volume = client.execute_action('KillTablet', json.dumps(query).encode())
            except Exception as e:
                logger.info("Kill tablet request failed for volume: {}, {}".format(
                    volume_id, e
                ))
                continue
            logger.info("Killed tablet {} for volume {}".format(t, volume_id))


if __name__ == '__main__':
    main()

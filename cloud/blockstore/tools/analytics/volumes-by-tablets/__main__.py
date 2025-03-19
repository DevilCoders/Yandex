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
    parser.add_argument('--nbs-port', help='nbs grpc port', default=9766)
    parser.add_argument('--input', help='input file', type=argparse.FileType('r'),
                        default=sys.stdin)
    parser.add_argument('--output', help='output file', type=argparse.FileType('w'),
                        default=sys.stdout)

    args = parser.parse_args()

    tablet_ids = args.input.read().splitlines()
    if len(tablet_ids) == 0:
        logger.info('No tablet ids to query disk ids for')
        sys.exit(0)

    client = CreateClient('{}:{}'.format(args.host, args.nbs_port), log=logging)
    volumes = client.list_volumes()

    res = {}

    for volume in volumes:
        try:
            description = nbs.describe_volume(client, volume)
        except Exception as e:
            logging.exception('Got exception in nbs.describe_volume: {}'.format(str(e)))
        else:
            # print description
            tablet_id = description['VolumeTabletId']
            res[tablet_id] = "volume %s" % volume
            partitions = description['Partitions']
            found_tab = False
            for partition in partitions:
                res[partition['TabletId']] = "partition %s" % volume
                found_tab = True
            if not found_tab:
                logger.warn('Failed to find tablet info for {}'.format(volume))

    for tablet_id in tablet_ids:
        if tablet_id in res:
            args.output.write('%s %s\n' % (tablet_id, res[tablet_id]))
        else:
            args.output.write('Not found\n')


run()

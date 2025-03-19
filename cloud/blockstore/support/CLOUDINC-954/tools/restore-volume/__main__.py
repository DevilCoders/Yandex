import argparse
import json
import logging
import sys

import cloud.blockstore.tools.nbsapi as nbs

from cloud.blockstore.public.sdk.python.client import CreateClient
import cloud.blockstore.public.sdk.python.protos as protos

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

formatter = logging.Formatter(fmt="%(message)s", datefmt='%m/%d/%Y %I:%M:%S')
ch = logging.StreamHandler()
ch.setLevel(logging.INFO)
ch.setFormatter(formatter)
logger.addHandler(ch)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', help='nbs server endpoint', default='localhost')
    parser.add_argument('--port', help='nbs grpc port', default=9766)
    parser.add_argument('--disk-id', help='disk id', type=str)
    parser.add_argument('--disk-ids', help='disk ids file', type=str)
    parser.add_argument('--descriptions', help='volume descriptions will be dumped to this file', type=str, required=True)
    parser.add_argument('--cont', help='open log files for append; TODO: read descriptions file and continue from the last disk', action='store_true')

    args = parser.parse_args()

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

    """
    to_skip = 0
    if args.cont:
        with open(args.descriptions) as f:
            lines = f.readlines()
            cnt = len(lines)
            while cnt > 0:
                try:

                if len(lines[cnt - 1].rstrip())
            line_count = len(f.readlines())
            if line_count > 0:
                to_skip = line_count - 1
    """

    descriptions = open(args.descriptions, "a" if args.cont else "w")
    reset_failed = open(args.descriptions + ".reset_failed", "a" if args.cont else "w")

    for disk_id in disk_ids:
        try:
            description = nbs.describe_volume(client, disk_id)
            descriptions.write(json.dumps(description))
            descriptions.write('\n')
            descriptions.flush()

            volume_tablet_id = description["VolumeTabletId"]

            response = nbs.reset_tablet(client, volume_tablet_id, 1)
            if response["Status"] != "OK":
                reset_failed.write(disk_id)
                reset_failed.write('\n')
                reset_failed.flush()
            else:
                vc = description["VolumeConfig"]
                version = int(vc["Version"])
                block_count = int(vc["Partitions"][0]["BlockCount"])
                mpw = vc.get("PerformanceProfileMaxPostponedWeight", 128000000)
                mpw += 1

                pp = protos.TVolumePerformanceProfile(MaxPostponedWeight=mpw)

                client.resize_volume(
                    disk_id,
                    block_count,
                    0,
                    version,
                    performance_profile=pp
                )
        except Exception as e:
            print("FAILED to restore %s, error: %s" % (disk_id, e))

    return 0


sys.exit(main())

import argparse
import json
import logging
import math
import os
import sys

import cloud.blockstore.public.sdk.python.protos as protos

from cloud.blockstore.public.sdk.python.client import CreateClient
from cloud.blockstore.public.sdk.python.client import ClientCredentials
from protobuf_to_dict import protobuf_to_dict


def json2pp(j):
    return protos.TVolumePerformanceProfile(
        MaxReadIops=j.get("MaxReadIops", 0),
        MaxWriteIops=j.get("MaxWriteIops", 0),
        MaxReadBandwidth=j.get("MaxReadBandwidth", 0),
        MaxWriteBandwidth=j.get("MaxWriteBandwidth", 0),
        ThrottlingEnabled=j.get("ThrottlingEnabled", False),
        BurstPercentage=j.get("BurstPercentage", 0),
        MaxPostponedWeight=j.get("MaxPostponedWeight", 0),
    ) if j is not None else protos.TVolumePerformanceProfile()


class VolumeInfo(object):

    def __init__(self, block_count, config_version, folder_id, performance_profile):
        self.block_count = block_count
        self.config_version = config_version
        self.folder_id = folder_id
        self.performance_profile = performance_profile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('endpoint', help='server endpoint')
    parser.add_argument('--port', help='server port', default=9766)
    parser.add_argument('--secure-port', help='server secure port', default=0)
    parser.add_argument('--iam-token-file', help='file with IAM token', default=os.path.expanduser('~') + "/.nbs-client/iam-token")
    parser.add_argument('--dry', help='dry run', action="store_true")
    parser.add_argument('--json-volume-info', help='if not set, this tool will read volume ids from stdin and call DescribeVolume for those disks to build volume info')
    parser.add_argument('-v', '--verbose', help='verbose mode', default=0, action='count')
    parser.add_argument('--storage-media-kind', help='StorageMediaKind filter', default=None, type=int)
    parser.add_argument('--allocate-separate-mixed-channels', help='disable NoSeparateMixedChannelAllocation flag', action="store_true")
    parser.add_argument('--force-resize', help='modify PerformanceProfile->MaxPostponedWeight by 1 byte to force a SS request', action="store_true")
    parser.add_argument('--folder-percentage', help='resize only certain percent of disks belonging to the same folder id', default=50, type=int)
    parser.add_argument('--dont-pass-performance-profile', help='do not pass performance profile in ResizeVolume request', action="store_true")

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
            token = f.readline().strip()
            credentials = ClientCredentials(auth_token=token)

    client = CreateClient('{}:{}'.format(args.endpoint, port), credentials, log=logger)

    volume_id2info = {}
    folder_id2volume_ids = {}

    if args.json_volume_info is not None:
        with open(args.json_volume_info) as f:
            d = json.load(f)
        for volume_id, info in d.iteritems():
            if args.storage_media_kind is not None and args.storage_media_kind != info["StorageMediaKind"]:
                continue

            volume_id2info[volume_id] = VolumeInfo(
                info["BlockCount"],
                info["Version"],
                info["FolderId"],
                info["PerformanceProfile"],
            )
    else:
        for line in sys.stdin:
            volume_id = line.strip()

            try:
                volume = client.describe_volume(volume_id)
                if args.dry:
                    print json.dumps(protobuf_to_dict(volume))
            except Exception as e:
                logger.info("describe_volume request failed: {}, {}".format(
                    volume_id, e
                ))
                continue

            pp = None
            if args.force_resize:
                pp = protobuf_to_dict(volume.PerformanceProfile)
                if "MaxPostponedWeight" not in pp:
                    logger.fatal("unexpected pp for disk %s: %s" % (volume_id, json.dumps(pp)))
                    return 1

                if pp["MaxPostponedWeight"] <= 128000000:
                    pp["MaxPostponedWeight"] += 1
                else:
                    pp["MaxPostponedWeight"] -= 1

            folder_id = "default"
            if hasattr(volume, 'FolderId'):
                folder_id = volume.FolderId

            volume_id2info[volume_id] = VolumeInfo(
                volume.BlocksCount,
                volume.ConfigVersion,
                folder_id,
                pp
            )

    for volume_id, info in volume_id2info.iteritems():
        if not info.config_version:
            logger.fatal("bad config version, volume=%s, config_version=%s" % (volume_id, info.config_version))
            return 1

    for volume_id, info in volume_id2info.iteritems():
        folder_id2volume_ids[info.folder_id] = []

    for volume_id, info in volume_id2info.iteritems():
        folder_id2volume_ids[info.folder_id].append(volume_id)

    for _, volume_ids in folder_id2volume_ids.iteritems():
        size = int(math.ceil(args.folder_percentage * len(volume_ids) / 100.0))
        for volume_id in volume_ids[:size]:
            info = volume_id2info[volume_id]

            if args.dry:
                if info.performance_profile is not None:
                    pp_str = json.dumps(info.performance_profile)
                else:
                    pp_str = ""

                logger.info("will resize volume {}, params: {}, {}, {}".format(
                    volume_id,
                    info.block_count,
                    info.config_version,
                    pp_str,
                ))
            else:
                try:
                    performance_profile_pb = None
                    if not args.dont_pass_performance_profile:
                        performance_profile_pb = json2pp(info.performance_profile)

                    client.resize_volume(
                        volume_id,
                        info.block_count,
                        0,  # channels_count, unused
                        info.config_version,
                        flags=protos.TResizeVolumeRequestFlags(
                            NoSeparateMixedChannelAllocation=not args.allocate_separate_mixed_channels
                        ),
                        performance_profile=performance_profile_pb,
                    )
                    logger.info("successfully resized volume %s" % volume_id)
                except Exception as e:
                    logger.error("resize_volume request failed: {}, {}".format(
                        volume_id,
                        e,
                    ))


main()

import argparse
import logging
import math
import sys
import time

import cloud.blockstore.public.sdk.python.protos as protos

from cloud.blockstore.public.sdk.python.client import CreateClient, ClientError

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

formatter = logging.Formatter(fmt="%(message)s", datefmt='%m/%d/%Y %I:%M:%S')
ch = logging.StreamHandler()
ch.setLevel(logging.INFO)
ch.setFormatter(formatter)
logger.addHandler(ch)


def run():
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', help='server', default='localhost')
    parser.add_argument('--port', help='blockstore server port', type=int,
                        default=9766)
    parser.add_argument('--monport', help='kikimr monitoring port', type=int,
                        default=8765)
    parser.add_argument('--path', help='scheme shard path', default='/Root/NBS')
    parser.add_argument('--max', help='max number of channels', type=int,
                        default=29)
    parser.add_argument('--unit-ssd', help='allocation unit for SSD disks, in Gb',
                        type=int, default=32)
    parser.add_argument('--unit-hdd', help='allocation unit for HDD disks, in Gb',
                        type=int, default=256)
    parser.add_argument('--volumes', help='file containing disk ids, one per line '
                        'for which channels count should be increased if necessary; '
                        'if not specified, all volumes found by NBS service are '
                        'considered', type=argparse.FileType('r'), default=None)
    parser.add_argument('--dry-run', help='only print disk ids of volumes for '
                        'which channels count can be increased', action='store_true')
    parser.add_argument('--batch-size', help='number of volumes inside a single '
                        'batch for processing volumes in batches with periods of '
                        'waiting between batches; non-positive values mean no '
                        'batching is done, all volumes are processed without any '
                        'delay', type=int, default=100)
    parser.add_argument('--batch-delay', help='delay in seconds between batches '
                        'for processing volumes in batches with periods of '
                        'waiting between batches', type=int, default=10)

    args = parser.parse_args()

    if args.unit_ssd <= 0:
        logger.warn('Wrong SSD unit size, must be positive integer')
        sys.exit(1)

    if args.unit_hdd <= 0:
        logger.warn('Wrong HDD unit size, must be positive integer')
        sys.exit(1)

    client = CreateClient(
        args.host + ':' + str(args.port),
        log=logger)

    volumes = []
    if args.volumes is None:
        volumes = client.list_volumes()
    else:
        volumes = args.volumes.read().splitlines()

    for index, disk_id in enumerate(volumes):
        if args.batch_size > 0 and args.batch_delay > 0 and \
                (index + 1) % args.batch_size == 0:
            logger.info(
                'Processed {} volumes, waiting for {} seconds before proceeding'.format(
                    (index + 1),
                    args.batch_delay))
            time.sleep(args.batch_delay)

        try:
            response = client.stat_volume(disk_id)
        except ClientError as e:
            logger.warn('Failed to stat volume {}: {}'.format(
                disk_id,
                str(e)))
            continue

        volume = response['Volume']
        block_size = volume.BlockSize
        blocks_count = volume.BlocksCount

        volume_size = block_size * blocks_count / 1073741824

        unit = args.unit_hdd
        if volume.StorageMediaKind == \
                protos.EStorageMediaKind.Value("STORAGE_MEDIA_SSD"):
            unit = args.unit_ssd

        channels_count = int(math.ceil(float(volume_size) / unit))
        if channels_count < 1:
            logger.debug(
                'Resetting too small channels count to 1; volume {}'
                ', size = {}, unit = {}'.format(
                    disk_id,
                    volume_size,
                    unit))
            channels_count = 1
        elif channels_count > args.max:
            logger.debug(
                'Resetting too large channels count to {}; volume '
                '{}, size = {}, unit = {}'.format(
                    args.max,
                    disk_id,
                    volume_size,
                    unit))
            channels_count = args.max

        current_channels_count = volume.ChannelsCount
        if current_channels_count >= channels_count:
            logger.debug(
                'Channels count {} for volume {} is already larger '
                'than or equal to the computed channels count {}'.format(
                    current_channels_count,
                    disk_id,
                    channels_count))
            continue

        if args.dry_run:
            logger.info(
                'Channels count can be increased from {} to {} for '
                'volume {}'.format(
                    current_channels_count,
                    channels_count,
                    disk_id))
            continue

        logger.info(
            'Increasing channels count from {} to {} for volume {}'.format(
                current_channels_count,
                channels_count,
                disk_id))
        config_version = volume.ConfigVersion

        try:
            client.resize_volume(disk_id, blocks_count, channels_count, config_version)
        except ClientError as e:
            logger.warn(
                'Caught exception on attempt to change channels count from {} '
                'to {} for volume {}: {}'.format(
                    current_channels_count,
                    channels_count,
                    disk_id,
                    str(e)))
            continue

        # call stat_volume to ensure the partition tablet is started after resize
        try:
            client.stat_volume(disk_id)
        except ClientError as e:
            logger.warn(
                'Caught exception on attempt to stat volume after adjusting '
                'the channels count: {}'.format(str(e)))


run()

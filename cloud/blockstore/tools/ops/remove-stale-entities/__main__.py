#!/usr/bin/env python

from .lib import (
    delete_stale_disks,
    delete_stale_filesystems,
    delete_stale_images,
    delete_stale_instances,
    delete_stale_snapshots,
)

from cloud.blockstore.pylibs import common as com

import argparse
import sys


def _add_common_arguments(parser: argparse.Namespace) -> None:
    parser.add_argument(
        '--regex',
        type=str,
        required=True,
        help='Name regex',
    )
    parser.add_argument(
        '--ttl',
        type=int,
        required=True,
        help='Number of seconds passed since entity creation',
    )


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser('PROG')

    verbose_quite_group = parser.add_mutually_exclusive_group()
    verbose_quite_group.add_argument(
        '--verbose',
        action='store_true',
        help='perform verbose logging',
    )
    verbose_quite_group.add_argument(
        '--quite',
        action='store_true',
        help='silent logging',
    )

    parser.add_argument(
        '--teamcity',
        action='store_true',
        help='use teamcity logging format',
    )
    parser.add_argument(
        '--profile-id',
        help='select ycp profile id to use',
        required=True,
    )
    parser.add_argument(
        '--zone-id',
        type=str,
        help='select ycp zone',
        default='ru-central1-a',
    )
    parser.add_argument(
        '--ipc-type',
        type=str,
        default='grpc',
        help='use specified ipc type',
    )

    subparsers = parser.add_subparsers(
        dest='entity_type',
        help='entity type',
    )

    # instance stuff
    instance_parser = subparsers.add_parser(
        'instance',
        help='run remove instances',
    )
    _add_common_arguments(instance_parser)

    # disk stuff
    disk_parser = subparsers.add_parser(
        'disk',
        help='run remove disks',
    )
    _add_common_arguments(disk_parser)

    # filesystem stuff
    filesystem_parser = subparsers.add_parser(
        'filesystem',
        help='run remove filesystem',
    )
    _add_common_arguments(filesystem_parser)

    # image stuff
    image_parser = subparsers.add_parser(
        'image',
        help='run remove images',
    )
    _add_common_arguments(image_parser)

    # snapshot stuff
    snapshot_parser = subparsers.add_parser(
        'snapshot',
        help='run remove snapshots',
    )
    _add_common_arguments(snapshot_parser)

    return parser.parse_args()


def main():
    args = _parse_args()
    logger = com.create_logger('yc-remove-entities', args)
    logger.info('=====START SCRIPT=====')
    logger.info(f'Cluster: {args.profile_id}')
    logger.info(f'Zone: {args.zone_id}')
    logger.info(f'Regex: {args.regex}')
    logger.info(f'TTL: {args.ttl}s')
    logger.info(f'===Try to list {args.entity_type}===')

    if args.entity_type == 'instance':
        delete_stale_instances(
            args.profile_id,
            args.zone_id,
            args.ipc_type,
            args.regex,
            args.ttl,
            logger,
        )
    elif args.entity_type == 'disk':
        delete_stale_disks(
            args.profile_id,
            args.zone_id,
            args.ipc_type,
            args.regex,
            args.ttl,
            logger,
        )
    elif args.entity_type == 'filesystem':
        delete_stale_filesystems(
            args.profile_id,
            args.zone_id,
            args.ipc_type,
            args.regex,
            args.ttl,
            logger,
        )
    elif args.entity_type == 'image':
        delete_stale_images(
            args.profile_id,
            args.zone_id,
            args.ipc_type,
            args.regex,
            args.ttl,
            logger,
        )
    elif args.entity_type == 'snapshot':
        delete_stale_snapshots(
            args.profile_id,
            args.zone_id,
            args.ipc_type,
            args.regex,
            args.ttl,
            logger,
        )
    else:
        logger.fatal(f'Failed to start script. "{args.entity_type}" is not valid type')
        sys.exit(1)

    logger.info('======END SCRIPT======')


if __name__ == '__main__':
    main()

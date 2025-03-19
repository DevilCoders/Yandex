from .acceptance_test_runner import AcceptanceTestRunner
from .eternal_acceptance_test_runner import EternalAcceptanceTestRunner
from .lib import Error

from cloud.blockstore.pylibs import common

import argparse
import sys


_DEFAULT_DISK_BLOCKSIZE = 4096
_DEFAULT_DISK_TYPE = 'network-ssd'
_DEFAULT_WRITE_SIZE_PERCENTAGE=100
_DEFAULT_INSTANCE_CORES = 8
_DEFAULT_INSTANCE_RAM = 8
_DEFAULT_IPC_TYPE = 'grpc'
_DEFAULT_PLATFORM_ID = 'standard-v2'
_DEFAULT_ZONE_ID = 'ru-central1-a'


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser('PROG')

    verbose_quite_group = parser.add_mutually_exclusive_group()
    verbose_quite_group.add_argument('-v', '--verbose', action='store_true')
    verbose_quite_group.add_argument('-q', '--quite', action='store_true')

    parser.add_argument(
        '--teamcity',
        action='store_true',
        help='use teamcity logging format')

    subparsers = parser.add_subparsers(dest='test_type', help='test types')

    # acceptance test type stuff
    acceptance_test_type_parser = subparsers.add_parser(
        'acceptance',
        help='will create acceptance test suite for single iteration')
    acceptance_test_type_parser.add_argument(
        '--test-suite',
        type=str,
        required=True,
        help='run specified test suite from {arc_root}/cloud/disk_manager/test'
             '/acceptance/test_runner/lib/test_cases.py')
    acceptance_test_type_parser.add_argument(
        '--verify-test',
        type=str,
        required=True,
        help='path to verify-test binary')

    # eternal test type stuff
    eternal_test_type_parser = subparsers.add_parser(
        'eternal',
        help='will perform acceptance test on single disk'
             ' with complete content checking')
    eternal_test_type_parser.add_argument(
        '--disk-size',
        type=int,
        required=True,
        help='disk size in GiB')
    eternal_test_type_parser.add_argument(
        '--disk-blocksize',
        type=int,
        default=_DEFAULT_DISK_BLOCKSIZE,
        help=f'disk blocksize in bytes (default is {_DEFAULT_DISK_BLOCKSIZE}')
    eternal_test_type_parser.add_argument(
        '--disk-type',
        type=str,
        default=_DEFAULT_DISK_TYPE,
        help=f'disk type (default is {_DEFAULT_DISK_TYPE}')
    eternal_test_type_parser.add_argument(
        '--disk-write-size-percentage',
        type=int,
        default=_DEFAULT_WRITE_SIZE_PERCENTAGE,
        help=f'verification write size percentage (default is'
             f' {_DEFAULT_WRITE_SIZE_PERCENTAGE}')

    # common stuff
    test_arguments_group = parser.add_argument_group('common arguments')
    test_arguments_group.add_argument(
        '-c',
        '--cluster',
        type=str,
        required=True,
        help='run test on specified cluster')
    test_arguments_group.add_argument(
        '--acceptance-test',
        type=str,
        required=True,
        help='path to acceptance-test binary')
    test_arguments_group.add_argument(
        '--conserve-snapshots',
        action='store_true',
        default=False,
        help='do not delete snapshot after acceptance test')
    test_arguments_group.add_argument(
        '--ipc-type',
        type=str,
        default=_DEFAULT_IPC_TYPE,
        help=f'use specified ipc type (default is {_DEFAULT_IPC_TYPE})')
    test_arguments_group.add_argument(
        '--placement-group-name',
        type=str,
        default=None,
        help='create vm with specified placement group')
    test_arguments_group.add_argument(
        '--compute-node',
        type=str,
        default=None,
        help='run acceptance test on specified compute node')
    test_arguments_group.add_argument(
        '--zone-id',
        type=str,
        default=_DEFAULT_ZONE_ID,
        help=f'zone id (default is {_DEFAULT_ZONE_ID})')
    test_arguments_group.add_argument(
        '--instance-cores',
        type=int,
        default=_DEFAULT_INSTANCE_CORES,
        help=f'cores for created vm (default is {_DEFAULT_INSTANCE_CORES})')
    test_arguments_group.add_argument(
        '--instance-ram',
        type=int,
        default=_DEFAULT_INSTANCE_RAM,
        help=f'RAM for created vm (in GiB)'
             f' (default is {_DEFAULT_INSTANCE_RAM})')
    test_arguments_group.add_argument(
        '--instance-platform-ids',
        nargs='*',
        type=str,
        default=[_DEFAULT_PLATFORM_ID],
        help=f'List of possible platforms (default is {_DEFAULT_PLATFORM_ID})')
    test_arguments_group.add_argument(
        '--debug',
        action='store_true',
        default=False,
        help='do not delete instance and disk, if fail')

    return parser.parse_args()


def main():
    args = _parse_args()
    logger = common.create_logger(
        f'yc-disk-manager-ci-acceptance-test-{args.test_type}-type',
        args)

    if args.test_type == 'acceptance':
        test_runner = AcceptanceTestRunner(args)
    elif args.test_type == 'eternal':
        test_runner = EternalAcceptanceTestRunner(args)
    else:
        logger.fatal(f'Failed to run test suite {args.test_type}:'
                     f' No such test type')

    with common.make_profiler(logger, not args.debug) as profiler:
        try:
            test_runner.run(profiler, logger)
        except Error as e:
            logger.fatal(f'Failed to run test suite: {e}')
            sys.exit(1)


if __name__ == '__main__':
    main()

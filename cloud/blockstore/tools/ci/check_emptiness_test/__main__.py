from concurrent.futures.thread import ThreadPoolExecutor

import argparse
import os
import sys
import threading

from cloud.blockstore.pylibs import common
from cloud.blockstore.pylibs.clusters import get_cluster_test_config
from cloud.blockstore.pylibs.ycp import YcpWrapper, make_ycp_engine

_SUBNET_NAME = 'cloudvmnets-%s'
_VERIFY_TEST_REMOTE_PATH = '/usr/bin/verify-test'
_DEVICE_NAME = '/dev/vdb'
_CMD_TO_SCAN_DISK = (f'{_VERIFY_TEST_REMOTE_PATH} --filesize %s '
                     f'--file {_DEVICE_NAME} --zero-check --iodepth %s --blocksize 4194304 2>&1')


class Error(Exception):
    pass


def _parse_args():
    parser = argparse.ArgumentParser()

    verbose_quite_group = parser.add_mutually_exclusive_group()
    verbose_quite_group.add_argument('-v', '--verbose', action='store_true')
    verbose_quite_group.add_argument('-q', '--quite', action='store_true')

    parser.add_argument('--teamcity', action='store_true', help='use teamcity logging format')

    test_arguments_group = parser.add_argument_group('test arguments')
    test_arguments_group.add_argument(
        '-c',
        '--cluster',
        type=str,
        required=True,
        help='run fio test on specified cluster',
    )
    test_arguments_group.add_argument(
        '--verify-test-path',
        type=str,
        required=True,
        help='path to verify-test tool, built from arcadia cloud/blockstore/tools/testing/verify-test',
    )
    test_arguments_group.add_argument(
        '--disk-size',
        type=int,
        default=1023,
        help='specify disk size in GB',
    )
    test_arguments_group.add_argument(
        '--io-depth',
        type=int,
        default=128,
        help='specify io depth',
    )
    test_arguments_group.add_argument(
        '--dry-run',
        action='store_true',
        help='dry run',
    )
    test_arguments_group.add_argument(
        '--compute-node',
        type=str,
        default=None,
        help='run fio test on specified node',
    )
    test_arguments_group.add_argument(
        '--host-group',
        type=str,
        default=None,
        help='specify host group for instance creation',
    )
    test_arguments_group.add_argument(
        '--ttl-instance-days',
        type=int,
        default=7,
        help='ttl for tmp instances'
    )

    return parser.parse_args()


def _scan_disk(args, zone_id: str, should_stop) -> str:
    logger = common.create_logger(zone_id, args)

    cluster = get_cluster_test_config(args.cluster, zone_id)
    # creating a small vm by default so that our disk access gets throttled
    # at the client level - otherwise this test might create too much stress
    # on blockstore-server
    cores = int(os.getenv("TEST_VM_CORES", "2"))
    memory = int(os.getenv("TEST_VM_RAM", "8"))
    ycp = YcpWrapper(
        cluster.name,
        cluster.ipc_type_to_folder_desc['grpc'],
        logger,
        make_ycp_engine(args.dry_run))
    helpers = common.make_helpers(args.dry_run)

    ycp.delete_tmp_instances(args.ttl_instance_days)

    with ycp.create_instance(
            cores,
            memory,
            compute_node=args.compute_node,
            host_group=args.host_group) as instance:
        with ycp.create_disk(args.disk_size, 'network-ssd-nonreplicated') as disk:
            with ycp.attach_disk(instance, disk):
                logger.info(f'Waiting until instance <id={instance.id}> becomes available via ssh')
                helpers.wait_until_instance_becomes_available_via_ssh(instance.ip)

                logger.info(f'Copying verify-test to instance <id={instance.id}>')
                with common.make_sftp_client(args.dry_run, instance.ip) as sftp:
                    sftp.put(args.verify_test_path, _VERIFY_TEST_REMOTE_PATH)
                    sftp.chmod(_VERIFY_TEST_REMOTE_PATH, 0o755)

                logger.info(f'Scanning disk with <id={disk.id}>')
                with common.make_ssh_client(args.dry_run, instance.ip) as ssh:
                    _, stdout, stderr = ssh.exec_command(
                        _CMD_TO_SCAN_DISK % (args.disk_size * 1024 ** 3, args.io_depth))
                    while not should_stop.is_set() and not stdout.channel.exit_status_ready():
                        pass
                    if stdout.channel.recv_exit_status():
                        stdout_str = ''.join(stdout.readlines())
                        should_stop.set()
                        raise Error(f'failed to scan disk on instance <id={instance.id} with'
                                    f' disk <id={disk.id}>\n{stdout_str}>')
                    else:
                        return (f'Successfully finished in zone <id={zone_id}>.\n'
                                f'instance <id={instance.id}>, disk <id={disk.id}>')


def _run_test_suite(args, logger):
    zones = ['ru-central1-a', 'ru-central1-b', 'ru-central1-c']
    if args.cluster == 'hw-nbs-stable-lab' or args.dry_run:
        zones = ['ru-central1-a']
    with ThreadPoolExecutor(max_workers=3) as executor:
        futures = []
        should_stop = threading.Event()
        try:
            for zone_id in zones:
                logger.info(f'Run test in {zone_id}')
                futures.append(executor.submit(
                    _scan_disk,
                    args,
                    zone_id,
                    should_stop))
        finally:
            should_stop.set()
            for future in futures:
                logger.info(future.result())


def main():
    args = _parse_args()
    logger = common.create_logger('main', args)

    try:
        _run_test_suite(args, logger)
    except Error as e:
        logger.fatal(f'Failed to check nrd disk for emptiness: {e}')
        sys.exit(1)


if __name__ == '__main__':
    main()

import argparse
import sys

from .lib import (
    generate_test_cases,
    TestCase,
    Error,
)

from cloud.blockstore.pylibs import common
from cloud.blockstore.pylibs.clients.solomon import make_solomon_client
from cloud.blockstore.pylibs.clusters import get_cluster_test_config
from cloud.blockstore.pylibs.ycp import Ycp, YcpWrapper, make_ycp_engine


_VERIFY_TEST_REMOTE_PATH = '/usr/bin/verify-test'
_TEST_INSTANCE_CORES = 4
_TEST_INSTANCE_MEMORY = 16  # 64MB-bs test suite requires at least 8GB RAM
_DEFAULT_ZONE_ID = 'ru-central1-b'
_NFS_DEVICE = 'nfs'
_NFS_MOUNT_PATH = '/test'
_NFS_TEST_FILE = '/test/test.txt'


def _parse_args():
    parser = argparse.ArgumentParser()

    verbose_quite_group = parser.add_mutually_exclusive_group()
    verbose_quite_group.add_argument('-v', '--verbose', action='store_true')
    verbose_quite_group.add_argument('-q', '--quite', action='store_true')

    parser.add_argument('--teamcity', action='store_true', help='use teamcity logging format')
    parser.add_argument('--dry-run', action='store_true', help='dry run')

    test_arguments_group = parser.add_argument_group('test arguments')
    test_arguments_group.add_argument(
        '-c',
        '--cluster',
        type=str,
        required=True,
        help='run corruption test at the specified cluster')
    test_arguments_group.add_argument(
        '--test-suite',
        type=str,
        required=True,
        help='run the specified test suite')
    test_arguments_group.add_argument(
        '--ipc-type',
        type=str,
        default='grpc',
        help='use the specified ipc type')
    test_arguments_group.add_argument(
        '--verify-test-path',
        type=str,
        required=True,
        help='path to verify-test tool, built from arcadia cloud/blockstore/tools/testing/verify-test')
    test_arguments_group.add_argument(
        '--compute-node',
        type=str,
        default=None,
        help='run test at the specified compute node')
    test_arguments_group.add_argument(
        '--zone-id',
        type=str,
        default=_DEFAULT_ZONE_ID,
        help=f'specify zone id, default is {_DEFAULT_ZONE_ID}'
    )
    test_arguments_group.add_argument(
        '--service',
        choices=['nfs', 'nbs'],
        default='nbs',
        help='specify to use nbs or nfs for test'
    )
    test_arguments_group.add_argument(
        '--ttl-instance-days',
        type=int,
        default=7,
        help='ttl for tmp instances'
    )

    return parser.parse_args()


def _run_test_case(test_case: TestCase, instance: Ycp.Instance, file: str, args, logger):
    logger.info(f'Running verify-test at instance <id={instance.id}>')
    with common.make_ssh_client(args.dry_run, instance.ip) as ssh:
        _, stdout, _ = ssh.exec_command(
            f'{_VERIFY_TEST_REMOTE_PATH} {test_case.verify_test_cmd_args} --file {file} 2>&1')
        exit_code = stdout.channel.recv_exit_status()
        if exit_code != 0:
            logger.error(f'Failed to execute verify-test with exit code {exit_code}:\n'
                         f'{"".join(stdout.readlines())}')
            raise Error(f'verify-test execution failed with exit code {exit_code}')
        else:
            logger.info(f'{"".join(stdout.readlines())}')


def _mount_fs(instance_ip: str, dry_run: bool, logger):
    logger.info('Mounting fs')
    with common.make_ssh_client(dry_run, instance_ip) as ssh, common.make_sftp_client(dry_run, instance_ip) as sftp:
        sftp.mkdir(_NFS_MOUNT_PATH)
        _, _, stderr = ssh.exec_command(f'mount -t virtiofs {_NFS_DEVICE} {_NFS_MOUNT_PATH} && '
                                        f'touch {_NFS_TEST_FILE}')
        exit_code = stderr.channel.recv_exit_status()
        if exit_code != 0:
            logger.error(f'Failed to mount fs\n'
                         f'{"".join(stderr.readlines())}')
            raise Error(f'failed to mount fs with exit code {exit_code}')


def _run_corruption_test(args, logger):
    cluster = get_cluster_test_config(args.cluster, args.zone_id)
    logger.info(f'Running corruption test suite at cluster <{cluster.name}>')

    test_cases = generate_test_cases(args.test_suite, args.service)
    logger.info(f'Generated {len(test_cases)} test cases for test suite <{args.test_suite}>')

    ycp = YcpWrapper(
        cluster.name,
        cluster.ipc_type_to_folder_desc[args.ipc_type],
        logger,
        make_ycp_engine(args.dry_run))
    solomon = make_solomon_client(args.dry_run, logger)
    helpers = common.make_helpers(args.dry_run)

    image = None
    if args.service == 'nfs':
        image = 'ubuntu2004'

    ycp.delete_tmp_instances(args.ttl_instance_days)

    with ycp.create_instance(
            cores=_TEST_INSTANCE_CORES,
            memory=_TEST_INSTANCE_MEMORY,
            compute_node=args.compute_node,
            image_name=image) as instance:
        nbs_version = solomon.get_current_nbs_version(
            cluster.get_solomon_cluster(instance.compute_node),
            instance.compute_node)
        logger.info(f'Compute node: {instance.compute_node}; NBS version: {nbs_version}')

        logger.info(f'Waiting until instance <id={instance.id}> becomes available via ssh')
        helpers.wait_until_instance_becomes_available_via_ssh(instance.ip)

        logger.info(f'Copying verify-test to instance <id={instance.id}>')
        with common.make_sftp_client(args.dry_run, instance.ip) as sftp:
            sftp.put(args.verify_test_path, _VERIFY_TEST_REMOTE_PATH)
            sftp.chmod(_VERIFY_TEST_REMOTE_PATH, 0o755)

        for test_case in test_cases:
            logger.info(f'Executing test case <{test_case.name}>')

            if args.service == 'nbs':
                with ycp.create_disk(size=test_case.size, type_id=test_case.type) as disk:
                    with ycp.attach_disk(instance, disk):
                        logger.info(f'Waiting until secondary disk appears'
                                    f' as a block device at instance <id={instance.id}>')
                        helpers.wait_for_block_device_to_appear(instance.ip, '/dev/vdb')
                        _run_test_case(test_case, instance, '/dev/vdb', args, logger)
            else:
                with ycp.create_fs(size=test_case.size, type_id=test_case.type) as fs:
                    with ycp.attach_fs(instance, fs, _NFS_DEVICE):
                        _mount_fs(instance.ip, args.dry_run, logger)
                        _run_test_case(test_case, instance, _NFS_TEST_FILE, args, logger)


def main():
    args = _parse_args()
    logger = common.create_logger('yc-nbs-ci-corruption-test', args)

    try:
        _run_corruption_test(args, logger)
    except (Error, YcpWrapper.Error) as e:
        logger.fatal(f'Failed to run corruption test: {e}')
        sys.exit(1)


if __name__ == '__main__':
    main()

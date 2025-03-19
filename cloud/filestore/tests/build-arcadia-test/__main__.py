import argparse
import string
import sys
import time

from cloud.blockstore.pylibs import common
from cloud.blockstore.pylibs.clusters import get_cluster_test_config
from cloud.blockstore.pylibs.ycp import YcpWrapper, make_ycp_engine

from library.python import resource

_TEST_INSTANCE_CORES = 16
_TEST_INSTANCE_MEMORY = 16
_TEST_FS_SIZE = 500  # GB

_IMAGE_NAME = 'ubuntu2004'
_DEFAULT_USER = 'robot-yc-nbs'

_MOUNT_PATH = '/test'

_SCRIPT_NAME = 'build.sh'
_SCRIPT_PATH = f'/{_MOUNT_PATH}/{_SCRIPT_NAME}'
_LOG_PATH = '/root/log.txt'

_ARCADIA_PATH = f'{_MOUNT_PATH}/arcadia'
_YA_PATH = f'{_ARCADIA_PATH}/ya'

_BINARY_NAME = 'yc-nfs-ci-build-arcadia-test'
_BUILD_PATH = 'cloud/filestore/tests/build-arcadia-test'

_DEVICE_NAME = 'nfs'
_DEFAULT_ZONE_ID = 'ru-central1-b'


class Error(Exception):
    pass


def _parse_args():
    parser = argparse.ArgumentParser()

    verbose_quite_group = parser.add_mutually_exclusive_group()
    verbose_quite_group.add_argument('-v', '--verbose', action='store_true')
    verbose_quite_group.add_argument('-q', '--quite', action='store_true')

    parser.add_argument(
        '--teamcity',
        action='store_true',
        help='use teamcity logging format')
    parser.add_argument('--dry-run', action='store_true', help='dry run')

    test_arguments_group = parser.add_argument_group('test arguments')
    test_arguments_group.add_argument(
        '-c',
        '--cluster',
        type=str,
        required=True,
        help='run corruption test at the specified cluster')
    test_arguments_group.add_argument(
        '--compute-node',
        type=str,
        default=None,
        help='run test at the specified compute node')
    test_arguments_group.add_argument(
        '--test-case',
        type=str,
        required=True,
        choices=['nbs', 'nfs']
    )
    test_arguments_group.add_argument(
        '--zone-id',
        type=str,
        default=_DEFAULT_ZONE_ID,
        help=f'specify zone id, default is {_DEFAULT_ZONE_ID}'
    )
    test_arguments_group.add_argument(
        '--debug',
        action='store_true',
        help='dont delete instance if test fail'
    )

    return parser


def safe_run_ssh_command(instance_ip: str, dry_run: bool, cmd: str, logger):
    with common.make_ssh_client(dry_run, instance_ip) as ssh:
        _, stdout, stderr = ssh.exec_command(cmd)
        exit_code = stdout.channel.recv_exit_status()
        if exit_code != 0:
            logger.error(f'Failed to run command: {cmd}; with exit code: {exit_code}:\n'
                         f'stderr: {"".join(stderr.readlines())}\n'
                         f'stdout: {"".join(stdout.readlines())}')
            raise Error('Failed to run command')

        out = stdout.read().decode("utf-8")
        logger.debug(f'Command: {cmd}\n'
                     f'stderr: {"".join(stderr.readlines())}\n'
                     f'stdout: {out}')

        return out


def create_fs_over_nbs_disk(instance_ip: str, dry_run: bool, logger):
    logger.info('Creating fs over nbs disk')
    with common.make_sftp_client(dry_run, instance_ip) as sftp:
        sftp.mkdir(_MOUNT_PATH)
        safe_run_ssh_command(
            instance_ip,
            dry_run,
            f'mkfs.ext4 /dev/vdb && mount /dev/vdb {_MOUNT_PATH}',
            logger)


def run_build(instance_ip: str, dry_run: bool, logger):
    logger.info('Running build script on instance')

    template = string.Template(resource.find(f'{_SCRIPT_NAME}').decode('utf8'))
    script = template.substitute(
        mountPath=_MOUNT_PATH,
        user=_DEFAULT_USER,
        arcadiaPath=_ARCADIA_PATH,
        yaPath=_YA_PATH,
        buildPath=_BUILD_PATH
    )

    with common.make_sftp_client(dry_run, instance_ip) as sftp:
        file = sftp.file(_SCRIPT_PATH, 'w')
        file.write(script)
        file.flush()
        sftp.chmod(_SCRIPT_PATH, 0o755)

    channel = common.make_channel(dry_run, instance_ip)
    channel.exec_command(f'{_SCRIPT_PATH} > {_LOG_PATH} 2>&1')

    while True:
        out = safe_run_ssh_command(instance_ip, dry_run, f'ps aux | grep {_SCRIPT_NAME}', logger)
        lines = out.count('\n')
        if dry_run or lines == 2:
            logger.info('Build ready')
            break
        logger.info('Build is still running:\n'
                    f'{out}'
                    'sleeping for 10 minutes')
        time.sleep(10 * 60)


def mount_fs(instance_ip: str, dry_run: bool, logger):
    logger.info('Mounting fs')
    with common.make_sftp_client(dry_run, instance_ip) as sftp:
        sftp.mkdir(_MOUNT_PATH)
        safe_run_ssh_command(
            instance_ip,
            dry_run,
            f'mount -t virtiofs {_DEVICE_NAME} {_MOUNT_PATH}',
            logger)


def verify_help(ycp: YcpWrapper, instance_ip: str, help_str: str, dry_run: bool, logger):
    logger.info('Verifying help on instance')
    try:
        res = safe_run_ssh_command(
            instance_ip,
            dry_run,
            f'{_ARCADIA_PATH}/{_BUILD_PATH}/{_BINARY_NAME} --help',
            logger)
    except Error as e:
        with common.make_sftp_client(dry_run, instance_ip) as sftp:
            sftp.get(_LOG_PATH, './log.txt')
        logger.error(f'failed to verify help:\n {e}')
        ycp.turn_off_auto_deletion()
        raise Error('failed to run help')

    if res.split() != help_str.split() and not dry_run:
        logger.error(f'--help differs from expected.\n'
                     f'Actual:\n{res}\n'
                     f'Expected:\n{help_str}')
        ycp.turn_off_auto_deletion()
        raise Error('failed to verify binary')


def clear_test_directories(instance_ip: str, dry_run: bool, logger):
    logger.info('Clearing test directories')

    safe_run_ssh_command(
        instance_ip,
        dry_run,
        f'arc unmount {_ARCADIA_PATH}',
        logger)

    safe_run_ssh_command(
        instance_ip,
        dry_run,
        f'find {_MOUNT_PATH} -mindepth 1 -delete',  # remove everything including .ya
        logger)


def validate_df_result(df: str, resource: str, instance_ip: str, dry_run: bool, logger):
    res = safe_run_ssh_command(
        instance_ip,
        dry_run,
        df,
        logger)
    logger.debug(f"{df}\n{res}")
    if dry_run:
        return

    lines = res.split('\n')
    if len(lines) != 3:
        raise Error(f'failed to run df:\n{res}')
    stats = lines[1].split()
    if len(stats) != 6:
        raise Error(f'unexpected df stats (expected 6 elements):\n{res}')
    if int(stats[2]) != 0:
        raise Error(f'invalid {resource} count (expected 0):\n{res}')


def validate_fs_stat(instance_ip: str, dry_run: bool, logger):
    logger.info('Validating FS stats')
    validate_df_result(f'df {_MOUNT_PATH}', 'blocks', instance_ip, dry_run, logger)
    validate_df_result(f'df -i {_MOUNT_PATH}', 'inodes', instance_ip, dry_run, logger)


def _run_build_test(parser, args, logger):
    cluster = get_cluster_test_config(args.cluster, args.zone_id)
    logger.info(f'Running build test suite at cluster <{cluster.name}>')

    ycp = YcpWrapper(
        cluster.name,
        cluster.ipc_type_to_folder_desc['grpc'],
        logger,
        make_ycp_engine(args.dry_run))
    helpers = common.make_helpers(args.dry_run)
    with ycp.create_instance(
            cores=_TEST_INSTANCE_CORES,
            memory=_TEST_INSTANCE_MEMORY,
            compute_node=args.compute_node,
            image_name=_IMAGE_NAME) as instance:
        logger.info(
            f'Waiting until instance <id={instance.id}> becomes available via ssh')
        helpers.wait_until_instance_becomes_available_via_ssh(instance.ip)

        if args.test_case == 'nbs':
            logger.info('Starting test with nbs disk')
            with ycp.create_disk(_TEST_FS_SIZE, 'network-ssd') as disk:
                with ycp.attach_disk(instance, disk):
                    create_fs_over_nbs_disk(instance.ip, args.dry_run, logger)
                    run_build(instance.ip, args.dry_run, logger)
                    verify_help(
                        ycp,
                        instance.ip,
                        parser.format_help(),
                        args.dry_run,
                        logger)
        elif args.test_case == 'nfs':
            logger.info('Starting test with filestore')
            with ycp.create_fs(_TEST_FS_SIZE, 'network-ssd') as fs:
                with ycp.attach_fs(instance, fs, _DEVICE_NAME):
                    mount_fs(instance.ip, args.dry_run, logger)
                    run_build(instance.ip, args.dry_run, logger)
                    verify_help(
                        ycp,
                        instance.ip,
                        parser.format_help(),
                        args.dry_run,
                        logger)
                    clear_test_directories(instance.ip, args.dry_run, logger)
                    validate_fs_stat(instance.ip, args.dry_run, logger)
        else:
            raise Error('Unknown test-case')


def main():
    parser = _parse_args()
    args = parser.parse_args()
    logger = common.create_logger('yc-nfs-ci-build-arcadia-test', args)

    try:
        _run_build_test(parser, args, logger)
    except (Error, YcpWrapper.Error) as e:
        logger.fatal(f'Failed to run build trunk test: {e}')
        sys.exit(1)


if __name__ == '__main__':
    main()

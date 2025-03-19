import argparse
import sys
from concurrent.futures import ThreadPoolExecutor

import paramiko

from cloud.blockstore.pylibs import common
from cloud.blockstore.pylibs.clusters import ClusterTestConfig, get_cluster_test_config
from cloud.blockstore.pylibs.ycp import YcpWrapper, make_ycp_engine, Ycp


_DEFAULT_ZONE_ID = 'ru-central1-a'
_VM_NAME_PREFIX = 'multi-client-test-vm%s'
_FS_NAME = 'multi-client-test-fs'
_NFS_MOUNT_PATH = '/test'
_CLIENT_BIN_REMOTE_PATH = '/usr/bin/client'
_DEVICE_NAME = 'nfs'


class Error(Exception):
    pass


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
        help='run fio test on specified cluster')
    test_arguments_group.add_argument(
        '--pairs',
        type=int,
        default=8,
        help='specify number of pairs reader-writer')
    test_arguments_group.add_argument(
        '--zone-id',
        type=str,
        default=_DEFAULT_ZONE_ID,
        help=f'specify zone id, default is {_DEFAULT_ZONE_ID}'
    )
    test_arguments_group.add_argument(
        '--run-custom-command',
        action='store_true',
        help='run custom command on all vm'
    )
    test_arguments_group.add_argument(
        '--scp-load-binary',
        action='store_true',
        help='put binary on all instances'
    )
    test_arguments_group.add_argument(
        '--setup-test',
        action='store_true',
        help='specify, if test running for the first time'
    )

    return parser.parse_args()


def put_binary(instance: Ycp.Instance, path: str, dry_run, logger):
    logger.info('Put client binary on instance')
    try:
        with common.make_sftp_client(dry_run, instance.ip) as sftp:
            sftp.put(path, _CLIENT_BIN_REMOTE_PATH)
            sftp.chmod(_CLIENT_BIN_REMOTE_PATH, 0o755)
    except IOError or paramiko.SFTPError as e:
        raise Error(f'Failed to copy binary to instance:\n{e}')


def _mount_fs(instance: Ycp.Instance, device_name: str, test_file_name: str, remount: bool, dry_run: bool, logger):
    logger.info('Mounting fs')
    with common.make_ssh_client(dry_run, instance.ip) as ssh:
        if remount:
            umout_cmd = f'umount {_NFS_MOUNT_PATH} &&'
        _, _, stderr = ssh.exec_command(f'{umout_cmd} '
                                        f'mount -t virtiofs {device_name} -o rw,x-mount.mkdir {_NFS_MOUNT_PATH} && '
                                        f'touch {_NFS_MOUNT_PATH}/{test_file_name}')
        exit_code = stderr.channel.recv_exit_status()
        if exit_code != 0:
            logger.error(f'Failed to mount fs\n'
                         f'{"".join(stderr.readlines())}')
            raise Error(f'failed to mount fs with exit code {exit_code}')


def setup_clients(number_of_client: int, instance: Ycp.Instance, scp: bool, remount: bool, dry_run: bool, logger):
    with common.make_ssh_client(dry_run, instance.ip) as ssh:
        _, _, stderr = ssh.exec_command('pkill client')
        stderr.channel.recv_exit_status()

    if scp:
        put_binary(instance, '../client/client', dry_run, logger)
    file_name = f'test-{number_of_client}.txt'
    _mount_fs(instance, _DEVICE_NAME, file_name, remount, dry_run, logger)
    cmd_writer = (f'{_CLIENT_BIN_REMOTE_PATH} --file {_NFS_MOUNT_PATH}/{file_name} '
                  f'--filesize 10 --request-size 1 --type write --sleep-between-writes 10 '
                  f'--sleep-before-start {number_of_client} > writer.log 2>&1')
    cmd_reader = (f'{_CLIENT_BIN_REMOTE_PATH} --file {_NFS_MOUNT_PATH}/{file_name} '
                  f'--filesize 10 --request-size 1 --type read '
                  f'--sleep-before-start {number_of_client + 1} > reader.log 2>&1')

    logger.info('Run writer')
    channel = common.make_channel(dry_run, instance.ip, 'root')
    channel.exec_command(cmd_writer)

    logger.info('Run reader')
    channel = common.make_channel(dry_run, instance.ip, 'root')
    channel.exec_command(cmd_reader)


def setup_instance(
        cluster: ClusterTestConfig,
        fs: Ycp.Filesystem,
        number_of_client: int,
        scp: bool,
        attach_fs: bool,
        remount: bool,
        dry_run: bool,
        logger):
    ycp = YcpWrapper(cluster.name, cluster.ipc_type_to_folder_desc['grpc'], logger, make_ycp_engine(dry_run))
    instance = create_vm_if_needed(ycp, fs, _VM_NAME_PREFIX % number_of_client, dry_run, attach_fs, logger)

    setup_clients(number_of_client, instance, scp, remount, dry_run, logger)
    return f'Successfully run client {number_of_client}'


def create_vm_if_needed(ycp, fs: Ycp.Filesystem, name: str, dry_run: bool, attach_fs: bool, logger) -> Ycp.Instance:
    instance = ycp.find_instance(name)
    if instance:
        if attach_fs:
            with ycp.attach_fs(instance, fs, device_name=_DEVICE_NAME, auto_detach=False):
                pass
        return instance

    logger.info('Instance not found')
    cores, memory = (4, 4)
    helpers = common.make_helpers(dry_run)
    with ycp.create_instance(
            name=name,
            cores=cores, memory=memory,
            image_name='ubuntu2004',
            auto_delete=False) as instance:
        with ycp.attach_fs(instance, fs, device_name=_DEVICE_NAME, auto_detach=False):
            logger.info(f'Waiting until instance <id={instance.id}> becomes available via ssh')
            helpers.wait_until_instance_becomes_available_via_ssh(instance.ip)
            return instance


def create_fs_if_needed(ycp, logger) -> (Ycp.Filesystem, bool):
    fs_list = ycp.list_filesystems()
    logger.info('Try to find fs')
    for fs in fs_list:
        if fs.name == _FS_NAME:
            return fs, False

    logger.info('FS not found')
    with ycp.create_fs(1024, 'network-ssd', name=_FS_NAME, auto_delete=False) as fs:
        return fs, True


def run_command_on_client(
        cluster: ClusterTestConfig,
        command: str,
        number_of_client: int,
        dry_run: bool,
        logger):
    ycp = YcpWrapper(cluster.name, cluster.ipc_type_to_folder_desc['grpc'], logger, make_ycp_engine(dry_run))

    instance = ycp.find_instance(_VM_NAME_PREFIX % number_of_client)
    if not instance:
        logger.warn(f'Instance {number_of_client} not found')
        return

    with common.make_ssh_client(dry_run, instance.ip) as ssh:
        _, stdout, stderr = ssh.exec_command(command)
        exit_code = stdout.channel.recv_exit_status()
        logger.info(f'Command finished with exit code {exit_code}:\n'
                    f'stdout:\n'
                    f'{"".join(stdout.readlines())}\n'
                    f'stderr:\n'
                    f'{"".join(stderr.readlines())}')
        if exit_code:
            raise Error('Failed to run command')


def run(args, logger):
    cluster = get_cluster_test_config(args.cluster, args.zone_id)
    ycp = YcpWrapper(cluster.name, cluster.ipc_type_to_folder_desc['grpc'], logger, make_ycp_engine(args.dry_run))
    fs, attach_fs = create_fs_if_needed(ycp, logger)

    command = ''
    if args.run_custom_command:
        logger.info('Input command:')
        command = input()

    fail = False
    with ThreadPoolExecutor(max_workers=5) as executor:
        futures = []
        for i in range(args.pairs):
            logger.info(f'Setting up {i} pair')
            sublogger = common.create_logger(f'Pair {i}', args)

            if args.run_custom_command:
                future = executor.submit(
                    run_command_on_client,
                    cluster,
                    command,
                    i,
                    args.dry_run,
                    sublogger)
            else:
                future = executor.submit(
                    setup_instance,
                    cluster,
                    fs,
                    i,
                    args.scp_load_binary,
                    attach_fs,
                    not args.setup_test,
                    args.dry_run,
                    sublogger)
            futures.append(future)

        for i in range(args.pairs):
            try:
                futures[i].result()
                logger.info(f'Successfully run test on instance number {i}')
            except Error as e:
                logger.error(f'Failed to run test on instance number {i}:\n{e}')
                fail = True
    if fail:
        raise Error('Fail to run test')


def main():
    args = _parse_args()
    logger = common.create_logger('yc-nbs-multi-client-test', args)

    try:
        run(args, logger)
    except Error as e:
        logger.fatal(f'Failed to run test: {e}')
        sys.exit(1)


if __name__ == '__main__':
    main()

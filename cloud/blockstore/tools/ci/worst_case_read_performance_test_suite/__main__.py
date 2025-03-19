import argparse
from concurrent.futures.thread import ThreadPoolExecutor
from datetime import datetime
import json
import sys
import threading
import time

from paramiko import SSHException

from .lib import (
    Error, YtHelper
)

from cloud.blockstore.pylibs import common
from cloud.blockstore.pylibs.clients.solomon import SolomonClient
from cloud.blockstore.pylibs.clusters import ClusterTestConfig, get_cluster_test_config
from cloud.blockstore.pylibs.ycp import Ycp, YcpWrapper, make_ycp_engine

_DISK_TYPE = 'network-ssd'
_DISK_SIZE = 320  # GB
_BLOCK_SIZE = 4  # KB
_BATCH_SIZE = 1024
_IO_DEPTH = 32
_CHECKPOINT_ID = 'test'
_DEFAULT_USER = 'robot-yc-nbs'
_DEFAULT_TIMEOUT = 5 * 60 * 60  # 5 hours


_FIO_LONG_WRITE_COMMAND = ('fio --name=fio-randwrite  --filename=/dev/vdb --size=100%%'
                           ' --rw=randwrite --time_based=1 --runtime=43200'
                           f' --bs=%s --iodepth={_IO_DEPTH} --ioengine=libaio'
                           ' --direct=1 --sync=1 --numjobs=1 --output-format=json')

_FIO_COMMAND_READ_ALL = ('fio --name=fio-read  --filename=/dev/vdb'
                         ' --size=100% --rw=read'
                         f' --bs=4M --iodepth={_IO_DEPTH} --ioengine=libaio'
                         ' --direct=1 --sync=1 --numjobs=1 --output-format=json')

_CREATE_CHECKPOINT_COMMAND = (f'blockstore-client CreateCheckpoint --disk-id %s'
                              f' --checkpoint-id {_CHECKPOINT_ID}')

_READ_ALL_COMMAND = (f'blockstore-client ReadBlocks --read-all --disk-id %s'
                     f' --checkpoint-id {_CHECKPOINT_ID}'
                     f' --io-depth {_IO_DEPTH} >> /dev/null')
_DEFAULT_ZONE_ID = 'ru-central1-b'


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
        help='run worst case read test on specified cluster')
    test_arguments_group.add_argument(
        '--ipc-type',
        type=str,
        default='grpc',
        help='use specified ipc type')
    test_arguments_group.add_argument(
        '--write-block-size',
        type=int,
        default=4,
        help='specify fio randwrite command blocksize in KB'
    )
    test_arguments_group.add_argument(
        '--zone-id',
        type=str,
        default=_DEFAULT_ZONE_ID,
        help=f'specify zone id, default is {_DEFAULT_ZONE_ID}'
    )
    test_arguments_group.add_argument(
        '--timeout',
        type=int,
        default=_DEFAULT_TIMEOUT,
        help='reading from checkpoint timeout'
    )
    test_arguments_group.add_argument(
        '--ttl-instance-days',
        type=int,
        default=7,
        help='ttl for tmp instances'
    )

    return parser.parse_args()


def long_write_fio_task(instance: Ycp.Instance, should_stop, bs: int):
    try:
        with common.ssh_client(instance.ip) as ssh_vm:
            _, stdout, stderr = ssh_vm.exec_command(_FIO_LONG_WRITE_COMMAND % bs)
            while not should_stop.is_set():
                if stdout.channel.exit_status_ready():
                    if stdout.channel.recv_exit_status():
                        raise Error(f'Failed to run fio random write command on instance <{instance.id}>\n'
                                    f'{stderr.read().decode("utf-8")}')
                    raise Error(f'Fio command unexpected stopped on instance <{instance.id}>\n'
                                f'{stdout.read().decode("utf-8")}')
            _, stdout2, stderr = ssh_vm.exec_command('sudo pkill -15 fio')
            if stdout2.channel.recv_exit_status():
                raise Error(f'Failed kill fio command \n {stderr.read().decode("utf-8")}')
            return json.loads('\n'.join(stdout.readlines()[4:]))  # first 4 lines in fio report is about termination
    except SSHException as e:
        raise Error(f'Error during connection via ssh to <{instance.compute_node}> {e}')
    finally:
        should_stop.set()


def run_test_with_checkpoint(instance: Ycp.Instance, disk_id: str, logger, should_stop, timeout: int):
    try:
        with common.ssh_client(instance.compute_node, user=_DEFAULT_USER) as ssh_vm:
            logger.info(f'Creating checkpoint <id={_CHECKPOINT_ID}> on disk <id={disk_id}>')
            _, stdout, stderr = ssh_vm.exec_command(_CREATE_CHECKPOINT_COMMAND % disk_id)
            if stdout.channel.recv_exit_status():
                raise Error(f'Failed to create checkpoint on compute node <{instance.compute_node}>\n'
                            f'{stderr.read().decode("utf-8")}')
        logger.info('Checkpoint successfully created')

        with common.ssh_client(instance.compute_node, user=_DEFAULT_USER) as ssh_vm:
            start_time = time.time()
            logger.info(f'Running read all command on disk <id={disk_id}> checkpoint <id={_CHECKPOINT_ID}>')
            _, stdout, stderr = ssh_vm.exec_command(_READ_ALL_COMMAND % disk_id)

            start = time.time()
            while not stdout.channel.exit_status_ready() and (time.time() - start) < timeout:
                time.sleep(60)

            if (time.time() - start) > timeout:
                raise Error('Timeout reading from checkpoint')

            if stdout.channel.recv_exit_status():
                raise Error(f'Failed to run readblocks commmand on compute node <{instance.compute_node}>\n'
                            f'{stderr.read().decode("utf-8")}')
            running_time = time.time() - start_time
            iops = (_DISK_SIZE * 1024 ** 2) / (_BATCH_SIZE * _BLOCK_SIZE * running_time)
            bandwidth = round(_BLOCK_SIZE * _BATCH_SIZE * iops)  # kb/s
            clat_average = running_time / ((_DISK_SIZE * 1024 ** 2) / (_BLOCK_SIZE * _BATCH_SIZE)) * 10 ** 6  # usec
            return {
                'read_iops': iops,
                'read_bw': bandwidth,
                'read_clat': clat_average
            }
    except SSHException as e:
        raise Error(f'Error during connection via ssh to <{instance.compute_node}> with <user={_DEFAULT_USER}>: {e}')
    finally:
        should_stop.set()


def run_test_without_checkpoint(instance: Ycp.Instance, logger, should_stop):
    try:
        with common.ssh_client(instance.ip) as ssh_vm:
            logger.info('Running fio read all command')
            _, stdout, stderr = ssh_vm.exec_command(_FIO_COMMAND_READ_ALL)
            if stdout.channel.recv_exit_status():
                raise Error(f'Failed to run fio cmd on instance <{instance.id}\n'
                            f'{stderr.read().decode("utf-8")}>')
            json_report = json.load(stdout)
            return {
                'read_iops': json_report['jobs'][0]['read']['iops'],
                'read_bw': json_report['jobs'][0]['read']['bw'],
                'read_clat': json_report['jobs'][0]['read']['clat']['mean']
            }
    except SSHException as e:
        raise Error(f'Error during connection via ssh to <ip={instance.ip}>: {e}')
    finally:
        should_stop.set()


def bs_formatted(bs: int):
    if bs % 1024 == 0:
        return '%sM' % (bs // 1024)
    else:
        return '%sK' % bs


def run_test(
        instance: Ycp.Instance,
        logger,
        cluster: ClusterTestConfig,
        disk_id: str,
        checkpoint: bool,
        bs: int,
        timeout: int):
    with ThreadPoolExecutor(max_workers=1) as executor:
        try:
            should_stop = threading.Event()
            future = executor.submit(long_write_fio_task, instance, should_stop, bs_formatted(bs))
            needed_disk_size = _DISK_SIZE * 0.8 * 1024 ** 3  # bytes
            logger.info('Waiting until disk space is 80% full')
            solomon_cluster = cluster.get_solomon_cluster(instance.compute_node)
            used_bytes = SolomonClient(logger).get_current_used_bytes_count(solomon_cluster, disk_id)
            while used_bytes < needed_disk_size:
                if should_stop.is_set():
                    _ = future.result()
                time.sleep(10)
                used_bytes = SolomonClient(logger).get_current_used_bytes_count(solomon_cluster, disk_id)
            if checkpoint:
                read_report = run_test_with_checkpoint(instance, disk_id, logger, should_stop, timeout)
            else:
                read_report = run_test_without_checkpoint(instance, logger, should_stop)
            write_report = future.result()
            return read_report, write_report
        finally:
            should_stop.set()


def _run_test_suite(args, logger):
    cluster = get_cluster_test_config(args.cluster, args.zone_id)
    today = datetime.today().strftime('%Y-%m-%d')

    test_suite = 'worst_case_read_without_checkpoint'
    checkpoint = False
    if cluster.name == 'hw-nbs-stable-lab':
        test_suite = 'worst_case_read_with_checkpoint'
        checkpoint = True

    ycp = YcpWrapper(cluster.name, cluster.ipc_type_to_folder_desc[args.ipc_type], logger, make_ycp_engine(False))
    helpers = common.make_helpers(False)
    cores, memory = (8, 8)

    ycp.delete_tmp_instances(args.ttl_instance_days)

    try:
        with ycp.create_instance(cores=cores, memory=memory) as instance:
            nbs_version = SolomonClient(logger).get_current_nbs_version(
                cluster.get_solomon_cluster(instance.compute_node), instance.compute_node)
            logger.info(
                f'Running worst case read performance test on cluster <{cluster.name}>; '
                f'NBS server version <{nbs_version}>')

            if checkpoint:
                logger.info(f'Check if compute node <id={instance.compute_node}> available via ssh')
                helpers.wait_until_instance_becomes_available_via_ssh(instance.compute_node, user=_DEFAULT_USER)

            yt = YtHelper(test_suite, cluster.name, nbs_version, today, logger)
            yt.init_structure()
            logger.info(f'Waiting until instance <id={instance.id}> becomes available via ssh')
            helpers.wait_until_instance_becomes_available_via_ssh(instance.ip)

            with ycp.create_disk(_DISK_SIZE, _DISK_TYPE) as disk:
                with ycp.attach_disk(instance, disk):
                    logger.info(
                        f'Waiting until secondary disk appears as block device on instance <id={instance.id}>')
                    helpers.wait_for_block_device_to_appear(instance.ip, '/dev/vdb')

                    read_report, write_report = run_test(
                        instance,
                        logger,
                        cluster,
                        disk.id,
                        checkpoint,
                        args.write_block_size,
                        args.timeout)
                    yt.publish_test_report({'disk_id': disk.id,
                                            'disk_size': _DISK_SIZE,
                                            'iodepth': _IO_DEPTH},
                                           read_report,
                                           write_report)
                    yt.announce_test_run()
    except YcpWrapper.Error as e:
        raise Error(f'Failed to execute {test_suite}: {e}')


def main():
    args = _parse_args()
    logger = common.create_logger('yc-nbs-ci-worst-case-read-performance-test-suite', args)

    try:
        _run_test_suite(args, logger)
    except Error as e:
        logger.fatal(f'Failed to run test suite: {e}')
        sys.exit(1)


if __name__ == '__main__':
    main()

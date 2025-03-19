import argparse
from datetime import datetime
import json
import logging
import paramiko
import socket
import sys

from .lib import (
    generate_test_cases,
    Error,
    TestCase,
    YtHelper,
    YtHelperStub,
)

from cloud.blockstore.pylibs import common
from cloud.blockstore.pylibs.clients.solomon import SolomonClient
from cloud.blockstore.pylibs.clusters import get_cluster_test_config
from cloud.blockstore.pylibs.ycp import Ycp, YcpWrapper, make_ycp_engine


_DEFAULT_ZONE_ID = 'ru-central1-b'
_DEFAULT_INSTANCE_CORES = 8
_DEFAULT_INSTANCE_RAM = 8


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()

    verbose_quite_group = parser.add_mutually_exclusive_group()
    verbose_quite_group.add_argument('-v', '--verbose', action='store_true')
    verbose_quite_group.add_argument('-q', '--quite', action='store_true')

    parser.add_argument(
        '--teamcity',
        action='store_true',
        default=False,
        help='use teamcity logging format')

    test_arguments_group = parser.add_argument_group('test arguments')
    test_arguments_group.add_argument(
        '-c',
        '--cluster',
        type=str,
        required=True,
        help='run fio test on specified cluster')
    test_arguments_group.add_argument(
        '--test-suite',
        type=str,
        required=True,
        help='run specified test suite')
    test_arguments_group.add_argument(
        '--ipc-type',
        type=str,
        default='grpc',
        help='use specified ipc type')
    test_arguments_group.add_argument(
        '-f',
        '--force',
        action='store_true',
        default=False,
        help='run all test cases and override YT results table')
    test_arguments_group.add_argument(
        '--placement-group-name',
        type=str,
        default=None,
        help='create vm with specified placement group')
    test_arguments_group.add_argument(
        '--compute-node',
        type=str,
        default=None,
        help='run fio test on specified compute node')
    test_arguments_group.add_argument(
        '--no-yt',
        action='store_true',
        default=False,
        help='do not use yt')
    test_arguments_group.add_argument(
        '--zone-id',
        type=str,
        default=_DEFAULT_ZONE_ID,
        help=f'specify zone id, default is {_DEFAULT_ZONE_ID}')
    test_arguments_group.add_argument(
        '--ttl-instance-days',
        type=int,
        default=7,
        help='ttl for tmp instances')
    test_arguments_group.add_argument(
        '--in-parallel',
        action='store_true',
        default=False,
        help='run all test cases in parallel')
    test_arguments_group.add_argument(
        '--image-name',
        type=str,
        default=None,
        help='use image with specified name to create disk')
    test_arguments_group.add_argument(
        '--instance-cores',
        type=int,
        default=_DEFAULT_INSTANCE_CORES,
        help=f'specify instance core count, default is {_DEFAULT_INSTANCE_CORES}')
    test_arguments_group.add_argument(
        '--instance-ram',
        type=int,
        default=_DEFAULT_INSTANCE_RAM,
        help=f'specify instance RAM in GiB, default is {_DEFAULT_INSTANCE_RAM}')
    test_arguments_group.add_argument(
        '--debug',
        action='store_true',
        default=False,
        help='do not delete instance and disk, if fail')

    return parser.parse_args()


def _get_fio_cmd_to_fill_disk(block_device: str) -> str:
    return (f'fio --name=fill-secondary-disk --filename={block_device}'
            f' --rw=write --bs=4M --iodepth=128 --direct=1 --sync=1'
            f' --ioengine=libaio --size={160 * 1024 ** 3}')


def _detach_disks(instance: Ycp.Instance,
                  disks: list[Ycp.Disk],
                  ycp: YcpWrapper,
                  logger: logging.Logger) -> None:
    error_str = ''
    for disk in disks:
        try:
            ycp.detach_disk(instance, disk)
        except YcpWrapper.Error as e:
            logger.error(f'failed to detach disk <id={disk.id}>')
            error_str += f'\n{e}'
            continue
    if len(error_str) > 0:
        raise Error('failed to detach disks: {error_str}')


def _delete_disks(disks: list[Ycp.Disk],
                  ycp: YcpWrapper,
                  logger: logging.Logger) -> None:
    error_str = ''
    for disk in disks:
        try:
            ycp.delete_disk(disk)
        except YcpWrapper.Error as e:
            logger.error(f'failed to delete disk <id={disk.id}>')
            error_str += f'\n{e}'
            continue
    if len(error_str) > 0:
        raise Error('failed to delete disks: {error_str}')


def _run_parallel_tests(test_cases: list[TestCase],
                        completed_test_cases: set[TestCase],
                        args: argparse.Namespace,
                        profiler: common.Profiler,
                        helpers: common.Helpers,
                        logger: logging.Logger,
                        ycp: YcpWrapper,
                        instance: Ycp.Instance,
                        yt) -> None:
    disks: list[Ycp.Disk] = list()
    for test_case in test_cases:
        with ycp.create_disk(
                size=test_case.disk_size,
                type_id=test_case.disk_type,
                bs=test_case.disk_bs,
                image_name=args.image_name,
                auto_delete=False) as disk:
            with ycp.attach_disk(
                    instance=instance,
                    disk=disk,
                    auto_detach=False):
                helpers.wait_for_block_device_to_appear(
                    instance.ip,
                    test_case.device_name)
                disks.append(disk)

    with common.ssh_client(instance.ip) as ssh:
        stdouts = list()
        stderrs = list()
        for test_case in test_cases:
            try:
                logger.info(f'Filling disk <name={test_case.device_name}> with'
                            f' random data on instance <id={instance.id}>')
                _, stdout, stderr = ssh.exec_command(
                    _get_fio_cmd_to_fill_disk(test_case.device_name))
                stdouts.append(stdout)
                stderrs.append(stderr)
            except (paramiko.SSHException, socket.error) as e:
                profiler.add_ip(instance.ip)
                if args.debug:
                    ycp.turn_off_auto_deletion()
                else:
                    _detach_disks(instance, disks, ycp, logger)
                    _delete_disks(disks, ycp, logger)
                raise Error(f'failed to finish test, problem with'
                            f' ssh connection: {e}')

        for test_index in range(len(stderrs)):
            if stderrs[test_index].channel.recv_exit_status():
                if not args.debug:
                    _detach_disks(instance, disks, ycp, logger)
                    _delete_disks(disks, ycp, logger)
                stderr_str = ''.join(stderrs[test_index].readlines())
                raise Error(f'failed to fill disk <id={disks[test_index].id},'
                            f' name={test_cases[test_index].device_name}>'
                            f' on instance <id={instance.id}>:\n{stderr_str}')

        stdouts.clear()
        stderrs.clear()
        for test_case in test_cases:
            try:
                logger.info(f'Running fio on disk <name={test_case.device_name}'
                            f'> on instance <id={instance.id}>')
                _, stdout, stderr = ssh.exec_command(test_case.fio_cmd)
                stdouts.append(stdout)
                stderrs.append(stderr)
                logger.info(f'Executing test case <{test_case.name}>')
            except (paramiko.SSHException, socket.error) as e:
                profiler.add_ip(instance.ip)
                if args.debug:
                    ycp.turn_off_auto_deletion()
                else:
                    _detach_disks(instance, disks, ycp, logger)
                    _delete_disks(disks, ycp, logger)
                raise Error(f'failed to finish test, problem with'
                            f' ssh connection: {e}')

        for test_index in range(len(stdouts)):
            if stdouts[test_index].channel.recv_exit_status():
                if not args.debug:
                    _detach_disks(instance, disks, ycp, logger)
                    _delete_disks(disks, ycp, logger)
                stderr_str = ''.join(stderrs[test_index].readlines())
                raise Error(f'failed to fill disk <id={disks[test_index].id},'
                            f' name={test_cases[test_index].device_name}>'
                            f' on instance <id={instance.id}>:\n{stderr_str}')

            yt.publish_test_report(
                instance.compute_node,
                disks[test_index].id,
                test_cases[test_index],
                json.load(stdouts[test_index]))
            completed_test_cases.add(test_cases[test_index])

    _detach_disks(instance, disks, ycp, logger)
    _delete_disks(disks, ycp, logger)


def _run_sequential_tests(test_cases: list[TestCase],
                          completed_test_cases: set[TestCase],
                          args: argparse.Namespace,
                          profiler: common.Profiler,
                          helpers: common.Helpers,
                          logger: logging.Logger,
                          ycp: YcpWrapper,
                          instance: Ycp.Instance,
                          yt) -> None:
    for test_case in test_cases:
        logger.info(f'Executing test case <{test_case.name}>')
        with ycp.create_disk(
                size=test_case.disk_size,
                type_id=test_case.disk_type,
                bs=test_case.disk_bs,
                image_name=args.image_name) as disk:
            with ycp.attach_disk(
                    instance=instance,
                    disk=disk):
                logger.info(f'Waiting until secondary disk appears as block'
                            f' device on instance <id={instance.id}>')
                helpers.wait_for_block_device_to_appear(
                    instance.ip,
                    test_case.device_name)

                with common.ssh_client(instance.ip) as ssh:
                    try:
                        logger.info(f'Filling disk with random data on instance'
                                    f' <id={instance.id}>')
                        _, _, stderr = ssh.exec_command(
                            _get_fio_cmd_to_fill_disk(test_case.device_name))
                        if stderr.channel.recv_exit_status():
                            stderr_str = ''.join(stderr.readlines())
                            if args.debug:
                                ycp.turn_off_auto_deletion()
                            raise Error(f'failed to fill disk on instance'
                                        f' <id={instance.id}>:\n{stderr_str}')

                        logger.info(f'Running fio on instance <id={instance.id}>')
                        _, stdout, stderr = ssh.exec_command(test_case.fio_cmd)
                        if stdout.channel.recv_exit_status():
                            stderr_str = ''.join(stderr.readlines())
                            if args.debug:
                                ycp.turn_off_auto_deletion()
                            raise Error(f'failed to run fio on instance'
                                        f' <id={instance.id}>:\n{stderr_str}')
                    except (paramiko.SSHException, socket.error) as e:
                        profiler.add_ip(instance.ip)
                        if args.debug:
                            ycp.turn_off_auto_deletion()
                        raise Error(f'failed to finish test, problem with ssh'
                                    f' connection: {e}')

                    yt.publish_test_report(
                        instance.compute_node,
                        disk.id,
                        test_case,
                        json.load(stdout))
            completed_test_cases.add(test_case)


def _run_test_suite(args: argparse.Namespace,
                    profiler: common.Profiler,
                    logger: logging.Logger) -> None:
    cluster = get_cluster_test_config(args.cluster, args.zone_id)
    today = datetime.today().strftime('%Y-%m-%d')

    ycp = YcpWrapper(
        cluster.name,
        cluster.ipc_type_to_folder_desc[args.ipc_type],
        logger,
        make_ycp_engine(False))
    helpers = common.make_helpers(False)
    cores, memory = (args.instance_cores, args.instance_ram)

    ycp.delete_tmp_instances(args.ttl_instance_days)

    try:
        with ycp.create_instance(
                cores=cores,
                memory=memory,
                compute_node=args.compute_node,
                placement_group_name=args.placement_group_name) as instance:
            nbs_version = SolomonClient(logger).get_current_nbs_version(
                cluster.get_solomon_cluster(instance.compute_node),
                instance.compute_node)
            logger.info(f'Running fio performance test suite on'
                        f' cluster <{cluster.name}>; NBS server version'
                        f' <{nbs_version}>')

            test_cases = generate_test_cases(args.test_suite, cluster.name)
            logger.info(f'Generated {len(test_cases)} test cases for test'
                        f' suite <{args.test_suite}>')

            if args.no_yt:
                yt = YtHelperStub()
            else:
                yt = YtHelper(
                    args.test_suite,
                    cluster.name,
                    nbs_version,
                    today,
                    logger)

            yt.init_structure()

            if not args.force:
                completed_test_cases: set[TestCase] = yt.fetch_completed_test_cases()
                logger.info(f'Found {len(completed_test_cases)} completed test'
                            f' cases for today`s date <{today}>')
                if len(completed_test_cases) == len(test_cases):
                    yt.announce_test_run()
                    return
            else:
                completed_test_cases: set[TestCase] = set()

            logger.info(f'Waiting until instance <id={instance.id}> becomes'
                        f' available via ssh')
            try:
                helpers.wait_until_instance_becomes_available_via_ssh(instance.ip)
            except (paramiko.SSHException, socket.error) as e:
                profiler.add_ip(instance.ip)
                if args.debug:
                    ycp.turn_off_auto_deletion()
                raise Error(f'failed to start test, instance not reachable'
                            f' via ssh: {e}')

            #  Skip completed test cases
            test_cases_to_run = []
            for test_case in test_cases:
                if not args.force and test_case in completed_test_cases:
                    logger.info(f'Test case <name={test_case.name}>'
                                f' will be skipped')
                    continue
                if cluster.name == 'hw-nbs-stable-lab' and args.test_suite == 'large_block_size':
                    logger.info('block size changed to 32K')
                    test_case.disk_bs = 32 * 1024  # bytes
                test_cases_to_run.append(test_case)

            if args.in_parallel:
                for test_index in range(len(test_cases_to_run)):
                    test_cases_to_run[test_index].device_name = f'/dev/vd{chr(ord("a")+test_index+1)}'
                _run_parallel_tests(
                    test_cases_to_run,
                    completed_test_cases,
                    args,
                    profiler,
                    helpers,
                    logger,
                    ycp,
                    instance,
                    yt)
            else:
                _run_sequential_tests(
                    test_cases_to_run,
                    completed_test_cases,
                    args,
                    profiler,
                    helpers,
                    logger,
                    ycp,
                    instance,
                    yt)
    except YcpWrapper.Error as e:
        raise Error(f'failed to run test, problem with ycp:\n{e}')

    # Announce test run for dashboard if at least half of test cases have passed
    if len(completed_test_cases) >= len(test_cases) // 2:
        yt.announce_test_run()

    yt.finalize_tables()

    if len(completed_test_cases) != len(test_cases):
        raise Error('failed to execute all the test cases; watch log for more info')


def main():
    args = _parse_args()
    logger = common.create_logger('yc-nbs-ci-fio-performance-test-suite', args)
    with common.make_profiler(logger, not args.debug) as profiler:
        try:
            _run_test_suite(args, profiler, logger)
        except Error as e:
            logger.fatal(f'Failed to run fio performance test suite: {e}')
            sys.exit(1)


if __name__ == '__main__':
    main()

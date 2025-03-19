from .lib import (
    check_ssh_connection,
    create_ycp,
    Error,
    YcpNewInstancePolicy
)

from cloud.blockstore.pylibs import common
from cloud.blockstore.pylibs.clients.solomon import SolomonClient
from cloud.blockstore.pylibs.clusters import get_cluster_test_config
from cloud.blockstore.pylibs.ycp import Ycp, YcpWrapper

from ydb.tests.library.harness.kikimr_runner import (
    get_unique_path_for_current_test,
    ensure_path_exists
)

from abc import ABC, abstractmethod
import argparse
import calendar
from datetime import datetime
import logging
import subprocess
from typing import List


class BaseAcceptanceTestRunner(ABC):

    @abstractmethod
    def run():
        '''Test run method'''

    def __init__(self, args: argparse.Namespace) -> None:
        self._args = args
        self._cluster = get_cluster_test_config(self._args.cluster,
                                                self._args.zone_id)
        self._get_output_ids_files(f'{self._args.test_type}_acceptance')
        self._iodepth = self._args.instance_cores * 4

    @property
    def _timestamp(self) -> int:
        return calendar.timegm(datetime.utcnow().utctimetuple())

    def _initialize_run(self,
                        profiler: common.Profiler,
                        logger: logging.Logger,
                        instance_name: str,
                        suffix: str) -> None:
        self._profiler = profiler
        self._logger = logger
        self._ycp = create_ycp(self._cluster.name,
                               self._args.zone_id,
                               self._args.ipc_type,
                               self._logger)
        self._entity_suffix = suffix

        # Create instance
        self._instance_policy = YcpNewInstancePolicy(
            cluster_name=self._args.cluster,
            zone_id=self._args.zone_id,
            ipc_type=self._args.ipc_type,
            logger=self._logger,
            name=instance_name,
            core_count=self._args.instance_cores,
            ram_count=self._args.instance_ram,
            compute_node=self._args.compute_node,
            placement_group=self._args.placement_group_name,
            platform_ids=self._args.instance_platform_ids,
            auto_delete=not self._args.debug)

    def _perform_acceptance_test_on_single_disk(self,
                                                disk: Ycp.Disk) -> List[str]:
        # Execute acceptance test on test disk
        self._logger.info(f'Executing acceptance test on disk <id={disk.id}>')
        acceptance_test_cmd = [
            f'{self._args.acceptance_test}', '--profile',
            f'{self._args.cluster}', '--folder-id',
            f'{self._ycp._folder_desc.folder_id}', '--zone-id',
            f'{self._args.zone_id}', '--src-disk-ids', f'{disk.id}',
            '--output-disk-ids', f'{self._output_disk_ids_file}',
            '--suffix', f'{self._entity_suffix}']

        if self._args.conserve_snapshots:
            acceptance_test_cmd.extend([
                '--output-snapshot-ids',
                self._output_snapshot_ids_file])

        if self._args.verbose:
            acceptance_test_cmd.append('--verbose')

        with subprocess.Popen(acceptance_test_cmd,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE) as proc:
            for line in iter(proc.stdout.readline, ''):
                if proc.poll() is not None:
                    break
                self._logger.info(line.rstrip().decode('utf-8'))
            if proc.returncode != 0:
                stderr_str = b''.join(proc.stderr.readlines()).decode('utf-8')
                raise Error(f'failed to execute {acceptance_test_cmd}:'
                            f' {stderr_str}')

        # Get all disk ids from instance
        self._logger.info(f'Reading file with disk ids'
                          f' <path={self._output_disk_ids_file}>')
        disk_ids = self._get_all_output_ids(self._output_disk_ids_file)
        self._logger.info(f'Generated disks: {disk_ids}')

        if self._args.conserve_snapshots:
            # Get all snapshot ids from instance
            self._logger.info(f'Reading file with snapshot ids'
                              f' <path={self._output_snapshot_ids_file}>')
            snapshot_ids = self._get_all_output_ids(
                self._output_snapshot_ids_file)
            self._logger.info(f'Generated snapshots: {snapshot_ids}')

        return disk_ids

    def _execute_ssh_cmd(self,
                         cmd: str,
                         ip: str) -> None:
        check_ssh_connection(ip, self._profiler)
        with common.ssh_client(ip) as ssh:
            _, stdout, stderr = ssh.exec_command(cmd)
            for line in iter(lambda: stdout.readline(2048), ''):
                self._logger.info(line.rstrip())
            if stderr.channel.recv_exit_status():
                stderr_str = b''.join(stderr.readlines()).decode('utf-8')
                raise Error(f'failed to execute command {cmd} on remote host'
                            f' {ip}: {stderr_str}')

    def _perform_verification_write(self,
                                    cmd: str,
                                    disk: Ycp.Disk,
                                    instance: Ycp.Instance) -> None:
        self._logger.info(f'Filling disk <id={disk.id}> with'
                          f' verification data on instance'
                          f' <id={instance.id}>')
        self._execute_ssh_cmd(cmd, instance.ip)

    def _perform_verification_read(self,
                                   cmd: str,
                                   disk: Ycp.Disk,
                                   instance: Ycp.Instance) -> None:
        self._logger.info(f'Verifying data on disk'
                          f' <id={disk.id}> on'
                          f' instance <id={instance.id}>')
        self._execute_ssh_cmd(cmd, instance.ip)

    def _get_output_ids_files(self, sub_folder: str) -> None:
        cached_ids_folder = get_unique_path_for_current_test(
            output_path='/tmp',
            sub_folder=sub_folder
        )
        ensure_path_exists(cached_ids_folder)
        self._output_disk_ids_file = cached_ids_folder + '/disks.txt'
        self._output_snapshot_ids_file = cached_ids_folder + '/snapshots.txt'

    def _get_all_output_ids(self, path_to_ids_file: str) -> List[str]:
        ids = []
        with open(path_to_ids_file) as ids_file:
            ids = [line.rstrip() for line in ids_file.readlines()]
        return ids

    def _delete_output_disks(self, disk_ids: List[str]) -> None:
        error_str = ''
        for disk_id in disk_ids:
            try:
                self._ycp.delete_disk(self._ycp.get_disk(disk_id))
            except YcpWrapper.Error as e:
                self._logger.error(f'failed to delete disk <id={disk_id}>')
                error_str += f'\n{e}'
                continue
        if len(error_str) > 0:
            raise Error('failed to delete disks: {error_str}')

    def _log_nbs_version(self, instance: Ycp.Instance) -> None:
        nbs_version = SolomonClient(self._logger).get_current_nbs_version(
            self._cluster.get_solomon_cluster(instance.compute_node),
            instance.compute_node)
        self._logger.info(f'Running test suite'
                          f' <cluster={self._cluster.name}>; NBS server'
                          f' <version={nbs_version}>')

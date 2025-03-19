from .base_acceptance_test_runner import BaseAcceptanceTestRunner

from .lib import (
    check_ssh_connection,
    size_prettifier,
    Error,
    YcpFindDiskPolicy,
    YcpNewDiskPolicy
)

from cloud.blockstore.pylibs import common

import logging
import math
import paramiko
import re


class EternalAcceptanceTestRunner(BaseAcceptanceTestRunner):

    @property
    def _main_block_device(self) -> str:
        return '/dev/vdb'

    @property
    def _secondary_block_device(self) -> str:
        return '/dev/vdc'

    def run(self, profiler: common.Profiler, logger: logging.Logger) -> None:
        self._initialize_run(
            profiler,
            logger,
            f'acceptance-test-{self._args.test_type}-{self._timestamp}',
            'eternal',
        )

        with self._instance_policy.obtain() as instance:
            self._log_nbs_version(instance)

            disk_name_prefix = (
                f'acceptance-test-{self._args.test_type}-'
                f'{size_prettifier(self._args.disk_size * (1024 ** 3))}'
                f'-{size_prettifier(self._args.disk_blocksize)}').lower()
            disk_name_regex = disk_name_prefix+'-[0-9]+'

            self._logger.info(f'Try to find disk'
                              f' <name_regex={disk_name_regex}>')

            # Try to find disk with test name regex
            # Or create it if nothing was found
            disk_policy = YcpFindDiskPolicy(
                cluster_name=self._args.cluster,
                zone_id=self._args.zone_id,
                ipc_type=self._args.ipc_type,
                logger=self._logger,
                name_regex=disk_name_regex)

            try:
                disk = disk_policy.obtain()
                self._logger.info(f'Disk <id={disk.id}, name={disk.name}>'
                                  f' was found')
                for instance_id in disk.instance_ids:
                    attached_instance = self._ycp.get_instance(instance_id)
                    self._logger.info(f'Disk <id={disk.id}> is attached to'
                                      f' instance <id={attached_instance.id}>.'
                                      f' Try to detach.')
                    self._ycp.detach_disk(attached_instance, disk)
                disk.instance_ids.clear()
            except Error as e:
                if re.match('^Failed to find disk', f'{e}'):
                    self._logger.info(f'Failed to find disk'
                                      f' <name_regex={disk_name_regex}>')
                    disk_policy = YcpNewDiskPolicy(
                        cluster_name=self._args.cluster,
                        zone_id=self._args.zone_id,
                        ipc_type=self._args.ipc_type,
                        logger=self._logger,
                        name=disk_name_prefix+f'-{self._timestamp}',
                        size=self._args.disk_size,
                        blocksize=self._args.disk_blocksize,
                        type_id=self._args.disk_type,
                        auto_delete=False)
                    with disk_policy.obtain() as ycp_disk:
                        disk = ycp_disk

            self._logger.info(
                f'Waiting until disk <id={disk.id}> will be attached'
                f' to instance <id={instance.id}> and secondary disk'
                f' appears as block device <name={self._main_block_device}>')

            with self._instance_policy.attach_disk(
                disk,
                self._main_block_device
            ):
                runtime = math.ceil(
                    (math.sqrt(0.476+1.24*self._args.disk_size)-0.69)/1.24)*60
                self._logger.info(f'Approximate time <sec={runtime}>')

                percentage = min(
                    100,
                    max(0, self._args.disk_write_size_percentage))
                self._logger.info(f'Disk filling <percent={percentage}>')

                self._perform_verification_write(
                    f'fio --name=fill-disk --filename={self._main_block_device}'
                    f' --rw=write --bsrange=1-64M --bs_unaligned'
                    f' --iodepth={self._iodepth} --ioengine=libaio'
                    f' --size={percentage}% --runtime={runtime}'
                    f' --sync=1',
                    disk,
                    instance)

            # Perform acceptance test on current disk (detached after fio)
            disk_ids = self._perform_acceptance_test_on_single_disk(disk)

            self._logger.info(
                f'Waiting until disk <id={disk.id}> will be attached'
                f' to instance <id={instance.id}> and secondary disk'
                f' appears as block device <name={self._main_block_device}>')

            with self._instance_policy.attach_disk(
                disk,
                self._main_block_device
            ):
                for disk_id in disk_ids:
                    output_disk = self._ycp.get_disk(disk_id)

                    # Attach output disk
                    self._logger.info(
                        f'Waiting until disk <id={output_disk.id}> will be'
                        f' attached to instance <id={instance.id}> and'
                        f' secondary disk appears as block device'
                        f' <name={self._secondary_block_device}>')

                    with self._instance_policy.attach_disk(
                        output_disk,
                        self._secondary_block_device
                    ):
                        byte_count = int((self._args.disk_size * (1024 ** 3))
                                          / self._iodepth)

                        clients = []
                        stdouts = []
                        stderrs = []
                        cmds = []
                        for i in range(self._iodepth):
                            offset = int(i * byte_count)
                            check_ssh_connection(instance.ip, self._profiler)
                            ssh = paramiko.SSHClient()
                            common.configure_ssh_client(ssh, instance.ip)
                            cmd = (f'cmp --verbose --bytes={byte_count}'
                                   f' --ignore-initial={offset}'
                                   f' {self._main_block_device}'
                                   f' {self._secondary_block_device}')
                            _, stdout, stderr = ssh.exec_command(cmd)
                            clients.append(ssh)
                            stdouts.append(stdout)
                            stderrs.append(stderr)
                            cmds.append(cmd)
                            self._logger.info(f'Verifying data'
                                              f' <index={i}, offset={offset},'
                                              f' bytes={byte_count}> on disk'
                                              f' <id={output_disk.id}> on'
                                              f' instance <id={instance.id}>')

                        error_message = ""
                        for i in range(self._iodepth):
                            stdout_str = stdouts[i].read().decode('utf-8')
                            stderr_str = stderrs[i].read().decode('utf-8')
                            self._logger.info(f'Verifying finished <index={i},'
                                              f' command="{cmds[i]}", stdout='
                                              f'{stdout_str}, stderr='
                                              f'{stderr_str}>')
                            if stderrs[i].channel.recv_exit_status():
                                error_message += (
                                    f'Error <command="{cmds[i]}", stderr='
                                    f'{stderr_str}>\n')
                            clients[i].close()

                        if len(error_message) != 0:
                            raise Error(error_message)

                # Delete all output disks
                self._delete_output_disks(disk_ids)

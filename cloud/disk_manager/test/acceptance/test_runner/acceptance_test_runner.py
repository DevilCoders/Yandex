from .base_acceptance_test_runner import BaseAcceptanceTestRunner

from .lib import (
    check_ssh_connection,
    generate_test_cases,
    Error,
    YcpNewDiskPolicy
)

from cloud.blockstore.pylibs import common

import logging
import paramiko
import socket


class AcceptanceTestRunner(BaseAcceptanceTestRunner):

    @property
    def _remote_verify_test_path(self) -> str:
        return '/usr/bin/verify-test'

    def run(self, profiler: common.Profiler, logger: logging.Logger) -> None:
        self._initialize_run(
            profiler,
            logger,
            f'acceptance-test-{self._args.test_type}-{self._args.test_suite}-'
            f'{self._timestamp}',
            'acceptance',
        )

        with self._instance_policy.obtain() as instance:
            self._log_nbs_version(instance)

            # Copy verify-test binary to instance
            self._logger.info(f'Copying <path={self._args.verify_test}> to'
                              f' <path={instance.ip}:'
                              f'{self._remote_verify_test_path}>')
            check_ssh_connection(instance.ip, self._profiler)

            try:
                with common.sftp_client(instance.ip) as sftp:
                    sftp.put(self._args.verify_test,
                             self._remote_verify_test_path)
                    sftp.chmod(self._remote_verify_test_path, 0o755)
            except (paramiko.SSHException, socket.error) as e:
                raise Error(f'Failed to copy file {self._args.verify_test}'
                            f' from local to remote host {instance.ip}:'
                            f'{self._remote_verify_test_path} via sftp: {e}')

            self._logger.info(f'<path={self._args.verify_test}> copied to'
                              f' <path={instance.ip}:'
                              f'{self._remote_verify_test_path}>')

            # Generate test cases from test_suite name
            test_cases = generate_test_cases(self._args.test_suite,
                                             self._cluster.name)
            self._logger.info(f'Generated <len={len(test_cases)}> test cases'
                              f' for test suite <name={self._args.test_suite}>')

            for test_case in test_cases:
                self._logger.info(f'Executing test case'
                                  f' <name={test_case.name % self._iodepth}>')

                # Create disk
                disk_policy = YcpNewDiskPolicy(
                    cluster_name=self._args.cluster,
                    zone_id=self._args.zone_id,
                    ipc_type=self._args.ipc_type,
                    logger=self._logger,
                    name=f'acceptance-test-{self._args.test_type}-'
                         f'{self._args.test_suite}-{self._timestamp}',
                    size=test_case.disk_size,
                    blocksize=test_case.disk_blocksize,
                    type_id=test_case.disk_type,
                    auto_delete=not self._args.debug)

                with disk_policy.obtain() as disk:
                    self._logger.info(
                        f'Waiting until disk <id={disk.id}> will be attached'
                        f' to instance <id={instance.id}> and secondary disk'
                        f' appears as block device'
                        f' <name={test_case.block_device}>')

                    with self._instance_policy.attach_disk(
                        disk,
                        test_case.block_device
                    ):
                        self._perform_verification_write(
                            test_case.verify_write_cmd % (
                                self._remote_verify_test_path,
                                self._iodepth),
                            disk,
                            instance)

                    # Perform acceptance test on current disk
                    disk_ids = self._perform_acceptance_test_on_single_disk(
                        disk)

                    for disk_id in disk_ids:
                        output_disk = self._ycp.get_disk(disk_id)

                        # Attach output disk
                        self._logger.info(
                            f'Waiting until disk <id={output_disk.id}> will be'
                            f' attached to instance <id={instance.id}> and'
                            f' secondary disk appears as block device'
                            f' <name={test_case.block_device}>')

                        with self._instance_policy.attach_disk(
                            output_disk,
                            test_case.block_device
                        ):
                            self._perform_verification_read(
                                test_case.verify_read_cmd % (
                                    self._remote_verify_test_path,
                                    self._iodepth),
                                output_disk,
                                instance)

                    # Delete all output disks
                    self._delete_output_disks(disk_ids)

from string import Template
import sys
from typing import Callable

from cloud.blockstore.pylibs import common
from cloud.blockstore.pylibs.ycp import Ycp, YcpWrapper, make_ycp_engine

from .lib import (
    get_test_config,
    Error,
    ParseHelper
)

from library.python import resource


def get_template(name: str) -> Template:
    return Template(resource.find(name).decode('utf8'))


class EternalTestHelper:
    _VM_NAME = '%s-test-vm'
    _DISK_NAME = '%s-test-disk'
    _FS_NAME = '%s-test-fs'

    _REMOTE_PATH = '/usr/bin/eternal-load'
    _LOCAL_PATH = '../eternal-load/bin/eternal-load'

    _LOG_PATH = '/tmp/eternal-load.log'
    _CONFIG_PATH = '/tmp/load-config.json'

    _START_LOAD_CMD = (f'/usr/bin/eternal-load --config-type generated --blocksize %d '
                       f'--file %s --filesize %d '
                       f'--iodepth %d --dump-config-path {_CONFIG_PATH} '
                       f'--write-rate %d '
                       f'--write-parts %d '
                       f'>> {_LOG_PATH} 2>&1')

    _START_LOAD_WITH_CONFIG_CMD = (f'/usr/bin/eternal-load --config-type file --restore-config-path {_CONFIG_PATH} '
                                   f'--file %s --dump-config-path {_CONFIG_PATH}  >> {_LOG_PATH} 2>&1')

    _FIO_CMD_TO_FILL_DISK = ('fio --name fill-secondary-disk --filename %s --rw write --bs 4M --iodepth 128'
                             f' --direct 1 --sync 1 --ioengine libaio --size 100%%  >> {_LOG_PATH} 2>&1')

    _DB_TEST_INIT_SCRIPT = '%s.sh'
    _DB_TEST_INIT_SCRIPT_PATH = '/usr/bin'
    _DB_REPORT_PATH = '/tmp/report.txt'

    _PGBENCH_INIT_CMD = (f'pgbench -U postgres -p 5432  pgbench -i '
                         f'--foreign-keys --no-vacuum -s 20000 >> {_DB_REPORT_PATH} 2>&1')
    _PGBENCH_TEST_CMD = (f'pgbench -U postgres -p 5432 pgbench -P 30 -T 999999999 '
                         f'-j 32 -c 32 >> {_DB_REPORT_PATH} 2>&1')

    _SYSBENCH_SCRIPT_PATH = '/usr/share/sysbench'
    _SYSBENCH_INIT_CMD = (f'sysbench --db-driver=mysql --mysql-user=root --mysql-password='' --mysql-db=sbtest'
                          f' --tables=10 --table-size=300000000 '
                          f'/usr/share/sysbench/oltp-custom.lua prepare --threads=32 >> {_DB_REPORT_PATH} 2>&1')
    _SYSBENCH_TEST_CMD = (f'sysbench --db-driver=mysql --mysql-user=root '
                          f'--mysql-db=sbtest --tables=10 --table-size=300000000 '
                          f'/usr/share/sysbench/oltp-custom.lua run '
                          f'--threads=32 --time=999999999 --report-interval=30 >> {_DB_REPORT_PATH} 2>&1')

    class Command:
        execute: Callable[[], None]
        parse_arguments: Callable[[ParseHelper], None]

        def __init__(self, execute, parse):
            self.execute = execute
            self.parse_arguments = parse

    def __init__(self, parser: ParseHelper, logger):
        self.parser = parser
        self.logger = logger

        self.FUNCTION_MAP[self.parser.get_args().command].parse_arguments(parser)
        self.args = self.parser.get_args()

        if self.args.test_case == 'all':
            self.all_test_configs = get_test_config(self.args, 'db' in self.args.command)
        else:
            self.test_config = get_test_config(self.args, 'db' in self.args.command)

            if self.test_config is None:
                raise Error('Unknown test case')

            self.ycp = YcpWrapper(
                self.args.cluster,
                self.test_config.ycp_config.folder,
                logger,
                make_ycp_engine(self.args.dry_run))

        self.helpers = common.make_helpers(self.args.dry_run)

    def find_instance(self) -> Ycp.Instance:
        instances = self.ycp.list_instances()
        if self.args.dry_run:
            return instances[0]
        for instance in instances:
            if instance.name == self._VM_NAME % self.args.test_case:
                return instance

    def copy_config(self, instance_ip: str):
        json = get_template('test-config.json').substitute(
            ioDepth=self.test_config.load_config.io_depth,
            fileSize=(self.test_config.disk_config or self.test_config.fs_config).size * 1024 ** 3,
            writeRate=self.test_config.load_config.write_rate,
            blockSize=(self.test_config.disk_config or self.test_config.fs_config).bs,
            filePath=self.test_config.test_file
        )
        with common.make_sftp_client(self.args.dry_run, instance_ip) as sftp:
            file = sftp.file(self._CONFIG_PATH, 'w')
            file.write(json)
            file.flush()

    def generate_run_load_command(self, fill_disk: bool = False) -> str:
        load_command = ''
        if fill_disk:
            load_command = self._FIO_CMD_TO_FILL_DISK % self.test_config.test_file + ' && '
        if self.test_config.load_config.use_requests_with_different_sizes:
            load_command += self._START_LOAD_WITH_CONFIG_CMD
        else:
            load_command += self._START_LOAD_CMD % (
                self.test_config.load_config.bs,
                self.test_config.test_file,
                (self.test_config.disk_config or self.test_config.fs_config).size,
                self.test_config.load_config.io_depth,
                self.test_config.load_config.write_rate,
                self.test_config.load_config.write_parts)
        return load_command

    def run_command_in_background(self, instance_ip: str, command: str):
        self.logger.info(f'Running command on instance:\n{command}')

        channel = common.make_channel(self.args.dry_run, instance_ip, 'root')
        channel.exec_command(command)

    @common.retry(5, 5, Error)
    def _wait_until_killing(self, instance_ip: str, command: str):
        with common.make_ssh_client(self.args.dry_run, instance_ip) as ssh:
            _, stdout, _ = ssh.exec_command(f'pgrep {command}')
            stdout.channel.recv_exit_status()

            out = "".join(stdout.readlines())
            if out != "":
                raise Error(f'{command} is still running')

    def kill_load_on_instance(self, instance_ip: str, command: str):
        self.logger.info(f'Killing {command} on instance')
        with common.make_ssh_client(self.args.dry_run, instance_ip) as ssh:
            _, stdout, stderr = ssh.exec_command(f'pkill {command}')
            stdout.channel.recv_exit_status()
            self._wait_until_killing(instance_ip, command)

    def copy_load_to_instance(self, instance_ip: str):
        self.logger.info('Copying write-load to instance')
        with common.make_sftp_client(self.args.dry_run, instance_ip) as sftp:
            sftp.put(self._LOCAL_PATH, self._REMOTE_PATH)
            sftp.chmod(self._REMOTE_PATH, 0o755)

    def copy_test_script_to_instance(self, instance_ip, name: str, path: str):
        self.logger.info(f'Copying test_script to instance with <ip={instance_ip}>')
        with common.make_sftp_client(self.args.dry_run, instance_ip) as sftp:
            file = sftp.file(f'{path}/{name}', 'w')
            file.write(resource.find(name).decode('utf8'))
            file.flush()

    def create_disk(self, instance: Ycp.Instance):
        try:
            with self.ycp.create_disk(
                    name=self._DISK_NAME % self.args.test_case,
                    bs=self.test_config.disk_config.bs,
                    size=self.test_config.disk_config.size,
                    type_id=self.test_config.disk_config.type,
                    placement_group_name=self.test_config.disk_config.placement_group_name,
                    image_name=self.test_config.disk_config.image_name,
                    auto_delete=False) as disk:
                return disk
        except YcpWrapper.Error as e:
            self.logger.info(f'Error occurs while creating disk {e}')
            self.ycp.delete_instance(instance)
            raise Error('Cannot create disk')

    def create_fs(self, instance: Ycp.Instance):
        try:
            with self.ycp.create_fs(
                    name=self._FS_NAME % self.args.test_case,
                    bs=self.test_config.fs_config.bs,
                    size=self.test_config.fs_config.size,
                    type_id=self.test_config.fs_config.type,
                    auto_delete=False) as fs:
                return fs
        except YcpWrapper.Error as e:
            self.logger.info(f'Error occurs while creating fs {e}')
            self.ycp.delete_instance(instance)
            raise Error('Cannot create fs')

    def _mount_fs(self, instance: Ycp.Instance):
        self.logger.info('Mounting fs')
        with common.make_ssh_client(self.args.dry_run, instance.ip) as ssh, \
             common.make_sftp_client(self.args.dry_run, instance.ip) as sftp:
            sftp.mkdir(self.test_config.fs_config.mount_path)
            _, _, stderr = ssh.exec_command(
                f'mount -t virtiofs {self.test_config.fs_config.device_name} '
                f'{self.test_config.fs_config.mount_path} && '
                f'touch {self.test_config.test_file}')

            exit_code = stderr.channel.recv_exit_status()
            if exit_code != 0:
                self.logger.error(f'Failed to mount fs\n'
                                  f'{"".join(stderr.readlines())}')
                raise Error(f'failed to mount fs with exit code {exit_code}')

    def create_and_configure_vm(self) -> Ycp.Instance:
        with self.ycp.create_instance(
                name=self._VM_NAME % self.args.test_case,
                cores=8,
                memory=8,
                image_name=self.test_config.ycp_config.image_name,
                compute_node=self.args.compute_node,
                placement_group_name=self.test_config.ycp_config.placement_group_name,
                host_group=self.args.host_group,
                auto_delete=False) as instance:
            self.logger.info(f'Waiting until instance ip=<{instance.ip}> becomes available via ssh')
            self.helpers.wait_until_instance_becomes_available_via_ssh(instance.ip)

            if self.test_config.disk_config is not None:
                disk = self.create_disk(instance)
                try:
                    with self.ycp.attach_disk(instance, disk, auto_detach=False):
                        pass
                except YcpWrapper.Error as e:
                    self.logger.info(f'Error occurs while attaching disk {e}')
                    self.ycp.delete_instance(instance)
                    self.ycp.delete_disk(disk)
                    raise Error('Cannot attach disk')
            else:
                fs = self.create_fs(instance)
                try:
                    with self.ycp.attach_fs(
                            instance,
                            fs,
                            device_name=self.test_config.fs_config.device_name,
                            auto_detach=False):
                        self._mount_fs(instance)
                except YcpWrapper.Error as e:
                    self.logger.info(f'Error occurs while attaching fs {e}')
                    self.ycp.delete_instance(instance)
                    self.ycp.delete_fs(fs)
                    raise Error('Cannot attach fs')

            return instance

    def handle_new_test_run(self):
        self.logger.info('Starting eternal test')

        instance = self.create_and_configure_vm()

        self.copy_load_to_instance(instance.ip)
        if self.test_config.load_config.use_requests_with_different_sizes:
            self.copy_config(instance.ip)

        self.run_command_in_background(
            instance.ip,
            self.generate_run_load_command(self.test_config.load_config.need_filling))

    def handle_continue_load(self):
        self.logger.info('Continuing load')

        instance = self.find_instance()
        self.copy_load_to_instance(instance.ip)
        self.run_command_in_background(instance.ip, self._START_LOAD_WITH_CONFIG_CMD)

    def rerun_load_on_instance(self, instance, need_kill: bool):
        self.logger.info(f'Rerunning load for test case {self.args.test_case}')

        if need_kill:
            self.kill_load_on_instance(instance.ip, 'eternal-load')

        if self.args.scp_binary:
            self.copy_load_to_instance(instance.ip)

        if self.test_config.load_config.use_requests_with_different_sizes:
            self.copy_config(instance.ip)

        self.run_command_in_background(instance.ip, self.generate_run_load_command(self.args.refill))

    def handle_rerun_load(self):
        if self.args.test_case == 'all':
            for test_case, config in self.all_test_configs:
                self.args.test_case = test_case
                self.test_config = config
                self.ycp = YcpWrapper(
                    self.args.cluster,
                    self.test_config.ycp_config.folder,
                    self.logger,
                    make_ycp_engine(self.args.dry_run))

                instance = self.find_instance()
                if instance is None:
                    self.logger.info(f'No instance for test case <{test_case}> on cluster <{self.args.cluster}>')
                    continue

                self.logger.info(f'Found instance id=<{instance.id}> for test case <{test_case}>')
                if not self.check_load(instance):
                    self.logger.info(f'Rerunning load for test case <{test_case}> on cluster <{self.args.cluster}>')
                    self.rerun_load_on_instance(instance, False)
                else:
                    self.logger.info('Eternal-load is already running')
        else:
            self.rerun_load_on_instance(self.find_instance(), True)

    def handle_stop_load(self):
        self.logger.info('Stopping load')
        instance = self.find_instance()
        self.kill_load_on_instance(instance.ip, 'eternal-load')

    def handle_delete_test_vm(self):
        self.logger.info('Deleting test')
        instance = self.find_instance()
        self.ycp.delete_instance(instance)

    def handle_add_auto_run(self):
        self.logger.info('Add auto run')
        instance = self.find_instance()

        crontab_cmd = self.generate_run_load_command()
        with common.make_ssh_client(self.args.dry_run, instance.ip) as ssh:
            _, _, stderr = ssh.exec_command(
                f'(crontab -l 2>/dev/null; echo "@reboot {crontab_cmd}") | crontab -')
            if stderr.channel.recv_exit_status():
                raise Error(f'Cannot add crontab job: {"".join(stderr.readlines())}')

    def rerun_db_load_script_on_instance(self, instance: Ycp.Instance):
        self.logger.info(f'Rerun load script for test case <{self.args.test_case}>')

        if self.test_config.db == 'mysql':
            self.kill_load_on_instance(instance.ip, 'sysbench')
            self.run_command_in_background(
                instance.ip,
                self._SYSBENCH_TEST_CMD)
        elif self.test_config.db == 'postgresql':
            self.kill_load_on_instance(instance.ip, 'pgbench')
            self.run_command_in_background(
                instance.ip,
                self._PGBENCH_TEST_CMD)

    def handle_rerun_db_load(self):
        if self.args.test_case == 'all':
            for test_case, config in self.all_test_configs:
                self.args.test_case = test_case
                self.test_config = config
                self.ycp = YcpWrapper(
                    self.args.cluster,
                    self.test_config.ycp_config.folder,
                    self.logger,
                    make_ycp_engine(self.args.dry_run))

                instance = self.find_instance()
                if instance is None:
                    self.logger.info(f'No instance for test case <{test_case}> on cluster <{self.args.cluster}>')
                    continue

                self.rerun_db_load_script_on_instance(instance)
        else:
            self.rerun_db_load_script_on_instance(self.find_instance())

    def handle_setup_new_db_test(self):
        self.logger.info('Starting new test with DB')

        instance = self.create_and_configure_vm()
        self.logger.info('Prepare database, executing script')
        with common.make_sftp_client(self.args.dry_run, instance.ip) as sftp:
            script_name = self._DB_TEST_INIT_SCRIPT % self.test_config.db
            file = sftp.file(f'{self._DB_TEST_INIT_SCRIPT_PATH}/{script_name}', 'w')
            file.write(resource.find(script_name).decode('utf8'))
            file.flush()
            sftp.chmod(f'{self._DB_TEST_INIT_SCRIPT_PATH}/{script_name}', 0o755)

        with common.make_ssh_client(self.args.dry_run, instance.ip) as ssh:
            _, stdout, stderr = ssh.exec_command(f'{self._DB_TEST_INIT_SCRIPT_PATH}/{script_name}')
            exit_code = stdout.channel.recv_exit_status()
            if exit_code != 0:
                self.logger.error(f'Failed to prepare db test:\n'
                                  f'stderr: {"".join(stderr.readlines())}\n'
                                  f'stdout: {"".join(stdout.readlines())}')
                raise Error('Failed to run command')

        if self.test_config.db == 'mysql':
            self.copy_test_script_to_instance(instance.ip, 'oltp-custom.lua', self._SYSBENCH_SCRIPT_PATH)
            self.run_command_in_background(
                instance.ip,
                self._SYSBENCH_INIT_CMD)
        else:
            self.run_command_in_background(
                instance.ip,
                self._PGBENCH_INIT_CMD)

    def check_load(self, instance: Ycp.Instance) -> bool:
        self.logger.info(f'Check if eternal load running on instance id=<{instance.id}>')
        with common.make_ssh_client(self.args.dry_run, instance.ip) as ssh:
            _, stdout, _ = ssh.exec_command('pgrep eternal-load')
            stdout.channel.exit_status_ready()
            out = ''.join(stdout.readlines())
            if not out:
                return False
        return True

    FUNCTION_MAP = {
        'setup-test': Command(handle_new_test_run, ParseHelper.parse_run_test_options),
        'stop-load': Command(handle_stop_load, lambda *args: None),
        'rerun-load': Command(handle_rerun_load, ParseHelper.parse_load_options),
        'continue-load': Command(handle_continue_load, lambda *args: None),
        'delete-test': Command(handle_delete_test_vm, lambda *args: None),
        'add-auto-run': Command(handle_add_auto_run, ParseHelper.parse_load_options),

        'rerun-db-load': Command(handle_rerun_db_load, lambda *args: None),
        'setup-db-test': Command(handle_setup_new_db_test, ParseHelper.parse_run_test_options)
    }

    def run(self):
        self.FUNCTION_MAP[self.args.command].execute(self)


def main():
    parser = ParseHelper(EternalTestHelper.FUNCTION_MAP.keys())
    parser.parse_command()
    logger = common.create_logger('yc-nbs-eternal-test-runner', parser.get_args())
    try:
        EternalTestHelper(parser, logger).run()
        logger.info(f'Successfully execute command: {parser.get_args().command}')
    except Error as e:
        logger.fatal(f'Failed to run vm or/and create disk: {e}')
        sys.exit(1)


if __name__ == '__main__':
    main()

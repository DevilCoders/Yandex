from cloud.blockstore.sandbox.utils import constants

from sandbox import common
from sandbox import sdk2
from sandbox.common.errors import TaskFailure
from sandbox.projects.common.teamcity import TeamcityArtifactsContext
from sandbox.sdk2.helpers import subprocess

import ast
import os
import logging
import shutil

LOGGER = logging.getLogger('nbs_teamcity_test')


class Task():
    def __init__(self, task):
        self._task = task

    def _parse_commands(self):
        LOGGER.debug('_parse_commands')

        cmds = ast.literal_eval(self._task.Parameters.cmd)
        return cmds

    def _copy_resource_data_to_working_dir(self):
        LOGGER.debug('_copy_resource_data_to_working_dir')

        binaries_path = sdk2.ResourceData(
            self._task.Parameters.test_binaries
        ).path
        for item in os.listdir(str(binaries_path)):
            src = binaries_path / item
            dst = self._task.path(item)
            if os.path.isdir(str(src)):
                shutil.copytree(str(src), str(dst))
            else:
                shutil.copy2(str(src), str(dst))

    def _copy_ycp_settings(self):
        LOGGER.debug('_copy_ycp_settings')

        if os.path.isfile(self._task.Parameters.ycp_config_path):
            LOGGER.debug('copy ycp config')
            path = os.path.expanduser('~/.config/ycp')
            if not os.path.isdir(path):
                os.makedirs(path)
            dst_file = os.path.join(path, 'config.yaml')
            if os.path.isfile(dst_file):
                os.remove(dst_file)
            shutil.copy(self._task.Parameters.ycp_config_path, dst_file)

    def _copy_ssh_settings(self):
        LOGGER.debug('_copy_ssh_settings')

        if os.path.isfile(self._task.Parameters.ssh_config_path):
            LOGGER.debug('copy ssh config')
            path = os.path.expanduser('~/.ssh')
            if not os.path.isdir(path):
                os.makedirs(path)
            dst_file = os.path.join(path, 'config')
            if os.path.isfile(dst_file):
                os.remove(dst_file)
            shutil.copy(self._task.Parameters.ssh_config_path, dst_file)

    def _setup_environment(self):
        LOGGER.debug('_setup_environment')

        os.environ['YT_OAUTH_TOKEN'] = str(
            self._task.Parameters.yt_oauth_token.data()[
                self._task.Parameters.yt_oauth_token.default_key
            ]
        )

    def _check_ready(self):
        LOGGER.debug('_check_ready')

        if os.path.isfile('/etc/hosts'):
            LOGGER.debug('Reading /etc/hosts')
            with open('/etc/hosts', 'r') as f:
                LOGGER.debug('{}'.format(f.read()))

        for item in os.listdir(str(self._task.path())):
            LOGGER.debug('Found %s in working dir', item)
        for item in os.listdir(os.path.expanduser('~')):
            LOGGER.debug('Found %s in home dir', item)
        for item in os.listdir('/usr/bin'):
            LOGGER.debug('Found %s in /usr/bin', item)

    def _get_acceptable_retry_count(self):
        LOGGER.debug('_get_acceptable_retry_count')

        if self._task.Parameters.retry_policy == 'no_retry':
            return 0
        elif self._task.Parameters.retry_policy == 'line':
            return max(
                0,
                self._task.Parameters.retry_line_count,
            )
        elif self._task.Parameters.retry_policy == 'complete':
            return max(
                0,
                self._task.Parameters.retry_complete_count,
            )
        else:
            raise TaskFailure('Wrong retry policy chosen')

    def _get_acceptable_node_restart_count(self):
        LOGGER.debug('_get_acceptable_node_restart_count')

        if self._task.Parameters.retry_policy == 'no_retry':
            return 0
        elif self._task.Parameters.retry_policy == 'line':
            return max(
                0,
                self._task.Parameters.sandbox_restarts,
            )
        elif self._task.Parameters.retry_policy == 'complete':
            return max(
                0,
                self._task.Parameters.sandbox_restarts,
            )
        else:
            raise TaskFailure('Wrong retry policy chosen')

    def _is_retries_count_exceeded(self):
        LOGGER.debug('_is_retries_count_exceeded')

        if self._task.Context.retries_performed > self._get_acceptable_retry_count():
            return True

        if self._task.Context.sandbox_restarted > self._get_acceptable_node_restart_count():
            return True

        return False

    def _create_retry_failed_message(self):
        LOGGER.debug('_create_retry_failed_message')

        return 'Task failed. Retries performed {} (max count {}). Nodes failed {} (max count {})'.format(
            self._task.Context.retries_performed,
            self._get_acceptable_retry_count(),
            self._task.Context.sandbox_restarted,
            self._get_acceptable_node_restart_count(),
        )

    def _execute_command(self, command, tac):
        LOGGER.debug('_execute_command')

        subprocess.run(
            command,
            cwd=str(self._task.path()),
            shell=True,
            check=True,
            stdout=tac.output,
            stderr=tac.output,
        )

    def _run_commands_without_retry(self, tac):
        LOGGER.debug('_run_commands_without_retry')

        for command_index in range(len(self._task.Context.commands)):
            with self._task.memoize_stage[
                'wait_command_{}_step'.format(command_index)
            ]:
                if command_index != self._task.Context.command_to_start:
                    raise TaskFailure('Current command {} should not be running, because next command should be {}'.format(
                        command_index,
                        self._task.Context.command_to_start,
                    ))

                LOGGER.info('Command to run: {}'.format(
                    self._task.Context.commands[command_index]
                ))
                self._execute_command(
                    self._task.Context.commands[command_index],
                    tac,
                )
                self._task.Context.command_to_start += 1
                self._task.Context.save()

    def _run_commands_with_line_retry(self, tac, total_retries):
        LOGGER.debug('_run_commands_with_line_retry')

        for command_index in range(len(self._task.Context.commands)):
            with self._task.memoize_stage[
                'wait_command_{}_step'.format(command_index)
            ](commit_on_entrance=False):
                if command_index != self._task.Context.command_to_start:
                    raise TaskFailure('Current command {} should not be running, because next command should be {}'.format(
                        command_index,
                        self._task.Context.command_to_start,
                    ))

                LOGGER.info('Command to run: {}'.format(
                    self._task.Context.commands[command_index]
                ))
                while not self._is_retries_count_exceeded():
                    try:
                        self._execute_command(
                            self._task.Context.commands[command_index],
                            tac,
                        )
                        break
                    except:
                        self._task.Context.retries_performed += 1
                        self._task.Context.save()
                        LOGGER.info('Command failed: {}'.format(
                            self._task.Context.commands[command_index]
                        ))
                        LOGGER.info('Retries performed: {}'.format(
                            self._task.Context.retries_performed,
                        ))
                if self._is_retries_count_exceeded():
                    raise TaskFailure(self._create_retry_failed_message())

                self._task.Context.command_to_start += 1
                self._task.Context.save()

    def _run_commands_with_complete_retry(self, tac, total_retries):
        LOGGER.debug('_run_commands_with_complete_retry')

        while not self._is_retries_count_exceeded():
            try:
                self._task.Context.command_to_start = 0
                self._task.Context.save()

                for command_index in range(len(self._task.Context.commands)):
                    if command_index != self._task.Context.command_to_start:
                        raise TaskFailure('Current command {} should not be running, because next command should be {}'.format(
                            command_index,
                            self._task.Context.command_to_start,
                        ))

                    LOGGER.info('Command to run: {}'.format(
                        self._task.Context.commands[command_index]
                    ))
                    self._execute_command(
                        self._task.Context.commands[command_index],
                        tac,
                    )
                    self._task.Context.command_to_start += 1
                    self._task.Context.save()

                break
            except:
                self._task.Context.retries_performed += 1
                self._task.Context.save()
                LOGGER.info('Command failed: {}'.format(
                    self._task.Context.commands[self._task.Context.command_to_start]
                ))
                LOGGER.info('Retries performed: {}'.format(
                    self._task.Context.retries_performed,
                ))

        if self._is_retries_count_exceeded():
            raise TaskFailure(self._create_retry_failed_message())

    def on_prepare(self):
        LOGGER.debug('on_prepare')

        self._setup_environment()
        self._copy_resource_data_to_working_dir()

        if self._task.Parameters.use_ycp:
            self._copy_ycp_settings()
        if self._task.Parameters.use_custom_ssh:
            self._copy_ssh_settings()

    def on_execute(self):
        LOGGER.debug('on_execute')

        with self._task.memoize_stage[
            'setup_command_to_start'
        ](commit_on_entrance=False):
            self._task.Context.commands = self._parse_commands()
            self._task.Context.current_fqdn = ''
            self._task.Context.command_to_start = 0
            self._task.Context.retries_performed = 0
            self._task.Context.sandbox_restarted = 0
            self._task.Context.save()

        fqdn = common.config.Registry().this.fqdn
        if len(self._task.Context.current_fqdn) != 0:
            LOGGER.info(
                'Task was restarted on host %s (previous %s)',
                fqdn,
                self._task.Context.current_fqdn,
            )
            self._task.Context.sandbox_restarted += 1

        self._task.Context.current_fqdn = fqdn
        self._task.Context.save()
        LOGGER.info(
            'Task %s started on host %s',
            self._task.id,
            self._task.Context.current_fqdn,
        )

        if self._is_retries_count_exceeded():
            raise TaskFailure(self._create_retry_failed_message())

        with sdk2.ssh.Key(
            private_part=self._task.Parameters.ssh_key.data()[
                self._task.Parameters.ssh_key.default_key
            ]
        ), TeamcityArtifactsContext(
            self._task.path(),
            tc_service_messages_ttl=constants.DEFAULT_TEAMCITY_SERVICE_MESSAGES_LOG_TTL,
            tc_artifacts_ttl=constants.DEFAULT_TEAMCITY_ARTIFACTS_TTL,
        ) as tac:

            self._check_ready()

            if self._task.Parameters.retry_policy == 'no_retry':
                self._run_commands_without_retry(tac)
            elif self._task.Parameters.retry_policy == 'line':
                self._run_commands_with_line_retry(
                    tac,
                    max(0, self._task.Parameters.retry_line_count),
                )
            elif self._task.Parameters.retry_policy == 'complete':
                self._run_commands_with_complete_retry(
                    tac,
                    max(0, self._task.Parameters.retry_complete_count),
                )
            else:
                raise TaskFailure('Wrong retry policy chosen')

        if len(self._task.Context.commands) != self._task.Context.command_to_start:
            raise TaskFailure('Command {} didn\'t finished properly'.format(
                self._task.Context.command_to_start
            ))

    def on_before_timeout(self, seconds):
        LOGGER.debug('on_before_timeout')

    def on_terminate(self):
        LOGGER.debug('on_terminate')

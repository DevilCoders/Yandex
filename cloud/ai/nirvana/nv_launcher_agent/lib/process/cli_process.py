import os

from subprocess import Popen, PIPE

from cloud.ai.nirvana.nv_launcher_agent.lib.process.base_process import BaseProcess, ProcessStatus
from cloud.ai.nirvana.nv_launcher_agent.lib.helpers import read_file_to_str
from cloud.ai.nirvana.nv_launcher_agent.lib.thread_logger import ThreadLogger


class CliProcess(BaseProcess):
    def __init__(self, name, args, log_dir, cwd=None, stdin=None, pipe_stdout=False, shell=False, docker_name=None):
        super().__init__(name, args, log_dir)
        self.cwd = cwd
        self.stdin = stdin
        self.pipe_stdout = pipe_stdout
        self.shell = shell
        self.docker_name = docker_name

        if self.pipe_stdout:
            self.__stdout_log = PIPE
        else:
            self.stdout_log_file = os.path.join(self.log_dir, f'{name}.stdout.log')
            self.__stdout_log = open(self.stdout_log_file, 'w')

        self.stderr_log_file = os.path.join(self.log_dir, f'{name}.stderr.log')
        self.__stderr_log = open(self.stderr_log_file, 'w')

        self.subprocess = None
        self.was_killed = False

    def start(self):
        if self.stdin is not None:
            self.subprocess = Popen(
                args=self.args,
                cwd=self.cwd,
                stdin=self.stdin,
                stdout=self.__stdout_log,
                stderr=self.__stderr_log,
                shell=self.shell
            )
            return
        self.subprocess = Popen(
            args=self.args,
            cwd=self.cwd,
            stdout=self.__stdout_log,
            stderr=self.__stderr_log,
            shell=self.shell
        )

    def close_files(self):
        if not self.pipe_stdout:
            self.__stdout_log.close()
        self.__stderr_log.close()

    @property
    def stdout(self):
        return read_file_to_str(self.stdout_log_file)

    @property
    def stderr(self):
        return read_file_to_str(self.stderr_log_file)

    @property
    def status(self) -> ProcessStatus:
        assert self.subprocess is not None
        if self.was_killed:
            return ProcessStatus.KILLED

        return_code = self.subprocess.poll()
        if return_code is None:
            return ProcessStatus.RUNNING

        if return_code == 0:
            return ProcessStatus.COMPLETED

        return ProcessStatus.FAILED

    def wait(self):
        self.subprocess.wait()

    def kill(self):
        self.was_killed = True

        if self.docker_name is not None:
            ThreadLogger.info(f'Killing docker {self.docker_name}')
            kill_docker_command = f'docker kill $(docker ps -a -q  --filter ancestor={self.docker_name})'
            process = Popen(kill_docker_command, stdout=PIPE, shell=True)
            output, error = process.communicate()
            ThreadLogger.info(f'Kill docker stdout: {output}')
            ThreadLogger.info(f'Kill docker stderr: {error}')

        self.subprocess.kill()
        self.close_files()

    def exit_code(self):
        if self.subprocess is None:
            raise Exception("Process was not started!")

        return self.subprocess.returncode

    def pid(self):
        if self.shell:
            raise Exception("Cannot get pid of shell process")
        return self.subprocess.pid

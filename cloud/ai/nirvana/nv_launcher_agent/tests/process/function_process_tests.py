# import pytest
import sys
from time import sleep

from cloud.ai.nirvana.nv_launcher_agent.lib.process.base_process import ProcessStatus
from cloud.ai.nirvana.nv_launcher_agent.lib.process.function_process import FunctionProcess


def process_exit_code():
    exit(7)


def process_stdout():
    sys.stdout.write("hello")


def process_stderr():
    sys.stderr.write("hello")


def process_running():
    sleep(1)


def process_args(a, b):
    print(int(a / b))


class TestFunctionProcess:
    def test_function_process_exit_code(self):
        process = FunctionProcess("process_exit_code", target=process_exit_code, log_dir='.')
        process.start()
        process.wait()
        assert process.exit_code() == 7

    def test_function_process_stdout(self):
        process = FunctionProcess("process_stdout", target=process_stdout, log_dir='.')
        process.start()
        process.wait()
        assert process.stdout == ["hello"]
        assert process.exit_code() == 0

    def test_function_process_stderr(self):
        process = FunctionProcess("process_stderr", target=process_stderr, log_dir='.')
        process.start()
        process.wait()
        assert process.stderr == ["hello"]
        assert process.exit_code() == 0

    def test_function_process_running(self):
        process = FunctionProcess("process_running", target=process_running, log_dir='.')
        process.start()
        assert process.status == ProcessStatus.RUNNING
        process.wait()
        assert process.status == ProcessStatus.COMPLETED
        assert process.exit_code() == 0

    def test_function_process_kill(self):
        process = FunctionProcess("process_kill", target=process_running, log_dir='.')
        process.start()
        assert process.status == ProcessStatus.RUNNING
        process.kill()
        assert process.status == ProcessStatus.KILLED

    def test_function_process_args(self):
        process = FunctionProcess("process_args", args=(6, 3), target=process_args, log_dir='.')
        process.start()
        process.wait()
        assert process.stdout == ["2\n"]
        assert process.exit_code() == 0

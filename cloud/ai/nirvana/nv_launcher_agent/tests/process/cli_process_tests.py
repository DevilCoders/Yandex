import pytest

from cloud.ai.nirvana.nv_launcher_agent.lib.process.base_process import ProcessStatus
from cloud.ai.nirvana.nv_launcher_agent.lib.process.cli_process import CliProcess


class TestCliProcess:

    def test_cli_process_stdout(self):
        process = CliProcess(name='process_stdout', args=['echo', 'message'], cwd='.', log_dir='.')
        process.start()
        process.wait()
        assert process.stdout == ["message\n"]
        assert process.exit_code() == 0

    def test_cli_process_kill(self):
        process = CliProcess(name='process_kill', args=['echo', 'message'], cwd='.', log_dir='.')
        process.start()
        process.kill()
        assert process.was_killed
        assert process.status == ProcessStatus.KILLED

    def test_cli_process_start(self):
        process = CliProcess(name='process_start', args=['echo', 'message'], cwd='.', log_dir='.')
        with pytest.raises(Exception):
            process.exit_code()

        process.start()
        process.wait()
        assert process.status == ProcessStatus.COMPLETED

    def test_cli_process_pipe(self):
        process = CliProcess(name='process_pipe', args=['echo', '1+1'], cwd='.', log_dir='.', pipe_stdout=True)

        process.start()
        process.wait()

        with pytest.raises(AttributeError):
            _ = process.stdout

    def test_cli_process_running_completed(self):
        process = CliProcess(name='process_run_completed', args=['sleep', '1'], cwd='.', log_dir='.')
        process.start()
        assert process.status == ProcessStatus.RUNNING
        process.wait()
        assert process.status == ProcessStatus.COMPLETED

    def test_cli_process_run_kill(self):
        process = CliProcess(name='process_run_kill', args=['sleep', '1'], cwd='.', log_dir='.')
        process.start()
        assert process.status == ProcessStatus.RUNNING
        process.kill()
        assert process.status == ProcessStatus.KILLED

from cloud.ai.nirvana.nv_launcher_agent.lib.process.base_process import ProcessStatus
from cloud.ai.nirvana.nv_launcher_agent.lib.process.cli_process import CliProcess
from cloud.ai.nirvana.nv_launcher_agent.lib.helpers import get_job_launcher_stderr_json
from cloud.ai.nirvana.nv_launcher_agent.lib.thread_logger import ThreadLogger


class JobProcess(CliProcess):
    def __init__(self, name, args, log_dir, cwd=None, stdin=None, pipe_stdout=False, shell=False, docker_name=None):
        super().__init__(name, args, log_dir, cwd, stdin, pipe_stdout, shell, docker_name)

    @property
    def status(self) -> ProcessStatus:
        assert self.subprocess is not None
        if self.was_killed:
            return ProcessStatus.KILLED

        return_code = self.subprocess.poll()
        if return_code is None:
            job_launcher_stderr_json = get_job_launcher_stderr_json(0, 'UNKNOWN', self.stderr)
            exit_status_from_json = job_launcher_stderr_json.get('exit_status')

            if exit_status_from_json is not None and exit_status_from_json != 0:
                ThreadLogger.info("Found nonzero exit code in job launcher output. Job will be killed!")
                return ProcessStatus.FAILED

            return ProcessStatus.RUNNING

        if return_code == 0:
            return ProcessStatus.COMPLETED

        return ProcessStatus.FAILED

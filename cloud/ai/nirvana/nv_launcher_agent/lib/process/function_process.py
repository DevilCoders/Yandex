from threading import Thread
import os

from cloud.ai.nirvana.nv_launcher_agent.lib.process.base_process import BaseProcess, ProcessStatus
from cloud.ai.nirvana.nv_launcher_agent.lib.helpers import TargetWrapper, read_file_to_str


class FunctionProcess(BaseProcess):
    def __init__(self, name, log_dir, target, args=(), kwargs=None):
        super().__init__(name, args, log_dir)
        if kwargs is None:
            kwargs = {}
        self.kwargs = kwargs
        self.was_killed = False
        self.subthread: Thread = None
        self.stdout_log_file = os.path.join(self.log_dir, f'{name}.stdout.log')
        self.target = TargetWrapper(target, self.stdout_log_file)
        self.pid = name

    def start(self):
        self.subthread = Thread(name=self.name, target=self.target, args=self.args, kwargs=self.kwargs)
        self.subthread.start()

    def close_files(self):
        pass

    @property
    def stdout(self):
        if os.path.exists(self.stdout_log_file):
            return read_file_to_str(self.stdout_log_file)

        return []

    @property
    def stderr(self):
        if self.target.exit_message is not None:
            return self.target.exit_message
        return []

    @property
    def _exitcode(self):
        return 0 if self.target.exit_message is None else 1

    @property
    def status(self) -> ProcessStatus:
        assert self.subthread is not None
        if self.was_killed:
            return ProcessStatus.KILLED

        self.subthread.join(timeout=0)

        if self.subthread.is_alive():
            return ProcessStatus.RUNNING

        if self._exitcode == 0:
            return ProcessStatus.COMPLETED

        return ProcessStatus.FAILED

    def wait(self):
        self.subthread.join()

    def kill(self):
        self.close_files()

    def exit_code(self):
        if self.subthread is None:
            raise Exception("Process was not started!")

        return self._exitcode

    def pid(self):
        return self.pid

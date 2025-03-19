from abc import abstractmethod, ABCMeta
from enum import Enum


from cloud.ai.nirvana.nv_launcher_agent.lib.thread_logger import ThreadLogger


class BaseProcess(metaclass=ABCMeta):
    def __init__(self, name, args, log_dir):
        self.name = name
        self.args = args
        self.log_dir = log_dir

    @abstractmethod
    def start(self):
        pass

    @abstractmethod
    def stdout(self):
        pass

    @abstractmethod
    def stderr(self):
        pass

    @abstractmethod
    def status(self):
        pass

    @abstractmethod
    def kill(self):
        pass

    @abstractmethod
    def close_files(self):
        pass

    @abstractmethod
    def wait(self):
        pass

    @abstractmethod
    def exit_code(self):
        pass

    @abstractmethod
    def pid(self):
        pass


class ProcessStatus(Enum):
    RUNNING = 'RUNNING'
    FAILED = 'FAILED'
    COMPLETED = 'COMPLETED'
    KILLED = 'KILLED'


def print_process_exit_status(process: BaseProcess):
    exit_code = process.exit_code()
    if exit_code is not None and exit_code != 0:
        ThreadLogger.info(f'{process.name} process FAILED with exit_code={exit_code}')
        ThreadLogger.info(f'stderr:\n{process.stderr}')
        return

    ThreadLogger.info(f'{process.name} process COMPLETED')

from .base import CloudInitModule


class RunCMD(CloudInitModule):
    def __init__(self, cmd: str):
        self.cmd = cmd

    @property
    def module_name(self) -> str:
        return 'runcmd'

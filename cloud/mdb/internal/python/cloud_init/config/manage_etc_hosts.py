from .base import CloudInitModule


class UpdateEtcHosts(CloudInitModule):
    scalar = True

    def __init__(self, update: bool):
        self.update = update

    @property
    def module_name(self) -> str:
        return 'manage_etc_hosts'

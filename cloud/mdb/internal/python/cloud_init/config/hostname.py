from .base import CloudInitModule


class Hostname(CloudInitModule):
    scalar = True

    def __init__(self, fqdn: str):
        self.fqdn = fqdn

    @property
    def module_name(self) -> str:
        return 'hostname'

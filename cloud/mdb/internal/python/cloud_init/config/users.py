from .base import CloudInitModule


class Users(CloudInitModule):
    sudo = 'ALL=(ALL) NOPASSWD:ALL'
    shell = '/bin/bash'

    def __init__(self, name: str, keys: list):
        self.name = name
        self.keys = keys

    @property
    def module_name(self) -> str:
        return 'users'

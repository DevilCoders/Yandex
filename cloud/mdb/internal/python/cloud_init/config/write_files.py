from .base import CloudInitModule


class WriteFiles(CloudInitModule):
    def __init__(self, path: str, content: str, permissions: str = '0640'):
        self.path = path
        self.content = content
        self.permissions = permissions

    @property
    def module_name(self) -> str:
        return 'write_files'

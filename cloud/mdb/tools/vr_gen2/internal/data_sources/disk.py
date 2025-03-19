import yaml

from .base import DiskTypeIdSourceBase


class DiskTypeIdSource(DiskTypeIdSourceBase):
    def __init__(self, file_content: str):
        data = yaml.load(file_content, Loader=yaml.CLoader)
        self.disk_map = {x['disk_type_ext_id'].replace('-', '_'): x['disk_type_id'] for x in data}

    def get_by_ext_id(self, ext_id) -> int:
        return self.disk_map[ext_id]

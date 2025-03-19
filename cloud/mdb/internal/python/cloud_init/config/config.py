from typing import List
from collections import defaultdict
from .base import CloudInitModule
from .legacy_key_value import LegacyKeyValue


class CloudConfig:
    legacy_values: List[LegacyKeyValue]
    scalar_modules: List[CloudInitModule]

    def __init__(self):
        self.list_modules = defaultdict(list)
        self.scalar_modules = []
        self.legacy_values = []

    def append_module_item(self, *objects: CloudInitModule):
        for obj in objects:
            if isinstance(obj, LegacyKeyValue):
                self.legacy_values.append(obj)
                continue
            if obj.scalar:
                self.scalar_modules.append(obj)
            else:
                self.list_modules[obj.module_name].append(obj)

    def headers(self):
        return '\n'.join(
            (
                '## template: jinja',
                '#cloud-config',
            )
        )

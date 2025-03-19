from typing import List

import yaml

from .base import FlavorSourceBase


class FlavorSource(FlavorSourceBase):
    def __init__(self, file_content: str):
        data = yaml.load(file_content, Loader=yaml.CLoader)
        self.flavors = [flavor for flavor in data if flavor.get('visible', True)]

    def filter_ids_by_type(self, pattern) -> List[str]:
        result = []
        for item in self.flavors:
            if item['type'] == pattern:
                result.append(item['id'])
        return result

    def filter_ids_by_name(self, pattern) -> List[str]:
        result = []
        for item in self.flavors:
            if item['name'] == pattern:
                result.append(item['id'])
        return result

    def get_by_id(self, id: str) -> dict:
        for candidate in self.flavors:
            if candidate['id'] == id:
                return candidate
        raise ValueError(f'cannot find flavor {id}')

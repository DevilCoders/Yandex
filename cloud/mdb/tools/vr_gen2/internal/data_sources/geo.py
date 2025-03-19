from typing import List

import yaml

from .base import GeoIdSourceBase


class GeoIdSource(GeoIdSourceBase):
    def __init__(self, file_content: str):
        data = yaml.load(file_content, Loader=yaml.CLoader)
        self.geo = {x['name']: x['geo_id'] for x in data}

    def get_without_names(self, excluded_geos: List[str]) -> List[int]:
        return [geo_id for name, geo_id in self.geo.items() if name not in excluded_geos]

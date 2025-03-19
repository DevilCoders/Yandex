from typing import Dict, Union

import yaml
from attr import attrs

from src.solomon_mr.DataPoint import DataPointLabels


@attrs(auto_attribs=True)
class JugglerConfig:
    host: str


@attrs(auto_attribs=True)
class ZKConfig:
    url: str
    lock: str


@attrs(auto_attribs=True)
class CheckData:
    left: str
    right: Union[str, float]
    scale: float

    @classmethod
    def from_dict(cls, data: Dict[str, Union[str, float]]):
        return cls(left=data['left'], right=data['right'], scale=data['scale'] if 'scale' in data else None)


class Config:
    def __init__(self, filename: str):
        with open(filename) as fin:
            raw = yaml.load(fin, Loader=yaml.FullLoader)

        self.juggler: JugglerConfig = JugglerConfig(raw['juggler']['host'])
        self.delay: int = raw['delay']
        self.average_over_secs: int = raw['average_over_secs']
        self.zookeeper: ZKConfig = ZKConfig(raw['zookeeper']['url'], raw['zookeeper']['lock'])
        self.output: DataPointLabels = DataPointLabels.from_dict(raw['output'])
        self.in_data: Dict[str, DataPointLabels] = \
            dict((k, DataPointLabels.from_dict(v)) for k, v in raw['in_data'].items())
        self.check: Dict[str, CheckData] = dict((k, CheckData.from_dict(v)) for k, v in raw['check'].items())
        self.totals: Dict[str, int] = raw['totals']

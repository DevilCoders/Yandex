from typing import Dict

from attr import attrs


@attrs(auto_attribs=True)
class DataPointLabels:
    cluster: str
    project: str
    service: str
    sensor: str

    @classmethod
    def from_dict(cls, data: Dict[str, str]):
        return cls(cluster=data['cluster'],
                   project=data['project'],
                   service=data['service'],
                   sensor=data['sensor'] if 'sensor' in data else None)


@attrs(auto_attribs=True)
class DataPoint:
    labels: DataPointLabels
    timestamp: int  # in seconds
    value: float


@attrs(auto_attribs=True)
class DataPointKV:
    sensor: str
    value: float

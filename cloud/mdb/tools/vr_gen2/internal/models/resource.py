import yaml
from dataclasses import dataclass, asdict
from typing import Optional


@dataclass
class DiskSizeRange:
    int8range: list


@dataclass
class Resource:
    cluster_type: str
    role: str
    max_hosts: int
    min_hosts: int
    flavor: str
    disk_type_id: int
    geo_id: int
    id: int
    default_disk_size: int
    disk_sizes: list = None
    feature_flag: Optional[str] = None
    disk_size_range: dict = None


class ResourceDumper(yaml.Dumper):
    pass


def resource_representer(dumper, value: Resource):
    return dumper.represent_mapping(yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, asdict(value))


ResourceDumper.add_representer(Resource, resource_representer)

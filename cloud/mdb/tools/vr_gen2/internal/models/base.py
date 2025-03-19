from typing import List, Optional

from dataclasses import dataclass, field


@dataclass
class HostCount:
    max: int = None
    min: int = 1


@dataclass
class Segment:
    start: int
    end: int


@dataclass
class DiskRange:
    min: str
    step: str
    upto: str


@dataclass
class DiskSize:
    int8range: Optional[Segment]
    custom_range: Optional[DiskRange]
    custom_sizes: Optional[List[str]]


@dataclass
class FlavorType:
    name: str = None
    type: str = None


@dataclass
class ValidResourceDef:
    flavor_types: List[FlavorType]
    host_count: List[HostCount]
    feature_flags: List[str]
    disk_sizes: List[DiskSize]
    default_disk_size: Optional[str]
    excluded_geo: List[str] = field(default_factory=list)


@dataclass
class ListDiskOptionsDef:
    gp2: List[ValidResourceDef] = field(default_factory=list)
    network_ssd: List[ValidResourceDef] = field(default_factory=list)
    network_hdd: List[ValidResourceDef] = field(default_factory=list)
    network_ssd_nonreplicated: List[ValidResourceDef] = field(default_factory=list)
    local_ssd: List[ValidResourceDef] = field(default_factory=list)

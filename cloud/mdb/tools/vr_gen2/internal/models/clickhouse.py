from dataclasses import dataclass

from .base import ListDiskOptionsDef


@dataclass
class ClickhouseClusterDef:
    clickhouse_cluster: ListDiskOptionsDef = None
    zk: ListDiskOptionsDef = None

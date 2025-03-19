from dataclasses import dataclass, field

from .clickhouse import ClickhouseClusterDef
from .kafka import KafkaClusterDef


@dataclass
class ClusterTypes:
    clickhouse_cluster: ClickhouseClusterDef = field(default=ClickhouseClusterDef)
    kafka_cluster: KafkaClusterDef = field(default=KafkaClusterDef)


@dataclass
class ResourceDefSchema:
    cluster_types: ClusterTypes

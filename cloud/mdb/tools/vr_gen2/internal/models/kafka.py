from dataclasses import dataclass

from .base import ListDiskOptionsDef


@dataclass
class KafkaClusterDef:
    kafka_cluster: ListDiskOptionsDef = None
    zk: ListDiskOptionsDef = None

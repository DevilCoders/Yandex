import typing
from dataclasses import dataclass
from enum import Enum

from cloud.ai.lib.python.serialization import YsonSerializable


class ClusterReferencesSource(Enum):
    ARCADIA = 'arcadia'


@dataclass
class ClusterReferencesArcadia(YsonSerializable):
    topic: str
    lang: str
    revision: int

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'source': ClusterReferencesSource.ARCADIA.value}


ClusterReferencesDescriptor = typing.Union[ClusterReferencesArcadia]


def get_cluster_references_descriptor(fields: dict) -> ClusterReferencesDescriptor:
    source = ClusterReferencesSource(fields['source'])
    if source == ClusterReferencesSource.ARCADIA:
        return ClusterReferencesArcadia.from_yson(fields)
    else:
        raise ValueError(f'Unknown cluster references source: {source}')


# what to do if another cluster contains word from already gathered clusters
# (including bots cluster center and other words)
class ClusterReferenceMergeStrategy(Enum):
    SKIP = 'skip'  # skip cluster with already seen word entirely


@dataclass
class ClusterReferencesInfo:
    descriptors: typing.List[ClusterReferencesDescriptor]
    merge_strategy: ClusterReferenceMergeStrategy

    def to_yson(self) -> dict:
        return {
            'descriptors': [d.to_yson() for d in self.descriptors],
            'merge_strategy': self.merge_strategy.value,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'ClusterReferencesInfo':
        return ClusterReferencesInfo(
            descriptors=[get_cluster_references_descriptor(f) for f in fields['descriptors']],
            merge_strategy=ClusterReferenceMergeStrategy(fields['merge_strategy']),
        )

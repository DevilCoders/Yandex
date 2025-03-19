# -*- coding: utf-8 -*-
"""
API for managing ClickHouse external dictionaries.
"""
from ...core.types import Operation
from ...utils import metadb
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.metadata import Metadata
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE
from .pillar import ClickhouseShardPillar, get_subcid_and_pillar
from .traits import ClickhouseOperations, ClickhouseRoles, ClickhouseTasks
from .utils import create_operation


class DictionaryMetadata(Metadata):
    """
    Metadata class for operations on ClickHouse external dictionaries.
    """

    def _asdict(self) -> dict:
        return {}


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.CREATE_DICTIONARY)
def create_dictionary_handler(cluster: dict, external_dictionary: dict, **_) -> Operation:
    """
    Handler for create ClickHouse external dictionary requests.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_DICTIONARIES_API')

    subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    pillar.add_dictionary(external_dictionary)
    metadb.update_subcluster_pillar(cluster['cid'], subcid, pillar)

    for shard in metadb.get_shards(cid=cluster['cid'], role=ClickhouseRoles.clickhouse):
        shard_pillar = ClickhouseShardPillar(shard['value'])
        shard_pillar.add_dictionary(external_dictionary)
        metadb.update_shard_pillar(cluster['cid'], shard['shard_id'], shard_pillar)

    return create_operation(
        task_type=ClickhouseTasks.dictionary_create,
        operation_type=ClickhouseOperations.add_dictionary,
        metadata=DictionaryMetadata(),
        cid=cluster['cid'],
        task_args={'target-dictionary': external_dictionary['name']},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.DELETE_DICTIONARY)
def delete_dictionary_handler(cluster: dict, external_dictionary_name: str, **_) -> Operation:
    """
    Handler for delete ClickHouse external dictionary requests.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_DICTIONARIES_API')

    subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    pillar.delete_dictionary(external_dictionary_name)
    metadb.update_subcluster_pillar(cluster['cid'], subcid, pillar)

    for shard in metadb.get_shards(cid=cluster['cid'], role=ClickhouseRoles.clickhouse):
        shard_pillar = ClickhouseShardPillar(shard['value'])
        shard_pillar.delete_dictionary(external_dictionary_name)
        metadb.update_shard_pillar(cluster['cid'], shard['shard_id'], shard_pillar)

    return create_operation(
        task_type=ClickhouseTasks.dictionary_delete,
        operation_type=ClickhouseOperations.delete_dictionary,
        metadata=DictionaryMetadata(),
        cid=cluster['cid'],
        task_args={'target-dictionary': external_dictionary_name},
    )

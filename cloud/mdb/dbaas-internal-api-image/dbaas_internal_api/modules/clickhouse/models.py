# -*- coding: utf-8 -*-
"""
API for managing ClickHouse ML models.
"""
from ...core.types import Operation
from ...utils import metadb
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.metadata import Metadata
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE
from .pillar import get_subcid_and_pillar
from .traits import ClickhouseOperations, ClickhouseTasks
from .utils import create_operation


class MlModelMetadata(Metadata):
    """
    Metadata class for operations on ClickHouse ML models.
    """

    def __init__(self, ml_model_name: str) -> None:
        self.ml_model_name = ml_model_name

    def _asdict(self) -> dict:
        return {'ml_model_name': self.ml_model_name}


@register_request_handler(MY_CLUSTER_TYPE, Resource.ML_MODEL, DbaasOperation.CREATE)
def create_ml_model_handler(cluster: dict, ml_model_name: str, uri: str, **kwargs) -> Operation:
    """
    Handler for create ClickHouse ML model requests.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_ML_MODELS_API')

    subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    pillar.add_model(ml_model_name, kwargs['type'], uri)
    metadb.update_subcluster_pillar(cluster['cid'], subcid, pillar)

    return create_operation(
        task_type=ClickhouseTasks.model_create,
        operation_type=ClickhouseOperations.model_create,
        metadata=MlModelMetadata(ml_model_name),
        cid=cluster['cid'],
        task_args={'target-model': ml_model_name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.ML_MODEL, DbaasOperation.MODIFY)
def modify_ml_model_handler(cluster: dict, ml_model_name: str, uri: str = None, **_) -> Operation:
    """
    Handler for create ClickHouse ML model requests.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_ML_MODELS_API')

    subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    pillar.update_model(ml_model_name, uri)
    metadb.update_subcluster_pillar(cluster['cid'], subcid, pillar)

    return create_operation(
        task_type=ClickhouseTasks.model_modify,
        operation_type=ClickhouseOperations.model_modify,
        metadata=MlModelMetadata(ml_model_name),
        cid=cluster['cid'],
        task_args={'target-model': ml_model_name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.ML_MODEL, DbaasOperation.DELETE)
def delete_ml_model_handler(cluster, ml_model_name, **_):
    """
    Deletes ClickHouse ML model.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_ML_MODELS_API')

    subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    pillar.delete_model(ml_model_name)

    metadb.update_subcluster_pillar(cluster['cid'], subcid, pillar)

    return create_operation(
        task_type=ClickhouseTasks.model_delete,
        operation_type=ClickhouseOperations.model_delete,
        metadata=MlModelMetadata(ml_model_name),
        cid=cluster['cid'],
        task_args={'target-model': ml_model_name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.ML_MODEL, DbaasOperation.INFO)
def get_ml_model_handler(cluster, ml_model_name, **_):
    """
    Returns ClickHouse ML model.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_ML_MODELS_API')

    _subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    return pillar.model(cluster['cid'], ml_model_name)


@register_request_handler(MY_CLUSTER_TYPE, Resource.ML_MODEL, DbaasOperation.LIST)
def list_ml_models_handler(cluster, **_):
    """
    Returns ClickHouse ML models.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_ML_MODELS_API')

    _subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    return {'ml_models': pillar.models(cluster['cid'])}

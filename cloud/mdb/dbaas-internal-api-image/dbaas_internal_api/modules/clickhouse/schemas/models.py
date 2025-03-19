# -*- coding: utf-8 -*-
"""
Schemas for ClickHouse ML models.
"""

from marshmallow.fields import Nested

from ....apis.schemas.common import ListResponseSchemaV1
from ....apis.schemas.fields import ClusterId, MappedEnum, Str
from ....utils.register import DbaasOperation, Resource, register_request_schema, register_response_schema
from ....utils.validation import Schema
from ..constants import MY_CLUSTER_TYPE
from ..traits import ClickhouseClusterTraits


class MlModelType(MappedEnum):
    """
    ML model type.
    """

    _mapping = {
        'ML_MODEL_TYPE_CATBOOST': 'catboost',
    }

    def __init__(self, **kwargs):
        super().__init__(self._mapping, **kwargs, skip_description=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.ML_MODEL, DbaasOperation.INFO)
class ClickhouseMlModelSchemaV1(Schema):
    """
    ClickHouse ML model schema.
    """

    clusterId = ClusterId(required=True)
    name = Str(required=True)
    type = MlModelType(required=True)
    uri = Str(required=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.ML_MODEL, DbaasOperation.LIST)
class ClickhouseListMlModelsResponseSchemaV1(ListResponseSchemaV1):
    """

    ClickHouse ML model list schema.
    """

    mlModels = Nested(ClickhouseMlModelSchemaV1, attribute='ml_models', many=True, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.ML_MODEL, DbaasOperation.CREATE)
class ClickhouseCreateMlModelRequestSchemaV1(Schema):
    """
    Schema for create ClickHouse ML model request.
    """

    mlModelName = Str(attribute='ml_model_name', validate=ClickhouseClusterTraits.ml_model_name.validate, required=True)
    type = MlModelType(required=True)
    uri = Str(validate=ClickhouseClusterTraits.ml_model_uri.validate, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.ML_MODEL, DbaasOperation.MODIFY)
class ClickhouseUpdateMlModelRequestSchemaV1(Schema):
    """
    Schema for update ClickHouse ML model request.
    """

    uri = Str(validate=ClickhouseClusterTraits.ml_model_uri.validate)

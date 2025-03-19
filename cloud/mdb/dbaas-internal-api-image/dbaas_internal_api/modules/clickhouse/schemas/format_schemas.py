# -*- coding: utf-8 -*-
"""
Schemas for ClickHouse format schemas.
"""

from marshmallow.fields import Nested

from ....apis.schemas.common import ListResponseSchemaV1
from ....apis.schemas.fields import ClusterId, MappedEnum, Str
from ....utils.register import DbaasOperation, Resource, register_request_schema, register_response_schema
from ....utils.validation import Schema
from ..constants import MY_CLUSTER_TYPE
from ..traits import ClickhouseClusterTraits


class FormatSchemaType(MappedEnum):
    """
    Format schema type.
    """

    _mapping = {
        'FORMAT_SCHEMA_TYPE_PROTOBUF': 'protobuf',
        'FORMAT_SCHEMA_TYPE_CAPNPROTO': 'capnproto',
    }

    def __init__(self, **kwargs):
        super().__init__(self._mapping, **kwargs, skip_description=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.FORMAT_SCHEMA, DbaasOperation.INFO)
class ClickhouseFormatSchemaSchemaV1(Schema):
    """
    ClickHouse format schema schema.
    """

    clusterId = ClusterId(required=True)
    name = Str(required=True)
    type = FormatSchemaType(required=True)
    uri = Str(required=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.FORMAT_SCHEMA, DbaasOperation.LIST)
class ClickhouseListFormatSchemasResponseSchemaV1(ListResponseSchemaV1):
    """
    ClickHouse format schema list schema.
    """

    formatSchemas = Nested(ClickhouseFormatSchemaSchemaV1, attribute='format_schemas', many=True, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.FORMAT_SCHEMA, DbaasOperation.CREATE)
class ClickhouseCreateFormatSchemaRequestSchemaV1(Schema):
    """
    Schema for create ClickHouse format schema request.
    """

    formatSchemaName = Str(
        attribute='format_schema_name', validate=ClickhouseClusterTraits.format_schema_name.validate, required=True
    )
    type = FormatSchemaType(required=True)
    uri = Str(validate=ClickhouseClusterTraits.format_schema_uri.validate, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.FORMAT_SCHEMA, DbaasOperation.MODIFY)
class ClickhouseUpdateFormatSchemaRequestSchemaV1(Schema):
    """
    Schema for update ClickHouse format schema request.
    """

    uri = Str(validate=ClickhouseClusterTraits.format_schema_uri.validate)

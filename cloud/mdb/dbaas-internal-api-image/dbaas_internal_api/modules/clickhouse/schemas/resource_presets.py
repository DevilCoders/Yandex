# -*- coding: utf-8 -*-
"""
Schemas for ClickHouse resource presets.
"""

from marshmallow.fields import List, Nested

from ....apis.schemas.common import ListResponseSchemaV1
from ....apis.schemas.objects import ResourcePresetSchemaV1
from ....utils.register import DbaasOperation, Resource, register_response_schema
from ..constants import MY_CLUSTER_TYPE
from .hosts import ClickhouseHostType


@register_response_schema(MY_CLUSTER_TYPE, Resource.RESOURCE_PRESET, DbaasOperation.INFO)
class ClickhouseResourcePresetSchemaV1(ResourcePresetSchemaV1):
    """
    ClickHouse resource preset schema.
    """

    hostTypes = List(ClickhouseHostType(), attribute='roles')


@register_response_schema(MY_CLUSTER_TYPE, Resource.RESOURCE_PRESET, DbaasOperation.LIST)
class ClickhouseListResourcePresetsSchemaV1(ListResponseSchemaV1):
    """
    ClickHouse resource preset list schema.
    """

    resourcePresets = Nested(ClickhouseResourcePresetSchemaV1, many=True, attribute='resource_presets', required=True)

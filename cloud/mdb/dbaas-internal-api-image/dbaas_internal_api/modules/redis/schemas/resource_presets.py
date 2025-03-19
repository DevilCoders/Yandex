"""
Schemas for Redis resource presets.
"""
from marshmallow.fields import Nested

from ....apis.schemas.common import ListResponseSchemaV1
from ....apis.schemas.objects import ResourcePresetSchemaV1
from ....utils.register import DbaasOperation, Resource, register_response_schema
from ..constants import MY_CLUSTER_TYPE


@register_response_schema(MY_CLUSTER_TYPE, Resource.RESOURCE_PRESET, DbaasOperation.INFO)
class RedisResourcePresetSchemaV1(ResourcePresetSchemaV1):
    """
    Redis resource preset schema.
    """


@register_response_schema(MY_CLUSTER_TYPE, Resource.RESOURCE_PRESET, DbaasOperation.LIST)
class RedisListResourcePresetsSchemaV1(ListResponseSchemaV1):
    """
    Redis resource preset list schema.
    """

    resourcePresets = Nested(RedisResourcePresetSchemaV1, many=True, attribute='resource_presets', required=True)

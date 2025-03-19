"""
Schemas for Redis resources.
"""
from marshmallow import Schema
from marshmallow.fields import Int

from ....apis.schemas.fields import Str


class RedisResourcesSchemaV1(Schema):
    """
    Redis resources schema for cluster create.
    """

    resourcePresetId = Str(attribute='resource_preset_id', required=True)
    diskTypeId = Str(attribute='disk_type_id')
    diskSize = Int(attribute='disk_size', required=True)


class RedisResourcesUpdateSchemaV1(Schema):
    """
    Redis resources schema for cluster update.
    """

    resourcePresetId = Str(attribute='resource_preset_id')
    diskTypeId = Str(attribute='disk_type_id')
    diskSize = Int(attribute='disk_size')

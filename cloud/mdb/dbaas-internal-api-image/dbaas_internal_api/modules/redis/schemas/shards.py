"""
Schemas for Redis shards.
"""

from marshmallow import ValidationError, validates_schema
from marshmallow.fields import Nested

from ....apis.schemas.common import ListResponseSchemaV1
from ....apis.schemas.fields import Str
from ....apis.schemas.objects import ShardSchemaV1
from ....utils.register import DbaasOperation, Resource, register_request_schema, register_response_schema
from ....utils.validation import Schema
from ..constants import MY_CLUSTER_TYPE
from ..traits import RedisClusterTraits
from .hosts import RedisHostSpecSchemaV1


@register_response_schema(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.INFO)
class RedisShardSchemaV1(ShardSchemaV1):
    """
    Redis shard schema.
    """


@register_response_schema(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.LIST)
class RedisListShardsResponseSchemaV1(ListResponseSchemaV1):
    """
    Redis host list schema.
    """

    shards = Nested(RedisShardSchemaV1, many=True, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.CREATE)
class RedisAddShardRequestSchemaV1(Schema):
    """
    Schema for add Redis shard request.
    """

    shardName = Str(attribute='shard_name', validate=RedisClusterTraits.shard_name.validate, required=True)
    hostSpecs = Nested(RedisHostSpecSchemaV1, attribute='host_specs', many=True)

    @validates_schema(skip_on_field_errors=True)
    def _validates_schema(self, data):
        shard_name = data['shard_name']
        for host in data['host_specs']:
            if host.get('shard_name', shard_name) != shard_name:
                raise ValidationError('Shard hosts must have the same shard name')

# -*- coding: utf-8 -*-
"""
Schemas for ClickHouse shards.
"""
from marshmallow import post_load
from marshmallow.fields import Boolean, Nested, List

from ....apis.schemas.common import ListResponseSchemaV1
from ....apis.schemas.fields import ClusterId, Str, UInt
from ....apis.schemas.objects import ResourcesSchemaV1, ResourcesUpdateSchemaV1, ShardSchemaV1
from ....modules.clickhouse.schemas.clusters import ClickhouseConfigSchemaV1, ClickhouseConfigSetSchemaV1
from ....utils.register import DbaasOperation, Resource, register_request_schema, register_response_schema
from ....utils.validation import Schema
from ..constants import MY_CLUSTER_TYPE
from ..traits import ClickhouseClusterTraits
from .hosts import ClickhouseHostSpecSchemaV1


class ClickhouseShardConfigSchemaV1(Schema):
    """
    ClickHouse shard config schema.
    """

    class ClickhouseSchema(Schema):
        """
        ClickHouse subcluster schema.
        """

        config = Nested(ClickhouseConfigSetSchemaV1, required=True)
        resources = Nested(ResourcesSchemaV1, required=True)
        weight = UInt()

    clickhouse = Nested(ClickhouseSchema, required=True)


class ClickhouseShardConfigSpecSchemaV1(Schema):
    """
    ClickHouse shard config spec schema for update shard operation.
    """

    class ClickhouseSchema(Schema):
        """
        ClickHouse subcluster schema.
        """

        config = Nested(ClickhouseConfigSchemaV1)
        resources = Nested(ResourcesUpdateSchemaV1)
        weight = UInt()

    clickhouse = Nested(ClickhouseSchema)


@register_response_schema(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.INFO)
class ClickhouseShardSchemaV1(ShardSchemaV1):
    """
    ClickHouse shard schema.
    """

    config = Nested(ClickhouseShardConfigSchemaV1)


@register_response_schema(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.LIST)
class ClickhouseListShardsResponseSchemaV1(ListResponseSchemaV1):
    """
    ClickHouse host list schema.
    """

    shards = Nested(ClickhouseShardSchemaV1, many=True, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.CREATE)
class ClickhouseAddShardRequestSchemaV1(Schema):
    """
    Schema for add ClickHouse shard request.
    """

    shardName = Str(attribute='shard_name', validate=ClickhouseClusterTraits.shard_name.validate, required=True)
    configSpec = Nested(ClickhouseShardConfigSpecSchemaV1, attribute='config_spec')
    hostSpecs = Nested(ClickhouseHostSpecSchemaV1, attribute='host_specs', many=True)
    copySchema = Boolean(attribute='copy_schema', missing=False)

    @post_load
    def _post_load(self, data):
        try:
            config = data['config_spec']['clickhouse']['config']
            if config.get('dictionaries') == []:
                del config['dictionaries']
            if config.get('compression') == []:
                del config['compression']
            if config.get('graphite_rollup') == []:
                del config['graphite_rollup']
        except KeyError:
            pass

        return data


@register_request_schema(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.MODIFY)
class ClickhouseUpdateShardRequestSchemaV1(Schema):
    """
    Schema for update ClickHouse shard request.
    """

    configSpec = Nested(ClickhouseShardConfigSpecSchemaV1, attribute='config_spec', required=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.SHARD_GROUP, DbaasOperation.INFO)
class ClickhouseShardGroupSchemaV1(Schema):
    """
    ClickHouse shard group schema.
    """

    clusterId = ClusterId(required=True)
    name = Str(required=True)
    description = Str(required=True)
    shardNames = List(Str(), attribute='shard_names', required=True)

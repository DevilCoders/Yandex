"""
Schemas for Redis hosts.
"""
from marshmallow.fields import Boolean, List, Nested

from ....apis.schemas.common import ListResponseSchemaV1
from ....apis.schemas.console import EstimateHostCreateRequestSchemaV1
from ....apis.schemas.fields import MappedEnum, UInt, Str
from ....apis.schemas.objects import HostSpecSchemaV1, HostSchemaV1
from ....health.health import ServiceStatus
from ....utils.register import DbaasOperation, Resource, register_request_schema, register_response_schema
from ....utils.validation import Schema, Range
from ..constants import MY_CLUSTER_TYPE
from ..traits import HostRole, RedisClusterTraits, ServiceType
from .resources import RedisResourcesSchemaV1


class RedisHostServiceSchemaV1(Schema):
    """
    Type of host services.
    """

    type = MappedEnum(
        {
            'REDIS': ServiceType.redis,
            'ARBITER': ServiceType.sentinel,
            'REDIS_CLUSTER': ServiceType.redis_cluster,
        }
    )
    health = MappedEnum(
        {
            'ALIVE': ServiceStatus.alive,
            'DEAD': ServiceStatus.dead,
            'UNKNOWN': ServiceStatus.unknown,
        }
    )


class RedisHostSchemaV1(HostSchemaV1):
    """
    Redis host schema.
    """

    role = MappedEnum(
        {
            'UNKNOWN': HostRole.unknown,
            'MASTER': HostRole.master,
            'REPLICA': HostRole.replica,
        }
    )
    resources = Nested(RedisResourcesSchemaV1, required=True)
    services = List(Nested(RedisHostServiceSchemaV1))
    shardName = Str(attribute='shard_name', validate=RedisClusterTraits.shard_name.validate)
    replicaPriority = UInt(attribute='replica_priority')


@register_response_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.LIST)
class RedisListHostsResponseSchemaV1(ListResponseSchemaV1):
    """
    Redis host list schema.
    """

    hosts = Nested(RedisHostSchemaV1, many=True, required=True)


class RedisHostSpecSchemaV1(HostSpecSchemaV1):
    """
    Redis host spec schema.
    """

    shardName = Str(attribute='shard_name', validate=RedisClusterTraits.shard_name.validate)
    replicaPriority = UInt(validate=Range(0, 1000), attribute='replica_priority')


class RedisHostUpdateSpecSchemaV1(Schema):
    """
    Redis host update spec schema.
    """

    hostName = Str(required=True, attribute='host_name')
    replicaPriority = UInt(validate=Range(0, 1000), attribute='replica_priority')
    assignPublicIp = Boolean(attribute='assign_public_ip')


@register_request_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_CREATE)
class RedisAddHostsRequestSchemaV1(Schema):
    """
    Schema for add Redis hosts request.
    """

    hostSpecs = Nested(RedisHostSpecSchemaV1, attribute='host_specs', many=True, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_MODIFY)
class RedisUpdateHostsRequestSchemaV1(Schema):
    """
    Schema for update Redis hosts request.
    """

    updateHostSpecs = Nested(RedisHostUpdateSpecSchemaV1, many=True, attribute='update_host_specs', required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_DELETE)
class RedisDeleteHostsRequestSchemaV1(Schema):
    """
    Schema for delete Redis hosts request.
    """

    hostNames = List(Str(), attribute='host_names', required=True)


class RedisHostSpecCostEstSchemaV1(Schema):
    """
    Used for billing estimation.
    Additional fields provide Resource data usually inferred from cluster`s info.
    """

    host = Nested(RedisHostSpecSchemaV1, required=True)
    resources = Nested(RedisResourcesSchemaV1, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BILLING_CREATE_HOSTS)
class RedisHostSpecCostEstRequestSchemaV1(EstimateHostCreateRequestSchemaV1):
    """
    Provides an estimate for host creation in console.
    """

    billingHostSpecs = Nested(RedisHostSpecCostEstSchemaV1, many=True, attribute='billing_host_specs', required=True)

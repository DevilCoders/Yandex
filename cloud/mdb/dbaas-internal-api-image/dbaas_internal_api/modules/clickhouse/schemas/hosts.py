# -*- coding: utf-8 -*-
"""
Schemas for ClickHouse hosts.
"""

from marshmallow.fields import Boolean, List, Nested

from ....apis.schemas.common import ListResponseSchemaV1
from ....apis.schemas.console import EstimateHostCreateRequestSchemaV1
from ....apis.schemas.fields import MappedEnum, Str
from ....apis.schemas.objects import HostSchemaV1, HostSpecSchemaV1, ResourcesSchemaV1
from ....health.health import ServiceStatus
from ....utils.register import DbaasOperation, Resource, register_request_schema, register_response_schema
from ....utils.validation import Schema
from ..constants import MY_CLUSTER_TYPE
from ..traits import ClickhouseClusterTraits, ClickhouseRoles, ServiceType


class ClickhouseHostType(MappedEnum):
    """
    'type' field.
    """

    def __init__(self, **kwargs):
        super().__init__(
            {
                'CLICKHOUSE': ClickhouseRoles.clickhouse,
                'ZOOKEEPER': ClickhouseRoles.zookeeper,
            },
            **kwargs,
            skip_description=True
        )


class ClickhouseHostServiceSchemaV1(Schema):
    """
    Type of host services.
    """

    type = MappedEnum(
        {
            'CLICKHOUSE': ServiceType.clickhouse,
            'ZOOKEEPER': ServiceType.zookeeper,
        }
    )
    health = MappedEnum(
        {
            'ALIVE': ServiceStatus.alive,
            'DEAD': ServiceStatus.dead,
            'UNKNOWN': ServiceStatus.unknown,
        }
    )


class ClickhouseHostSchemaV1(HostSchemaV1):
    """
    ClickHouse host schema.
    """

    type = ClickhouseHostType(required=True)
    shardName = Str(attribute='shard_name', validate=ClickhouseClusterTraits.shard_name.validate)
    services = List(Nested(ClickhouseHostServiceSchemaV1))


@register_response_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.LIST)
class ClickhouseListHostsResponseSchemaV1(ListResponseSchemaV1):
    """
    ClickHouse host list schema.
    """

    hosts = Nested(ClickhouseHostSchemaV1, many=True, required=True)


class ClickhouseHostSpecSchemaV1(HostSpecSchemaV1):
    """
    ClickHouse host spec schema.
    """

    type = ClickhouseHostType(required=True)
    shardName = Str(attribute='shard_name', validate=ClickhouseClusterTraits.shard_name.validate)


@register_request_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_CREATE)
class ClickhouseAddHostsRequestSchemaV1(Schema):
    """
    Schema for add ClickHouse hosts request.
    """

    hostSpecs = List(Nested(ClickhouseHostSpecSchemaV1), attribute='host_specs', required=True)
    copySchema = Boolean(attribute='copy_schema', missing=False)


@register_request_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_DELETE)
class ClickhouseDeleteHostsRequestSchemaV1(Schema):
    """
    Schema for delete ClickHouse hosts request.
    """

    hostNames = List(Str(), attribute='host_names', required=True)


class ClickhouseHostSpecCostEstSchemaV1(Schema):
    """
    Used for billing estimation.
    Additional fields provide Resource data usually inferred from cluster`s info.
    """

    host = Nested(ClickhouseHostSpecSchemaV1, required=True)
    resources = Nested(ResourcesSchemaV1, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BILLING_CREATE_HOSTS)
class ClickhouseHostSpecCostEstRequestSchemaV1(EstimateHostCreateRequestSchemaV1):
    """
    Provides an estimate for host creation in console.
    """

    billingHostSpecs = Nested(
        ClickhouseHostSpecCostEstSchemaV1, many=True, attribute='billing_host_specs', required=True
    )

"""
Schemas for Redis clusters.
"""
from marshmallow import ValidationError, validates_schema
from marshmallow.fields import Bool, List, Nested
from marshmallow.validate import Length

from ....apis.schemas.cluster import (
    CreateClusterRequestSchemaV1,
    ManagedClusterSchemaV1,
    RestoreClusterRequestSchemaV1,
    StartClusterFailoverRequestSchemaV1,
    UpdateClusterRequestSchemaV1,
)
from ....apis.schemas.common import ListResponseSchemaV1
from ....apis.schemas.fields import Str
from ....utils.register import DbaasOperation, Resource, register_request_schema, register_response_schema
from ....utils.validation import OneOf, Schema
from ..constants import MY_CLUSTER_TYPE
from ..traits import RedisClusterTraits
from .configs import (
    PersistenceMode,
    RedisClusterConfigSchemaV1,
    RedisClusterConfigSpecCreateSchemaV1,
    RedisClusterConfigSpecUpdateSchemaV1,
)
from .hosts import RedisHostSpecSchemaV1


@register_response_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.INFO)
class RedisClusterSchemaV1(ManagedClusterSchemaV1):
    """
    Redis cluster schema.
    """

    config = Nested(RedisClusterConfigSchemaV1, required=True)
    sharded = Bool()
    tlsEnabled = Bool(attribute='tls_enabled')
    persistenceMode = Str(validate=OneOf(PersistenceMode().mapping.keys()), attribute='persistence_mode')


@register_response_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.LIST)
class RedisListClustersResponseSchemaV1(ListResponseSchemaV1):
    """
    Redis cluster list schema.
    """

    clusters = Nested(RedisClusterSchemaV1, many=True, required=True)


class RedisClusterRequestMixin:
    persistenceMode = PersistenceMode(attribute='persistence_mode')


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.CREATE)
@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.BILLING_CREATE)
class RedisCreateClusterRequestSchemaV1(RedisClusterRequestMixin, CreateClusterRequestSchemaV1):
    """
    Schema for create Redis cluster request.
    """

    name = Str(validate=RedisClusterTraits.cluster_name.validate, required=True)
    configSpec = Nested(RedisClusterConfigSpecCreateSchemaV1, attribute='config_spec', required=True)
    hostSpecs = List(Nested(RedisHostSpecSchemaV1), validate=Length(min=1), attribute='host_specs', required=True)
    sharded = Bool(missing=False)
    tlsEnabled = Bool(missing=False, attribute='tls_enabled')

    @validates_schema(skip_on_field_errors=True)
    def _validates_schema(self, data):
        sharded = data['sharded']
        for host in data['host_specs']:
            if sharded and 'shard_name' not in host:
                raise ValidationError('Shard name is not specified for host in {}.'.format(host['zone_id']))
            if not sharded and 'shard_name' in host:
                raise ValidationError("Can't define shards in standalone mode.")


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.RESTORE)
class RedisRestoreClusterRequestSchemaV1(RedisClusterRequestMixin, RestoreClusterRequestSchemaV1):
    """
    Schema for restore Redis cluster request.
    """

    configSpec = Nested(RedisClusterConfigSpecCreateSchemaV1, attribute='config_spec', required=True)
    hostSpecs = List(Nested(RedisHostSpecSchemaV1), validate=Length(min=1), attribute='host_specs', required=True)
    tlsEnabled = Bool(missing=False, attribute='tls_enabled')


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.MODIFY)
class RedisUpdateClusterRequestSchemaV1(RedisClusterRequestMixin, UpdateClusterRequestSchemaV1):
    """
    Schema for update Redis cluster request.
    """

    configSpec = Nested(RedisClusterConfigSpecUpdateSchemaV1, attribute='config_spec')
    name = Str(validate=RedisClusterTraits.cluster_name.validate)


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.START_FAILOVER)
class RedisStartClusterFailoverRequestSchemaV1(StartClusterFailoverRequestSchemaV1):
    """
    Schema for failover Redis cluster request.
    """

    hostNames = List(Str(), attribute='host_names')


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.REBALANCE)
class RedisRebalanceClusterRequestSchemaV1(Schema):
    """
    Schema for Redis rebalance cluster request.
    """


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.STOP)
class RedisStopClusterRequestSchemaV1(Schema):
    """
    Schema for stop cluster request.
    """

    pass


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.DELETE)
class RedisDeleteClusterRequestSchemaV1(Schema):
    """
    Schema for stop cluster request.
    """

    pass

"""
Schemas for Redis backups.
"""
from marshmallow.fields import List, Nested, Str

from ....apis.schemas.backups import BaseBackupSchemaV1
from ....apis.schemas.common import ListResponseSchemaV1
from ....utils.register import DbaasOperation, Resource, register_response_schema
from ..constants import MY_CLUSTER_TYPE
from ..traits import RedisClusterTraits


@register_response_schema(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.INFO)
class RedisBackupSchemaV1(BaseBackupSchemaV1):
    """
    Schema for Redis backup.
    """

    sourceShardNames = List(Str(validate=RedisClusterTraits.shard_name.validate), attribute='shard_names')


@register_response_schema(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.LIST)
class RedisListClusterBackupsReponseSchemaV1(ListResponseSchemaV1):
    """
    Schema for Redis backups listing.
    """

    backups = Nested(RedisBackupSchemaV1, many=True, required=True)

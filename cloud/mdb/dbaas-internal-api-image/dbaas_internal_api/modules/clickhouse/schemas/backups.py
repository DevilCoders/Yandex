# -*- coding: utf-8 -*-
"""
Schemas for ClickHouse backups.
"""

from marshmallow.fields import List, Nested

from ....apis.schemas.backups import BaseBackupSchemaV1
from ....apis.schemas.common import ListResponseSchemaV1
from ....apis.schemas.fields import Str
from ....utils.register import DbaasOperation, Resource, register_response_schema
from ..constants import MY_CLUSTER_TYPE
from ..traits import ClickhouseClusterTraits


@register_response_schema(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.INFO)
class ClickhouseBackupSchemaV1(BaseBackupSchemaV1):
    """
    Schema for Clickhouse backup
    """

    sourceShardNames = List(Str(validate=ClickhouseClusterTraits.shard_name.validate), attribute='shard_names')


@register_response_schema(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.LIST)
class ClickhouseListClusterBackupsReponseSchemaV1(ListResponseSchemaV1):
    """
    Schema for Clickhouse backups listing
    """

    backups = Nested(ClickhouseBackupSchemaV1, many=True, required=True)

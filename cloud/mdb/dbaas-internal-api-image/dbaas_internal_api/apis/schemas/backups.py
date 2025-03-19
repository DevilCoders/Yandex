# -*- coding: utf-8 -*-
"""
DBaaS Internal API backups schemas
"""

from marshmallow import Schema
from marshmallow.fields import DateTime

from .common import ListRequestSchemaV1
from .fields import FolderId, Str


class BaseBackupSchemaV1(Schema):
    """
    Base backup schema.
    """

    id = Str(required=True)
    folderId = FolderId(required=True)
    createdAt = DateTime(attribute='end_time', required=True)
    sourceClusterId = Str(attribute='cluster_id', required=True)
    startedAt = DateTime(attribute='start_time', required=True)


class ListBackupsRequestSchemaV1(ListRequestSchemaV1):
    """
    List backups request schema
    """

    folderId = FolderId(required=True)


class ListClusterBackupsRequestSchemaV1(ListRequestSchemaV1):
    """
    List backups in cluster
    """

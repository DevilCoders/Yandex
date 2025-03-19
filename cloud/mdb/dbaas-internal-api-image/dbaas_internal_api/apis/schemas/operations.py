# -*- coding: utf-8 -*-
"""
DBaaS Internal API task-related schemas
"""

from marshmallow import Schema
from marshmallow.fields import Boolean, DateTime, Dict, Nested

from .common import ErrorSchemaV1, ListRequestSchemaV1, ListResponseSchemaV1
from .fields import FolderId, Str


class OperationSchemaV1(Schema):
    """
    Operation schema.
    """

    id = Str(required=True)
    description = Str()
    createdAt = DateTime(attribute='created_at', required=True, description='Operation start time')
    createdBy = Str(attribute='created_by')
    modifiedAt = DateTime(attribute='modified_at', description='Operation modification time')
    done = Boolean(required=True)
    metadata = Dict()
    error = Nested(ErrorSchemaV1)
    response = Dict()


class OperationListSchemaV1(ListResponseSchemaV1):
    """
    Operation list schema.
    """

    operations = Nested(OperationSchemaV1, many=True, required=True)


class ListOperationsRequestSchema(ListRequestSchemaV1):
    """
    List backups request schema
    """

    folderId = FolderId(required=True)

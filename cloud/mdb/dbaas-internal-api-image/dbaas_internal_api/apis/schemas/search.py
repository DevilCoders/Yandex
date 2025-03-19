"""
Search API schemas
"""
from marshmallow import Schema
from marshmallow.fields import Boolean

from ...utils.validation import GrpcTimestamp


class ReindexCloudRequestSchema(Schema):
    """
    Reindex cloud request
    """

    reindex_timestamp = GrpcTimestamp()
    include_deleted = Boolean(missing=False)

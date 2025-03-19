"""Schemas for the request/response objects."""
from marshmallow import Schema, fields


class MysqlPillarConfigSchema(Schema):
    """Cluster pillar config schema."""

    users = fields.Dict(keys=fields.Str(), values=fields.Dict(), required=True)
    zk_hosts = fields.List(fields.Str(), required=True)


class MysqlPillarDataSchema(Schema):
    """Cluster pillar data schema."""

    sox_audit = fields.Boolean(truthy=True, required=True)
    mysql = fields.Nested(MysqlPillarConfigSchema, required=True)


class MysqlPillarSchema(Schema):
    """Cluster pillar schema."""

    data = fields.Nested(MysqlPillarDataSchema, required=True)

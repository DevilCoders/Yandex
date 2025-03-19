"""Schemas for the request/response objects."""
from marshmallow import Schema, fields


class PgPillarConfigSchema(Schema):
    """Cluster pillar config schema."""

    pgusers = fields.Dict(keys=fields.Str(), values=fields.Dict(), required=True)


class PgPillarDataSchema(Schema):
    """Cluster pillar data schema."""

    sox_audit = fields.Boolean(truthy=True, required=True)
    config = fields.Nested(PgPillarConfigSchema, required=True)


class PgPillarSchema(Schema):
    """Cluster pillar schema."""

    data = fields.Nested(PgPillarDataSchema, required=True)

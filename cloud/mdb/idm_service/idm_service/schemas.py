"""Schemas for the request/response objects."""
import json

from marshmallow import Schema, ValidationError, fields


class JSONField(fields.Field):
    """A string serialized to a JSON format."""

    def _deserialize(self, value, attr, data):
        try:
            result = json.loads(value)
            if not result:
                raise ValidationError('Empty dict')
            return result
        except json.JSONDecodeError:
            raise ValidationError('Invalid JSON value by key {}'.format(attr))


class GrantRequestSchema(Schema):
    """Add/Remove role request schema."""

    login = fields.Str(required=True)
    role = JSONField(required=True)

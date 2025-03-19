# coding: utf-8
"""
tests for console helpers
"""

import json

import marshmallow
import marshmallow.validate

from dbaas_internal_api.apis.schemas.fields import MappedEnum
from dbaas_internal_api.utils.apispec import schema_to_jsonschema
from dbaas_internal_api.utils.validation import Schema as OurBaseSchema

from ..fixtures import app_config  # pylint: disable=unused-import


class NodeSchema(marshmallow.Schema):
    """
    node
    """

    geo = marshmallow.fields.Str()
    cores = marshmallow.fields.Integer(min=1, max=32)


class ResourcesSchema(marshmallow.Schema):
    """
    resources
    """

    host = marshmallow.fields.List(marshmallow.fields.Nested(NodeSchema), min=3, max=42)


class SpeedField(MappedEnum):
    """
    speed field map
    """

    def __init__(self, **kwargs):
        super().__init__(
            {
                'FAST': 'fast',
                'SUPER_FAST': 'super-fast',
                'SUPER_PUPER_FAST': 'super-puper-fast',
            },
            **kwargs
        )


class ConfigSchema(marshmallow.Schema):
    """
    config schema
    """

    speed = SpeedField(required=True)
    beStrict = marshmallow.fields.Boolean(default=True, attribute='be_strict')


class CreateSchema(marshmallow.Schema):
    """
    create schema
    """

    resources = marshmallow.fields.Nested(ResourcesSchema())
    config = marshmallow.fields.Nested(ConfigSchema)


expected_schema = {
    'Node': {
        'type': 'object',
        'properties': {'geo': {'type': 'string'}, 'cores': {'type': 'integer', 'format': 'int32'}},
    },
    'Resources': {'type': 'object', 'properties': {'host': {'type': 'array', 'items': {'$ref': '#/Node'}}}},
    'Config': {
        'type': 'object',
        'properties': {
            'speed': {
                'type': 'string',
                'enum': ['FAST', 'SUPER_FAST', 'SUPER_PUPER_FAST'],
                'description': {'FAST': 'fast', 'SUPER_FAST': 'super-fast', 'SUPER_PUPER_FAST': 'super-puper-fast'},
            },
            'beStrict': {'type': 'boolean'},
        },
        'required': ['speed'],
    },
    'test': {'type': 'object', 'properties': {'resources': {'$ref': '#/Resources'}, 'config': {'$ref': '#/Config'}}},
}


def _to_js(py_dicts):
    return json.dumps(py_dicts, indent=4, sort_keys=True)


def _assert_schemas_equal(actual, expected):
    actual_js = _to_js(actual)
    expected_js = _to_js(expected)
    assert actual_js == expected_js


def test_tiny(app_config):
    """
    test with small schema (usefull in debug cases)
    """
    # pylint: disable=unused-argument, redefined-outer-name
    _assert_schemas_equal(
        schema_to_jsonschema(ResourcesSchema, 'test'),
        {
            'Node': {
                'type': 'object',
                'properties': {'geo': {'type': 'string'}, 'cores': {'type': 'integer', 'format': 'int32'}},
            },
            'test': {'type': 'object', 'properties': {'host': {'type': 'array', 'items': {'$ref': '#/Node'}}}},
        },
    )


def test_schema_with_regexp(app_config):
    """
    test with regexp validator
    """

    # pylint: disable=unused-argument, redefined-outer-name
    class _SWN(marshmallow.Schema):
        name = marshmallow.fields.Str(validate=marshmallow.validate.Regexp('[A-Z]+[a-z]+'))

    _assert_schemas_equal(
        schema_to_jsonschema(_SWN, 'S'),
        {
            'S': {
                'type': 'object',
                'properties': {
                    'name': {
                        'type': 'string',
                        'pattern': '[A-Z]+[a-z]+',
                    }
                },
            }
        },
    )


def test_vanilla_schema_to_jsonschema(app_config):
    """
    test for marshmallow.Schema
    """
    # pylint: disable=unused-argument, redefined-outer-name
    _assert_schemas_equal(schema_to_jsonschema(CreateSchema, 'test'), expected_schema)


class HiddenSection(marshmallow.Schema):
    """
    Hidden section
    """

    hidden = marshmallow.fields.Bool()


class CreateSchemaWithConsoleHiddenAndInternalFields(OurBaseSchema):
    """
    create schema
    """

    resources = marshmallow.fields.Nested(ResourcesSchema())
    config = marshmallow.fields.Nested(ConfigSchema)
    hidden_section = marshmallow.fields.Nested(HiddenSection, console_hidden=True)
    internal_section = marshmallow.fields.Nested(HiddenSection, internal=True)
    hidden_field = marshmallow.fields.Bool(console_hidden=True)
    internal_field = marshmallow.fields.Bool(internal=True)


def test_for_our(app_config):
    """
    test for our base schema with console_hidden and internal fields
    """
    # pylint: disable=unused-argument, redefined-outer-name
    _assert_schemas_equal(schema_to_jsonschema(CreateSchemaWithConsoleHiddenAndInternalFields, 'test'), expected_schema)


class ExtSchema(marshmallow.Schema):
    name = marshmallow.fields.Str(required=True)


class DatabaseSchema(marshmallow.Schema):
    extentions = marshmallow.fields.Nested(ExtSchema, many=True)


class ClusterSchema(marshmallow.Schema):
    databases = marshmallow.fields.Nested(DatabaseSchema, many=True)


cluster_expected_schema = {
    "Database": {"properties": {"extentions": {"items": {"$ref": "#/Ext"}, "type": "array"}}, "type": "object"},
    "Ext": {"properties": {"name": {"type": "string"}}, "required": ["name"], "type": "object"},
    "test": {"properties": {"databases": {"items": {"$ref": "#/Database"}, "type": "array"}}, "type": "object"},
}


def test_nested_with_many(app_config):
    """
    test with nested arrays
    """
    # pylint: disable=unused-argument, redefined-outer-name
    _assert_schemas_equal(schema_to_jsonschema(ClusterSchema, 'test'), cluster_expected_schema)


def test_dict_handle(app_config):
    """
    test dict
    """

    # pylint: disable=unused-argument, redefined-outer-name
    class _S(marshmallow.Schema):
        labels = marshmallow.fields.Dict()

    _assert_schemas_equal(
        schema_to_jsonschema(_S, 'test'),
        {
            'test': {
                'properties': {
                    'labels': {
                        'type': 'object',
                    }
                },
                'type': 'object',
            }
        },
    )

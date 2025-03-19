# -*- coding: utf-8 -*-
"""
Extensions to flask_apispec.
"""
from apispec import APISpec
from apispec.ext.marshmallow import MarshmallowPlugin
from apispec.ext.marshmallow.common import get_fields  # type: ignore
from apispec.ext.marshmallow.openapi import OpenAPIConverter
from flask import current_app

from ..config import get_internal_schema_fields_expose


def get_unfiltered_fields(schema):
    """
    Get fields with filtering by `console_hidden` and `internal` tags.
    """
    result = {}
    expose_internal_fields = get_internal_schema_fields_expose()
    for key, value in get_fields(schema).items():
        if value.metadata.get('console_hidden'):
            continue

        if not expose_internal_fields and value.metadata.get('internal'):
            continue

        result[key] = value

    return result


class FilteringOpenAPIConverter(OpenAPIConverter):
    """
    Marshmallow openapi converter with filter for internal fields
    """

    # pylint: disable=arguments-differ,unexpected-keyword-arg

    def schema2jsonschema(self, schema):
        """
        Return the JSON Schema for a given marshmallow Schema with internal fields filter
        """
        fields = get_unfiltered_fields(schema)
        meta = getattr(schema, "Meta", None)
        partial = getattr(schema, "partial", None)
        ordered = getattr(schema, "ordered", False)

        jsonschema = self.fields2jsonschema(fields, partial=partial, ordered=ordered)

        if hasattr(meta, "title"):
            jsonschema["title"] = meta.title
        if hasattr(meta, "description"):
            jsonschema["description"] = meta.description

        return jsonschema


class FilteringMarshmallowPlugin(MarshmallowPlugin):
    """
    APISpec plugin with filter for internal fields
    """

    # pylint: disable=arguments-differ,unexpected-keyword-arg

    def init_spec(self, spec):
        """
        Special version of init spec with openapi converter replace
        """
        super().init_spec(spec)
        self.openapi = FilteringOpenAPIConverter(
            openapi_version=spec.openapi_version,
            schema_name_resolver=self.schema_name_resolver,
            spec=spec,
        )


def _fix_schema_refs(schema):
    """
    Fix $ref in jsonschema for cluster modify helper
    """
    for key, value in schema.items():
        if key == '$ref':
            schema[key] = value.replace('/components/schemas', '')
        elif isinstance(value, dict):
            schema[key] = _fix_schema_refs(value)

    return schema


def schema_to_jsonschema(schema, name):
    """
    convert marshmallow schema to jsonschema
    """
    # pylint: disable=no-member
    spec = APISpec(**current_app.config['APISPEC_SPEC'], plugins=[FilteringMarshmallowPlugin()])
    spec.components.schema(name=name, schema=schema)
    return _fix_schema_refs(spec.components.to_dict()['schemas'])

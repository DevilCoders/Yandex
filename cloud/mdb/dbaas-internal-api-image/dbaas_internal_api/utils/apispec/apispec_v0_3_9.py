# -*- coding: utf-8 -*-
"""
Extention for apispec 0.39.0
"""

import copy
from collections import OrderedDict

import marshmallow
from apispec import APISpec
from apispec.ext.marshmallow import MarshmallowPlugin
from apispec.ext.marshmallow.openapi import OpenAPIConverter
from flask import current_app

from dbaas_internal_api.apis.schemas import fields as our_fields

from ...modules.postgres.fields import PostgresqlSizeKB, PostgresqlSizeMB, PostgresqlTimespanMs
from ..config import get_internal_schema_fields_expose


def _get_fields(schema):
    """
    Return fields from schema

    backported from apispec 1.33
    apispec.ext.marshmallow.common.get_fields
    """
    if hasattr(schema, "fields"):
        fields = schema.fields
    elif hasattr(schema, "_declared_fields"):
        # pylint: disable=protected-access
        fields = copy.deepcopy(schema._declared_fields)
    else:
        raise ValueError("{!r} doesn't have either `fields` or `_declared_fields`.".format(schema))
    Meta = getattr(schema, "Meta", None)  # pylint: disable=invalid-name
    return _filter_excluded_fields(fields, Meta)


def _filter_excluded_fields(fields, Meta):  # pylint: disable=invalid-name
    """
    Filter fields that should be ignored in the OpenAPI spec

    backported from apispec 1.33
    apispec.ext.marshmallow.common.filter_excluded_fields
    """
    exclude = list(getattr(Meta, "exclude", []))
    filtered_fields = OrderedDict((key, value) for key, value in fields.items() if key not in exclude)

    return filtered_fields


def get_unfiltered_fields(schema):
    """
    Get fields with filtering by `console_hidden` and `internal` tags.
    """
    result = {}
    expose_internal_fields = get_internal_schema_fields_expose()
    for key, value in _get_fields(schema).items():
        if value.metadata.get('console_hidden'):
            continue

        if not expose_internal_fields and value.metadata.get('internal'):
            continue

        result[key] = value

    return result


def _nested2name(nested_field):
    name = nested_field.schema.__class__.__name__
    if name.endswith('Schema'):
        name = name[: -len('Schema')]
    return name


class _OpenAPIConverterBackports(OpenAPIConverter):
    """
    Workaround for old apispec in A.y-t
    """

    # pylint: disable=arguments-differ,unexpected-keyword-arg

    def __init__(self, openapi_version, spec):
        super().__init__(openapi_version)

        # apispec 1.3.3 compatibility
        self.spec = spec
        # handle our units as Int
        for our_custom_int in [
            our_fields.UInt,
            our_fields.GrpcInt,
            our_fields.GrpcUInt,
            PostgresqlSizeKB,
            PostgresqlSizeMB,
            PostgresqlTimespanMs,
            our_fields.MillisecondsMappedToSeconds,
        ]:
            self.field_mapping[our_custom_int] = self.field_mapping[marshmallow.fields.Int]
        for our_custom_bool in [
            our_fields.IntBoolean,
            our_fields.BooleanMappedToInt,
        ]:
            self.field_mapping[our_custom_bool] = self.field_mapping[marshmallow.fields.Boolean]
        self.field_mapping[marshmallow.fields.Dict] = ('object', None)
        self.field_mapping[our_fields.Labels] = ('object', None)
        # remember what schemas we add
        # as key we store name, as value we store class
        self._added_refs = {}

    def field2property(self, field, use_refs=True, dump=True, name=None):
        """
        Backport of Refs logic from 1.3.3
        """

        # https://github.com/marshmallow-code/apispec/issues/354
        # References for nested Schema are stored automatically.
        if isinstance(field, marshmallow.fields.Nested):
            nested_name = _nested2name(field)
            # for Nested(many) store schema for array item
            field_type = field.nested if field.metadata.get('many') else field.schema
            if not isinstance(field_type, type):
                field_type = field_type.__class__

            # In some cases (same class name in diffrent modules)
            # we can generate same ref schema.
            # So store ref_name with type.
            for cls_suffix in range(1, 100):
                if nested_name in self._added_refs:
                    if self._added_refs[nested_name] is not field_type:
                        nested_name = _nested2name(field) + str(cls_suffix)
                        continue
                break
            else:
                raise RuntimeError(f'Unable to create unique ref name for {field}. Last is {nested_name}')

            field.metadata['ref'] = f'#/{nested_name}'
            if nested_name not in self._added_refs:
                self._added_refs[nested_name] = field_type
                self.spec.definition(name=nested_name, schema=field_type)

        props = super().field2property(field=field, use_refs=use_refs, dump=dump, name=name)
        return self.__handle_regexps(props, field)

    def __handle_regexps(self, props, field):
        """
        backport regexp handles
        """
        # Add support for outputting field patterns from Regexp validators
        # https://github.com/marshmallow-code/apispec/pull/364
        regex_validators = [v for v in field.validators if isinstance(v, marshmallow.validate.Regexp)]
        if regex_validators:
            props['pattern'] = regex_validators[0].regex.pattern
        return props


class FilteringOpenAPIConverter(_OpenAPIConverterBackports):
    """
    Marshmallow openapi converter with filter for internal fields
    """

    # pylint: disable=arguments-differ,unexpected-keyword-arg

    def schema2jsonschema(self, schema, use_refs=True, dump=True, name=None):
        """
        Return the JSON Schema for a given marshmallow Schema with internal fields filter
        """
        fields = get_unfiltered_fields(schema)
        meta = getattr(schema, "Meta", None)

        jsonschema = self.fields2jsonschema(fields, schema=schema, use_refs=use_refs, dump=dump, name=name)

        if hasattr(meta, "title"):
            jsonschema["title"] = meta.title
        if hasattr(meta, "description"):
            jsonschema["description"] = meta.description

        # return copy of jsonschema,
        # cause it resolve lazily.
        # So our hack with nested schema references not works
        return copy.deepcopy(jsonschema)


class FilteringMarshmallowPlugin(MarshmallowPlugin):
    """
    APISpec plugin with filter for internal fields
    """

    def init_spec(self, spec):
        """
        Special version of init spec with openapi converter replace
        """
        super().init_spec(spec)
        self.openapi = FilteringOpenAPIConverter(openapi_version=spec.openapi_version, spec=spec)


def schema_to_jsonschema(schema, name):
    """
    Convert marshmallow schema to jsonschema
    """
    # pylint: disable=no-member
    spec = APISpec(**current_app.config['APISPEC_SPEC'], plugins=[FilteringMarshmallowPlugin()])
    spec.definition(name=name, schema=schema)
    spec_ret = spec.to_dict()
    try:
        schema_spec = spec_ret['components']['schemas']
    except (KeyError, TypeError) as exc:
        raise RuntimeError(
            'Unexpected APISpec.to_dict() return. We expect dict with preset $.componets.schemas path'
        ) from exc
    return schema_spec

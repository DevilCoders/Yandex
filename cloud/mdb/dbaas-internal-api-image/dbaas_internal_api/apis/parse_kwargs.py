"""
DBaaS Internal API marshal Args parse utils
"""
import enum
from functools import wraps
from typing import Any, Callable, Iterable, Tuple, Type

from marshmallow import Schema
from webargs import flaskparser

from ..utils.register import DbaasOperation, Resource, get_request_schema


def _assert_is_schema_type(schema) -> None:
    if not isinstance(schema, type):
        raise RuntimeError(f'Schema paramer should be a class not a instance: {schema}')
    if not issubclass(schema, Schema):
        raise RuntimeError('Should be marshmallow.Schema subclass')


def _parse_args(schema: Schema, locations: Tuple[str]) -> Any:
    return flaskparser.parser.parse(schema, locations=locations)


class Locations(enum.Flag):
    """
    Request locations to parse
    """

    query = enum.auto()
    json = enum.auto()
    form = enum.auto()
    default = query | json | form


def _encode_location(arg_location: Locations) -> Iterable[str]:
    if arg_location & Locations.query:
        yield 'query'
    if arg_location & Locations.json:
        yield 'json'
    if arg_location & Locations.form:
        yield 'form'


class with_schema:  # pylint: disable=invalid-name
    """
    Inject keyword arguments from the specified schema into the
    decorated view function.
    """

    def __init__(self, schema_type: Type[Schema], locations: Locations = Locations.default) -> None:
        _assert_is_schema_type(schema_type)
        self.schema = schema_type(strict=True)
        self.locations = tuple(_encode_location(locations))

    def __call__(self, fun: Callable) -> Callable:
        @wraps(fun)
        def _wrapper(*args, **kwargs):
            request_kwargs = _parse_args(self.schema, self.locations)
            fun_kwargs = dict(**kwargs, **request_kwargs)
            return fun(*args, **fun_kwargs)

        return _wrapper


class with_resource:  # pylint: disable=invalid-name
    """
    Inject keyword arguments from the schema
    defined by resource, resource_operation and cluster_type
    into the decorated view function.
    """

    def __init__(
        self, resource: Resource, resource_operation: DbaasOperation, locations: Locations = Locations.default
    ) -> None:
        self.resource = resource
        self.resource_operation = resource_operation
        self.locations = tuple(_encode_location(locations))

    def __call__(self, fun: Callable) -> Callable:
        @wraps(fun)
        def _wrapper(*args, **kwargs):
            try:
                cluster_type = kwargs['cluster_type']
            except KeyError:
                raise RuntimeError('Unable to find cluster_type in kwargs')

            schema_type = get_request_schema(cluster_type, self.resource, self.resource_operation)
            schema_obj = schema_type(strict=True)
            request_kwargs = _parse_args(schema_obj, self.locations)

            # pass _schema too, some our handlers require it
            func_kwargs = dict(
                _schema=schema_obj,
                **kwargs,
                **request_kwargs,
            )

            return fun(*args, **func_kwargs)

        return _wrapper

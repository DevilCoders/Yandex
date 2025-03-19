# coding: utf-8
"""
DBaaS Internal API marshal helpers
"""

from functools import wraps
from typing import Any, Callable, Type

from marshmallow import Schema

from ..utils.register import DbaasOperation, Resource, get_response_schema


def _marshal_result(result: Any, schema: Schema) -> Any:
    dumped = schema.dump(result)
    output = dumped.data
    return output


class with_schema:  # pylint: disable=invalid-name
    """
    Marshal the return value of the decorated view function using the
    specified schema.
    """

    def __init__(self, schema_type: Type[Schema]) -> None:
        self.schema = schema_type()

    def __call__(self, fun: Callable) -> Callable:
        @wraps(fun)
        def _wrapper(*args, **kwargs):
            result = fun(*args, **kwargs)
            return _marshal_result(result, self.schema)

        return _wrapper


class with_resource:  # pylint: disable=invalid-name
    """
    Marshal the return value of the decorated view function using the
    schema defined by resource, operation and cluster_type (defined from request)
    """

    def __init__(self, resource: Resource, resource_operation: DbaasOperation) -> None:
        self.resource = resource
        self.resource_operation = resource_operation

    def __call__(self, fun: Callable) -> Callable:
        @wraps(fun)
        def _wrapper(*args, **kwargs):
            result = fun(*args, **kwargs)
            # It's not a bug, that I get cluster_type, after handler.
            # It's handler responsibility to raise BadRequest
            # for unsupported (cluster_type, resource, operation) combinations
            try:
                cluster_type = kwargs['cluster_type']
            except KeyError:
                raise RuntimeError('Unable to find cluster_type in kwargs')

            schema_type = get_response_schema(cluster_type, self.resource, self.resource_operation)
            return _marshal_result(result, schema_type())

        return _wrapper

"""
Defines common schemas like listing or base objects.
"""
from marshmallow import Schema
from marshmallow.fields import Dict, Int, List
from marshmallow.validate import Range

from .fields import Filter, Str


class ErrorSchemaV1(Schema):
    """
    Error schema.
    """

    code = Int()
    message = Str()
    details = List(Dict())


class ListResponseSchemaV1(Schema):
    """
    Base schema for list responses.
    """

    nextPageToken = Str(
        required=False, attribute='next_page_token', description='Token that can be used to display next page.'
    )


class ListRequestSchemaV1(Schema):
    """
    Base schema for list requests.
    """

    pageSize = Int(
        attribute='page_size',
        description='Number of results per page.',
        default=100,
        validate=Range(min=1, max=1000),
    )
    pageToken = Str(attribute='page_token', description='Token to request the next page in listing.')


class FilteredListRequestSchemaV1(ListRequestSchemaV1):
    """
    Schema for list request with filter and pagination
    """

    filters = Filter()


class ListRequestLargeOutputSchemaV1(Schema):
    """
    Schema for list requests of large byte output.
    Listing hadoop job, for example.
    """

    pageSize = Int(
        attribute='page_size',
        description='Number of results per page.',
        missing=2**20,
        default=2**20,
        validate=Range(min=1, max=2**20),
    )
    pageToken = Str(attribute='page_token', description='Token to request the next page in listing.')

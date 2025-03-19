# -*- coding: utf-8 -*-
"""
DBaaS Internal API helpers
"""

from base64 import b64decode, b64encode
from functools import wraps
from typing import Any, Callable, Dict, Optional, Sequence, TypeVar  # noqa

import aniso8601
from flask_restful import abort


# flake8: Possible hardcoded password ...
NEXT_PAGE_TOKEN_FIELD = 'next_page_token'  # noqa
PAGE_TOKEN_FIELD = 'page_token'  # noqa
PAGE_SIZE_FIELD = 'page_size'
DEFAULT_PAGE_SIZE = 100

T = TypeVar('T')  # pylint: disable=invalid-name


class Column:
    """
    Dict item as column
    """

    def __init__(self, field: str, field_type: Callable[[str], T]) -> None:
        self.field = field
        assert callable(field_type), 'field type must be a callable that returns normalized value'
        self.field_type = field_type

    def as_page_token(self, element: dict) -> str:
        """
        Make page_token from element
        """
        return str(element[self.field])

    def from_page_token(self, token_value: str) -> T:
        """
        Cast token value from str
        """
        return self.field_type(token_value)


class AttributeColumn(Column):
    """
    Attribute as column
    """

    def as_page_token(self, element: T) -> str:
        """
        Make page_token from element
        """
        return str(getattr(element, self.field))


def date(value):
    """Force correct date"""
    return aniso8601.parse_datetime(value, delimiter=' ')


def parse_page_token(columns: Sequence[Column], page_token: Optional[str]) -> dict:
    """
    Parse a page token:
        if set, must be 'last_element?more_element?...'
    returns page_size and last element tokens
    """
    tokens = {}  # type: Dict[str, Any]
    # No token set. First page?
    if page_token is None:
        # Empty dict -- output is expected to be used in kwargs` update
        # like this -- **parse_page_token(...), in other words,
        # use functions` defaults if no token.
        return tokens

    try:
        # Number of columns can vary, we need to account for that
        # fact.
        col_kvs = b64decode(page_token.encode('utf8')).decode('utf8').split('?', len(columns) + 1)
        for col_val, col_spec in zip(col_kvs, columns):
            # e.g. "page_token_cid": <str obj>,
            # "page_token_created_at": <datetime obj>
            tokens['page_token_{0}'.format(col_spec.field)] = col_spec.from_page_token(col_val)
    except (ValueError, AttributeError, TypeError, KeyError):
        # Bail out if we are unable to read directions.
        abort(400, message='Invalid pagination token')

    # e.g.:
    # {
    #     'page_token_name': <str>,
    #     'page_token_created_at': <datetime obj>,
    # }
    return tokens


def form_page_token(columns: Sequence[Column], res: Any) -> str:
    """Form page token using columns specification"""
    token_fields = []
    # Construct token
    for col_spec in columns:
        token_fields.append(col_spec.as_page_token(res))
    token_bytes = b64encode('?'.join(token_fields).encode('utf8'))
    return str(token_bytes, 'ascii')


def supports_pagination(columns: Sequence[Column], items_field: str) -> Callable:
    """
    Decorator that translates a flat list of ordered elements into
    a dictionary {'items': <list>, <page_token_field>: <next_page_token>},
    also taking care of parsing and filling those tokens.
    """
    assert columns, 'columns must not be empty'
    assert isinstance(columns, (list, tuple)), 'columns arg must be a list or tuple'
    for col in columns:
        assert isinstance(col, Column), 'column must be an instance of Column'

    def wrapper(fun):
        """Internal wrapper"""

        @wraps(fun)
        def pagination_wrapper(*args, **kwargs):
            """
            Token is None: (Assuming it to be "first page case")
            1. Pass None as last elements.
            2. Upon getting a return value (which is a list of items), form a
               page_token. For that we need the last element and current
               page_size.

            If token is set (and correct): (Assuming we are in the middle)
            1. Pass last element in kwargs (assuming the function "will do the
               right thing" with it.
            2. Upon getting a return value (a list of elements), extract the
               last value and put it in the token.
            """
            page_size = kwargs.pop(PAGE_SIZE_FIELD, DEFAULT_PAGE_SIZE)
            page_token = kwargs.pop(PAGE_TOKEN_FIELD, None)
            tokens = parse_page_token(columns, page_token)
            # Update kwargs with tokens and limit
            kwargs.update(tokens)
            # Add one to use as a marker so we can check if there is
            # any data left
            kwargs['limit'] = page_size + 1

            # Execute actual func
            res = fun(*args, **kwargs)

            # Any data left? Do we need a next_page_token?
            if len(res) <= page_size:
                return {
                    items_field: res,
                }

            # Construct token
            try:
                # Remove extra result that was used as a marker
                res = res[:-1]
                # Form token from the last result
                # It is expected to be used in '>' comparison
                token = form_page_token(columns, res[-1])
                return {
                    items_field: res,
                    NEXT_PAGE_TOKEN_FIELD: token,
                }
            except BaseException:
                abort(400, message='Failed to form pagination token')

        return pagination_wrapper

    return wrapper

"""
Command-line parameters.
"""

import json
import os
import re
import sys
from json import JSONDecodeError

import click
from click import ClickException

import dateutil.parser
import humanfriendly
from cloud.mdb.cli.common.utils import parse_timestamp, Nullable


class ListParamType(click.ParamType):
    """
    Command-line parameter type for lists.
    """

    name = 'list'

    def __init__(self, type=None, separator=r'[,\s]+'):
        self.type = type
        self.separator = separator

    def convert(self, value, param, ctx):
        value = self._preprocess(value)
        result = [v.strip() for v in re.split(self.separator, value) if v]

        if self.type:
            if isinstance(self.type, click.ParamType):
                result = [self.type(v, param=param, ctx=ctx) for v in result]
            else:
                result = [self.type(v) for v in result]

        return result

    @staticmethod
    def _preprocess(value):
        if value == '-':
            return sys.stdin.read()

        if value.startswith('@'):
            with open(os.path.expanduser(value[1:])) as f:
                return f.read()

        return value


class NullableParamType(click.ParamType):
    """
    Command-line parameter for nullable values. It allows to designate the cases: unset value vs. do not change value.
    """

    name = 'nullable'

    def __init__(self, type=None, null_values=('null', 'none')):
        self.type = type
        self.null_values = null_values

    def convert(self, value, param, ctx):
        if value in self.null_values:
            value = None
        else:
            if self.type:
                if isinstance(self.type, click.ParamType):
                    value = self.type(value, param=param, ctx=ctx)
                else:
                    value = self.type(value)

        return Nullable(value)


class FieldsParamType(ListParamType):
    """
    Command-line parameter type for fields filter.
    """

    name = 'fields'

    def __init__(self, possible_values=None):
        if possible_values:
            field_type = click.Choice(possible_values + ['all'])
        else:
            field_type = str
        super().__init__(field_type)

    def convert(self, value, param, ctx):
        if value == 'all':
            return None

        return super().convert(value, param, ctx)


class JsonParamType(click.ParamType):
    """
    Command-line parameter type for JSON values.
    """

    name = 'json'

    def convert(self, value, param, ctx):
        try:
            value = self._preprocess(value)
            if re.fullmatch(r'\s*([\[{"].*|true|false|null|\d+(\.\d+)?)\s*', value, re.MULTILINE | re.DOTALL):
                return json.loads(self._preprocess(value))
            else:
                return value.strip()
        except JSONDecodeError as e:
            raise ClickException(f'Invalid JSON value for the parameter "{param.name}". {str(e)}')

    @staticmethod
    def _preprocess(value):
        if value == '-':
            return sys.stdin.read()

        if value.startswith('@'):
            with open(os.path.expanduser(value[1:])) as f:
                return f.read()

        return value


class StringParamType(click.ParamType):
    """
    Command-line parameter type for string values. It supports reading from file and stdin.
    """

    name = 'string'

    def convert(self, value, param, ctx):
        if value == '-':
            return sys.stdin.read()

        if value.startswith('@'):
            with open(os.path.expanduser(value[1:])) as f:
                return f.read()

        return value


class DateTimeParamType(click.ParamType):
    """
    Command-line parameter type for timestamp values.
    """

    name = 'datetime'

    def convert(self, value, param, ctx):
        try:
            return parse_timestamp(ctx, value)
        except dateutil.parser.ParserError as e:
            raise ClickException(f'Invalid timestamp value for the parameter "{param.name}": {str(e)}')


class BytesParamType(click.ParamType):
    """
    Command-line parameter type for bytes values.
    """

    name = 'bytes'

    def convert(self, value, param, ctx):
        if isinstance(value, str):
            return humanfriendly.parse_size(value, binary=True)
        else:
            return value

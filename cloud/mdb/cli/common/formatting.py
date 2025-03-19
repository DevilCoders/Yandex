"""
Formatting module.
"""

import csv
import json
import sys
from collections import OrderedDict
from datetime import datetime, timedelta
from decimal import Decimal
from itertools import chain
from typing import Mapping

from google.protobuf.json_format import MessageToDict
from google.protobuf.message import Message
from psycopg2.extras import DictRow, Range
from tabulate import tabulate

import humanfriendly
from cloud.mdb.cli.common.utils import get_timezone
from cloud.mdb.cli.common.yaml import dump_yaml
from deepdiff.helper import notpresent
from pygments import highlight
from pygments.formatters.terminal256 import Terminal256Formatter
from pygments.lexers.data import JsonLexer, YamlLexer
from pygments.style import Style
from pygments.token import Token
from termcolor import colored


class FormatStyle(Style):
    styles = {
        Token.Name.Tag: 'bold ansibrightblue',
        Token.Punctuation: 'bold ansiwhite',
        Token.String: 'ansigreen',
    }


def print_header(header):
    print(header)
    print('-' * len(header))


def print_response(
    ctx,
    value,
    format=None,
    default_format='json',
    field_formatters=None,
    table_formatter=None,
    fields=None,
    ignored_fields=None,
    quiet=None,
    id_key=None,
    separator=None,
    limit=None,
):
    if format is None:
        format = ctx.obj.get('format') or default_format

    if separator is None:
        separator = ','
    else:
        separator = separator.replace(r'\n', '\n')

    value = _purify_value(ctx, value, formatters=field_formatters, include_keys=fields, exclude_keys=ignored_fields)

    if limit and isinstance(value, list):
        value = value[:limit]

    if quiet:
        if id_key is None:
            id_key = 'id'

        if isinstance(value, list):
            result = separator.join(item[id_key] for item in value)
        else:
            result = value[id_key]

        print(result)
        return

    if format in ('table', 'csv'):
        if table_formatter:
            value = [table_formatter(v) for v in value]

        if format == 'table':
            print_table(value)
        else:
            print_csv(value)

    elif format == 'yaml':
        print_yaml(value)

    else:
        print_json(value)


def _purify_value(ctx, value, formatters=None, include_keys=None, exclude_keys=None):
    if isinstance(value, Message):
        return _purify_value(
            ctx, MessageToDict(value), formatters=formatters, include_keys=include_keys, exclude_keys=exclude_keys
        )

    if isinstance(value, (DictRow, Mapping)):
        result = OrderedDict()
        for key in include_keys or value.keys():
            if exclude_keys and key in exclude_keys:
                continue

            item = value[key]

            formatter = formatters.get(key) if formatters else None
            item = formatter(item) if formatter else _purify_value(ctx, item)

            result[key] = item

        return result

    if isinstance(value, list):
        return [
            _purify_value(ctx, item, formatters=formatters, include_keys=include_keys, exclude_keys=exclude_keys)
            for item in value
        ]

    if isinstance(value, Range):
        return f'{value.lower}-{value.upper}' if value else None

    if isinstance(value, datetime):
        return format_timestamp(ctx, value)

    if isinstance(value, timedelta):
        return str(value)

    if isinstance(value, Decimal):
        return str(value)

    return value


def print_diff(diff, key_separator='.'):
    """
    Print structural diff between 2 values.
    """
    if not diff:
        return

    items = chain.from_iterable(diff.values())
    for item in sorted(items, key=lambda i: i.path(output_format='list')):
        _print_diff_item(item, key_separator=key_separator)


def _print_diff_item(item, key_separator):
    item_path = item.path(output_format='list')
    if item_path:
        print('@ ' + key_separator.join(str(value) for value in item_path))

    if item.t1 is not notpresent:
        _print_diff_item_value(item.t1, '- ', 'red')

    if item.t2 is not notpresent:
        _print_diff_item_value(item.t2, '+ ', 'green')


def _print_diff_item_value(value, prefix, color):
    value = json.dumps(value, indent=2, ensure_ascii=False)
    value = '\n'.join(f'{prefix}{line}' for line in value.splitlines())
    if sys.stdout.isatty():
        value = colored(value, color=color)
    print(value)


def print_json(value):
    json_dump = json.dumps(value, indent=2, ensure_ascii=False)
    if sys.stdout.isatty():
        print(highlight(json_dump, JsonLexer(), Terminal256Formatter(style=FormatStyle)), end='')
    else:
        print(json_dump)


def print_yaml(value):
    yaml_dump = dump_yaml(value)
    if sys.stdout.isatty():
        print(highlight(yaml_dump, YamlLexer(), Terminal256Formatter(style=FormatStyle)), end='')
    else:
        print(yaml_dump)


def print_table(value):
    print(tabulate(value, headers='keys'))


def print_csv(value):
    if value:
        writer = csv.DictWriter(sys.stdout, fieldnames=value[0].keys())
        writer.writeheader()
        writer.writerows(value)


def format_list(value):
    return ','.join(value)


def format_bytes(value):
    if value is None:
        return None

    if isinstance(value, str):
        value = int(value)

    if value > 0:
        return humanfriendly.format_size(value, binary=True)
    elif value < 0:
        return '-{0}'.format(humanfriendly.format_size(-value, binary=True))
    else:
        return '0'


def format_bytes_per_second(value):
    if value is None:
        return None

    if value != 0:
        return f'{format_bytes(value)}/s'
    else:
        return '0'


def format_timestamp(ctx, value):
    value = value.astimezone(get_timezone(ctx))
    result = value.strftime('%Y-%m-%d %H:%M:%S')
    result += f'.{int(value.microsecond / 1000):03d}'
    return result


def format_duration(value):
    return humanfriendly.format_timespan(value)

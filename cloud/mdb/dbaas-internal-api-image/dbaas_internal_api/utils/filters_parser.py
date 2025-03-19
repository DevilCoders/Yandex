# coding: utf-8
"""
Filters language parser
"""

import datetime
import re
import sys
from enum import Enum, unique
from pathlib import Path
from typing import List, NamedTuple, Union

import aniso8601
from lark import Lark, Transformer
from lark import UnexpectedInput as LarkUnexpectedInput

import library.python.resource as _ya_res


@unique
class Operator(Enum):
    """
    Operators enum
    """

    equals = '='
    not_equals = '!='
    less = '<'
    less_or_equals = '<='
    greater = '>'
    greater_or_equals = '>='
    in_ = 'IN'
    not_in = 'NOT IN'


ValueT = Union[str, int, bool, datetime.date, datetime.datetime]

Filter = NamedTuple(
    'Filter', [('attribute', str), ('operator', Operator), ('value', Union[ValueT, List[ValueT]]), ('filter_str', str)]
)

_root_path = Path(__file__).parent.parent


class ResouceNotFoundError(Exception):
    """
    resource not found
    """


class FilterSyntaxError(Exception):
    """
    Filter syntax error
    """

    def __str__(self):
        # pylint: disable=unpacking-non-sequence
        context, _, column, error_details = self.args
        if error_details:
            error_details = ' (%s)' % error_details
        return ('Filter syntax error{details} ' 'at or near {column}.\n{context}').format(
            details=error_details or '', column=column, context=context.rstrip()
        )


_ERROR_SAMPLES = {
    'invalid attribute name': [
        '42 = 42',
        "'foo' = 42",
    ],
    'missing attribute name': [
        '',
        '  ',
        'foo = 1 AND',
    ],
    'missing operator': [
        'foo',
        'foo ',
        'a = 1 AND b ',
    ],
    'unexpected operator': [
        'foo <> 1',
        'a <>',
    ],
    'missing value': [
        'foo = ',
        'foo=',
        'bar != ',
        'a IN ',
    ],
}


def unescape_string(string):
    r"""
    Unescape string.

    Replace:
        \\  -> \
        \\t -> \t
        \\r -> \r
        \\n -> \n

    Any other \c replaced with c
    """
    # don't use python's builtin string format,
    # cause it do additional replaments like '\b', '\v' ...
    unescaped = string
    for from_char, to_char in ((r'\t', '\t'), (r'\n', '\n'), (r'\r', '\r')):
        unescaped = unescaped.replace(from_char, to_char)

    # finally replace any \char to char
    # include our special case like:
    #  \' -> ', \" -> ", \\ -> \    ""
    return re.sub(r'\\(.)', r'\1', unescaped)


class _LarkFiltersTransformer(Transformer):
    """
    Transform ast rules to types
    """

    def __init__(self, text):
        super().__init__(visit_tokens=False)
        self._text = text

    def number(self, token):
        """
        Make number
        """
        return int(token[0])

    def string(self, token):
        """
        Make string.
        Remove leading and trailing commas
        """
        return unescape_string(token[0][1:-1])

    values_list = list

    def boolean(self, token):
        """
        Make boolean
        """
        return token[0].lower() == 'true'

    def filter(self, tree):
        """
        Make filter
        """
        attribute_name_tree, operator_and_value_tree = tree

        filter_start_col = attribute_name_tree.column - 1
        filter_end_col = operator_and_value_tree.end_column - 1
        filter_str = self._text[filter_start_col:filter_end_col]

        attribute = attribute_name_tree.children[0]
        operator, value = operator_and_value_tree.children
        return Filter(
            attribute=str(attribute), operator=Operator[operator.data], value=value, filter_str=filter_str.strip()
        )

    def filters_list(self, rule_list):
        """
        Make filter list

        Remove anything except Filters from rules
        """
        return [r for r in rule_list if isinstance(r, Filter)]

    def timestamp(self, tree):
        """
        Make date or datatime from
        """
        if len(tree) == 1:
            return aniso8601.parse_date(tree[0])
        return aniso8601.parse_datetime(''.join(str(tp) for tp in tree))


def _get_resource(resource_key: str) -> str:
    """
    get resource text for resource

    for tier0 int-api resource_key is path from dbaas_internal_api package
    """
    res_bytes = _ya_res.find(resource_key)
    if res_bytes is None:
        raise ResouceNotFoundError(f'Unable to find "{resource_key}" resource.' ' Verify that you add it to ya.make')
    return str(res_bytes, encoding='utf-8')


def _load_grammar():
    """
    Load and compile grammar
    """
    # use LALR parser, cause:
    # * Earley can't propagate postions
    # * LALR produce friendly errors
    gr_text = _get_resource('utils/filters_grammar.lark')
    return Lark(
        gr_text,
        parser='lalr',
        start='filters_list',
        propagate_positions=True,
    )


class _LarkFilterParser:
    """
    Filter parser
    """

    def __init__(self) -> None:
        self._grammar = _load_grammar()

    def _error_details(self, l_exc: LarkUnexpectedInput) -> str:
        try:
            return l_exc.match_examples(self._grammar.parse, _ERROR_SAMPLES)
        except AssertionError:
            return ''

    def parse(self, data: str) -> List[Filter]:
        """
        Parse filter string
        """
        try:
            tree = self._grammar.parse(data)
        except LarkUnexpectedInput as l_exc:
            line = l_exc.line  # type: ignore
            column = l_exc.column  # type: ignore
            raise FilterSyntaxError(l_exc.get_context(data), line, column, self._error_details(l_exc))
        transformer = _LarkFiltersTransformer(data)
        return transformer.transform(tree)


_default_parser = _LarkFilterParser()
parse = _default_parser.parse


def _main():
    filter_text = sys.stdin.read().rstrip()
    try:
        parsed = parse(filter_text)
        print(parsed)  # noqa: T001
    except FilterSyntaxError as exc:
        print(str(exc))  # noqa: T001


if __name__ == '__main__':
    _main()

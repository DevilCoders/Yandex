# -*- coding: utf-8 -*-
"""
TSKV Formatter tests
"""

import pytest
from hamcrest import assert_that, has_entries

from dbaas_common.tskv import tskv_escape


class Test_tskv_escape:  # pylint: disable=invalid-name
    """
    Test for tskv_escape helper
    """

    # pylint: disable=no-self-use, missing-docstring
    def test_string_wo_special_chars(self):
        assert tskv_escape('Is it works?') == 'Is it works?'

    @pytest.mark.parametrize(
        'val,escaped_val',
        [
            ('str with \\ slash', 'str with \\\\ slash'),
            ('str with \tab', 'str with \\tab'),
            ('str with \n\r\0 endlines', 'str with \\n\\r\\0 endlines'),
            ('str with = equals sign', 'str with \\= equals sign'),
        ],
    )
    def test_string_with_char_for_escape(self, val, escaped_val):
        assert tskv_escape(val) == escaped_val

    @pytest.mark.parametrize(
        'val,escaped_val',
        [
            (100, '100'),
            (100.1, '100.1'),
        ],
    )
    def test_escape_number(self, val, escaped_val):
        assert tskv_escape(val) == escaped_val

    def test_dict(self):
        assert tskv_escape({'foo': 'bar'}) == '{"foo": "bar"}'

    def test_list(self):
        assert tskv_escape(['foo', 'bar']) == "['foo', 'bar']"

    def test_generic_object(self):
        class GenericClassWithRepr:  # pylint: disable=too-few-public-methods
            """
            Generic class with repr
            """

            def __repr__(self):
                return 'I\t return => strange repr'

        assert tskv_escape(GenericClassWithRepr()) == 'I\\t return \\=> strange repr'


def formatted_to_dict(formatted):
    """
    Extract formatted tskv values to dict for better comparison
    """
    splitted = formatted.split('\t')
    return {x.split('=')[0]: x.split('=')[1] for x in splitted[1:]}


def test_basic_message(stream, logger):
    """
    Test with minimal logging message
    """
    logger.error('Test')
    formatted = stream.getvalue().strip()
    assert formatted.startswith('tskv\t')
    got = formatted_to_dict(formatted)
    assert_that(got, has_entries({'message': 'Test'}))


def test_num_format(stream, logger):
    """
    Test numbers formatting
    """
    logger.error(
        'See extra',
        extra={
            'integer_field': 123,
            'float_field': 0.5,
        },
    )
    formatted = stream.getvalue().strip()
    got = formatted_to_dict(formatted)
    assert_that(
        got,
        has_entries(
            {
                'integer_field': '123',
                'float_field': '0.5',
            }
        ),
    )


def test_dict_format(stream, logger):
    """
    Test dict formatting
    """
    logger.error(
        'See extra',
        extra={
            'some_dict': {
                'a': 1,
            },
        },
    )
    formatted = stream.getvalue().strip()
    got = formatted_to_dict(formatted)
    assert_that(got, has_entries({'some_dict': '{"a": 1}'}))

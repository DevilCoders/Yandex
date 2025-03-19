"""
Test for filters language
"""

from datetime import date, datetime, timedelta

import pytest
from dateutil.tz import tzoffset, tzutc

from dbaas_internal_api.utils.filters_parser import Filter, FilterSyntaxError, Operator, parse

# pylint: disable=invalid-name, missing-docstring


class Test_parse:
    @pytest.mark.parametrize(
        'flt_str',
        [
            'foo=1',
            'foo = 1',
            'foo\t=\t1',
            'foo \t = \t 1',
        ],
    )
    def test_condition_with_ws(self, flt_str):
        assert parse(flt_str) == [
            Filter('foo', Operator.equals, 1, flt_str),
        ]

    @pytest.mark.parametrize(
        'flt_str',
        [
            'foo != "bar"',
            "foo != 'bar'",
        ],
    )
    def test_string(self, flt_str):
        assert parse(flt_str) == [
            Filter('foo', Operator.not_equals, 'bar', flt_str),
        ]

    @pytest.mark.parametrize(
        'flt_str',
        [
            "banana <= 'apple\\'s'",
            'banana <= "apple\'s"',
        ],
    )
    def test_string_with_quote(self, flt_str):
        assert parse(flt_str) == [
            Filter('banana', Operator.less_or_equals, "apple's", flt_str),
        ]

    @pytest.mark.parametrize(
        'flt_str',
        [
            'foo > "bar\\"baz"',
        ],
    )
    def test_string_with_double_quote(self, flt_str):
        assert parse(flt_str) == [
            Filter('foo', Operator.greater, 'bar"baz', flt_str),
        ]

    @pytest.mark.parametrize(
        ['flt_str', 'value_str'],
        [
            ('f="bar \\t baz"', 'bar \t baz'),
            ('f="bar \\r baz"', 'bar \r baz'),
            ('f="bar \\n baz"', 'bar \n baz'),
            ('f="bar \\\\\\t baz"', 'bar \\\t baz'),
        ],
    )
    def test_string_with_special_escape(self, flt_str, value_str):
        assert parse(flt_str) == [
            Filter('f', Operator.equals, value_str, flt_str),
        ]

    @pytest.mark.parametrize(
        ['flt_str', 'value_str'],
        [
            ('f="bar \\v baz"', 'bar v baz'),
            ('f="bar \\b baz"', 'bar b baz'),
        ],
    )
    def test_string_with_disallowed_special_escape(self, flt_str, value_str):
        assert parse(flt_str) == [
            Filter('f', Operator.equals, value_str, flt_str),
        ]

    @pytest.mark.parametrize(
        ['flt_str', 'bool_value'],
        [
            ('f=true', True),
            ('f= TrUe', True),
            ('f= FALSE', False),
        ],
    )
    def test_bool(self, flt_str, bool_value):
        assert parse(flt_str) == [
            Filter('f', Operator.equals, bool_value, flt_str),
        ]

    def test_date(self):
        assert parse('day = 2018-05-24')[0].value == date(year=2018, month=5, day=24)

    def test_datetime_with_tz(self):
        assert parse('ts = 2018-05-24T01:01+03')[0].value == datetime(
            year=2018, month=5, day=24, hour=1, minute=1, tzinfo=tzoffset('msk', timedelta(hours=3))
        )

    def test_datetime_with_utc(self):
        assert parse('ts = 2018-05-24T01:01Z')[0].value == datetime(
            year=2018, month=5, day=24, hour=1, minute=1, tzinfo=tzutc()
        )

    def test_datetime_without_tz(self):
        assert parse('ts = 2018-05-24T01:01')[0].value == datetime(
            year=2018, month=5, day=24, hour=1, minute=1, tzinfo=None
        )

    def test_more_then_one_condition(self):
        assert parse('f != 1 AND g="g" and h >= 2021-01-01') == [
            Filter('f', Operator.not_equals, 1, 'f != 1'),
            Filter('g', Operator.equals, 'g', 'g="g"'),
            Filter('h', Operator.greater_or_equals, date(2021, 1, 1), 'h >= 2021-01-01'),
        ]

    @pytest.mark.parametrize(
        ['flt_str', 'operator'],
        [
            ('foo=1', Operator.equals),
            ('foo != 1', Operator.not_equals),
            ('foo < 100', Operator.less),
            ('foo <= 100', Operator.less_or_equals),
            ('foo > 100', Operator.greater),
            ('foo >= 0', Operator.greater_or_equals),
            ('foo IN (1, 2, 3)', Operator.in_),
            ('foo NOT IN (1, 2, 3)', Operator.not_in),
        ],
    )
    def test_operator(self, flt_str, operator):
        assert parse(flt_str)[0].operator == operator

    @pytest.mark.parametrize(
        ['broken_flt', 'error_re'],
        [
            (' ', 'missing attribute name'),
            ('name = 1 AND', 'missing attribute name'),
            ('name', 'missing operator'),
            ('foo <> 1', 'Filter syntax error'),
            ('name =', 'missing value'),
            ('name in 2012-01-01', 'Filter syntax error'),
            ('foo = 1 OR bar = 2', 'Filter syntax error'),
        ],
    )
    def test_fail_for_broken_filters(self, broken_flt, error_re):
        with pytest.raises(FilterSyntaxError, match=error_re):
            parse(broken_flt)

    def test_dont_allow_multiline_filters(self):
        with pytest.raises(FilterSyntaxError):
            parse(
                """name
            =
            1
            """
            )

    def test_highlight(self):
        error_str = """Filter syntax error (missing value) at or near 3.
ts=
  ^"""
        try:
            parse('ts=')
        except FilterSyntaxError as exc:
            assert str(exc) == error_str
        else:
            raise AssertionError('No exception raised')

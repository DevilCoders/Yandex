"""
Tests for our matchers
"""

import pytest
from hamcrest import assert_that, equal_to, greater_than, has_entry, has_length

from .matchers import JsonPathSyntaxError, at_path

# pylint: disable=missing-docstring, invalid-name


class TestMatchJsonPath:
    """
    Tests from MatchJsonPath
    """

    def test_raise_for_invalid_js_path(self):
        with pytest.raises(JsonPathSyntaxError, match='Invalid json path: <//XPath>'):
            at_path('//XPath')

    def test_matches_for_exists_items(self):
        assert_that(
            {
                'data': {
                    'clickhouse': {
                        'config': {
                            'log_level': 'debug',
                        },
                    },
                },
            },
            at_path('$.data.clickhouse.config'),
        )

    def test_maches_when_path_find_more_then_one_item(self):
        assert_that(
            [
                {'fqdn', 'foo'},
                {
                    'fqdn': 'bar',
                },
            ],
            at_path('$.[*].fqdn'),
        )

    def test_not_match_for_not_existed_item(self):
        error_strings = [
            'Expected: data exists at <\\$.xxx> path' "\\s+but: nothing found in <{'foo': 'bar'}>",
        ]
        with pytest.raises(
            AssertionError,
            match=''.join(error_strings),
        ):
            assert_that(
                {
                    'foo': 'bar',
                },
                at_path('$.xxx'),
            )

    def test_not_matched_item_with_additional_matcher(self):
        error_strings = [
            "Expected: 'warning' ",
            'at path <\\$.data.clickhouse.config.log_level>',
            "\\s+ but: was 'debug'",
        ]
        with pytest.raises(AssertionError, match=''.join(error_strings)):
            assert_that(
                {
                    'data': {
                        'clickhouse': {
                            'config': {
                                'log_level': 'debug',
                            },
                        },
                    },
                },
                at_path('$.data.clickhouse.config.log_level', equal_to('warning')),
            )

    def test_when_not_matched_one_element(self):
        error_strings = [
            'Expected: a dictionary containing',
            '.*',
            'at path <\\$.data.postgres.users\\.\\*>',
            "\\s+but: was <{'password': 'small-pass'}>",
        ]
        with pytest.raises(AssertionError, match=''.join(error_strings)):
            assert_that(
                {
                    'data': {
                        'postgres': {
                            'users': {
                                'bob': {
                                    'password': 'big-user-password',
                                },
                                'alice': {
                                    'password': 'small-pass',
                                },
                            },
                        },
                    },
                },
                at_path('$.data.postgres.users.*', has_entry('password', has_length(greater_than(10)))),
            )

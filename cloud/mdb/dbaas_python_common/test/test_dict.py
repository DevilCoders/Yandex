# -*- coding: utf-8 -*-
"""
Dict-related functions tests
"""

from hamcrest import assert_that, equal_to

from dbaas_common.dict import combine_dict


def test_nonintersect():
    """
    Test with 2 non-intersecting dicts
    """
    assert_that(combine_dict({'a': 1}, {'b': 2}), equal_to({'a': 1, 'b': 2}))


def test_substitution():
    """
    Test equal keys substitution
    """
    assert_that(combine_dict({'a': 1}, {'a': 2}), equal_to({'a': 2}))


def test_deepmerge():
    """
    Test deep dicts merging
    """
    assert_that(
        combine_dict(
            {
                'a': {
                    'b': 1,
                    'c': 3,
                },
            },
            {
                'a': {
                    'b': 2,
                },
            },
        ),
        equal_to(
            {
                'a': {
                    'b': 2,
                    'c': 3,
                },
            }
        ),
    )

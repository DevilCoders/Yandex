"""
Test for finding not assigned PostgreSQL grants
"""
import pytest

from dbaas_internal_api.modules.postgres.utils import get_assignable_grants, are_grants_cyclic


@pytest.mark.parametrize(
    ['username', 'all_roles', 'expected'],
    [
        ('user', {'a': [], 'user': []}, ['a']),
        (
            #      * user
            #     ↙ ↘
            #  a *   * b
            'user',
            {'a': [], 'user': ['a', 'b'], 'b': []},
            ['a', 'b'],
        ),
        (
            #      * user
            #     ↙ ↘
            #  a *   * b
            'b',
            {'a': [], 'user': ['a', 'b'], 'b': []},
            ['a'],
        ),
        (
            #             * root
            #      ↙   ↙  ↓   ↘     ↘
            #  a *  b *   * c  * d   * user
            'user',
            {'a': [], 'b': [], 'c': [], 'd': [], 'user': [], 'root': ['a', 'b', 'c', 'd', 'user']},
            ['a', 'b', 'c', 'd'],
        ),
        (
            #    a *      * other
            #      ↓      ↓
            #    b *      * other-grant
            #      ↓
            #    c *
            #      ↓
            #      * user
            #      ↓
            #    d *
            #     ↙ ↘
            #  e *   * f
            'user',
            {
                'a': ['b'],
                'b': ['c'],
                'c': ['user'],
                'user': ['d'],
                'd': ['e', 'f'],
                'e': [],
                'f': [],
                'other': ['other-grant'],
                'other-grant': [],
            },
            ['d', 'e', 'f', 'other', 'other-grant'],
        ),
    ],
)
def test_get_not_assigned_grants(username, all_roles, expected):
    assert get_assignable_grants(username, all_roles) == expected


@pytest.mark.parametrize(
    ['all_roles', 'expected'],
    [
        ({'a': [], 'user': []}, False),
        (
            #      * user
            #     ↙ ↘
            #  a *   * b
            {'a': [], 'user': ['a', 'b'], 'b': []},
            False,
        ),
        (
            #      * user
            #     ↙ ↘
            #  a * → * b
            {'a': ['b'], 'user': ['a', 'b'], 'b': []},
            False,
        ),
        (
            #      * user
            #     ↙ ↖
            #  a * → * b
            {'a': ['b'], 'b': ['user'], 'user': ['a']},
            True,
        ),
        (
            #    a * ↖
            #      ↓   ↑
            #    b *   ↑
            #      ↓   ↑
            #    c *   ↑
            #      ↓   ↑
            #    d *   ↑
            #      ↓  ↗
            #      * e
            {'a': ['b'], 'b': ['c'], 'c': ['d'], 'd': ['e'], 'e': ['a']},
            True,
        ),
        (
            #    a *      * other
            #      ↓      ↓
            #    b *      * other-grant
            #      ↓
            #    c *
            #      ↓
            #      * user
            #      ↓
            #    d *
            #     ↙ ↘
            #  e *   * f
            {
                'a': ['b'],
                'b': ['c'],
                'c': ['user'],
                'user': ['d'],
                'd': ['e', 'f'],
                'e': [],
                'f': [],
                'other': ['other-grant'],
                'other-grant': [],
            },
            False,
        ),
    ],
)
def test_are_grants_cyclic(all_roles, expected):
    assert are_grants_cyclic(all_roles) == expected

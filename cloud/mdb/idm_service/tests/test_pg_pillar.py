"""Tests on Pillar class."""
# pylint: disable=redefined-outer-name
import time
from idm_service.pg_pillar import PgPillar

import pytest

from idm_service.exceptions import InvalidUserOriginError, UnsupportedClusterError

TEST_CRYPTO = {
    'client_pub_key': '4w8-WJzgQh9pRrDhe9vjczBKidyIX0-scbdOIYEYizc=',
    'api_sec_key': 'aI4bkf8RGy4EMHoPq-f04GRxCZyYwYsZAXhdA9ImsG4=',
}


@pytest.fixture
def pillar(users):
    """Create basic pillar from users."""
    data = {
        'data': {
            'sox_audit': True,
            'config': {
                'pgusers': users,
            },
            'unmanaged_dbs': [
                {
                    'db1': {},
                }
            ],
        },
    }
    return PgPillar(data)


@pytest.mark.parametrize(
    'data',
    [
        {
            'data': {
                'config': {},
                'sox_audit': True,
            },
        },
        {
            'data': {
                'config': {
                    'pgusers': {},
                },
            },
        },
    ],
)
def test_pillar_validation(data):
    """Constuctor fails on incorrect data."""
    with pytest.raises(UnsupportedClusterError):
        PgPillar(data)


@pytest.mark.parametrize(
    'users,login,role',
    [
        (
            {
                'bob': {
                    'grants': [],
                },
            },
            'bob',
            'writer',
        ),
        (
            {
                'bob': {
                    'grants': [],
                    'origin': 'not-idm',
                },
            },
            'bob',
            'writer',
        ),
    ],
)
def test_pillar_add_role_origin(pillar, login, role):
    """New role appears in the grants list."""
    with pytest.raises(InvalidUserOriginError):
        pillar.add_role(login, role)


@pytest.mark.parametrize(
    'users,login,role',
    [
        ({}, 'alice', 'reader'),
        (
            {
                'bob': {
                    'grants': [],
                    'origin': 'idm',
                },
            },
            'bob',
            'writer',
        ),
    ],
)
def test_pillar_add_role(pillar, login, role):
    """New role appears in the grants list."""
    pillar.add_role(login, role)
    assert role in pillar.users[login]['grants']


@pytest.mark.parametrize(
    'users,login,role',
    [
        ({}, 'alice', 'reader'),
        (
            {
                'bob': {
                    'grants': ['writer'],
                },
            },
            'bob',
            'writer',
        ),
    ],
)
def test_pillar_remove_role(pillar, login, role):
    """Role disappears from the grants list."""
    pillar.remove_role(login, role)
    assert (login not in pillar.users) or (role not in pillar.users[login]['grants'])


@pytest.mark.parametrize(
    'users,expected',
    [
        (
            {
                'root': {
                    'last_password_update': 0,
                },
                'bob': {
                    'origin': 'idm',
                    'last_password_update': 0,
                },
                'ann': {
                    'origin': 'idm',
                    'last_password_update': int(time.time()),
                },
            },
            ['bob'],
        )
    ],
)
def test_have_stale_passwords(pillar, expected):
    """Returns correct list of users."""
    assert set(pillar.have_stale_passwords()) == set(expected)


@pytest.mark.parametrize(
    'users,expected',
    [
        (
            {
                'bob': {
                    'origin': 'idm',
                    'last_password_update': 0,
                },
                'ann': {
                    'origin': 'idm',
                    'last_password_update': int(time.time()),
                },
            },
            ['bob'],
        )
    ],
)
def test_rotate_passwords(pillar, expected):
    """Rotates passwords correctly."""
    result = pillar.rotate_passwords(TEST_CRYPTO)
    assert set(result.keys()) == set(expected)
    assert not pillar.have_stale_passwords()

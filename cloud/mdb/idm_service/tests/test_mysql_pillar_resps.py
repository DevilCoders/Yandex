"""Tests on Pillar class."""
# pylint: disable=redefined-outer-name
from idm_service.mysql_pillar import MysqlPillar
import pytest


@pytest.fixture
def pillar(resps):
    """Create basic pillar from resps."""
    data = {
        'data': {
            'sox_audit': True,
            'mysql': {
                'users': {
                    'user1': {
                        'proxy_user': None,
                    },
                },
                'databases': [
                    {
                        'db1': {},
                    }
                ],
                'zk_hosts': [],
            },
        },
    }
    if resps:
        # For testing uninitialized struct in pillar
        data['data']['idm_resps'] = resps
    return MysqlPillar(data)


@pytest.mark.parametrize('resps,login', [(None, 'alice'), ([], 'alice'), (['bob'], 'alice'), (['alice'], 'alice')])
def test_pillar_add_resp(pillar, login):
    """New role appears in the resps list."""
    pillar.add_resp(login)
    assert login in pillar.resps


@pytest.mark.parametrize('resps,login', [(None, 'alice'), ([], 'alice'), (['bob'], 'alice'), (['alice'], 'alice')])
def test_pillar_remove_resp(pillar, login):
    """New role appears in the resps list."""
    pillar.remove_resp(login)
    assert login not in pillar.resps

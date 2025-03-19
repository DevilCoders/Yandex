"""
Tests for MongoDB utils
"""

from copy import deepcopy
from datetime import timedelta

from dbaas_internal_api.modules.mongodb.pillar import MongoDBPillar
from dbaas_internal_api.modules.mongodb.utils import get_user_database_task_timeout

BASE_PILLAR = {
    'data': {
        'mongodb': {
            'databases': {},
            'users': {},
            'zk_hosts': [],
            'shards': {},
            'cluster_name': 'test_cluster_name',
            'version': {'major_num': '500', 'major_human': '5.0'},
            'cluster_auth': 'keyfile',
            'config': {},
            'keyfile': 'keyfile_contents',
        },
    }
}


def get_user_spec(number):
    """
    Get userN userspec
    """
    return {
        'name': f'user{number}',
        'password': f'password{number}',
        'permissions': [],
    }


def get_db_spec(number):
    """
    Get dbN dbspec
    """
    return {'name': f'db{number}'}


def test_user_database_task_timeout_min(mocker):
    mocker.patch('dbaas_internal_api.modules.mongodb.pillar.encrypt', return_value='ENCRYPTED!')
    pillar = MongoDBPillar.load(deepcopy(BASE_PILLAR))
    pillar.add_user(get_user_spec(1))
    pillar.add_database(get_db_spec(1))
    assert get_user_database_task_timeout(pillar) == timedelta(seconds=3600)


def test_user_database_task_timeout_10k(mocker):
    mocker.patch('dbaas_internal_api.modules.mongodb.pillar.encrypt', return_value='ENCRYPTED!')
    pillar = MongoDBPillar.load(deepcopy(BASE_PILLAR))
    pillar.add_users([get_user_spec(x) for x in range(100)])
    pillar.add_databases([get_db_spec(x) for x in range(100)])
    assert get_user_database_task_timeout(pillar) == timedelta(seconds=10000)


def test_user_database_task_timeout_max(mocker):
    mocker.patch('dbaas_internal_api.modules.mongodb.pillar.encrypt', return_value='ENCRYPTED!')
    pillar = MongoDBPillar.load(deepcopy(BASE_PILLAR))
    pillar.add_users([get_user_spec(x) for x in range(1000)])
    pillar.add_databases([get_db_spec(x) for x in range(1000)])
    assert get_user_database_task_timeout(pillar) == timedelta(seconds=24 * 3600)

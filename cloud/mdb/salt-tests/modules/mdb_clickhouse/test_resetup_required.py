# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt.salt._modules import mdb_clickhouse
from cloud.mdb.salt_tests.common.mocks import mock_grains, mock_pillar
from cloud.mdb.internal.python.pytest.utils import parametrize


@parametrize(
    {
        'id': 'with all data preset',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'databases': ['user_database'],
                        'zk_hosts': ['vla-1.db.yandex.net', 'man-1.db.yandex.net', 'sas-1.db.yandex.net'],
                    },
                },
            },
            'grains': {'id': 'fqdn.yandex.net'},
            's3_objects': ['initialized/fqdn.yandex.net'],
            'present_files': [
                '/var/lib/clickhouse/metadata/default/cluster_master_election.sql',
                '/var/lib/clickhouse/metadata/default/shard_master_election.sql',
                '/var/lib/clickhouse/metadata/user_database.sql',
            ],
            'result': False,
        },
    },
    {
        'id': 'no data present',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'databases': ['user_database'],
                        'zk_hosts': ['vla-1.db.yandex.net', 'man-1.db.yandex.net', 'sas-1.db.yandex.net'],
                    },
                },
            },
            'grains': {'id': 'fqdn.yandex.net'},
            's3_objects': ['initialized/fqdn.yandex.net'],
            'present_files': [],
            'result': True,
        },
    },
    {
        'id': 'no user databases present',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'databases': ['user_database'],
                        'zk_hosts': ['vla-1.db.yandex.net', 'man-1.db.yandex.net', 'sas-1.db.yandex.net'],
                    },
                },
            },
            'grains': {'id': 'fqdn.yandex.net'},
            's3_objects': ['initialized/fqdn.yandex.net'],
            'present_files': [
                '/var/lib/clickhouse/metadata/default/cluster_master_election.sql',
                '/var/lib/clickhouse/metadata/default/shard_master_election.sql',
            ],
            'result': False,
        },
    },
    {
        'id': 'mdbbdns tables not preset',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'databases': ['user_database'],
                        'zk_hosts': ['vla-1.db.yandex.net', 'man-1.db.yandex.net', 'sas-1.db.yandex.net'],
                    },
                },
            },
            'grains': {'id': 'fqdn.yandex.net'},
            's3_objects': ['initialized/fqdn.yandex.net'],
            'present_files': [
                '/var/lib/clickhouse/metadata/user_database.sql',
            ],
            'result': False,
        },
    },
    {
        'id': 'not initialized host',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'databases': ['user_database'],
                        'zk_hosts': ['vla-1.db.yandex.net', 'man-1.db.yandex.net', 'sas-1.db.yandex.net'],
                    },
                },
            },
            'grains': {'id': 'fqdn.yandex.net'},
            's3_objects': [],
            'present_files': [],
            'result': False,
        },
    },
)
def test_resetup_required(pillar, grains, s3_objects, present_files, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    mock_grains(mdb_clickhouse.__salt__, grains)
    mdb_clickhouse.__salt__['mdb_s3.client'] = lambda: None
    mdb_clickhouse.__salt__['mdb_s3.object_exists'] = lambda _, key: key in s3_objects
    mdb_clickhouse.__salt__['file.find'] = lambda key: present_files

    assert mdb_clickhouse.resetup_required() == result

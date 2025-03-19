# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt.salt._modules import mdb_network
from cloud.mdb.salt.salt._modules.mdb_network import (
    CLOUD_BEAVER_PREPROD_PROJECT_ID,
    CLOUD_BEAVER_PROD_PROJECT_ID,
    CLOUD_YQL_PROD_PROJECT_ID,
    DATA_TRANSFER_PREPROD_PROJECT_ID,
    DATA_TRANSFER_PROD_PROJECT_ID,
    YANDEX_QUERY_PREPROD_PROJECT_ID,
    YANDEX_QUERY_PROD_PROJECT_ID,
)
from cloud.mdb.salt_tests.common.mocks import mock_grains, mock_pillar
from cloud.mdb.internal.python.pytest.utils import parametrize


@parametrize(
    {
        'id': 'default',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f801:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {},
            'testing': False,
            'args': {},
            'result': [],
        },
    },
    {
        'id': 'enabled access flags, prod env',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f801:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'access': {
                        'web_sql': True,
                        'data_lens': True,
                        'metrika': True,
                        'serverless': True,
                        'yandex_query': True,
                        'data_transfer': True,
                    },
                },
            },
            'testing': False,
            'args': {},
            'result': [
                '0x660',
                '0x453e',
                '0x4b9c',
                '0xf82f',
                '0xf83d',
                CLOUD_YQL_PROD_PROJECT_ID,
                CLOUD_BEAVER_PROD_PROJECT_ID,
                YANDEX_QUERY_PROD_PROJECT_ID,
                DATA_TRANSFER_PROD_PROJECT_ID,
            ],
        },
    },
    {
        'id': 'enabled access flags, testing env',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f801:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'access': {
                        'web_sql': True,
                        'data_lens': True,
                        'metrika': True,
                        'serverless': True,
                        'yandex_query': True,
                        'data_transfer': True,
                    },
                },
            },
            'testing': True,
            'args': {},
            'result': [
                '0x660',
                '0x453d',
                '0x453e',
                '0xfc1f',
                '0xfc15',
                '0x4b9c',
                '0xf82f',
                '0xf83d',
                CLOUD_YQL_PROD_PROJECT_ID,
                CLOUD_BEAVER_PREPROD_PROJECT_ID,
                CLOUD_BEAVER_PROD_PROJECT_ID,
                YANDEX_QUERY_PREPROD_PROJECT_ID,
                YANDEX_QUERY_PROD_PROJECT_ID,
                DATA_TRANSFER_PREPROD_PROJECT_ID,
                DATA_TRANSFER_PROD_PROJECT_ID,
            ],
        },
    },
    {
        'id': 'custom external projects',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f801:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'access': {
                        'web_sql': True,
                        'data_lens': True,
                        'metrika': True,
                        'serverless': True,
                    },
                    'external_project_ids': ['0xf101', '0xf102'],
                },
            },
            'testing': False,
            'args': {},
            'result': [
                '0x660',
                '0x453e',
                '0xf101',
                '0xf102',
                '0x4b9c',
                '0xf82f',
                '0xf83d',
                CLOUD_YQL_PROD_PROJECT_ID,
                CLOUD_BEAVER_PROD_PROJECT_ID,
            ],
        },
    },
    {
        'id': 'user project matches one of external projects, exclude_user_project_id=False',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f83d:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'access': {
                        'web_sql': True,
                        'data_lens': True,
                        'metrika': True,
                        'serverless': True,
                    },
                },
            },
            'testing': False,
            'args': {},
            'result': [
                '0x660',
                '0x453e',
                '0x4b9c',
                '0xf82f',
                '0xf83d',
                CLOUD_YQL_PROD_PROJECT_ID,
                CLOUD_BEAVER_PROD_PROJECT_ID,
            ],
        },
    },
    {
        'id': 'user project matches one of external projects, exclude_user_project_id=True',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f83d:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'access': {
                        'web_sql': True,
                        'data_lens': True,
                        'metrika': True,
                        'serverless': True,
                    },
                },
            },
            'testing': False,
            'args': {
                'exclude_user_project_id': True,
            },
            'result': [
                '0x660',
                '0x453e',
                '0x4b9c',
                '0xf82f',
                CLOUD_YQL_PROD_PROJECT_ID,
                CLOUD_BEAVER_PROD_PROJECT_ID,
            ],
        },
    },
    {
        'id': 'user project matches no one of external projects, exclude_user_project_id=True',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:faaa:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'access': {
                        'web_sql': True,
                        'data_lens': True,
                        'metrika': True,
                        'serverless': True,
                    },
                },
            },
            'testing': False,
            'args': {
                'exclude_user_project_id': True,
            },
            'result': [
                '0x660',
                '0x453e',
                '0x4b9c',
                '0xf82f',
                '0xf83d',
                CLOUD_YQL_PROD_PROJECT_ID,
                CLOUD_BEAVER_PROD_PROJECT_ID,
            ],
        },
    },
)
def test_get_external_project_ids(grains, pillar, testing, args, result):
    mock_grains(mdb_network.__salt__, grains)
    mock_pillar(mdb_network.__salt__, pillar)
    mdb_network.__salt__['dbaas.is_testing'] = lambda: testing
    assert sorted(mdb_network.get_external_project_ids(**args)) == sorted(result)

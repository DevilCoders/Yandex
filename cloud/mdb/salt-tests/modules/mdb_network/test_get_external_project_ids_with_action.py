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
            'result': {},
        },
    },
    {
        'id': 'default, firewall:reject_enabled=True',
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
                'firewall': {
                    'reject_enabled': True,
                },
            },
            'testing': False,
            'args': {},
            'result': {},
        },
    },
    {
        'id': 'default, data:dbaas:vtype=porto',
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
                    'dbaas': {
                        'vtype': 'porto',
                    },
                },
            },
            'testing': False,
            'args': {},
            'result': {},
        },
    },
    {
        'id': 'default, firewall:reject_enabled=True, data:dbaas:vtype=porto',
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
                    'dbaas': {
                        'vtype': 'porto',
                    },
                },
                'firewall': {
                    'reject_enabled': True,
                },
            },
            'testing': False,
            'args': {},
            'result': {
                '0x453d': 'REJECT',
                '0x453e': 'REJECT',
                '0x4b9c': 'REJECT',
                '0x660': 'REJECT',
                '0xf82f': 'REJECT',
                '0xf83d': 'REJECT',
                '0xfc15': 'REJECT',
                '0xfc1f': 'REJECT',
                '0xfc58': 'REJECT',
                CLOUD_YQL_PROD_PROJECT_ID: 'REJECT',
                CLOUD_BEAVER_PREPROD_PROJECT_ID: 'REJECT',
                CLOUD_BEAVER_PROD_PROJECT_ID: 'REJECT',
                YANDEX_QUERY_PREPROD_PROJECT_ID: 'REJECT',
                YANDEX_QUERY_PROD_PROJECT_ID: 'REJECT',
                DATA_TRANSFER_PREPROD_PROJECT_ID: 'REJECT',
                DATA_TRANSFER_PROD_PROJECT_ID: 'REJECT',
            },
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
            'result': {
                '0x453e': 'ACCEPT',
                '0x4b9c': 'ACCEPT',
                '0x660': 'ACCEPT',
                '0xf82f': 'ACCEPT',
                '0xf83d': 'ACCEPT',
                CLOUD_YQL_PROD_PROJECT_ID: 'ACCEPT',
                CLOUD_BEAVER_PROD_PROJECT_ID: 'ACCEPT',
                YANDEX_QUERY_PROD_PROJECT_ID: 'ACCEPT',
                DATA_TRANSFER_PROD_PROJECT_ID: 'ACCEPT',
            },
        },
    },
    {
        'id': 'enabled access flags, prod env, firewall:reject_enabled=True, data:dbaas:vtype=porto',
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
                    'dbaas': {
                        'vtype': 'porto',
                    },
                },
                'firewall': {
                    'reject_enabled': True,
                },
            },
            'testing': False,
            'args': {},
            'result': {
                '0x453d': 'REJECT',
                '0x453e': 'ACCEPT',
                '0x4b9c': 'ACCEPT',
                '0x660': 'ACCEPT',
                '0xf82f': 'ACCEPT',
                '0xf83d': 'ACCEPT',
                '0xfc15': 'REJECT',
                '0xfc1f': 'REJECT',
                '0xfc58': 'REJECT',
                CLOUD_YQL_PROD_PROJECT_ID: 'ACCEPT',
                CLOUD_BEAVER_PREPROD_PROJECT_ID: 'REJECT',
                CLOUD_BEAVER_PROD_PROJECT_ID: 'ACCEPT',
                YANDEX_QUERY_PREPROD_PROJECT_ID: 'REJECT',
                YANDEX_QUERY_PROD_PROJECT_ID: 'ACCEPT',
                DATA_TRANSFER_PREPROD_PROJECT_ID: 'REJECT',
                DATA_TRANSFER_PROD_PROJECT_ID: 'ACCEPT',
            },
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
            'result': {
                '0x453d': 'ACCEPT',
                '0x453e': 'ACCEPT',
                '0x4b9c': 'ACCEPT',
                '0x660': 'ACCEPT',
                '0xf82f': 'ACCEPT',
                '0xf83d': 'ACCEPT',
                '0xfc15': 'ACCEPT',
                '0xfc1f': 'ACCEPT',
                '0xfc58': 'ACCEPT',
                CLOUD_YQL_PROD_PROJECT_ID: 'ACCEPT',
                CLOUD_BEAVER_PREPROD_PROJECT_ID: 'ACCEPT',
                CLOUD_BEAVER_PROD_PROJECT_ID: 'ACCEPT',
                YANDEX_QUERY_PREPROD_PROJECT_ID: 'ACCEPT',
                YANDEX_QUERY_PROD_PROJECT_ID: 'ACCEPT',
                DATA_TRANSFER_PREPROD_PROJECT_ID: 'ACCEPT',
                DATA_TRANSFER_PROD_PROJECT_ID: 'ACCEPT',
            },
        },
    },
    {
        'id': 'enabled access flags, testing env, firewall:reject_enabled=True, data:dbaas:vtype=porto',
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
                    'dbaas': {
                        'vtype': 'porto',
                    },
                },
                'firewall': {
                    'reject_enabled': True,
                },
            },
            'testing': True,
            'args': {},
            'result': {
                '0x453d': 'ACCEPT',
                '0x453e': 'ACCEPT',
                '0x4b9c': 'ACCEPT',
                '0x660': 'ACCEPT',
                '0xf82f': 'ACCEPT',
                '0xf83d': 'ACCEPT',
                '0xfc15': 'ACCEPT',
                '0xfc1f': 'ACCEPT',
                '0xfc58': 'ACCEPT',
                CLOUD_YQL_PROD_PROJECT_ID: 'ACCEPT',
                CLOUD_BEAVER_PREPROD_PROJECT_ID: 'ACCEPT',
                CLOUD_BEAVER_PROD_PROJECT_ID: 'ACCEPT',
                YANDEX_QUERY_PREPROD_PROJECT_ID: 'ACCEPT',
                YANDEX_QUERY_PROD_PROJECT_ID: 'ACCEPT',
                DATA_TRANSFER_PREPROD_PROJECT_ID: 'ACCEPT',
                DATA_TRANSFER_PROD_PROJECT_ID: 'ACCEPT',
            },
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
                        'yandex_query': True,
                        'data_transfer': True,
                    },
                    'external_project_ids': ['0xf101', '0xf102'],
                },
            },
            'testing': False,
            'args': {},
            'result': {
                '0x453e': 'ACCEPT',
                '0x4b9c': 'ACCEPT',
                '0x660': 'ACCEPT',
                '0xf101': 'ACCEPT',
                '0xf102': 'ACCEPT',
                '0xf82f': 'ACCEPT',
                '0xf83d': 'ACCEPT',
                CLOUD_YQL_PROD_PROJECT_ID: 'ACCEPT',
                CLOUD_BEAVER_PROD_PROJECT_ID: 'ACCEPT',
                YANDEX_QUERY_PROD_PROJECT_ID: 'ACCEPT',
                DATA_TRANSFER_PROD_PROJECT_ID: 'ACCEPT',
            },
        },
    },
    {
        'id': 'custom external projects, firewall:reject_enabled=True, data:dbaas:vtype=porto',
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
                    'external_project_ids': ['0xf101', '0xf102'],
                    'dbaas': {
                        'vtype': 'porto',
                    },
                },
                'firewall': {
                    'reject_enabled': True,
                },
            },
            'testing': False,
            'args': {},
            'result': {
                '0x453d': 'REJECT',
                '0x453e': 'ACCEPT',
                '0x4b9c': 'ACCEPT',
                '0x660': 'ACCEPT',
                '0xf101': 'ACCEPT',
                '0xf102': 'ACCEPT',
                '0xf82f': 'ACCEPT',
                '0xf83d': 'ACCEPT',
                '0xfc15': 'REJECT',
                '0xfc1f': 'REJECT',
                '0xfc58': 'REJECT',
                CLOUD_YQL_PROD_PROJECT_ID: 'ACCEPT',
                CLOUD_BEAVER_PREPROD_PROJECT_ID: 'REJECT',
                CLOUD_BEAVER_PROD_PROJECT_ID: 'ACCEPT',
                YANDEX_QUERY_PREPROD_PROJECT_ID: 'REJECT',
                YANDEX_QUERY_PROD_PROJECT_ID: 'ACCEPT',
                DATA_TRANSFER_PREPROD_PROJECT_ID: 'REJECT',
                DATA_TRANSFER_PROD_PROJECT_ID: 'ACCEPT',
            },
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
                        'yandex_query': True,
                        'data_transfer': True,
                    },
                },
            },
            'testing': False,
            'args': {},
            'result': {
                '0x453e': 'ACCEPT',
                '0x4b9c': 'ACCEPT',
                '0x660': 'ACCEPT',
                '0xf82f': 'ACCEPT',
                '0xf83d': 'ACCEPT',
                CLOUD_YQL_PROD_PROJECT_ID: 'ACCEPT',
                CLOUD_BEAVER_PROD_PROJECT_ID: 'ACCEPT',
                YANDEX_QUERY_PROD_PROJECT_ID: 'ACCEPT',
                DATA_TRANSFER_PROD_PROJECT_ID: 'ACCEPT',
            },
        },
    },
    {
        'id': 'user project matches one of external projects, exclude_user_project_id=False, firewall:reject_enabled=True, data:dbaas:vtype=porto',
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
                        'yandex_query': True,
                        'data_transfer': True,
                    },
                    'dbaas': {
                        'vtype': 'porto',
                    },
                },
                'firewall': {
                    'reject_enabled': True,
                },
            },
            'testing': False,
            'args': {},
            'result': {
                '0x453d': 'REJECT',
                '0x453e': 'ACCEPT',
                '0x4b9c': 'ACCEPT',
                '0x660': 'ACCEPT',
                '0xf82f': 'ACCEPT',
                '0xf83d': 'ACCEPT',
                '0xfc15': 'REJECT',
                '0xfc1f': 'REJECT',
                '0xfc58': 'REJECT',
                CLOUD_YQL_PROD_PROJECT_ID: 'ACCEPT',
                CLOUD_BEAVER_PREPROD_PROJECT_ID: 'REJECT',
                CLOUD_BEAVER_PROD_PROJECT_ID: 'ACCEPT',
                YANDEX_QUERY_PREPROD_PROJECT_ID: 'REJECT',
                YANDEX_QUERY_PROD_PROJECT_ID: 'ACCEPT',
                DATA_TRANSFER_PREPROD_PROJECT_ID: 'REJECT',
                DATA_TRANSFER_PROD_PROJECT_ID: 'ACCEPT',
            },
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
                        'yandex_query': True,
                        'data_transfer': True,
                    },
                },
            },
            'testing': False,
            'args': {
                'exclude_user_project_id': True,
            },
            'result': {
                '0x453e': 'ACCEPT',
                '0x4b9c': 'ACCEPT',
                '0x660': 'ACCEPT',
                '0xf82f': 'ACCEPT',
                CLOUD_YQL_PROD_PROJECT_ID: 'ACCEPT',
                CLOUD_BEAVER_PROD_PROJECT_ID: 'ACCEPT',
                YANDEX_QUERY_PROD_PROJECT_ID: 'ACCEPT',
                DATA_TRANSFER_PROD_PROJECT_ID: 'ACCEPT',
            },
        },
    },
    {
        'id': 'user project matches one of external projects, exclude_user_project_id=True, firewall:reject_enabled=True, data:dbaas:vtype=porto',
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
                        'yandex_query': True,
                        'data_transfer': True,
                    },
                    'dbaas': {
                        'vtype': 'porto',
                    },
                },
                'firewall': {
                    'reject_enabled': True,
                },
            },
            'testing': False,
            'args': {
                'exclude_user_project_id': True,
            },
            'result': {
                '0x453d': 'REJECT',
                '0x453e': 'ACCEPT',
                '0x4b9c': 'ACCEPT',
                '0x660': 'ACCEPT',
                '0xf82f': 'ACCEPT',
                '0xfc15': 'REJECT',
                '0xfc1f': 'REJECT',
                '0xfc58': 'REJECT',
                CLOUD_YQL_PROD_PROJECT_ID: 'ACCEPT',
                CLOUD_BEAVER_PREPROD_PROJECT_ID: 'REJECT',
                CLOUD_BEAVER_PROD_PROJECT_ID: 'ACCEPT',
                YANDEX_QUERY_PREPROD_PROJECT_ID: 'REJECT',
                YANDEX_QUERY_PROD_PROJECT_ID: 'ACCEPT',
                DATA_TRANSFER_PREPROD_PROJECT_ID: 'REJECT',
                DATA_TRANSFER_PROD_PROJECT_ID: 'ACCEPT',
            },
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
                        'yandex_query': True,
                        'data_transfer': True,
                    },
                },
            },
            'testing': False,
            'args': {
                'exclude_user_project_id': True,
            },
            'result': {
                '0x453e': 'ACCEPT',
                '0x4b9c': 'ACCEPT',
                '0x660': 'ACCEPT',
                '0xf82f': 'ACCEPT',
                '0xf83d': 'ACCEPT',
                CLOUD_YQL_PROD_PROJECT_ID: 'ACCEPT',
                CLOUD_BEAVER_PROD_PROJECT_ID: 'ACCEPT',
                YANDEX_QUERY_PROD_PROJECT_ID: 'ACCEPT',
                DATA_TRANSFER_PROD_PROJECT_ID: 'ACCEPT',
            },
        },
    },
    {
        'id': 'user project matches no one of external projects, exclude_user_project_id=True, firewall:reject_enabled=True, data:dbaas:vtype=porto',
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
                        'yandex_query': True,
                        'data_transfer': True,
                    },
                    'dbaas': {
                        'vtype': 'porto',
                    },
                },
                'firewall': {
                    'reject_enabled': True,
                },
            },
            'testing': False,
            'args': {
                'exclude_user_project_id': True,
            },
            'result': {
                '0x453d': 'REJECT',
                '0x453e': 'ACCEPT',
                '0x4b9c': 'ACCEPT',
                '0x660': 'ACCEPT',
                '0xf82f': 'ACCEPT',
                '0xf83d': 'ACCEPT',
                '0xfc15': 'REJECT',
                '0xfc1f': 'REJECT',
                '0xfc58': 'REJECT',
                CLOUD_YQL_PROD_PROJECT_ID: 'ACCEPT',
                CLOUD_BEAVER_PREPROD_PROJECT_ID: 'REJECT',
                CLOUD_BEAVER_PROD_PROJECT_ID: 'ACCEPT',
                YANDEX_QUERY_PREPROD_PROJECT_ID: 'REJECT',
                YANDEX_QUERY_PROD_PROJECT_ID: 'ACCEPT',
                DATA_TRANSFER_PREPROD_PROJECT_ID: 'REJECT',
                DATA_TRANSFER_PROD_PROJECT_ID: 'ACCEPT',
            },
        },
    },
)
def test_get_external_project_ids_with_action(grains, pillar, testing, args, result):
    mock_grains(mdb_network.__salt__, grains)
    mock_pillar(mdb_network.__salt__, pillar)
    mdb_network.__salt__['dbaas.is_testing'] = lambda: testing
    assert mdb_network.get_external_project_ids_with_action(**args) == result

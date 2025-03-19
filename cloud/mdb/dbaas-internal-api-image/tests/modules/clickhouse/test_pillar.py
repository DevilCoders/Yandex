"""
Test for ClickHousePillar helper
"""
from collections import OrderedDict

import pytest
from copy import deepcopy
from hamcrest import assert_that, contains, contains_inanyorder, equal_to, has_entries, has_entry

from dbaas_internal_api.core.exceptions import DbaasClientError, DbaasError
from dbaas_internal_api.modules.clickhouse.pillar import ClickhousePillar, ZookeeperPillar
from dbaas_internal_api.utils.helpers import merge_dict
from dbaas_internal_api.utils.version import Version

# flake8: noqa
from ...fixtures import app_config
from ...matchers import at_path

# pylint: disable=missing-docstring, invalid-name

TEST_CID = 'c21cefe0-55e0-4038-989f-69bb12bd35e2'


class TestClickHousePillar:
    SAMPLE_PILLAR = {
        'data': {
            'clickhouse': {
                'databases': ['foo', 'bar'],
                'config': {
                    'log_level': 'debug',
                },
                'users': {
                    'alice': {
                        'password': 'foo-pass',
                        'hash': 'foo-pass-hash',
                        'databases': {
                            'foo': {},
                        },
                    },
                    'bob': {
                        'password': 'bar-pass',
                        'hash': 'bar-pass-hash',
                        'databases': {
                            'bar': {},
                        },
                    },
                },
                'zk_hosts': [
                    'zk1.test',
                    'zk2.test',
                    'zk3.test',
                ],
                'ch_version': '18.12.17',
            },
            'firewall': {},
        },
    }

    def mk(self, override=None) -> ClickhousePillar:
        pillar_data = deepcopy(self.SAMPLE_PILLAR)
        if override:
            merge_dict(pillar_data, override)
        return ClickhousePillar.load(pillar_data)

    def test_make(self, mocker):
        mocker.patch('dbaas_internal_api.modules.clickhouse.pillar.gen_encrypted_password', return_value='ENCRYPTED!')

        p = ClickhousePillar.make()
        assert_that(
            p.as_dict(),
            at_path(
                '$.data.clickhouse',
                has_entry(
                    'interserver_credentials',
                    {
                        'user': 'interserver',
                        'password': 'ENCRYPTED!',
                    },
                ),
            ),
        )

    def test_get_databases(self):
        assert self.mk().database_names == ['foo', 'bar']

    def test_set_database(self):
        p = self.mk()
        p.database_names = ['good', 'names']
        assert_that(p.as_dict(), at_path('$.data.clickhouse.databases', contains_inanyorder('good', 'names')))

    def test_get_pillar_users(self):
        assert self.mk().pillar_users == self.SAMPLE_PILLAR['data']['clickhouse']['users']

    def test_set_pillar_users(self):
        p = self.mk()
        p.pillar_users = {
            'mellory': {
                'password': 'foo',
                'password-hash': 'bar',
                'databases': '*',
            },
        }
        assert p.as_dict()['data']['clickhouse']['users'] == {
            'mellory': {
                'password': 'foo',
                'password-hash': 'bar',
                'databases': '*',
            },
        }

    def test_get_users(self):
        assert_that(
            self.mk().users(TEST_CID),
            contains(
                {
                    'name': 'alice',
                    'cluster_id': TEST_CID,
                    'permissions': [
                        {
                            'database_name': 'foo',
                        }
                    ],
                    'settings': {},
                    'quotas': [],
                },
                {
                    'name': 'bob',
                    'cluster_id': TEST_CID,
                    'permissions': [
                        {
                            'database_name': 'bar',
                        }
                    ],
                    'settings': {},
                    'quotas': [],
                },
            ),
        )

    def test_get_user_sort_users_by_name(self):
        p = self.mk()
        pillar_users = OrderedDict()
        pillar_users['test_user'] = {}
        pillar_users['another_test_user'] = {}
        p.pillar_users = pillar_users
        assert_that(p.users(TEST_CID), contains(has_entry('name', 'another_test_user'), has_entry('name', 'test_user')))

    def test_add_user(self, mocker):
        mocker.patch('dbaas_internal_api.modules.clickhouse.pillar.encrypt', return_value='ENCRYPTED!')
        p = self.mk()
        p.add_user(
            {
                'name': 'mellory',
                'password': 'blah',
                'permissions': [
                    {
                        'database_name': 'foo',
                    },
                    {
                        'database_name': 'bar',
                    },
                ],
            }
        )

        assert_that(
            p.as_dict(),
            at_path(
                '$.data.clickhouse.users',
                has_entry(
                    'mellory',
                    {
                        'password': 'ENCRYPTED!',
                        'hash': 'ENCRYPTED!',
                        'databases': {
                            'foo': {},
                            'bar': {},
                        },
                        'settings': {},
                        'quotas': [],
                    },
                ),
            ),
        )

    def test_add_user_with_data_filters(self, mocker):
        mocker.patch('dbaas_internal_api.modules.clickhouse.pillar.encrypt', return_value='ENCRYPTED!')
        p = self.mk()
        p.add_user(
            {
                'name': 'trudy',
                'password': 'blah',
                'permissions': [
                    {
                        'database_name': 'foo',
                        'data_filters': [
                            {
                                'table_name': 'baz',
                                'filter': 'qux = 1',
                            }
                        ],
                    }
                ],
            }
        )

        assert_that(
            p.as_dict(),
            at_path(
                '$.data.clickhouse.users',
                has_entry(
                    'trudy',
                    {
                        'password': 'ENCRYPTED!',
                        'hash': 'ENCRYPTED!',
                        'databases': {
                            'foo': {
                                'tables': {
                                    'baz': {
                                        'filter': 'qux = 1',
                                    },
                                },
                            },
                        },
                        'settings': {},
                        'quotas': [],
                    },
                ),
            ),
        )

    def test_upset_existed_user(self, mocker):
        mocker.patch('dbaas_internal_api.modules.clickhouse.pillar.encrypt', return_value='ENCRYPTED!')
        p = self.mk()
        p.update_user(
            'alice',
            password='blah',
            permissions=[
                {
                    'database_name': 'foo',
                },
                {
                    'database_name': 'bar',
                },
            ],
            settings={},
            quotas=[],
        )

        assert_that(
            p.as_dict(),
            at_path(
                '$.data.clickhouse.users',
                has_entry(
                    'alice',
                    {
                        'password': 'ENCRYPTED!',
                        'hash': 'ENCRYPTED!',
                        'databases': {
                            'foo': {},
                            'bar': {},
                        },
                        'settings': {},
                        'quotas': [],
                    },
                ),
            ),
        )

    def test_delete_user(self):
        p = self.mk()
        p.delete_user('alice')
        assert 'alice' not in p.pillar_users

    def test_zk_hosts(self):
        assert self.mk().zk_hosts == ['zk1.test', 'zk2.test', 'zk3.test']

    def test_get_db_options(self):
        assert self.mk().config == {'log_level': 'debug'}

    def test_update_db_options(self):
        p = self.mk()
        p.update_config({'consistant': True})
        p.update_config({'fast': True})

        assert_that(
            p.as_dict(),
            at_path(
                '$.data.clickhouse.config',
                has_entries(
                    consistant=True,
                    fast=True,
                    log_level='debug',
                ),
            ),
        )

    def test_get_version(self):
        version = self.mk().version
        assert version.major == 18
        assert version.minor == 12
        assert version.string == '18.12.17'

    def test_set_version(self, app_config):
        p = self.mk()
        p.set_version(Version(major=21, minor=4))
        assert_that(p.as_dict(), at_path('$.data.clickhouse.ch_version', equal_to('21.4.5.46')))

    def test_none_cloud_storage(self):
        p = self.mk(
            {
                'data': {
                    'cloud_storage': None,
                },
            }
        )
        assert p.cloud_storage == {}
        assert p.cloud_storage_bucket is None

    def test_add_kafka_topic(self, mocker):
        mocker.patch('dbaas_internal_api.modules.clickhouse.pillar.encrypt', return_value='ENCRYPTED!')

        p = self.mk()
        p.update_config(
            {
                'kafka_topics': [
                    {'name': 'a', 'settings': {'sasl_password': 'PASSWORD'}},
                ]
            }
        )

        assert (
            p.as_dict()['data']['clickhouse']['config']['kafka_topics'][0]['settings']['sasl_password'] == 'ENCRYPTED!'
        )

    def test_update_kafka_topics(self, mocker):
        mocker.patch('dbaas_internal_api.modules.clickhouse.pillar.encrypt', return_value='ENCRYPTED!')
        p = self.mk(
            {
                'data': {
                    'clickhouse': {
                        'config': {
                            'kafka_topics': [
                                {
                                    'name': 'a',
                                    'settings': {'sasl_password': 'ENCRYPTED!'},
                                }
                            ]
                        }
                    }
                }
            }
        )
        p.update_config(
            {
                'kafka_topics': [
                    {'name': 'a', 'settings': {'sasl_password': None}},
                    {'name': 'b', 'settings': {'sasl_password': 'PASSWORD'}},
                ]
            }
        )
        p.update_config(
            {
                'kafka_topics': [
                    {'name': 'a', 'settings': {'sasl_password': None}},
                    {'name': 'b', 'settings': {'sasl_password': ''}},
                    {'name': 'c', 'settings': {'sasl_password': 'PASSWORD'}},
                ]
            }
        )

        for topic in p.as_dict()['data']['clickhouse']['config']['kafka_topics']:
            assert topic['settings']['sasl_password'] == 'ENCRYPTED!'

    def test_delete_kafka_topic(self, mocker):
        mocker.patch('dbaas_internal_api.modules.clickhouse.pillar.encrypt', return_value='ENCRYPTED!')
        p = self.mk(
            {
                'data': {
                    'clickhouse': {
                        'config': {
                            'kafka_topics': [
                                {
                                    'name': 'a',
                                    'settings': {'sasl_password': 'ENCRYPTED!'},
                                },
                                {
                                    'name': 'b',
                                    'settings': {'sasl_password': 'ENCRYPTED!'},
                                },
                                {
                                    'name': 'c',
                                    'settings': {'sasl_password': 'ENCRYPTED!'},
                                },
                            ]
                        }
                    }
                }
            }
        )
        p.update_config(
            {
                'kafka_topics': [
                    {'name': 'a', 'settings': {'sasl_password': None}},
                    {'name': 'c', 'settings': {'sasl_password': None}},
                ]
            }
        )

        topics = p.as_dict()['data']['clickhouse']['config']['kafka_topics']
        assert topics[0]['name'] == 'a' and topics[0]['settings']['sasl_password'] == 'ENCRYPTED!'
        assert topics[1]['name'] == 'c' and topics[1]['settings']['sasl_password'] == 'ENCRYPTED!'


class TestZookeeperPillar:
    SAMPLE_PILLAR = {'data': {'zk': {'nodes': {}}}}

    @staticmethod
    def assert_nodes(p, nodes):
        assert_that(p.as_dict(), at_path('$.data.zk', has_entry('nodes', nodes)))

    def test_add_delete_nodes(self):
        p = ZookeeperPillar.load(deepcopy(TestZookeeperPillar.SAMPLE_PILLAR))
        p.add_nodes(['host1', 'host2', 'host3'])
        self.assert_nodes(p, {'host1': 1, 'host2': 2, 'host3': 3})

        p.add_node('host4')
        self.assert_nodes(p, {'host1': 1, 'host2': 2, 'host3': 3, 'host4': 4})

        p.delete_node('host2')
        self.assert_nodes(p, {'host1': 1, 'host3': 3, 'host4': 4})

        p.delete_node('host4')
        self.assert_nodes(
            p,
            {
                'host1': 1,
                'host3': 3,
            },
        )

        p.add_node('host2')
        self.assert_nodes(
            p,
            {
                'host1': 1,
                'host2': 2,
                'host3': 3,
            },
        )

        p.delete_node('host1')
        p.delete_node('host2')
        p.delete_node('host3')
        self.assert_nodes(p, {})

        p.add_node('host1')
        p.add_node('host2')
        p.add_node('host3')
        self.assert_nodes(
            p,
            {
                'host1': 1,
                'host2': 2,
                'host3': 3,
            },
        )

    def test_exceptions(self):
        p = ZookeeperPillar.load(deepcopy(TestZookeeperPillar.SAMPLE_PILLAR))
        p.add_nodes(['host1', 'host2', 'host3'])

        with pytest.raises(DbaasClientError):
            p.delete_node('unknown_host')

        with pytest.raises(DbaasError):
            p.add_node('host1')

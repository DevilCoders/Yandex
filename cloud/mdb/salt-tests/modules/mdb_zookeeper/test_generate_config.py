# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from collections import OrderedDict

import pytest

from cloud.mdb.salt.salt._modules import mdb_zookeeper
from cloud.mdb.salt_tests.common.mocks import mock_pillar


@pytest.mark.parametrize(
    ids=[
        'No reconfigEnabled, everything overridden',
        'No reconfigEnabled, everything overridden, enable_zk_tls',
        'reconfigEnabled set to true, everything overridden',
        'reconfigEnabled set to true, dynamicConfigFile kept',
    ],
    argnames=['pillar', 'params', 'nodes', 'zk_users', 'config_old', 'expected'],
    argvalues=[
        (
            {},
            {
                '1key': '1value',
                '0key': '0value',
            },
            {
                '0fqdn': 1,
                '1fqdn': 2,
            },
            {
                'super': {'password': 'rootpass'},
            },
            {'key': 'value'},
            OrderedDict(
                [
                    ('0key', '0value'),
                    ('1key', '1value'),
                    ('server.1', '0fqdn:2888:3888:participant;[::]:2181'),
                    ('server.2', '1fqdn:2888:3888:participant;[::]:2181'),
                ]
            ),
        ),
        (
            {
                'data': {
                    'unmanaged': {
                        'enable_zk_tls': True,
                    },
                },
            },
            {
                '1key': '1value',
                '0key': '0value',
            },
            {
                '0fqdn': 1,
                '1fqdn': 2,
            },
            {
                'super': {'password': 'rootpass'},
            },
            {'key': 'value'},
            OrderedDict(
                [
                    ('0key', '0value'),
                    ('1key', '1value'),
                    ('DigestAuthenticationProvider.superDigest', 'super:w9OGs+jPzie5rLL8Ve5vPrNO6LM='),
                    ('ssl.keyStore.password', 'dummypassword'),
                    ('ssl.quorum.keyStore.password', 'dummypassword'),
                    ('ssl.quorum.trustStore.password', 'dummypassword'),
                    ('ssl.trustStore.password', 'dummypassword'),
                    ('server.1', '0fqdn:2888:3888:participant;[::]:2181'),
                    ('server.2', '1fqdn:2888:3888:participant;[::]:2181'),
                ]
            ),
        ),
        (
            {
                'data': {
                    'unmanaged': {
                        'enable_zk_tls': False,
                    },
                },
            },
            {'0key': '0value', '1key': '1value', 'reconfigEnabled': True, 'clientPort': 666},
            {
                '0fqdn': 1,
                '1fqdn': 2,
            },
            {
                'super': {'password': 'rootpass'},
                'duper': {'password': 'otherpass'},
            },
            {'key': 'value'},
            OrderedDict(
                [
                    ('0key', '0value'),
                    ('1key', '1value'),
                    ('reconfigEnabled', True),
                    ('server.1', '0fqdn:2888:3888:participant;[::]:666'),
                    ('server.2', '1fqdn:2888:3888:participant;[::]:666'),
                ]
            ),
        ),
        (
            {},
            {'0key': '0value', '1key': '1value', 'reconfigEnabled': True},
            {
                '0fqdn': 1,
                '1fqdn': 2,
            },
            {
                'duper': {'password': 'otherpass'},
            },
            {
                'key': 'value',
                'dynamicConfigFile': '/path/to/dyn.conf',
            },
            OrderedDict(
                [
                    ('0key', '0value'),
                    ('1key', '1value'),
                    ('reconfigEnabled', True),
                    ('dynamicConfigFile', '/path/to/dyn.conf'),
                ]
            ),
        ),
    ],
)
def test_generate_config(pillar, params, nodes, zk_users, config_old, expected):
    mock_pillar(mdb_zookeeper.__salt__, pillar)
    result = mdb_zookeeper.generate_config(config_old, params, nodes, zk_users)
    result = unificate_passwords(result)
    assert result == expected


def unificate_passwords(config):
    # Random password in generated config
    passwords = [
        'ssl.keyStore.password',
        'ssl.trustStore.password',
        'ssl.quorum.keyStore.password',
        'ssl.quorum.trustStore.password',
    ]
    for password in passwords:
        if password in config:
            config[password] = 'dummypassword'
    return config


def test_render_config():
    config = {'1key': '1value', '0key': '0value', '2key': '2value'}
    result = mdb_zookeeper.render_config(config)
    assert result == '0key=0value\n' '1key=1value\n' '2key=2value\n'

# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt.salt._modules import mdb_elasticsearch
from cloud.mdb.salt_tests.common.mocks import mock_pillar

test_pillar = {
    'data': {
        'elasticsearch': {
            'version': '7.10.0',
            'auth': {
                'providers': [{'name': 'saml', 'type': 'saml', 'settings': {'order': 1}}],
                'realms': {
                    'native': [{'name': 'uniq', 'settings': {'order': 1}}],
                    'saml': [
                        {'name': 'saml1', 'settings': {'order': 4}},
                        {'name': 'saml2', 'settings': {'order': 5}, 'files': {'metadata': 'c2Q='}},
                    ],
                },
            },
        }
    }
}


def test_auth_files():
    mock_pillar(mdb_elasticsearch.__salt__, test_pillar)
    assert mdb_elasticsearch.auth_files() == [
        ('saml2_metadata', 'data:elasticsearch:auth:realms:saml:1:files:metadata')
    ]


def test_auth_chain_kibana():
    mock_pillar(mdb_elasticsearch.__salt__, test_pillar)
    assert mdb_elasticsearch.auth_chain_kibana() == {
        'xpack.security.authc.providers': {'basic.Native': {'order': 2}, 'saml.saml': {'order': 1}}
    }


def test_auth_chain_elastic():
    mock_pillar(mdb_elasticsearch.__salt__, test_pillar)
    assert mdb_elasticsearch.auth_chain_elastic() == {
        'xpack.security.authc.realms': {
            'file.File': {'order': 0},
            'native.uniq': {'order': 1},
            'saml.saml1': {'order': 4},
            'saml.saml2': {'order': 5},
        }
    }

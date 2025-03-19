# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt.salt._modules import mdb_elasticsearch
from cloud.mdb.salt_tests.common.mocks import mock_pillar, mock_version_cmp
from cloud.mdb.internal.python.pytest.utils import parametrize

SERVER_NAME_RESULT = (
    'c-cid.ro.mdb.cloud-preprod.yandex.net c-cid.rw.mdb.cloud-preprod.yandex.net '
    'rc1a-x.db.yandex.net rc1a-x.mdb.cloud-preprod.yandex.net'
)
SERVER_NAME_RESULT_NO_CID = 'rc1a-x.db.yandex.net rc1a-x.mdb.cloud-preprod.yandex.net'


@parametrize(
    {
        'id': 'Produce server_name parameter string',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid',
                    },
                },
            },
            'fqdn': 'rc1a-x.mdb.cloud-preprod.yandex.net',
            'result': SERVER_NAME_RESULT,
        },
    },
    {
        'id': 'Produce server_name parameter string without cluster id',
        'args': {
            'pillar': {},
            'fqdn': 'rc1a-x.mdb.cloud-preprod.yandex.net',
            'result': SERVER_NAME_RESULT_NO_CID,
        },
    },
)
def test_nginx_server_names(pillar, fqdn, result):
    mock_pillar(mdb_elasticsearch.__salt__, pillar)
    assert mdb_elasticsearch.nginx_server_names(fqdn=fqdn) == result


def test_version_ok():
    version = '7.10.2'
    mock_pillar(
        mdb_elasticsearch.__salt__,
        {
            'data': {
                'elasticsearch': {
                    'version': version,
                },
            },
        },
    )
    assert mdb_elasticsearch.version() == version


def test_version_greater_or_equal():
    version = '7.10'
    mock_version_cmp(mdb_elasticsearch.__salt__)
    mock_pillar(
        mdb_elasticsearch.__salt__,
        {
            'data': {
                'elasticsearch': {
                    'version': version,
                },
            },
        },
    )
    assert mdb_elasticsearch.version_ge(version)
    assert mdb_elasticsearch.version_ge('7.6')
    assert not mdb_elasticsearch.version_ge('7.11')


def test_version_less_than():
    version = '7.10'
    mock_version_cmp(mdb_elasticsearch.__salt__)
    mock_pillar(
        mdb_elasticsearch.__salt__,
        {
            'data': {
                'elasticsearch': {
                    'version': version,
                },
            },
        },
    )
    assert not mdb_elasticsearch.version_lt(version)
    assert mdb_elasticsearch.version_lt('7.11')


def test_pillar_plugins():
    mock_pillar(
        mdb_elasticsearch.__salt__,
        {
            'data': {
                'elasticsearch': {
                    'plugins': ['someone'],
                },
            },
        },
    )
    assert mdb_elasticsearch.pillar_plugins() == set(['someone', 'repository-s3'])

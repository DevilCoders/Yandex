# coding: utf-8

from __future__ import print_function, unicode_literals

from cloud.mdb.salt.salt._modules import mdb_s3
from cloud.mdb.salt_tests.common.mocks import mock_pillar
from cloud.mdb.internal.python.pytest.utils import parametrize


@parametrize(
    {
        'id': 'https+path schema, with protocol overriding',
        'args': {
            'pillar': {
                'data': {
                    's3': {
                        'endpoint': 'https+path://s3.mds.yandex.net',
                    },
                },
            },
            'args': [],
            'result': 'https://s3.mds.yandex.net',
        },
    },
    {
        'id': 'http+path schema, with protocol overriding',
        'args': {
            'pillar': {
                'data': {
                    's3': {
                        'endpoint': 'http+path://s3.mds.yandex.net',
                    },
                },
            },
            'args': [],
            'result': 'http://s3.mds.yandex.net',
        },
    },
    {
        'id': 'https+path schema, without protocol overriding',
        'args': {
            'pillar': {
                'data': {
                    's3': {
                        'endpoint': 'https+path://s3.mds.yandex.net',
                    },
                },
            },
            'args': ['http'],
            'result': 'http://s3.mds.yandex.net',
        },
    },
    {
        'id': 'http+path schema, without protocol overriding',
        'args': {
            'pillar': {
                'data': {
                    's3': {
                        'endpoint': 'http+path://s3.mds.yandex.net',
                    },
                },
            },
            'args': ['http'],
            'result': 'http://s3.mds.yandex.net',
        },
    },
)
def test_endpoint(pillar, args, result):
    mock_pillar(mdb_s3.__salt__, pillar)
    assert mdb_s3.endpoint(*args) == result

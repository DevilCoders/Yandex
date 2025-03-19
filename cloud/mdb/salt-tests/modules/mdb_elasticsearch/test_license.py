# -*- coding: utf-8 -*-

from __future__ import unicode_literals

import pytest

from cloud.mdb.salt.salt._modules import mdb_elasticsearch
from cloud.mdb.salt_tests.common.mocks import mock_pillar
from cloud.mdb.internal.python.pytest.utils import parametrize


@parametrize(
    {
        'id': 'OSS licensed cluster',
        'args': {
            'edition': 'oss',
            'pillar': {
                'data': {
                    'elasticsearch': {
                        'edition': 'oss',
                    },
                },
            },
            'result': True,
        },
    },
    {
        'id': 'Platinum licensed cluster',
        'args': {
            'edition': 'platinum',
            'pillar': {
                'data': {
                    'elasticsearch': {
                        'edition': 'platinum',
                    },
                },
            },
            'result': True,
        },
    },
    {
        'id': 'Default license is OSS',
        'args': {
            'edition': 'oss',
            'pillar': {},
            'result': True,
        },
    },
    {
        'id': 'Default license is not Platinum',
        'args': {
            'edition': 'platinum',
            'pillar': {},
            'result': False,
        },
    },
    {
        'id': 'Gold license is not Enterprise',
        'args': {
            'edition': 'enterprise',
            'pillar': {
                'data': {
                    'elasticsearch': {
                        'edition': 'gold',
                    },
                },
            },
            'result': False,
        },
    },
)
def test_licensed_for(edition, pillar, result):
    mock_pillar(mdb_elasticsearch.__salt__, pillar)
    assert mdb_elasticsearch.licensed_for(edition) == result


@parametrize(
    {
        'id': 'SuperPuper license does not exist',
        'args': {
            'edition': 'superpuper',
            'pillar': {
                'data': {
                    'elasticsearch': {
                        'edition': 'gold',
                    },
                },
            },
        },
    },
)
def test_unknown_licensed_for_fails(edition, pillar):
    mock_pillar(mdb_elasticsearch.__salt__, pillar)
    with pytest.raises(AssertionError):
        assert mdb_elasticsearch.licensed_for(edition)

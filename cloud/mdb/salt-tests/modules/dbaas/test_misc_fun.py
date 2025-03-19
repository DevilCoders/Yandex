# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt.salt._modules import dbaas
from cloud.mdb.salt_tests.common.mocks import mock_pillar, mock_grains
from cloud.mdb.internal.python.pytest.utils import parametrize


_PILLAR_DATA = {
    'controlplane': {'data': {}},
    'dataplane': {
        'data': {
            'dbaas': {
                'cluster_id': 'xxx',
            },
        },
    },
}

_GRAINS_DATA = {
    'compute-prod': {
        'id': 'rc1a-abyrvalg.mdb.yandexcloud.net',
    },
    'compute-prod-controlplane': {
        'id': 'something01k.yandexcloud.net',
    },
}


# Domain tricks
@parametrize(
    {
        'id': 'Correct management hostname in dataplane',
        'args': {
            'grains': _GRAINS_DATA['compute-prod'],
            'domain': 'db.yandex.net',
            'result': 'rc1a-abyrvalg.db.yandex.net',
        },
    },
    {
        'id': 'Correct management hostname in controlplane',
        'args': {
            'grains': _GRAINS_DATA['compute-prod-controlplane'],
            'domain': 'control.yandexcloud.net',
            'result': 'something01k.control.yandexcloud.net',
        },
    },
)
def test_domain_change(grains, domain, result):
    mock_grains(dbaas.__salt__, grains)
    assert dbaas.to_domain(domain) == result


# Dataplane check
@parametrize(
    {
        'id': 'Dataplane is detected correctly',
        'args': {
            'pillar': _PILLAR_DATA['dataplane'],
            'result': True,
        },
    },
    {
        'id': 'Controlplane is detected correctly',
        'args': {
            'pillar': _PILLAR_DATA['controlplane'],
            'result': False,
        },
    },
)
def test_is_dataplane(pillar, result):
    mock_pillar(dbaas.__salt__, pillar)
    assert dbaas.is_dataplane() == result

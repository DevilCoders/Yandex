# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt.salt._modules import dbaas
from cloud.mdb.salt_tests.common.mocks import mock_pillar
from cloud.mdb.internal.python.pytest.utils import parametrize


_PILLAR_DATA = {
    'porto-prod': {
        'data': {
            'dbaas': {
                'vtype': 'porto',
            },
        },
        'yandex': {
            'environment': 'prod',
        },
    },
    'porto-test': {
        'data': {
            'dbaas': {
                'vtype': 'porto',
            },
        },
        'yandex': {
            'environment': 'qa',
        },
    },
    'compute-prod': {
        'data': {
            'dbaas': {
                'vtype': 'compute',
            },
        },
        'yandex': {
            'environment': 'compute-prod',
        },
    },
    'compute-preprod': {
        'data': {
            'dbaas': {
                'vtype': 'compute',
            },
        },
        'yandex': {
            'environment': 'qa',
        },
    },
}


# Installation names
@parametrize(
    {
        'id': 'Installation is porto',
        'args': {
            'pillar': _PILLAR_DATA['porto-prod'],
            'result': 'porto',
        },
    },
    {
        'id': 'Installation is compute',
        'args': {
            'pillar': _PILLAR_DATA['compute-preprod'],
            'result': 'compute',
        },
    },
    {
        'id': 'Installation is undefined',
        'args': {
            'pillar': {},
            'result': 'porto',
        },
    },
)
def test_detect_installation(pillar, result):
    mock_pillar(dbaas.__salt__, pillar)
    assert dbaas.installation() == result


# Is prod?
@parametrize(
    {
        'id': 'porto-prod is production',
        'args': {
            'pillar': _PILLAR_DATA['porto-prod'],
            'result': True,
        },
    },
    {
        'id': 'porto-test is not prod',
        'args': {
            'pillar': _PILLAR_DATA['porto-test'],
            'result': False,
        },
    },
    {
        'id': 'compute-preprod is not prod',
        'args': {
            'pillar': _PILLAR_DATA['compute-preprod'],
            'result': False,
        },
    },
    {
        'id': 'compute-prod is production',
        'args': {
            'pillar': _PILLAR_DATA['compute-prod'],
            'result': True,
        },
    },
)
def test_detect_production(pillar, result):
    mock_pillar(dbaas.__salt__, pillar)
    assert dbaas.is_prod() == result


# Is compute?
@parametrize(
    {
        'id': 'porto-prod is not compute',
        'args': {
            'pillar': _PILLAR_DATA['porto-prod'],
            'result': False,
        },
    },
    {
        'id': 'compute-preprod is compute',
        'args': {
            'pillar': _PILLAR_DATA['compute-preprod'],
            'result': True,
        },
    },
)
def test_detect_compute(pillar, result):
    mock_pillar(dbaas.__salt__, pillar)
    assert dbaas.is_compute() == result


# Is porto?
@parametrize(
    {
        'id': 'porto-prod is in porto',
        'args': {
            'pillar': _PILLAR_DATA['porto-prod'],
            'result': True,
        },
    },
    {
        'id': 'compute-preprod is not in porto',
        'args': {
            'pillar': _PILLAR_DATA['compute-preprod'],
            'result': False,
        },
    },
)
def test_detect_porto(pillar, result):
    mock_pillar(dbaas.__salt__, pillar)
    assert dbaas.is_porto() == result

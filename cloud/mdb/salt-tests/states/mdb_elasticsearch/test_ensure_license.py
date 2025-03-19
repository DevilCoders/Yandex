# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.internal.python.pytest.utils import parametrize
from cloud.mdb.salt_tests.common.mocks import mock_pillar
from cloud.mdb.salt.salt._states import mdb_elasticsearch
from cloud.mdb.salt.salt._modules.mdb_elasticsearch import __salt__

from mock import MagicMock, call


@parametrize(
    {
        'id': 'Nothing in pillar',
        'args': {
            'pillar': {},
            'opts': {'test': False},
            'installed_license': {"license": {"uid": "123", "type": "basic"}},
            'edition': 'platinum',
            'calls': {},
            'result': {'name': 'name', 'result': True, 'changes': {}, 'comment': 'License already in sync.'},
        },
    },
    {
        'id': 'Nothing changed',
        'args': {
            'pillar': {
                'data': {
                    'elasticsearch': {
                        'license': {'platinum': '{"license": {"uid": "123"}}', 'gold': '{"license": {"uid": "1234"}}'}
                    }
                }
            },
            'opts': {'test': False},
            'installed_license': {"license": {"uid": "123"}},
            'edition': 'platinum',
            'calls': {},
            'result': {'name': 'name', 'result': True, 'changes': {}, 'comment': 'License already in sync.'},
        },
    },
    {
        'id': 'Test update license',
        'args': {
            'pillar': {
                'data': {
                    'elasticsearch': {
                        'license': {'platinum': '{"license": {"uid": "123"}}', 'gold': '{"license": {"uid": "1234"}}'}
                    }
                }
            },
            'opts': {'test': True},
            'installed_license': {"license": {"uid": "1234"}},
            'edition': 'platinum',
            'calls': {},
            'result': {
                'name': 'name',
                'result': None,
                'changes': {'license': 'updated'},
                'comment': 'License would be updated',
            },
        },
    },
    {
        'id': 'Real update license',
        'args': {
            'pillar': {
                'data': {
                    'elasticsearch': {
                        'license': {'platinum': '{"license": {"uid": "123"}}', 'gold': '{"license": {"uid": "1234"}}'}
                    }
                }
            },
            'opts': {'test': False},
            'installed_license': {"license": {"uid": "1234"}},
            'edition': 'platinum',
            'calls': {'update_license': [call('{"license": {"uid": "123"}}')]},
            'result': {
                'name': 'name',
                'result': True,
                'changes': {'license': 'updated'},
                'comment': 'License was updated',
            },
        },
    },
    {
        'id': 'Nothing changed basic',
        'args': {
            'pillar': {
                'data': {
                    'elasticsearch': {
                        'license': {'platinum': '{"license": {"uid": "123"}}', 'gold': '{"license": {"uid": "1234"}}'}
                    }
                }
            },
            'opts': {'test': False},
            'installed_license': {"license": {"uid": "1234", "type": "basic"}},
            'edition': 'basic',
            'calls': {},
            'result': {'name': 'name', 'result': True, 'changes': {}, 'comment': 'License already in sync.'},
        },
    },
    {
        'id': 'Test remove license',
        'args': {
            'pillar': {
                'data': {
                    'elasticsearch': {
                        'license': {'platinum': '{"license": {"uid": "123"}}', 'gold': '{"license": {"uid": "1234"}}'}
                    }
                }
            },
            'opts': {'test': True},
            'installed_license': {"license": {"uid": "123", "type": "platinum"}},
            'edition': 'basic',
            'calls': {},
            'result': {
                'name': 'name',
                'result': None,
                'changes': {'license': 'removed'},
                'comment': 'License would be removed',
            },
        },
    },
    {
        'id': 'Real remove license',
        'args': {
            'pillar': {
                'data': {
                    'elasticsearch': {
                        'license': {'platinum': '{"license": {"uid": "123"}}', 'gold': '{"license": {"uid": "1234"}}'}
                    }
                }
            },
            'opts': {'test': False},
            'installed_license': {"license": {"uid": "123", "type": "platinum"}},
            'edition': 'basic',
            'calls': {'delete_license': [call()]},
            'result': {
                'name': 'name',
                'result': True,
                'changes': {'license': 'removed'},
                'comment': 'License was removed',
            },
        },
    },
)
def test_ensure_license(pillar, opts, installed_license, edition, calls, result):
    mock_pillar(mdb_elasticsearch.__salt__, pillar)
    mock_pillar(__salt__, pillar)
    mdb_elasticsearch.__opts__ = opts

    mdb_elasticsearch.__salt__['mdb_elasticsearch.get_license'] = lambda: installed_license

    for function in calls.keys():
        mdb_elasticsearch.__salt__['mdb_elasticsearch.{0}'.format(function)] = MagicMock()

    assert mdb_elasticsearch.ensure_license('name', edition) == result

    for function, function_calls in calls.items():
        assert mdb_elasticsearch.__salt__['mdb_elasticsearch.{0}'.format(function)].call_args_list == function_calls

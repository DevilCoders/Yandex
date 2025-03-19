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
            'pillar_plugins': [],
            'installed_plugins': [],
            'update_plugins': [],
            'calls': {},
            'result': {'name': 'name', 'result': True, 'changes': {}, 'comment': 'All plugins already in sync.'},
        },
    },
    {
        'id': 'Nothing changed',
        'args': {
            'pillar': {},
            'opts': {'test': False},
            'pillar_plugins': ['analysis-icu'],
            'installed_plugins': ['analysis-icu'],
            'update_plugins': [],
            'calls': {},
            'result': {'name': 'name', 'result': True, 'changes': {}, 'comment': 'All plugins already in sync.'},
        },
    },
    {
        'id': 'Test install and remove plugins',
        'args': {
            'pillar': {},
            'opts': {'test': True},
            'pillar_plugins': ['analysis-icu', 'mapper-size'],
            'installed_plugins': ['analysis-icu', 'mapper-lenght'],
            'update_plugins': [],
            'calls': {},
            'result': {
                'name': 'name',
                'result': None,
                'changes': {'plugins_install': ['mapper-size'], 'plugins_remove': ['mapper-lenght']},
                'comment': '',
            },
        },
    },
    {
        'id': 'Real install and remove plugins',
        'args': {
            'pillar': {},
            'opts': {'test': False},
            'pillar_plugins': ['analysis-icu', 'mapper-size', 'analysis-phonetic'],
            'installed_plugins': ['analysis-phonetic', 'analysis-icu', 'mapper-lenght'],
            'update_plugins': [],
            'calls': {'remove_plugin': [call('mapper-lenght')], 'install_plugin': [call('mapper-size')]},
            'result': {
                'name': 'name',
                'result': True,
                'changes': {'plugins_installed': ['mapper-size'], 'plugins_removed': ['mapper-lenght']},
                'comment': '',
            },
        },
    },
    {
        'id': 'Just update plugin',
        'args': {
            'pillar': {},
            'opts': {'test': False},
            'pillar_plugins': ['analysis-icu', 'analysis-phonetic'],
            'installed_plugins': ['analysis-phonetic', 'analysis-icu'],
            'update_plugins': ['analysis-icu'],
            'calls': {'remove_plugin': [call('analysis-icu')], 'install_plugin': [call('analysis-icu')]},
            'result': {
                'name': 'name',
                'result': True,
                'changes': {'plugins_installed': ['analysis-icu'], 'plugins_removed': ['analysis-icu']},
                'comment': '',
            },
        },
    },
    {
        'id': 'Not update removed plugin',
        'args': {
            'pillar': {},
            'opts': {'test': False},
            'pillar_plugins': ['analysis-phonetic'],
            'installed_plugins': ['analysis-phonetic', 'analysis-icu'],
            'update_plugins': ['analysis-icu'],
            'calls': {'remove_plugin': [call('analysis-icu')], 'install_plugin': []},
            'result': {
                'name': 'name',
                'result': True,
                'changes': {'plugins_installed': [], 'plugins_removed': ['analysis-icu']},
                'comment': '',
            },
        },
    },
)
def test_ensure_plugins(pillar, opts, pillar_plugins, installed_plugins, update_plugins, calls, result):
    mock_pillar(mdb_elasticsearch.__salt__, pillar)
    mock_pillar(__salt__, pillar)
    mdb_elasticsearch.__opts__ = opts

    mdb_elasticsearch.__salt__['mdb_elasticsearch.pillar_plugins'] = lambda: pillar_plugins

    def mock_installed_and_update_plugins():
        return installed_plugins, update_plugins

    mdb_elasticsearch.__salt__['mdb_elasticsearch.installed_and_update_plugins'] = mock_installed_and_update_plugins

    for function in calls.keys():
        mdb_elasticsearch.__salt__['mdb_elasticsearch.{0}'.format(function)] = MagicMock()

    assert mdb_elasticsearch.ensure_plugins('name') == result

    for function, function_calls in calls.items():
        assert mdb_elasticsearch.__salt__['mdb_elasticsearch.{0}'.format(function)].call_args_list == function_calls

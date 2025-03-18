# coding: utf-8

from __future__ import unicode_literals

import pytest

from ids.connector.plugins_lib import PluginBase, get_disjunctive_plugin_chain
from ids.exceptions import ConfigurationError


class PluginWithoutRequiredParams(PluginBase):
    def prepare_params(self, params):
        params[self.__class__.__name__] = None


class PluginWithRequiredParams1(PluginWithoutRequiredParams):
    required_params = ['a', 'b']


class PluginWithRequiredParams2(PluginWithoutRequiredParams):
    required_params = ['1', '2']


def test_optional_plugin_chain_with_paramless():
    plugin = get_disjunctive_plugin_chain([PluginWithoutRequiredParams])(None)
    plugin.check_required_params({})
    plugin.check_required_params({'a', 'b'})

    params = {}
    plugin.check_required_params(params)
    plugin.prepare_params(params)
    assert 'PluginWithoutRequiredParams' in params

    plugin = get_disjunctive_plugin_chain([PluginWithoutRequiredParams, PluginWithRequiredParams1])(None)

    plugin.check_required_params({})
    plugin.check_required_params({'a', 'b'})

    params = {}
    plugin.check_required_params(params)
    plugin.prepare_params(params)
    assert 'PluginWithoutRequiredParams' in params
    assert 'PluginWithRequiredParams1' not in params

    params = {'a': 1, 'b': 1}
    plugin.check_required_params(params)
    plugin.prepare_params(params)
    assert 'PluginWithoutRequiredParams' in params
    assert 'PluginWithRequiredParams1' not in params

    plugin = get_disjunctive_plugin_chain([PluginWithRequiredParams1, PluginWithoutRequiredParams])(None)

    params = {}
    plugin.check_required_params(params)
    plugin.prepare_params(params)
    assert 'PluginWithoutRequiredParams' in params
    assert 'PluginWithRequiredParams1' not in params

    params = {'a': 1, 'b': 1}
    plugin.check_required_params(params)
    plugin.prepare_params(params)
    assert 'PluginWithoutRequiredParams' not in params
    assert 'PluginWithRequiredParams1' in params

def test_optional_plugin_chain_without_paramless():
    plugin = get_disjunctive_plugin_chain([PluginWithRequiredParams1, PluginWithRequiredParams2])(None)

    with pytest.raises(ConfigurationError):
        plugin.check_required_params({'a'})

    plugin.check_required_params({'a', 'b'})
    plugin.check_required_params({'1', '2'})
    plugin.check_required_params({'a', 'b', '1', '2'})

    params = {'1': 1, '2': 1}
    plugin.check_required_params(params)
    plugin.prepare_params(params)
    assert 'PluginWithRequiredParams2' in params
    assert 'PluginWithRequiredParams1' not in params

    params = {'a': 1, 'b': 1, '1': 1, '2': 1}
    plugin.check_required_params(params)
    plugin.prepare_params(params)
    assert 'PluginWithRequiredParams1' in params
    assert 'PluginWithRequiredParams2' not in params

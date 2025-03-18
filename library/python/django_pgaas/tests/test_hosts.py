# coding: utf-8
from __future__ import unicode_literals

import mock
import pytest

from django_pgaas import HostManager


def test_get_rank():
    hm = HostManager([], current_dc='myt')
    assert hm.get_rank('myt') == 0


def test_sorted_hosts():
    hosts = [('foo', 'sas'), ('bar', 'man'), ('baz', 'vla')]
    hm = HostManager(hosts, current_dc='man')
    assert hm.sorted_hosts == ['bar', 'baz', 'foo']


def test_host_string():
    hosts = [('foo', 'sas'), ('bar', 'man'), ('baz', 'vla')]
    hm = HostManager(hosts, current_dc='man')
    assert hm.host_string == 'bar,baz,foo'


@pytest.mark.parametrize('dc', ('fooo', None))
def test_invalid_current_dc(dc):

    hosts = [('foo', 'sas'), ('bar', 'man'), ('baz', 'vla')]

    hm = HostManager(hosts, current_dc=dc)

    # Expect hosts in some arbitrary order
    assert set(hm.sorted_hosts) == {'baz', 'foo', 'bar'}


@pytest.mark.parametrize('env_var', ('QLOUD_DATACENTER', 'DEPLOY_NODE_DC'))
def test_current_dc_from_env(monkeypatch, env_var):
    monkeypatch.setenv(env_var, 'man')

    hosts = [('foo', 'sas'), ('bar', 'man'), ('baz', 'vla')]

    hm = HostManager(hosts)
    assert hm.sorted_hosts == ['bar', 'baz', 'foo']


def test_create_from_yc():

    yc_hosts = [
        {"name": "foo", "options": {"geo": "sas", "type": "postgresql"}},
        {"name": "bar", "options": {"geo": "man", "type": "postgresql"}},
        {"name": "baz", "options": {"geo": "vla", "type": "postgresql"}},
    ]

    hm = HostManager.create_from_yc(yc_hosts, current_dc='man')
    assert hm.sorted_hosts == ['bar', 'baz', 'foo']


@pytest.mark.parametrize('randomize_same_dc', (True, False))
def test_randomize_same_dc(randomize_same_dc):
    yc_hosts = [
        {"name": "foo", "options": {"geo": "sas", "type": "postgresql"}},
        {"name": "bar", "options": {"geo": "man", "type": "postgresql"}},
        {"name": "baz", "options": {"geo": "man", "type": "postgresql"}},
        {"name": "qux", "options": {"geo": "man", "type": "postgresql"}},
    ]
    hm = HostManager.create_from_yc(
        yc_hosts, current_dc='man', randomize_same_dc=randomize_same_dc
    )
    assert hm._randomize_same_dc == randomize_same_dc

    with mock.patch('django_pgaas.hosts.random', side_effect=[0.0, 0.1, 0.2, 0.3]):
        assert hm.host_string == 'bar,baz,qux,foo'

    with mock.patch('django_pgaas.hosts.random', side_effect=[0.3, 0.2, 0.1, 0.0]) as mocked:
        if randomize_same_dc:
            assert hm.host_string == 'qux,baz,bar,foo'
        else:
            assert hm.host_string == 'bar,baz,qux,foo'
            assert not mocked.called

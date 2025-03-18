"""Tests hosts."""

from __future__ import unicode_literals

import os
import random

from tests.util import get_json

import pytest

unstable_only = pytest.mark.unstable_only


def test_get(wbe):
    host = 'beatrice.search.yandex.net'
    assert wbe.api.get("/unstable/hosts/{}".format(host))["host"] == host


def test_get_all(wbe):
    wbe.api.get("/unstable/hosts")["host_names"]


def test_get_group_hosts(wbe):
    service_hosts = wbe.api.get("/unstable/groups/ALL_GENCFG_NEW/hosts")
    zero_hosts = wbe.api.get("/unstable/groups/ALL_GENCFG_NEW/hosts?excluded_groups=ALL_GENCFG_WBE_NEW")
    assert service_hosts["hosts_info"]["hosts"]
    assert not zero_hosts["hosts_info"]["hosts"]


@unstable_only
def test_move_hosts(wbe):
    msk_reserved_hosts = wbe.api.get("/unstable/groups/MSK_RESERVED")["hosts"]
    msk_web_hosts = wbe.api.get("/unstable/groups/MSK_WEB_BASE")["hosts"]

    assert msk_reserved_hosts
    assert (not (set(msk_reserved_hosts) & set(msk_web_hosts)))

    target_host = sorted(msk_reserved_hosts)[0]

    add_request = {
        'action': 'add',
        'hosts': [target_host],
    }
    wbe.api.post("/unstable/groups/MSK_WEB_BASE/hosts", add_request)
    assert (target_host in wbe.api.get("/unstable/groups/MSK_WEB_BASE")["hosts"])

    remove_request = {
        'action': 'remove',
        'target': 'MSK_RESERVED',
        'hosts': [target_host],
    }
    wbe.api.post("/unstable/groups/MSK_WEB_BASE/hosts", remove_request)
    assert (target_host not in wbe.api.get("/unstable/groups/MSK_WEB_BASE")["hosts"])


@unstable_only
def test_move_slave_hosts(wbe):
    def find_master_and_slave_groups(wbe):
        groups = wbe.api.get('/unstable/groups')["groups"]
        for name, group in groups.items():
            group["name"] = name
            group["slaves"] = []

        masters = []
        for group in groups.values():
            if group["master"]:
                groups[group["master"]]["slaves"].append(group)
            else:
                masters.append(group)

        masters = sorted(masters, cmp=lambda x, y: cmp(x["name"], y["name"]))
        for master in masters:
            slaves = master["slaves"]
            master["hosts"] = wbe.api.get("/unstable/groups/%s" % master["name"])["hosts"]
            independant_slaves = [slave for slave in slaves if slave["donor"] is None]
            for slave in independant_slaves:
                slave["hosts"] = wbe.api.get("/unstable/groups/%s" % slave["name"])["hosts"]

                diff = set(master["hosts"]) - set(slave["hosts"])
                if diff and master["hosts"]:
                    return master["name"], slave["name"], sorted(diff)[0]
        assert False

    master_group, slave_group, target_host = find_master_and_slave_groups(wbe)

    add_request = {
        'action': 'add',
        'hosts': [target_host],
    }
    wbe.api.post("/unstable/groups/%s/hosts" % slave_group, add_request)
    assert (target_host in wbe.api.get("/unstable/groups/%s" % slave_group)["hosts"])

    remove_request = {
        'action': 'remove',
        'hosts': [target_host],
    }
    wbe.api.post("/unstable/groups/%s/hosts" % slave_group, remove_request)
    assert (target_host not in wbe.api.get("/unstable/groups/%s" % slave_group)["hosts"])

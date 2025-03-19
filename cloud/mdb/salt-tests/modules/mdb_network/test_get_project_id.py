# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt.salt._modules import mdb_network


def test_get_project_id():
    tests = {
        '2a02:6b8:c0e:501:0:f806:0:b9': '0xf806',
        '2a02:6b8:b010:9604:0:1589:b264:f644': '0x1589',
    }
    for ip, pid in tests.items():
        assert mdb_network.get_project_id(ip) == pid

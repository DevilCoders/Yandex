"""
Test bot enrichment
"""

import json
import yatest

from bot_lib.enrich import __ssd_count as ssd_count
from bot_lib.enrich import __ram as ram
from bot_lib.enrich import __ram_total as ram_total
from bot_lib.enrich import __calc_field as calc_field
from bot_lib.enrich import __hdd_total as hdd_total
from bot_lib.enrich import __ssd_list as ssd_list
from bot_lib.fields import __load_attrs as load_attrs
from bot_lib.fields import ATTRS as attrs_f
from bot_lib.fields import SERVERS as servers_f
from bot_lib.fields import RAM as ram_f
from bot_lib.fields import NODES as nodes_f


def _load_srv():
    with open(yatest.common.source_path("cloud/mdb/tools/bot/tests/srv.json"), 'r') as f:
        return json.load(f)["data"]


class TestEnrich:

    def test_ssd_count(self):
        assert ssd_count(_load_srv()) == 4

    def test_ram_count(self):
        assert len(ram(_load_srv())) == 8

    def test_ram_total(self):
        assert ram_total(_load_srv()) == 8*16

    def test_ssd_count_refl(self):
        assert calc_field(_load_srv(), 'SSD_COUNT') == 4

    def test_hdd_count_refl(self):
        assert calc_field(_load_srv(), 'HDD_COUNT') == 0

    def test_hdd_total(self):
        assert hdd_total(_load_srv()) == 0

    def test_ssd_list(self):
        assert ssd_list(_load_srv()) == "[600, 480, 900, 960]"

    def test_load_attrs(self):
        load_attrs()
        assert len(attrs_f.keys()) == 4

    def test_server_attrs_count(self):
        load_attrs()
        assert len(servers_f.keys()) == 20

    def test_test_unit_size(self):
        load_attrs()
        assert servers_f['UNITSIZE'] == 'attribute14'

    def test_ram_attrs_count(self):
        load_attrs()
        assert len(ram_f.keys()) == 6

    def test_nodes_attrs_count(self):
        load_attrs()
        assert len(nodes_f.keys()) == 20

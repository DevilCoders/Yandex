import yatest

import requests_mock

from bot_lib.bot_client import BotClient, BOT_ID_INV, BOT_ID_FQDN
from bot_lib.fields import DEFAULT_CALC_FIELDS
from bot_lib.fields import _DEFAULT_FIELDS


def _load_srv(inv):
    with open(yatest.common.source_path("cloud/mdb/tools/bot/tests/{}.json".format(inv)), 'r') as f:
        return f.read()


class TestBotClient:

    def test_inv_900918002(self):
        with requests_mock.Mocker() as m:
            m.get("https://bot.yandex-team.ru/api/consistof.php?inv=900918002&format=json",
                          text=_load_srv("900918002"), status_code=200)
        c = BotClient()
        data, disks, mem, total = c.servers(BOT_ID_INV, ['900918002'])
        assert len(data[0].keys()) == len(_DEFAULT_FIELDS + DEFAULT_CALC_FIELDS)
        assert data[0]['DISKSLOTS'] == '4'
        assert data[0]['CPUMODEL'] == 'INTEL XEON E5-2650 V2'

        assert len(disks.keys()) == 1
        assert len(disks[list(disks.keys())[0]]) == 4
        assert len(mem[list(mem.keys())[0]]) == 8
        assert total['COUNT'] == 1
        assert total['SSD_TOTAL'] == 240
        assert total['RAM_TOTAL'] == 128
        assert total['HDD_TOTAL'] == 6000

    def test_inv_101335004(self):
        with requests_mock.Mocker() as m:
            m.get("https://bot.yandex-team.ru/api/consistof.php?inv=101335004&format=json",
                          text=_load_srv("101335004"), status_code=200)
        c = BotClient()
        data, disks, mem, total = c.servers(BOT_ID_INV, ['101335004'])
        assert len(data[0].keys()) == len(_DEFAULT_FIELDS + DEFAULT_CALC_FIELDS)
        assert data[0]['DISKSLOTS'] == '8'
        assert data[0]['CPUMODEL'] == "INTEL XEON E5-2660 V4"

        assert len(disks.keys()) == 1
        assert len(disks[list(disks.keys())[0]]) == 8
        assert len(mem[list(mem.keys())[0]]) == 8
        assert total['COUNT'] == 1
        assert total['SSD_TOTAL'] == 15360
        assert total['RAM_TOTAL'] == 256
        assert total['HDD_TOTAL'] == 0

    def test_two_invs(self):
        with requests_mock.Mocker() as m:
            m.get("https://bot.yandex-team.ru/api/consistof.php?inv=900918002&format=json",
                  text=_load_srv("900918002"), status_code=200)
            m.get("https://bot.yandex-team.ru/api/consistof.php?inv=900918363&format=json",
                  text=_load_srv("900918363"), status_code=200)

        c = BotClient()
        data, disks, mem, total = c.servers(BOT_ID_INV, ['900918002', '900918363'])
        assert len(data) == 2
        assert total['COUNT'] == 2
        assert total['SSD_TOTAL'] == 3440
        assert total['RAM_TOTAL'] == 384
        assert total['HDD_TOTAL'] == 6000

    def test_fqdn(self):
        with requests_mock.Mocker() as m:
            m.get("https://bot.yandex-team.ru/api/consistof.php?name=sas2-9179.search.yandex.net&format=json",
                          text=_load_srv("sas2-9179.search.yandex.net"), status_code=200)
        c = BotClient()
        data, disk, mem, total = c.servers(BOT_ID_FQDN, ['sas2-9179.search.yandex.net'])
        assert len(data[0].keys()) == len(_DEFAULT_FIELDS + DEFAULT_CALC_FIELDS)
        assert data[0]['DISKSLOTS'] == '4'
        assert data[0]['CPUMODEL'] == 'INTEL XEON E5-2650 V2'

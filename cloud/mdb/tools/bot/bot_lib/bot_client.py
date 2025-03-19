"""
Bot helper API Client
"""
import logging
import requests
from typing import List, Dict

from .enrich import enrich_server, humanize

from .fields import _DEFAULT_FIELDS, REV_SRV_HUMAN_ATTRS
from .fields import DEFAULT_CALC_FIELDS
from .fields import SERVERS, NODES, DISK_DRIVERS, RAM
from .fields import C, C_TYPE_FIELD, C_DISKDRIVES, C_RAM

BOT_ID_INV = 0
BOT_ID_FQDN = 1


class BotClient(object):

    def __init__(self, logger=None, logging_level=logging.WARNING, api_url="https://bot.yandex-team.ru/api/"):
        self.api = api_url
        self.__BOT_URL_MAP = {BOT_ID_INV: "inv", BOT_ID_FQDN: "name"}

        if logger is not None:
            self.logger = logger
        else:
            logging.basicConfig()
            logger = logging.getLogger()
            logger.setLevel(logging_level)
            self.logger = logger

    def _find_by_id(self, id_type, id) -> (bool, str, object):
        assert id_type in self.__BOT_URL_MAP.keys()
        uri_template = "consistof.php?{}={}&format=json"
        resp = requests.get(self.api + uri_template.format(self.__BOT_URL_MAP[id_type], id))
        if resp.status_code != 200 and resp.status_code != 404:
            raise Exception(f"Not OK: {resp.status_code}")
        result = resp.json()
        if result["res"] == 1:
            logging.debug("Got data: {}".format(result["data"]))
            return True, None, result["data"]
        if result["res"] == 0:
            logging.warning(result["errmsg"])
            return False, result["errmsg"], None
        if result["res"] == 2:
            logging.warning(result["msg"])
            return False, result["msg"], None
        self.logger.error("Invalid statement: {}".format(result))
        return False, "invalid statement", None

    @staticmethod
    def _disks(servers: List) -> Dict:
        result = {}
        for s in servers:
            id = s[REV_SRV_HUMAN_ATTRS["INV"]]
            disks = [c for c in s[C] if c[C_TYPE_FIELD] == C_DISKDRIVES]
            disks_result = []
            for d_raw in disks:
                d = {}
                for k, v in DISK_DRIVERS.items():
                    d[k] = d_raw[v]
                d["INV"] = d_raw[REV_SRV_HUMAN_ATTRS["INV"]]
                disks_result.append(d)
            result[id] = disks_result
        return result

    @staticmethod
    def _mem(servers: List) -> Dict:
        result = {}
        for s in servers:
            id = s[REV_SRV_HUMAN_ATTRS["INV"]]
            mems = [c for c in s[C] if c[C_TYPE_FIELD] == C_RAM]
            mem_result = []
            for m_raw in mems:
                m = {}
                for k, v in RAM.items():
                    m[k] = m_raw[v]
                m["INV"] = m_raw[REV_SRV_HUMAN_ATTRS["INV"]]
                mem_result.append(m)
            result[id] = mem_result
        return result

    @staticmethod
    def _total(servers: List) -> Dict:
        result = {"COUNT": len(servers), "PHISICALCORES": sum(int(s["PHISICALCORES"]) for s in servers if s["PHISICALCORES"].isdigit()),
                  "RAM_TOTAL": sum(int(s["RAM_TOTAL"]) for s in servers if s["RAM_TOTAL"]),
                  "SSD_COUNT": sum(int(s["SSD_COUNT"]) for s in servers if s["SSD_COUNT"]),
                  "SSD_TOTAL": sum(int(s["SSD_TOTAL"]) for s in servers if s["SSD_TOTAL"]),
                  "HDD_COUNT": sum(int(s["HDD_COUNT"]) for s in servers if s["HDD_COUNT"]),
                  "HDD_TOTAL": sum(int(s["HDD_TOTAL"]) for s in servers if s["HDD_TOTAL"])}
        return result

    def servers(self, id_type: int, ids: List[str]) -> (List, Dict, Dict, Dict):
        assert id_type in self.__BOT_URL_MAP.keys()
        rf = self._refine_input(ids)
        raw_data = []
        for i in rf:
            found, msg, data = self._find_by_id(id_type, i)
            if found:
                raw_data.append(data)
        enriched_data = []
        disks_result = self._disks(raw_data)
        mem_result = self._mem(raw_data)
        for r in raw_data:
            enriched_data.append(humanize(self.reduce_fields(enrich_server(r))))
        total_result = self._total(enriched_data)
        return enriched_data, disks_result, mem_result, total_result

    def server_raw(self, id_type, id) -> (bool, str, object):
        assert id is not None and len(id) > 0
        assert id_type in self.__BOT_URL_MAP.keys()
        found, msg, data = self._find_by_id(id_type, id)
        return found, msg, data

    @staticmethod
    def _refine_input(input_list: List[str]) -> List[str]:
        result = [i.strip().lower() for i in input_list]
        return set(result)

    @staticmethod
    def reduce_fields(srv: List[str]):
        fields = _DEFAULT_FIELDS + DEFAULT_CALC_FIELDS
        r = {}
        item_type = None
        if srv["item_segment3"] == "SERVERS":
            item_type = SERVERS
        else:
            item_type = NODES

        for f in fields:
            if f not in srv.keys():
                rf = item_type[f]
                r[f] = srv[rf]
            else:
                r[f] = srv[f]
        return r

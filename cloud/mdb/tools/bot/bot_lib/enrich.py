import copy
from typing import Dict, List

from .fields import DEFAULT_CALC_FIELDS, DISK_DRIVERS, C_RAM, RAM, SRV_HUMAN_ATTRS, C_TYPE_FIELD, C_DISKDRIVES


def __components(data):
    return data["Components"]


def __disks(data):
    c = __components(data)
    disks = [i for i in c if i[C_TYPE_FIELD] == C_DISKDRIVES]
    return disks


def __get_ssds(data):
    disks = __disks(data)
    return [d for d in disks if d[DISK_DRIVERS["DISKTYPE"]] == "SSD"]


def __hdds(data):
    disks = __disks(data)
    return [d for d in disks if d[DISK_DRIVERS["DISKTYPE"]] == "HDD"]


def __hdd_count(data):
    return len(__hdds(data))


def __ssd_count(data):
    return len(__get_ssds(data))


def __ssd_total(data):
    ssds = __get_ssds(data)
    return sum([int(i[DISK_DRIVERS["DISKCAPACITY_GB"]]) for i in ssds])


def __ssd_list(data):
    ssds = __get_ssds(data)
    return str([int(i[DISK_DRIVERS["DISKCAPACITY_GB"]]) for i in ssds])


def __hdd_total(data):
    hdds = __hdds(data)
    return sum([int(i[DISK_DRIVERS["DISKCAPACITY_GB"]]) for i in hdds])


def __hdd_list(data):
    hdds = __hdds(data)
    return str([int(i[DISK_DRIVERS["DISKCAPACITY_GB"]]) for i in hdds])


def __ram(data):
    c = __components(data)
    return [i for i in c if i[C_TYPE_FIELD] == C_RAM]


def __ram_total(data):
    ram = __ram(data)
    return sum([int(i[RAM["RAMSIZE_GB"]]) for i in ram])


def __ram_list(data):
    ram = __ram(data)
    return str([int(i[RAM["RAMSIZE_GB"]]) for i in ram])


def __calc_field(data, field: str):
    func_name = "__" + field.lower()
    return globals()[func_name](data)


def enrich_server(raw_server_data: List, fields=DEFAULT_CALC_FIELDS):
    result = copy.deepcopy(raw_server_data)
    for f in fields:
        result[f] = __calc_field(result, f)
    return result


def humanize(src: Dict) -> Dict:
    result = {}
    for sk in src.keys():
        if sk in SRV_HUMAN_ATTRS.keys():
            result[SRV_HUMAN_ATTRS[sk]] = src[sk]
        else:
            result[sk] = src[sk]
    return result

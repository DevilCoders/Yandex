"""
Tskv utils
Format description: https://wiki.yandex-team.ru/statbox/LogRequirements/#tskvformat

"""
import re
from collections import OrderedDict


def tskv_key_escape(key):
    escaped = key.encode('unicode-escape').decode()
    escaped = re.sub('=', '\\=', escaped)
    return escaped


def tskv_key_unescape(key):
    unescaped = re.sub('\\\\=', '=', key)
    return unescaped


def tskv_value_escape(key):
    escaped = key.encode('unicode-escape').decode()
    return escaped


def dict_to_tskv(d):
    result = "tskv\t"
    result += "\t".join(["{}={}".format(tskv_key_escape(k),
                                        tskv_value_escape(str(d[k])))
                         for k in d])
    return result


def tskv_to_dict(tskv):
    result = OrderedDict()
    parts = tskv.split('\t')[1:]
    for kv in parts:
        (k, v) = re.split('(?<!\\\)=', kv, 1)
        k = tskv_key_unescape(k)
        result[k] = v
    return result

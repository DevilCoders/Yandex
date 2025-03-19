"""Aux functions to manipulate yaml-configs"""

import collections.abc
from typing import Dict


def merge_dicts(dct: Dict, merge_dct: Dict) -> Dict:
    """Recursive dict merge (works similar to dict.update)"""
    result = dct.copy()

    for k, v in merge_dct.items():
        if (k in dct) and (isinstance(dct[k], dict) and isinstance(merge_dct[k], collections.abc.Mapping)):
            result[k] = merge_dicts(dct[k], merge_dct[k])
        else:
            result[k] = merge_dct[k]

    return result

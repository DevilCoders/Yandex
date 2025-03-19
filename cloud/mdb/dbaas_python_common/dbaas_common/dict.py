# -*- coding: utf-8 -*-
"""
DBaaS config dict merge
"""

import collections.abc


def combine_dict(dict1, dict2):
    """
    This (relatively) simple function performs
    deep-merge of 2 dicts returning new dict
    """

    def _update(orig, update):
        for key, value in update.items():
            if isinstance(value, collections.abc.Mapping):
                orig[key] = _update(orig.get(key, {}), value)
            else:
                orig[key] = update[key]
        return orig

    _result = {}
    _update(_result, dict1)
    _update(_result, dict2)
    return _result

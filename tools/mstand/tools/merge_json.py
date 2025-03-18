#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

import copy

import yaqutils.json_helpers as ujson
import yaqutils.six_helpers as usix


def update(dst, src):
    if isinstance(dst, dict) and isinstance(src, dict):
        for key, value in usix.iteritems(src):
            if key in dst:
                update(dst[key], value)
            else:
                dst[key] = value
    elif isinstance(dst, list) and isinstance(src, list):
        dst.extend(src)
    else:
        assert dst == src


def merge(srcs):
    result = None
    for src in srcs:
        if result is None:
            result = copy.deepcopy(src)
        else:
            update(result, src)
    return result


def read_and_merge(paths):
    return merge(ujson.load_from_file(path) for path in paths)

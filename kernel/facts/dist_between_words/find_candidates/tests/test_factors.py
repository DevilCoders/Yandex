#!/usr/bin/env python
# -*- encoding:utf-8 -*-

from __future__ import print_function
from collections import Counter


def build_obj_fact(obj1, obj2, num):
    c1 = Counter(obj1[:num])
    c2 = Counter(obj2[:num])
    diff = sum(filter(lambda v: v > 0, (c1 - c2).itervalues()))
    return 1 - float(diff) / float(min(len(obj1), len(obj2), num))


def test_1():
    assert build_obj_fact(["a", "a", "b"], ["c", "d", "e"], 5) == 0.0
    assert build_obj_fact(["a", "h", "b"], ["c", "d", "e"], 5) == 0.0


def test_2():
    assert build_obj_fact(["a", "b", "c"], ["c", "b", "a"], 5) == 1.0
    assert build_obj_fact(["a", "a", "a"], ["a", "a", "a"], 5) == 1.0


def test_3():
    assert abs(build_obj_fact(["a", "b", "c"], ["a", "b", "d"], 5) - 2.0 / 3) < 0.001
    assert abs(build_obj_fact(["a", "b", "c"], ["d", "a", "d"], 5) - 1.0 / 3) < 0.001

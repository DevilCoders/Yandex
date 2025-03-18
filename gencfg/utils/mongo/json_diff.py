import unittest

__all__ = ['diff', 'merge']

"""
    assumptions:
        any list is set
        if it cannot be presented as a set, it's used as a terminal object
"""


class Types:
    List = 'list'
    Dict = 'dict'
    Terminal = 'term'

    @classmethod
    def get_type(cls, obj):
        if isinstance(obj, dict):
            return Types.Dict
        if isinstance(obj, list):
            try:
                set(obj)
                return Types.List
            except TypeError:
                return Types.Terminal
        return Types.Terminal


def both(a, b, type_):
    return Types.get_type(a) == Types.get_type(b) == type_


def both_dif(base, dif, type_):
    if type_ == dict:
        return both(base, dif, Types.Dict) and 's' in dif
    if type_ == list:
        return isinstance(base, type_) and isinstance(dif, dict) and 'p' in dif
    return False


def copy(data):
    if isinstance(data, list):
        return data[:]
    if isinstance(data, dict):
        return data.copy()
    return data


def eq(a, b):
    if a == b:
        return True
    if Types.get_type(a) != Types.get_type(b):
        return False
    if Types.get_type(a) == Types.List:
        return set(a) == set(b)
    if Types.get_type(a) == Types.Dict:
        return all((eq(a.get(key), b.get(key)) for key in set(a.keys() + b.keys())))
    return False


class DiffDict(object):
    def __init__(self, base=None, other=None):
        self.set = {}
        self.unset = set()
        if base is not None and other is not None:
            self.__init(base, other)

    def __init(self, base, other):
        for key in set(base).intersection(set(other)):
            if not eq(base[key], other[key]):
                self.set[key] = diff(base[key], other[key])
        for key in set(base).difference(set(other)):
            self.unset.add(key)
        for key in set(other).difference(set(base)):
            self.set[key] = other[key]

    @classmethod
    def load_dict(cls, dct):
        self = cls()
        self.set = dct['s']
        self.unset = dct.get('u', set())
        return self

    def dump_dict(self):
        if not self.unset:
            res = {'s': self.set}
        else:
            res = {'s': self.set, 'u': list(self.unset)}
        return res

    def merge(self, base):
        base = copy(base)
        for key in self.unset:
            del base[key]
        for key, value in self.set.iteritems():
            base[key] = merge(base.get(key), value)
        return base


class DiffList(object):
    def __init__(self, base=None, other=None):
        self.plus = set()
        self.minus = set()
        if base is not None and other is not None:
            self.__init(base, other)

    def __init(self, base, other):
        base, other = set(base), set(other)
        for key in base.difference(other):
            self.minus.add(key)
        for key in other.difference(base):
            self.plus.add(key)

    @classmethod
    def load_dict(cls, dct):
        self = cls()
        for one in dct['p']:
            self.plus.add(one)
        for one in dct.get('m', []):
            self.minus.add(one)
        return self

    def dump_dict(self):
        if self.minus:
            return {'p': list(self.plus), 'm': list(self.minus)}
        else:
            return {'p': list(self.plus)}

    def merge(self, base):
        base = set(base)
        for val in self.minus:
            base.remove(val)
        for val in self.plus:
            base.add(val)
        return list(base)


def diff(base, other):
    if both(base, other, Types.Dict):
        return DiffDict(base, other).dump_dict()
    if both(base, other, Types.List):
        return DiffList(base, other).dump_dict()
    return other


def merge(base, dif):
    base = copy(base)
    if both_dif(base, dif, dict):
        return DiffDict.load_dict(dif).merge(base)
    if both_dif(base, dif, list):
        return DiffList.load_dict(dif).merge(base)
    return dif


# TESTS


class Tests(unittest.TestCase):
    def test_all(self):
        vals = [
            {'a': 1, 'b': 2},
            {'a': [1, 2, 3], 'c': [1, 'a']},
            {'a': 1},
            {'a': {'a': 1, 'b': {'c': [1, 2, 3, 4]}}},
            {'a': {'a': 1, 'b': {'c': [3, 2, 1, 4, 5]}}},
            [1, 2, 3],
            {'a': None, 'b': None, (1, 2): [3, 2, 1]},
            {('yandex.ru', 9090): {'a': 1, 'tags': ['a', 'b']}},
            None,
            12,
            (1, 2),
            ((1, 2), 3, 5),
            {(1, 2): [1, 2], 'a': [1, 2, 2]},
            {'a': {'a': {'a': {1: [1, 2, 3]}}}},
            {'s': {'u': [1, 2], 's': {'a': 1}}},
        ]
        lists_of_dicts = [
            {'a': [{}, {'a': 1}]},
            {'a': [{'a': 1}, {'b': 1}], 'b': [{'a': {'b': [{'a': 1}, {'b': [{'a': 1}]}]}}]}
        ]
        vals += lists_of_dicts
        for a in vals:
            for b in vals:
                dif = diff(a, b)
                restored = merge(a, dif)
                assert eq(b, restored)

    def test_both(self):
        assert both([1], [2], Types.List)
        assert not both([1], {}, Types.List)
        assert both({}, {}, Types.Dict)
        assert both_dif({'1': 1}, {'s': [], 'u': []}, dict)
        assert not both_dif({(1, 2): '1'}, {'m': [], 'p': []}, dict)
        assert both_dif([1, 2], {'m': [], 'p': []}, list)
        assert not both_dif([1, 2], [1, 2], list)

    def test_DiffList(self):
        a = DiffList([1, 2], [2, 1])
        assert not (a.minus or a.plus)
        a = DiffList([1, 2, 2, 3], [1])
        assert a.minus == {2, 3} and a.plus == set()
        assert a.merge([1, 2, 2, 3]) == [1]

    def test_DiffDict(self):
        a = DiffDict({1: 1, 2: 2}, {'1': 1, 2: 2})
        assert a.unset == {1}, a.set == {'1': 1}
        assert a.merge({1: 1, 2: 2}) == {'1': 1, 2: 2}


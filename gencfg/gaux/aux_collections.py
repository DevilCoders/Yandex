"""
    Various collection structures, which (for unknown reason) are not found in standard or pip packages:
        - combination of OrderedDict and defaultdict
        - calculating of object size (with all nested objects recursively)
"""

import sys

from numbers import Number
from collections import Set, Mapping, deque, OrderedDict, Callable


class DefaultOrderedDict(OrderedDict):
    """
        Combination of OrderedDict and defaultdict
    """

    def __init__(self, cons=None, *args, **kwargs):
        if (cons is not None and
                not isinstance(cons, Callable)):
            raise TypeError('first argument must be callable')
        OrderedDict.__init__(self, *args, **kwargs)
        self.cons = cons

    def __getitem__(self, key):
        try:
            return OrderedDict.__getitem__(self, key)
        except KeyError:
            return self.__missing__(key)

    def __missing__(self, key):
        if self.cons is None:
            raise KeyError(key)
        self[key] = value = self.cons()
        return value

    def __reduce__(self):
        if self.cons is None:
            args = tuple()
        else:
            args = self.cons,
        return type(self), args, None, None, self.items()

    def copy(self):
        return self.__copy__()

    def __copy__(self):
        return type(self)(self.cons, self)


def getsize(obj):
    """Recursively iterate to sum size of object & members."""

    def append_stats(result, extra):
        """Add statistics from `extra` to `result`"""
        for obj_type, (size, cnt) in extra.iteritems():
            if obj_type not in result:
                result[obj_type] = (0, 0)
            result[obj_type] = (result[obj_type][0] + size, result[obj_type][1] + cnt)

    def getsize_recurse(obj, _seen_ids=set()):
        """Recursive calculating of object size"""
        obj_id = id(obj)
        if obj_id in _seen_ids:
            return dict()

        _seen_ids.add(obj_id)
        result = {obj.__class__: (sys.getsizeof(obj), 1)}

        if isinstance(obj, (basestring, Number, xrange, bytearray)):
            pass  # bypass remaining control flow and return
        elif isinstance(obj, (tuple, list, Set, deque)):
            for subobj in obj:
                append_stats(result, getsize_recurse(subobj, _seen_ids))
        elif isinstance(obj, Mapping) or hasattr(obj, 'iteritems'):
            for k, v in getattr(obj, 'iteritems')():
                append_stats(result, getsize_recurse(k, _seen_ids))
                append_stats(result, getsize_recurse(v, _seen_ids))
        # Check for custom object instances - may subclass above too
        if hasattr(obj, '__dict__'):
            append_stats(result, getsize_recurse(vars(obj), _seen_ids))
        if hasattr(obj, '__slots__'):  # can have __slots__ with __dict__
            for s in obj.__slots__:
                if hasattr(obj, s):
                    append_stats(result, getsize_recurse(getattr(obj, s), _seen_ids))
        return result

    result = getsize_recurse(obj)

    return result

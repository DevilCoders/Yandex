import types
import collections


def convert_type(python_type):
    if python_type == int:
        return 'integer'
    elif python_type == float:
        return 'numeric'
    elif python_type == bool:
        return 'bool'
    else:
        return 'string'


class BaseSerializable(object):
    def to_dict(self):
        raise NotImplementedError

    def __dict__(self):
        return self.to_dict()


def is_array(v):
    return isinstance(v, collections.Iterable) and not isinstance(v, types.StringTypes)

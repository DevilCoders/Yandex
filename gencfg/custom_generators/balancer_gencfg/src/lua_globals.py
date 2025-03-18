#!/skynet/python/bin/python

import copy
from collections import OrderedDict


def lua_global_to_string(v):
    assert v.__class__ == LuaGlobal

    result = copy.deepcopy(v)
    result.valuetype = str
    return result


class LuaFuncCall(object):
    __slots__ = ['name', 'params']

    def __init__(self, name, params):
        self.name = name
        self.params = params


class LuaGlobal(object):
    __slots__ = ['name', 'value', 'valuetype', 'lprefix', 'rprefix']

    def __init__(self, name, value):
        self.name = name
        self.value = value
        self.lprefix = []
        self.rprefix = []

        self.valuetype = self.value.__class__
        if self.valuetype not in (str, int, float, bool, dict, OrderedDict, list, LuaFuncCall):
            raise Exception("LuaGlobal: Invalid type %s for %s" % (self.valuetype, self.name))

    def _check(self, v):
        if v.__class__ == LuaGlobal:
            valuetype = v.valuetype
        else:
            valuetype = v.__class__

        if valuetype != self.valuetype:
            raise Exception("LuaGlobal: Ariphmetic with different types %s and %s" % (self.valuetype, valuetype))

    def update_globals(self, lua_globals):
        if self.name in lua_globals and self.value != lua_globals[self.name] and self.valuetype in (str, int, float, bool, OrderedDict, list):
            raise Exception("LuaGlobal: Variable %s with different values %s and %s" % (
                self.name, self.value, lua_globals[self.name]))
        lua_globals[self.name] = self.value

        for v in self.lprefix + self.rprefix:
            if v.__class__ == LuaGlobal:
                v.update_globals(lua_globals)

    def __add__(self, v):
        self._check(v)

        result = copy.deepcopy(self)
        result.rprefix.append(v)
        return result

    def __radd__(self, v):
        self._check(v)

        result = copy.deepcopy(self)
        result.lprefix.append(v)
        return result

    def __str__(self):
        if self.valuetype == str:
            strings = ['"%s"' % x for x in self.lprefix]
            strings.append(self.name)
            strings.extend(str(x) if type(x) == LuaGlobal else '"%s"' % x for x in self.rprefix)
            return ' .. '.join(strings)
        elif self.valuetype in (int, float):
            strings = [str(x) for x in self.lprefix]
            strings.append(self.name)
            strings.extend(str(x) for x in self.rprefix)
            return ' + '.join(strings)
        else:
            if len(self.lprefix) > 0 or len(self.rprefix) > 0:
                raise Exception("Lua variable <%s> of type <%s> has non-empty left of right prefix" % (
                                self.name, self.valuetype))
        return str(self.name)


# def __str__(self):
#        return self.name

class LuaAnonymousKey(object):
    __slots__ = ['comment']

    def __init__(self, comment=''):
        self.comment = comment


class LuaComment(object):
    __slots__ = ['comment']

    def __init__(self, comment):
        self.comment = comment


class LuaPrefixes(object):
    __slots__ = ['data']

    def __init__(self):
        self.data = []

    def _add_prefix(self, extra):
        if isinstance(extra, str):
            if extra not in self.data:
                self.data.append(extra)
        elif extra.__class__ == self.__class__:
            for s in extra.data:
                self._add_prefix(s)

    def __add__(self, extra):
        result = copy.deepcopy(self)
        result._add_prefix(extra)
        return result

    def __radd__(self, extra):
        result = copy.deepcopy(self)
        result._add_prefix(extra)
        return result

    def __iadd__(self, extra):
        self._add_prefix(extra)
        return self


class LuaProdOrTesting(object):
    __slots__ = ['prod_groups', 'testing_groups']

    def __init__(self, prod_groups, testing_groups):
        self.prod_groups = prod_groups
        self.testing_groups = testing_groups


class LuaGlobalSection(object):
    __slots__ = ['module_func', 'module_params']

    def __init__(self, module_func, module_params):
        self.module_func = module_func
        self.module_params = module_params


class LuaBackendList(object):
    """Special class with backend list"""
    __slots__ = ('backends', 'backends_hash')

    def __init__(self, backends, backends_hash):
        self.backends = backends
        self.backends_hash = backends_hash

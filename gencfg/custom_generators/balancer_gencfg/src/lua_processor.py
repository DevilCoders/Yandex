#!/skynet/python/bin/python

from collections import OrderedDict
from lua_globals import LuaGlobal, LuaFuncCall, LuaProdOrTesting


def recurse_modify(modules, modify_func):
    if modules.__class__ in (dict, OrderedDict):
        for k, v in modules.iteritems():
            modules[k] = recurse_modify(v, modify_func)
    elif modules.__class__ in (list, tuple):
        return [recurse_modify(x, modify_func) for x in modules]
    elif modules.__class__ == LuaFuncCall:
        modules.params = modify_func(modules.params)
        return modules
    elif modules.__class__ == LuaGlobal:
        return modify_func(modules)
    elif modules.__class__ == LuaProdOrTesting:
        return recurse_modify(modify_func(modules), modify_func)
    else:
        return modules

    return modules


def recurse_process(modules, extra, process_func):
    if modules.__class__ in (dict, OrderedDict):
        process_func(modules, extra)
        for k, v in modules.iteritems():
            recurse_process(v, extra, process_func)
    elif modules.__class__ in (list, tuple):
        process_func(modules, extra)
        for i in range(len(modules)):
            recurse_process(modules[i], extra, process_func)
    elif modules.__class__ == LuaFuncCall:
        process_func(modules, extra)
        recurse_process(modules.params, extra, process_func)
    elif modules.__class__ == LuaGlobal:
        recurse_process(modules.value, extra, process_func)
        process_func(modules, extra)
    else:
        return


def process_lua_global(m, extra):
    if m.__class__ == LuaGlobal:
        m.update_globals(extra)


def process_lua_globals(modules):
    result = OrderedDict()
    recurse_process(modules, result, process_lua_global)
    return result


RESERVED_LUA_NAMES = {'and', 'break', 'do', 'else', 'elseif', 'end', 'false', 'for', 'function', 'if', 'in', 'local',
                      'nil', 'not', 'or', 'repeat', 'return', 'then', 'true', 'until', 'while'}


def check_lua_keywords(modules, extra):
    if modules.__class__ in (dict, OrderedDict):
        for k in modules:
            if k in RESERVED_LUA_NAMES:
                raise Exception("Found reserved lua keyword <%s> as variable name" % k)


def choose_testing(m):
    if m.__class__ == LuaProdOrTesting:
        return m.testing_groups
    else:
        return m


def choose_production(m):
    if m.__class__ == LuaProdOrTesting:
        return m.prod_groups
    else:
        return m


if __name__ == '__main__':
    print process_lua_globals({
        'first': LuaGlobal('aaa', 15),
        'second': LuaGlobal('bbb', '123'),
        'ccc': {
            'xxx': 'ttt' + LuaGlobal('bbb', '123') + 'yyy',
            'yyy': LuaGlobal('aaa', 15) + 123,
        }
    })

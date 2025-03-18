#!/skynet/python/bin/python

import re
from collections import OrderedDict

from lua_globals import LuaComment, LuaGlobal, LuaAnonymousKey, LuaFuncCall, LuaBackendList

INDENT = '  '
MAX_LEN = 80


def add_indent(s, level):
    return '\n'.join(level * INDENT + x if x else x for x in s.split('\n'))


_DEFAULT_LUA_IDENTIFIER_RE = re.compile('[a-zA-Z_][0-9a-zA-Z_]*$')


def valid_lua_identifier(s):
    if _DEFAULT_LUA_IDENTIFIER_RE.match(s):
        return s
    else:
        return '["%s"]' % s


def recurse_print_lua(config, keypath, level):
    if config.__class__ == LuaBackendList:  # FIXME: looks like hack
        config = config.backends

    if not issubclass(config.__class__, dict):
        keypath = map(lambda x: "??" if x.__class__ == LuaAnonymousKey else x, keypath)
        raise Exception("Config class %s, config content %s, keypath: %s" % (
                        config.__class__, config, " -> ".join(keypath)))

    result = []
    for k, v in config.iteritems():
        if v is None:
            continue

        keypath.append(k)

        if v.__class__ in (int, float):
            if k.__class__ == LuaAnonymousKey:
                result.append('%(value)s,' % {'value': v})
            else:
                result.append('%(key)s = %(value)s;' % {'key': valid_lua_identifier(k), 'value': v})
        elif v.__class__ == bool:
            result.append('%(key)s = %(value)s;' % {'key': valid_lua_identifier(k), 'value': 'true' if v else 'false'})
        elif v.__class__ == str:
            if k.__class__ == LuaAnonymousKey:
                result.append('"%(value)s",' % {'value': v})
            else:
                result.append('%(key)s = "%(value)s";' % {'key': valid_lua_identifier(k), 'value': v})
        elif v.__class__ == LuaComment:
            result.append('--[[ %s --]]' % v.comment)
        elif v.__class__ == LuaGlobal:
            result.append('%(key)s = %(value)s;' % {'key': valid_lua_identifier(k), 'value': v})
        elif v.__class__ == LuaFuncCall:
            if k.__class__ == LuaAnonymousKey:
                result.append('%(func)s({\n' % {'func': v.name} + recurse_print_lua(v.params, keypath, 1) + '\n}),')
            else:
                result.append(
                    '%(key)s = %(func)s({\n' % {'key': k, 'func': v.name} + recurse_print_lua(v.params, keypath,
                                                                                              1) + '\n});')
        elif k.__class__ == LuaAnonymousKey:
            subresult = recurse_print_lua(v, keypath, 0)
            if len(subresult) < 90 and subresult.find('\n') == -1:
                result.append('{ %s };%s' % (subresult, k.comment))
            else:
                result.append(
                    '{\n%(sub)s\n};%(comment)s' % {'sub': add_indent(subresult, level), 'comment': k.comment})
        else:
            result.append('''%(key)s = {
%(sub)s
}; -- %(key)s''' % {'key': valid_lua_identifier(k), 'sub': recurse_print_lua(v, keypath, 1)})
        keypath.pop()

    compacted_result = []
    last_elems = []
    for elem in result:
        if elem.find('\n') >= 0 or elem.find('{') >= 0:
            if len(last_elems):
                compacted_result.append(' '.join(last_elems))
            compacted_result.append(elem)
            last_elems = []
        else:
            compacted_result.append(' '.join(last_elems))
            last_elems = [elem]
    if len(last_elems):
        compacted_result.append(' '.join(last_elems))

    return add_indent('\n'.join(compacted_result).strip(), level)


def print_lua(config):
    return recurse_print_lua(config, [], 0)


def print_lua_globals(config):
    result = []
    lua_func_call_result = []
    for name, value in config.iteritems():
        if value.__class__ in (int, float):
            result.append("if %s == nil then %s = %s; end" % (name, name, value))
        elif value.__class__ == str:
            result.append("if %s == nil then %s = \"%s\"; end" % (name, name, value))
        elif value.__class__ == bool:
            result.append("if %s == nil then %s = %s; end" % (name, name, "true" if value else "false"))
        elif issubclass(value.__class__, dict):
            result.append("""%s = {
%s
}; -- %s""" % (name, recurse_print_lua(value, [], 1), name))
        elif issubclass(value.__class__, list):
            value_as_dict = OrderedDict(map(lambda x: (LuaAnonymousKey(), x), value))
            result.append("""%s = {
%s
}; -- %s""" % (name, recurse_print_lua(value_as_dict, [], 1), name))
        elif value.__class__ == LuaFuncCall:
            lua_func_call_result.append('%s = %s({\n' % (name, value.name) + recurse_print_lua(value.params, [], 1) + '\n});')
        else:
            raise Exception("OOPS")
    return '\n'.join(result + lua_func_call_result)


if __name__ == '__main__':
    config = {'regexp': {
        'second': {
            'match_fsm': {'URI': '/clck/.*'},
            'errordocument': {'status': 404},
        },
        'first': {
            'match_fsm': {'URI': '/edit'},
            'hasher': {
                'errordocument': {'status': 505},
                'mode': 'subnet'}
        }
    }
    }

    print print_lua(config)

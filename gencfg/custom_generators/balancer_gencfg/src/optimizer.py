#!/usr/bin/env python

import pprint
from collections import OrderedDict
from collections import defaultdict
import re

from lua_globals import LuaGlobal, LuaAnonymousKey, LuaFuncCall, LuaComment, LuaBackendList


def _stable_hash(v):
    if v is None:  # hash(None) is different for each run
        return 646582
    return hash(v)


def _hash_pair(v1, v2):
    return _stable_hash(v1 * v2 + v1 + v2 + 9223372036854775837)


def _hash_list(lst):
    """
        Make hash from list of hashes
    """

    if len(lst) == 2:
        return _stable_hash(lst[0] * lst[1] + lst[0] + lst[1] + 9223372036854775837)
    elif len(lst) == 0:
        return 0
    else:
        result = lst[0]
        for elem in lst[1:]:
            result = _stable_hash(result * elem + result + elem + 9223372036854775837)
        return result


def _hash_data_dict(d):
    """
        Make hash of dict, consisting of basic types as keys and values
    """

    if len(d) == 0:
        return 0

    keys = sorted(d.keys())
    lst = map(lambda x: _stable_hash(x), keys) + map(lambda x: _stable_hash(d[x]), keys)
    return _hash_list(lst)


"""
    Go recursively through tree and find nodes with same hash. Write info on this nodes in
    out_dict: <hash> -> [<node1>, <node2>, ...] in order to join this nodes into shared in future.
"""


def recurse_find_same_dicts(modules, out_dict=None):
    sub_hashes = []

    if modules.__class__ in (str, int, float, bool):
        my_hash = _stable_hash(modules)
    elif modules.__class__ in (OrderedDict, dict):
        for k, v in modules.iteritems():
            sub_value_hash = recurse_find_same_dicts(v, out_dict)

            if k.__class__ == LuaAnonymousKey:
                sub_key_hash = 0
            else:
                sub_key_hash = _stable_hash(k)
            sub_hash = _hash_pair(sub_key_hash, sub_value_hash)
            if out_dict is not None:
                if sub_hash in out_dict:
                    out_dict[sub_hash].append((modules, k))
                else:
                    out_dict[sub_hash] = [(modules, k)]

            sub_hashes.append(sub_hash)

        if modules.__class__ == dict:
            sub_hashes.sort()

        my_hash = _hash_list(sub_hashes)
    elif modules.__class__ == LuaGlobal:
        # TODO(velavokr): SRSLY?
        if modules.__class__ in (str, int, bool, float):
            my_hash = _hash_list(
                map(lambda x: _stable_hash(x), [modules.name, modules.value] + modules.lprefix + modules.rprefix))
        else:
            my_hash = _hash_list(map(lambda x: _stable_hash(x), [modules.name, recurse_find_same_dicts(
                modules.value)] + modules.lprefix + modules.rprefix))
    elif modules.__class__ == LuaFuncCall:
        my_hash = _hash_pair(_stable_hash(modules.name), recurse_find_same_dicts(modules.params, {}))
    elif modules.__class__ == LuaAnonymousKey:
        my_hash = 0
    elif modules.__class__ == LuaBackendList:
        my_hash = _hash_list([_stable_hash(x) for x in modules.backends_hash])
    elif modules.__class__ in (list, tuple):
        for elem in modules:
            sub_hashes.append(recurse_find_same_dicts(elem, out_dict))
        sub_hashes.sort()
        my_hash = _hash_list(sub_hashes)
    elif modules.__class__ == LuaComment:
        my_hash = _stable_hash(modules.comment)
    else:
        my_hash = _stable_hash(modules)
    return my_hash


def recurse_count_shared(modules, shared_counts):
    if modules.__class__ in (dict, OrderedDict):
        for k, v in modules.iteritems():
            if k == 'shared':
                shared_counts[v['uuid']] += 1
            recurse_count_shared(v, shared_counts)
    elif modules.__class__ in (list, tuple):
        for elem in modules:
            recurse_count_shared(elem, shared_counts)


def recurse_remove_once_shared(modules, found_once_shared):
    if modules.__class__ in (dict, OrderedDict):
        if 'shared' in modules:
            if modules['shared']['uuid'] in found_once_shared:
                keys = modules['shared'].keys()

                assert (len(keys) == 2)

                submodule_name = keys[0] if keys[0] != 'uuid' else keys[1]
                submodule_value = modules['shared'][submodule_name]

                if modules.__class__ == dict:
                    modules.pop('shared')
                    modules[submodule_name] = submodule_value
                else:
                    new_modules = [(submodule_name, submodule_value) if k == 'shared' else (k, v) for k, v in
                                   modules.items()]
                    modules.clear()
                    for k, v in new_modules:
                        modules[k] = v

        for k, v in modules.iteritems():
            recurse_remove_once_shared(v, found_once_shared)
    elif modules.__class__ in (list, tuple):
        for elem in modules:
            recurse_remove_once_shared(elem, found_once_shared)


def add_shared(modules):
    modules_by_hash = OrderedDict()
    recurse_find_same_dicts(modules, modules_by_hash)

    for key, value in reversed(modules_by_hash.items()):
        if len(value) == 1:
            continue
        if value[0][1] not in ['errorlog', 'http', 'accesslog', 'statistics', 'hasher', 'headers',
                               'antirobot_wrapper', 'pinger', 'balancer', 'balancer2']:
            continue

        if len(value) > 1:
            uuid = str(key)

            first_submodule, first_subkey = value[0]
            first_submodule['shared'] = OrderedDict([('uuid', uuid), (first_subkey, first_submodule[first_subkey])])
            first_submodule.pop(first_subkey)

            for submodule, subkey in value[1:]:
                submodule['shared'] = OrderedDict([('uuid', uuid)])
                submodule.pop(subkey)

    # not get rid off shared found only once
    shared_counts = defaultdict(int)
    recurse_count_shared(modules, shared_counts)

    found_once_shared = set(map(lambda (x, y): x, filter(lambda (x, y): y == 1, shared_counts.iteritems())))
    recurse_remove_once_shared(modules, found_once_shared)


def recurse_add_stats_attrs(modules, attr):
    if issubclass(modules.__class__, dict):
        if 'stats_attr' in modules:
            if modules['stats_attr'] == '':
                if attr != '':
                    modules['stats_attr'] = attr
                else:
                    modules.pop('stats_attr')
            else:
                attr = modules['stats_attr']
        for k, v in modules.iteritems():
            recurse_add_stats_attrs(v, attr)
    elif isinstance(modules, (list, tuple)):
        for elem in modules:
            recurse_add_stats_attrs(elem, attr)
    elif modules.__class__ == LuaFuncCall:
        if modules.name == 'genBalancer':  # FIXME: kinda hack
            recurse_add_stats_attrs(modules.params, '%s-backend' % attr)
        else:
            recurse_add_stats_attrs(modules.params, attr)


def add_stats_attrs(modules):
    recurse_add_stats_attrs(modules, '')


# ======================= SEPE-12320 ====================================
def recurse_collect_report_uuids(modules):
    result = set()
    if issubclass(modules.__class__, dict):
        if 'report' in modules and modules['report'].get('uuid') is not None:
            my_uuid = modules['report']['uuid']

            if my_uuid not in result:
                result.add(my_uuid)

        for k, v in modules.iteritems():
            result |= recurse_collect_report_uuids(v)

    elif isinstance(modules, (list, tuple)):
        for elem in modules:
            result |= recurse_collect_report_uuids(elem)

    return result


def recurse_join_report_modules(modules, found_report_uuids, found_report_uuids_global):
    """
    Recursively find modules of type report with same uuid.
    If already found in param found_reports, fix it, otherwise add to found_reports.

    :param found_report_uuids:  set of currently found uuids in report modules
    """
    if issubclass(modules.__class__, dict):
        if 'report' in modules:
            if modules['report']['uuid'] is not None:
                my_uuid = modules['report']['uuid']

                if my_uuid not in found_report_uuids:
                    found_report_uuids.add(my_uuid)
                else:
                    modules['report']['uuid'] = None
                    modules['report']['events'] = None

                    if modules['report']['refers'] is not None:
                        modules['report']['refers'] = '%s,%s' % (modules['report']['refers'], my_uuid)
                    else:
                        modules['report']['refers'] = my_uuid

            if modules['report']['refers'] is not None:
                for refer in modules['report']['refers'].split(','):
                    if refer not in found_report_uuids_global:
                        raise Exception("Found refer <%s>, which is not defined yet" % refer)

            # FIXME: do not reorder refers anymore
            if modules['report']['refers'] is not None:
                items = [('refers', modules['report']['refers'])] + filter(lambda (x, y): x != 'refers',
                                                                           modules['report'].items())
                modules['report'].clear()
                for k, v in items:
                    modules['report'][k] = v

        for k, v in modules.iteritems():
            recurse_join_report_modules(v, found_report_uuids, found_report_uuids_global)

    elif isinstance(modules, (list, tuple)):
        for elem in modules:
            recurse_join_report_modules(elem, found_report_uuids, found_report_uuids_global)


def join_report_modules(modules):
    found_report_uuids_global = recurse_collect_report_uuids(modules)
    recurse_join_report_modules(modules, set(), found_report_uuids_global)


def remove_stats_events(modules):
    if issubclass(modules.__class__, dict):
        if 'report' in modules:
            modules['report']['events'] = None

        for k, v in modules.iteritems():
            remove_stats_events(v)

    elif isinstance(modules, (list, tuple)):
        for elem in modules:
            remove_stats_events(elem)


# =========================== GENCFG-231 ================================


def _get_group_comment(d):
    for v in d.itervalues():
        if v.__class__ == LuaComment and v.comment.startswith("group "):
            return re.sub('\W', '_', v.comment.partition(' ')[2])
    return None


def _calc_backends_hash(backends, treepath):
    my_hash = abs(_hash_list(map(lambda x: _hash_data_dict(x), backends.values())))

    """
        Check for tier1 balancer. Treepath looks like [..., (balancer, ...), (<balancing type>, ...)]
    """
    assert (treepath[-2][0]) in ['balancer', 'balancer2']
    groupname = _get_group_comment(treepath[-2][1])
    if groupname is not None:
        return ("backends_%s" % groupname, my_hash)

    """
        Check for tier2 balancer. Treepath looks like [..., (balancer, ...), (<balancing type>, ...), (<location>, ...), (balancer, ...), (<balancing type>, ...)]
    """
    if len(treepath) > 4 and treepath[-5][0] in ['balancer', 'balancer2']:
        groupname = _get_group_comment(treepath[-5][1])
        if groupname is not None:
            return ("backends_%s_%s" % (groupname, treepath[-3][0]), my_hash)

    """
        Check if we have exactly one backend. If yes, add its name to variable
    """
    if len(backends) == 1:
        encoded_host = re.sub('\W', '_', next(backends.itervalues())['host'])
        return ("backends_%s" % encoded_host, my_hash)

    # did not find any valid group
    return ("backends_%s" % (my_hash), my_hash)


def recurse_add_backends_as_globals(modules, backend_globals, backend_hashes, treepath):
    if issubclass(modules.__class__, dict):
        for k, v in modules.iteritems():
            treepath.append((k, v))
            recurse_add_backends_as_globals(v, backend_globals, backend_hashes, treepath)
            treepath.pop()
    elif isinstance(modules, (list, tuple)):
        for i, elem in enumerate(modules):
            treepath.append((i, elem))
            recurse_add_backends_as_globals(elem, backend_globals, backend_hashes, treepath)
            treepath.pop()
    elif modules.__class__ == LuaFuncCall:
        if modules.name == 'genBalancer':
            # backends = modules.params['backends']
            if isinstance(modules.params['backends'], LuaBackendList):
                modules.params['backends'] = modules.params['backends'].backends

            backends_var_name, backends_hash = _calc_backends_hash(modules.params['backends'], treepath)

            # check if we already have variable with same name but other backends
            if backends_var_name in backend_hashes and backends_hash != backend_hashes[backends_var_name]:
                backends_var_name = '%s_%s' % (backends_var_name, backends_hash)

            if backends_var_name not in backend_globals:
                backend_globals[backends_var_name] = modules.params['backends']
                backend_hashes[backends_var_name] = backends_hash
            modules.params['backends'] = LuaGlobal(backends_var_name, modules.params['backends'])


def add_backends_as_globals(modules_dict, lua_extra):
    backend_globals = OrderedDict()
    recurse_add_backends_as_globals(modules_dict, backend_globals, {}, [])

    assert len(set(backend_globals.keys()) & set(
        lua_extra.keys())) == 0, "Got bug in add_backends_as_globals: found backends global variables which are already in lua_extra"

    lua_extra.update(backend_globals)

    lua_extra['replace_backends_result'] = LuaFuncCall('ReplaceGlobalVariables', {})


def recurse_find_section_by_name(modules_dict, section_name):
    """
        Find section by its name. Return list of section in order to check afterwards, if they are same

        :param modules_dict(dict or list or ...): balancer config as dict-like structure
        :param section_name(str): section name (should be dict key)

        :return (list): List of sections with <section_name> in childs
    """

    result = []

    if issubclass(modules_dict.__class__, dict):
        if section_name in modules_dict:
            result.append(modules_dict)
        else:
            for k, v in modules_dict.iteritems():
                result.extend(recurse_find_section_by_name(v, section_name))
    elif isinstance(modules_dict, (list, tuple)):
        for elem in modules_dict:
            result.extend(recurse_find_section_by_name(elem, section_name))

    return result


# =======================================================================

if __name__ == '__main__':
    # config = { 'a' : {
    #                   'x' : 'y',
    #                   'z' : 1234,
    #                   't' : {
    #                        'aa' : 'bb',
    #                        'cc' : 'dd',
    #                   },
    #               },
    #               't' : {
    #                   'aa' : 'bb',
    #                   'cc' : 'dd'
    #               },
    #            }

    config = {
        'a': {'x': {'y': 'z'}},
        'b': {'x': {'y': 'z'}}
    }

    pp = pprint.PrettyPrinter(indent=4)
    pp.pprint(config)
    add_shared(config)
    pp.pprint(config)

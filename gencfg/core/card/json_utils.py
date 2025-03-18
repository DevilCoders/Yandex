"""Utils for saving loading json"""

import copy

from core.card.node import SchemeLeaf, SchemeNode, Scheme
from core.card.types import NoneOrType, ByteSizeType, ByteSize, DateType, Date
from core.igroups import IGroup

import gaux.aux_decorators


def card_node_field_type_paths_recurse(scheme_node, leaf_node_type):
    """Find all paths in scheme with leaf of <leaf_nod_type> type"""

    result = []
    if isinstance(scheme_node, SchemeLeaf):
        if isinstance(scheme_node.card_type, leaf_node_type) or (isinstance(scheme_node.card_type, NoneOrType) and isinstance(scheme_node.card_type.base, leaf_node_type)):
            result.append([])
    else:
        if scheme_node.is_list_node():
            sub_node_result = card_node_field_type_paths_recurse(scheme_node['_list'], leaf_node_type)
            sub_node_result  = [['_list'] + x for x in sub_node_result]
            result.extend(sub_node_result)
        else:
            for k in scheme_node:
                sub_node_result = card_node_field_type_paths_recurse(scheme_node[k], leaf_node_type)
                sub_node_result  = [[k] + x for x in sub_node_result]
                result.extend(sub_node_result)

    return result


@gaux.aux_decorators.memoize
def card_node_field_type_paths(scheme, leaf_node_type):
    result = card_node_field_type_paths_recurse(scheme.get_cached(), leaf_node_type)

    return [tuple(x) for x in result]


@gaux.aux_decorators.memoize
def create_default_dict(scheme):
    status, jsoned = __create_default_dict_recurse(scheme.get_cached())
    fix_card_node_json_for_save(scheme, jsoned, ignore_missing=True)

    return jsoned


def __replace_by_path_recurse(jsoned, path, converter_func, ignore_missing=False):
    """Replace value"""

    path_elem = path[0]
    path = path[1:]

    if not path:
        if (not ignore_missing) or (path_elem in jsoned):
            jsoned[path_elem] = converter_func(jsoned[path_elem])
        return
    elif path_elem == '_list':
        for jsoned_elem in jsoned:
            __replace_by_path_recurse(jsoned_elem, path, converter_func, ignore_missing=ignore_missing)
    else:
        __replace_by_path_recurse(jsoned[path_elem], path, converter_func, ignore_missing=ignore_missing)


def fix_card_node_json_for_save(scheme, jsoned, ignore_missing=False):
    """Prepare card node json to save as json (e. g. converting ByteSize to simple string)"""

    # convert internal objects to default type (e. g. ByteSize to str)
    CONVERTERS = [
        (ByteSizeType, lambda x: None if x is None else x.text),
        (DateType, lambda x: None if x is None else str(x)),
    ]

    for leaf_node_type, converter_func in CONVERTERS:
        leaf_node_paths = card_node_field_type_paths(scheme, leaf_node_type)
        for path in leaf_node_paths:
            __replace_by_path_recurse(jsoned, list(path), converter_func, ignore_missing=ignore_missing)

    # fix master group
    if jsoned.get('master', None) is not None:
        jsoned['master'] = jsoned['master'].card.name


def remove_defaults_for_save(scheme, jsoned):
    """Remove subdicts with only default values (RX-367)"""
    default_jsoned = create_default_dict(scheme)
    __recurse_remove_defaults(jsoned, default_jsoned)


def fix_card_node_json_for_load(scheme, jsoned, ignore_missing=False):
    """Prepare card node json to convert to card node (e. g. converting string with memory size to ByteSize)"""

    CONVERTERS = [
        (ByteSizeType, lambda x: None if x is None else ByteSize(x)),
        (DateType, lambda x: None if x is None else Date.create_from_text(x)),
    ]

    for leaf_node_type, converter_func in CONVERTERS:
        leaf_node_paths = card_node_field_type_paths(scheme, leaf_node_type)
        for path in leaf_node_paths:
            __replace_by_path_recurse(jsoned, list(path), converter_func, ignore_missing=ignore_missing)


def add_defaults_for_load(scheme, jsoned):
    """Remove subdicts with only default values (RX-367)"""
    default_jsoned = create_default_dict(scheme)
    __recurse_add_defaults(jsoned, default_jsoned)


def __create_default_dict_recurse(scheme_node):
    """Create multi-level dict with default values, corresponding to scheme (RX-367)"""
    if isinstance(scheme_node, SchemeLeaf):
        if scheme_node.has_default_value:
            return (True, scheme_node.default)
        else:
            return (False, None)
    elif scheme_node.is_list_node():
        substatus, subresult = __create_default_dict_recurse(scheme_node['_list'])
        if substatus:
            return (True, dict(list_=subresult))
        else:
            return (True, dict())
    else:
        result = dict()
        for k in scheme_node:
            substatus, subresult = __create_default_dict_recurse(scheme_node[k])
            if substatus:
                result[k] = subresult
        return (True, result)


def __recurse_remove_defaults(jsoned, default_jsoned):
    """Recursively remove defaults from jsoned (RX-364)"""

    # nothing to do if default_jsoned is empty
    if default_jsoned == dict():
        return

    if isinstance(jsoned, dict):
        # remove bad keys
        keys_to_remove = []
        for k in jsoned:
           if (k in default_jsoned) and __recurse_equal(jsoned[k], default_jsoned[k]):
                keys_to_remove.append(k)
        for key in keys_to_remove:
            jsoned.pop(key)
        # traverse recursively
        for k, v in jsoned.iteritems():
            __recurse_remove_defaults(v, default_jsoned.get(k, dict()))
    elif isinstance(jsoned, list):
        if 'list_' in default_jsoned:
            for v in jsoned:
                __recurse_remove_defaults(v, default_jsoned['list_'])


def __recurse_add_defaults(jsoned, default_jsoned):
    """Recursively add defaults to jsoned upon loading from card (RX-364)"""

    # nothing to do if default_jsoned is empty
    if default_jsoned == dict():
        return

    if isinstance(jsoned, dict):
        for k in jsoned:
            __recurse_add_defaults(jsoned[k], default_jsoned.get(k, dict()))
        for k in default_jsoned:
            if k not in jsoned:
                jsoned[k] = copy.copy(default_jsoned[k])
    elif isinstance(jsoned, list):
        if isinstance(default_jsoned, dict):
            if 'list_' in default_jsoned:
                for v in jsoned:
                    __recurse_add_defaults(v, default_jsoned['list_'])


def __recurse_equal(d1, d2):
    """Check if two dictionaries are equal (RX-364)"""
    if isinstance(d1, dict):
        if isinstance(d2, dict):
            if set(d1.keys()) != set(d2.keys()):
                return False
            for k in d1:
                if not __recurse_equal(d1[k], d2[k]):
                    return False
            return True
        else:
            return False
    elif isinstance(d1, list):
        if isinstance(d2, list):
            if len(d1) != len(d2):
                return False
            for elem1, elem2 in zip(d1, d2):
                if not __recurse_equal(elem1, elem2):
                    return False
            return True
        else:
            return False
    else:
        return d1 == d2

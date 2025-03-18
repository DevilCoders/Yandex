"""
    A bunch of functions to modify yaml dicts, loaded from config files. Manipulations includes
        - fixing include names;
        - recurse processing of includes by substitution includes with includes content
        - removing _del sections;
        ...
"""

import copy
import os
import re
from collections import OrderedDict

from gaux.aux_ast import convert_to_ast_and_eval
from custom_generators.intmetasearchv2.aux_utils import create_include_path_from_string, is_subpath


def _find_anchor(lst):
    if isinstance(lst, list):
        for k, v in lst:
            if k == '_a':
                return v
    return None


def _find_elem_by_name_and_anchor(lst, name, anchor):
    for k, v in lst:
        if name == k and anchor == _find_anchor(v):
            return v
    return None


def _has_elem_by_name_and_anchor(lst, name, anchor):
    for k, v in lst:
        if name == k and anchor == _find_anchor(v):
            return True
    return False


def _remove_elem_by_name_and_anchor(lst, name, anchor):
    for index, (k, v) in enumerate(lst):
        if name == k and anchor == _find_anchor(v):
            lst.pop(index)
            return
    raise Exception("Elem with name <%s> and anchor <%s> not found" % (name, anchor))


def _is_del_node(lst):
    if isinstance(lst, list):
        lst_keys = map(lambda (x, y): x, lst)
        if '_del' in lst_keys:
            extra_keys = set(lst_keys) - {'_del', '_a'}
            if len(extra_keys) > 0:
                raise Exception("Found extra keys <%s> in <_del> node" % ",".join(extra_keys))

            del_value = filter(lambda (x, y): x == '_del', lst)[0][1]
            if del_value:
                return True
    return False


def _is_extended_node(lst):
    if isinstance(lst, list):
        lst_keys = map(lambda (x, y): x, lst)
        if '_extended' in lst_keys:
            del_value = filter(lambda (x, y): x == '_extended', lst)[0][1]
            if del_value:
                return True
    return False


def recurse_add_fname_to_includes(value, fname):
    """
        Some include strings do not contain filename to include from. In this case we must add "current" file name as file included from.
        This script modifies value inplace.

        :param value(list of (str, anyobj): key-value stored of current node
        :param fname(str): current file name

        :return: None
    """

    if isinstance(value, list):
        for index, (subname, subvalue) in enumerate(value):
            if subname == '_include':
                if subvalue.find(':') < 0:  # include from same file, like <SearchSource[something]>
                    value[index] = (subname, "%s:%s" % (fname, subvalue))
                else:  # include from some other file, like <somefile:SearchSource[something]>
                    other_fname, _, other_data = subvalue.partition(':')
                    newname = os.path.join(os.path.dirname(fname), other_fname)
                    value[index] = (subname, "%s:%s" % (newname, other_data))
            elif isinstance(subvalue, str) and subvalue.startswith('_include:'):
                value[index] = (subname, "_include:%s:%s" % (fname, subvalue.partition(':')[2]))
            else:
                recurse_add_fname_to_includes(subvalue, fname)


def recurse_find_includes(name, value):
    """
        Find all of _include sections
    """
    if name == '_include':
        return [create_include_path_from_string(value)]

    if isinstance(value, list):
        result = []
        for subname, subvalue in value:
            result.extend(recurse_find_includes(subname, subvalue))
        return result
    elif isinstance(value, (int, bool, str, float, type(None))):
        return []
    else:
        raise Exception("Unknown type <%s>" % (type(value)))


def recurse_find_node_by_path(data, path, position):
    if position == len(path):
        return data

    name, anchor = path[position]
    next_elem = _find_elem_by_name_and_anchor(data, name, anchor)
    if next_elem is not None or _has_elem_by_name_and_anchor(data, name, anchor):
        return recurse_find_node_by_path(next_elem, path, position + 1)

    raise Exception("Failed to find include path <%s>, not found position <%s>" % (path, path[position]))


def recurse_merge_nodes(main_node, include_node, main_node_path, include_node_path):
    """
        This function goes recursively main and include node, replacing elems with same name. As a result we get
        modified main_node (main_node is not modified in-place)

        :param main_node(list of list of ...): subtree of configs structure
        :param include_node(list of list of ...): subtree of configs structure to be merged to main node
        :param path(list of pair(str, str)): path of (nodename, anchor) to reach <main_node> element from root
        :param path(list of pair(str, str)): path of (nodename, anchor) to reach <include_node> element from root

        :return (None): all changes are applied inplace
    """

    if (main_node is not None) and \
            (include_node is not None) and \
                    type(main_node) != type(include_node):
        raise Exception("Found different types <%s> and <%s> while merging nodes <%s> and <%s>:\n    <%s> main path\n    <%s> include path" % (
                        type(main_node), type(include_node), main_node, include_node, main_node_path, include_node_path))

    if isinstance(include_node, list):
        result = []
        for name, main_node_child in main_node:
            anchor = _find_anchor(main_node_child)

            include_node_child = _find_elem_by_name_and_anchor(include_node, name, anchor)
            has_include_node_child = _has_elem_by_name_and_anchor(include_node, name, anchor)

            if (include_node_child is None):
                if has_include_node_child:
                    result.append((name, include_node_child))
                else:
                    result.append((name, main_node_child))
            elif _is_del_node(include_node_child):  # process _del nodes
                pass
            else:
                main_node_path.append((name, anchor))
                include_node_path.append((name, anchor))
                result.append(
                    (name, recurse_merge_nodes(main_node_child, include_node_child, main_node_path, include_node_path)))

        for name, include_node_child in include_node:
            if _find_elem_by_name_and_anchor(main_node, name, _find_anchor(include_node_child)) is None:
                result.append((name, include_node_child))

        return result
    else:
        return include_node


def recurse_apply_inline_include(root, node):
    """
        Go nodes recursively, find leaf nodes starting with <_include:> and replace them with _include value
    """
    if isinstance(node, (int, bool, float, type(None))):
        return node
    elif isinstance(node, str):
        if node.startswith('_include:'):
            include_path = node.partition(':')[2]
            return recurse_find_node_by_path(root, create_include_path_from_string(include_path), 0)
        else:
            return node
    elif isinstance(node, list):
        return map(lambda (subname, subvalue): (subname, recurse_apply_inline_include(root, subvalue)), node)
    else:
        raise Exception("Unknown type <%s>" % (type(node)))


def recurse_apply_include(root, node, path):
    """
        Go node recursively, find all _include elements and replace them by element from include

        :param root(list of list of ...): all configs represented as list of list of list ...
        :param node(list of list of ...): element in root node
        :param path(list of pair(str, str)): path of (nodename, anchor) to reach node element

        :return (None): all changed are applied inplace
    """

    if isinstance(node, (int, str, bool, float, type(None))):
        return
    elif isinstance(node, list):
        included_nodes_paths = dict()
        # first replaced all _include by content of include nodes
        new_subnodes = copy.copy(node)
        while len(filter(lambda (name, value): name == '_include', new_subnodes)) > 0:
            old_new_subnodes = new_subnodes
            new_subnodes = []
            for subname, subvalue in old_new_subnodes:
                if subname == '_include':
                    include_path = create_include_path_from_string(subvalue)

                    if is_subpath(path, include_path):
                        raise Exception("Found self-include at path <%s>: %s" % (path, include_path))

                    included_nodes = recurse_find_node_by_path(root, include_path, 0)

                    for included_node in included_nodes:
                        new_subnodes.append(included_node)
                        included_nodes_paths[id(included_node[1])] = include_path
                else:
                    new_subnodes.append((subname, subvalue))

        # group all subnodes with same anchors
        new_subnodes_by_name = OrderedDict()
        for subname, subvalue in new_subnodes:
            key = subname, _find_anchor(subvalue)
            if key not in new_subnodes_by_name:
                new_subnodes_by_name[key] = [subvalue]
            else:
                new_subnodes_by_name[key].append(subvalue)

        # merge subnodes
        new_subnodes = []
        for (subname, anchor), subnodes in new_subnodes_by_name.iteritems():
            if subname == '_a':  # we do not want anchors to be rewritten by includes
                del subnodes[1:]

            # skip all nodes up to last del node
            del_nodes = filter(_is_del_node, subnodes)
            if len(del_nodes) > 0:
                del subnodes[:subnodes.index(del_nodes[-1]) + 1]
            if len(subnodes) == 0:
                continue

            main_node = subnodes[0]
            for include_node in subnodes[1:]:
                if id(main_node) in included_nodes_paths:
                    main_node_path = included_nodes_paths[id(main_node)]
                else:
                    main_node_path = copy.copy(path)
                if id(include_node) in included_nodes_paths:
                    include_node_path = included_nodes_paths[id(include_node)]
                else:
                    include_node_path = copy.copy(path)

                main_node = recurse_merge_nodes(main_node, include_node, main_node_path, include_node_path)
            new_subnodes.append((subname, main_node))

        node[:] = new_subnodes

        for subnode, subvalue in node:
            path.append((subnode, _find_anchor(subvalue)))
            recurse_apply_include(root, subvalue, path)
            path.pop()
    else:
        raise Exception("Unknown type <%s>" % (type(node)))


def recurse_check_same_nodes(node, path):
    found = set()
    if isinstance(node, list):
        for subname, subnode in node:
            if _is_del_node(subnode):
                continue

            anchor = _find_anchor(subnode)

            if (subname, anchor) in found:
                raise Exception("Node <%s> found at least twice at path <%s>" % ((subname, anchor), path))
            found.add((subname, anchor))

            recurse_check_same_nodes(subnode, path + [(subname, anchor)])


def recurse_apply_extended(node_name, node_value):
    """
        Recursively unfold tree of heirs into flat structure.
        For example, structure
        ####################################################################
        - Node1:
            - key1: value1
            - key2: value2
            - key3: value3
            - Node2:
                - _extended: True
                - key2: another_value2
                - Node3:
                    - _extended: True
                    - key1: babababa
        ####################################################################
        will be converted into
        ####################################################################
        - Node1:
            - key1: value1
            - key2: value2
            - key3: value3
        - Node2:
            - key1: value1
            - key2: another_value2
            - key3: value3
        - Node3:
            - _key1: babababa
            - _key2: another_value2
            - _key3: value3
        ####################################################################
    """

    if isinstance(node_value, (int, bool, str, float, type(None))):
        return [(node_name, node_value)]
    elif isinstance(node_value, list):
        result = []

        new_subnodes = sum(
            map(lambda (subnode_name, subnode_value): recurse_apply_extended(subnode_name, subnode_value), node_value),
            [])

        not_extended_subnodes = filter(lambda (name, value): not _is_extended_node(value), new_subnodes)
        extended_subnodes = filter(lambda (name, value): _is_extended_node(value), new_subnodes)

        # construct result consisting of our node and all extended subnodes as flat list
        del node_value[:]
        node_value.extend(not_extended_subnodes)
        result.append((node_name, node_value))

        for subnode_name, subnode_value in extended_subnodes:
            subnode_value = recurse_merge_nodes(not_extended_subnodes,
                                                filter(lambda (name, value): name != '_extended', subnode_value), [],
                                                [])
            result.append((subnode_name, subnode_value))

        return result
    else:
        raise Exception("Unknown type <%s>" % (type(node)))


def recurse_apply_conditionals(root, node, path):
    """
        Recursively unfold all _test nodes, based on _test expression value. For example structure
        ##########################################################################################
        - flag1: True
        - flag2: False
        - Node:
            - _test:
                - _expr: not flag1
                - value1: key1
            - _test(not flag2):
                - value2: key2
        ##########################################################################################
        will be converted into
        ##########################################################################################
        - flag1: True
        - flag2: False
        - Node:
            - value2: key2
        ##########################################################################################
    """

    node_name, node_value = node

    if isinstance(node_value, (int, bool, str, float, type(None))):
        return [(node_name, node_value)]
    elif isinstance(node_value, list):
        path.append(node_name)

        if node_name.startswith("_test"):
            # process common subnodes
            common_subnodes = filter(lambda (subnode_name, subnode_value): subnode_name != '_expr', node_value)
            result = sum(map(lambda x: recurse_apply_conditionals(root, x, path), common_subnodes), [])

            # find _expr node and evaluate it
            if node_name == "_test":
                expr_subnodes = filter(lambda (subnode_name, subnode_value): subnode_name == '_expr', node_value)
                if len(expr_subnodes) != 1:
                    raise Exception("Have <%d> _expr nodes at path <%s>" % (len(expr_nodes), path))
                _, expr_value = filter(lambda (subnode_name, subnode_value): subnode_name == '_expr', node_value)[0]

            elif re.match("^_test\((.*)\)$", node_name):
                expr_value = re.match("^_test\((.*)\)$", node_name).group(1)
            else:
                raise Exception("Can not parse <%s> as conditional at path <%s>" % (node_name, path))

            # expr_value it python-like expression
            def identifier_func(s):
                retval = recurse_find_node_by_path(root, create_include_path_from_string(s, fname=root[0][0]), 0)
                return retval

            if not convert_to_ast_and_eval(expr_value, identifier_func):
                result = []
        else:
            result = [(node_name, sum(map(lambda x: recurse_apply_conditionals(root, x, path), node_value), []))]

        path.remove(node_name)

        return result
    else:
        raise Exception("Unknown type <%s>" % (type(node)))

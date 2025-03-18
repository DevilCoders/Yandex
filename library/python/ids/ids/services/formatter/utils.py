# coding: utf-8

from __future__ import unicode_literals


def is_type_matched(node, wanted_type):
    """
    @param node: WikiDictNode
    @param wanted_type: str
    @return: bool
    """
    return node.type == wanted_type


def is_attrs_matched(node, wanted_attrs):
    """
    Совпадением считается случай, когда wanted_attrs являются подмножеством
    node.attrs.
    @param node: WikiDictNode
    @param wanted_attrs: dict of attrs
    @return: bool
    """
    given = node.attrs
    for key, value in wanted_attrs.items():
        if key not in given or given[key] != value:
            return False

    return True


def filter_nodes(node, type, attrs=None):
    """
    Рекурсивно обойти дерево wiki-узел и вернуть узлы с node_type и node_attrs.
    @param node: WikiNode
    @param type: тип узла
    @param attrs: атрибуты (необязательный)
    @return: generator of nodes.
    """
    attrs = attrs or {}

    if is_type_matched(node, type) and is_attrs_matched(node, attrs):
        yield node

    for sub_node in node.content:
        for found in filter_nodes(sub_node, type, attrs):
            yield found

# -*- coding: utf-8 -*-
"""
DBaaS Internal API helpers
"""
import collections.abc
from copy import deepcopy
from typing import Any, Dict, TypeVar


def merge_dict(dest_dict: dict, second: collections.abc.Mapping) -> None:
    """
    Deep merge two dictionaries. Like `dict.update`, but instead of
    updating only top-level keys, perform recursive dict merge.
    NOTE: Function uses recursion, it is not the best way to do this,
    but we assume that dicts will not be deep enough to notice this.
    """
    for key, value in second.items():
        if key in dest_dict and isinstance(dest_dict[key], dict) and isinstance(value, collections.abc.Mapping):
            merge_dict(dest_dict[key], value)
        else:
            dest_dict[key] = value


def copy_merge_dict(dest_dict: dict, second: dict) -> dict:
    """
    Same as merge_dict(), but operates on a copy
    """
    dest_copy = deepcopy(dest_dict)
    merge_dict(dest_copy, second)
    return dest_copy


def remove_none_dict(source_dict):
    """
    Remove all keys with None values and return new dict
    """
    new_dict = {}
    for key, value in source_dict.items():
        if isinstance(value, dict):
            value = remove_none_dict(value)
        if value is not None:
            new_dict[key] = value
    return new_dict


def get_value_by_path(dictionary, path):
    """
    Get dictionary value by path
    """
    if not path:
        return dictionary
    if dictionary:
        return get_value_by_path(dictionary.get(path[0], {}), path[1:])

    return None


K = TypeVar('K')  # pylint: disable=invalid-name


def first_key(mapping: Dict[K, Any]) -> K:
    """
    Return first (in a sort order) key from dictionary
    """
    try:
        return next(iter(sorted(mapping.keys())))  # type: ignore
    except StopIteration:
        raise IndexError('first key from empty dict')


def first_value(mapping: Dict[Any, K]) -> K:
    """
    Return first value from dictionary
    """
    try:
        return next(iter(mapping.values()))
    except StopIteration:
        raise IndexError('first value from empty dict')


def coalesce(*args):
    """
    Return first argument that value other than `None`. Default value is `None`.
    """
    for item in args:
        if item is not None:
            return item
    return None

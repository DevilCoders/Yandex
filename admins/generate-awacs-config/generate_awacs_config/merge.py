from collections.abc import Mapping, Iterable
from copy import deepcopy


def _merge_lists(left: list, right: list):
    """
    Merges the right list into the left one. Mutates the left list.
    ~item removes an item from the left list rather than adding.
    """
    for item in right:
        if item is None:
            left.clear()
            continue
        if isinstance(item, str):
            if item.startswith('~'):
                left.remove(item[1:])
                continue
        left.append(item)


def _merge_dicts(left: dict, right: dict):
    """
    Merges the right dict into the left one. Mutates the left dict.
    """
    for name in right:
        # just copy inexistent items
        if name not in left:
            left[name] = right[name]
            continue

        # copy dicts
        if isinstance(right[name], Mapping):
            _merge_dicts(left[name], right[name])
            continue

        # copy lists
        if isinstance(right[name], Iterable):
            _merge_lists(left[name], right[name])
            continue

        left[name] = right[name]


def merge_dicts(left: dict, right: dict) -> dict:
    """
    Merges the right dict into the left one. Does not mutate data.
    """
    if not isinstance(left, Mapping) or not isinstance(right, Mapping):
        raise RuntimeError('Both arguments must be dict-like')

    dst = deepcopy(left)
    _merge_dicts(dst, right)
    return dst

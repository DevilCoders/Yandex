import six

from .split import Split as PathSplit
from .tree import Tree as PathTree

from .split import get_common_prefix as _get_split_common_prefix


"""
Path library provides generic tools for dealing with paths, where path is defined as tuple (path-split).

`PathSplit` represents path itself and is in fact just a tuple, augmented with some convenient helpers.

`PathTree` is a simple path trie suitable for file/directory tree traversals and subtree maniputaions.
"""


__all__ = [
    'Path',
    'PathSplit',
    'PathTree',
]


class Path(PathSplit):
    """
    Path as a tuple.

    >>> Path()
    >>> Path((u'some', u'path'))
    >>> Path(u'some/path')

    See `PathSplit` for details.
    """

    def __new__(cls, value=()):
        if isinstance(value, six.string_types):
            return super(Path, cls).parse(value)
        return super(Path, cls).__new__(cls, value)

    def is_subpath_of(self, other):
        if isinstance(other, six.string_types):
            other = Path(other)
        return super(Path, self).is_subpath_of(other)


def get_common_prefix(paths):
    """
    Get common prefix for a sequence of paths.

    >>> get_common_prefix([u'some/path', u'some/other/path'])
    (u'some',)
    >>> get_common_prefix([u'some/path', u'some/other/path']).form
    'some'

    :param paths: paths to process (either tuples or strings)
    :return: common prefix
    """

    return Path(_get_split_common_prefix(Path(path) for path in paths))

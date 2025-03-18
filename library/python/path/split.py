import itertools


class Split(tuple):
    """
    Path split (path as a tuple). Can be used as a tuple drop-in replacement.

    >>> Split()  # root
    >>> Split((u'some', u'path'))
    >>> Split([u'some', u'path'])

    Path is immutable. All tuple operations (join, component and subpath extraction) are functional:

    >>> Split((u'some', u'path')) + Split((u'subpath',))  # Split((u'some', u'path', u'subpath'))
    >>> Split((u'some', u'path'))[1]  # u'path'
    >>> Split((u'some', u'path', u'subpath'))[1:]  # Split((u'path', u'subpath'))
    """

    _sep = u'/'

    def __add__(self, other):
        return Split(super(Split, self).__add__(other))

    def __getitem__(self, v):
        res = super(Split, self).__getitem__(v)
        return Split(res) if isinstance(v, slice) else res

    # CPython compat
    def __getslice__(self, i, j):
        return self.__getitem__(slice(i, j))

    @property
    def form(self):
        """Path as string."""

        return self._sep.join(self)

    @property
    def basename(self):
        """Basename or None (for root)."""

        if not self:
            return None
        return self[-1]

    @property
    def dirname(self):
        """Dirname or None (for root)."""

        if not self:
            return None
        return Split(self[:-1])

    @classmethod
    def parse(cls, s):
        """
        Construct path from a string.

        :param s: path as a string, '/'-separated (unicode supported)
        :return: path split
        """

        return cls((x for x in s.split(cls._sep) if x))

    def is_subpath_of(self, other):
        if len(self) < len(other):
            return False

        return all((o == s for (o, s) in zip(other, self)))


def get_common_prefix(paths):
    """
    Get common prefix for a sequence of paths.

    :param paths: path split instances to process
    :return: common prefix path split instance
    """

    return Split(x[0] for x in itertools.takewhile(lambda x: len(frozenset(x)) == 1, itertools.izip(*paths)))

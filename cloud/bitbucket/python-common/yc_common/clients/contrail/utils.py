import json
import os.path

from uuid import UUID
from pathlib import PurePosixPath, _PosixFlavour

import collections.abc

from yc_common import logging

from .exceptions import AbsPathRequiredError


log = logging.get_logger(__name__)
log.setLevel(logging.INFO)  # Disable excessive logging


class FQName(collections.abc.Sequence):

    def __init__(self, init=None):
        if isinstance(init, str):
            self._data = init.split(':')
        elif isinstance(init, list):
            self._data = init
        elif isinstance(init, FQName):
            self._data = init._data
        else:
            self._data = []

    def __getitem__(self, idx):
        return self._data[idx]

    def __len__(self):
        return len(self._data)

    def __eq__(self, other):
        return self._data == other

    def __repr__(self):
        return repr(self._data)

    def __str__(self):
        return ':'.join(self._data)

    def __bytes__(self):
        return ':'.join(self._data).encode("latin-1")

    def __lt__(self, b):
        return len(str(self)) < len(str(b))

    def __gt__(self, b):
        return not self.__lt__(b)


class Observable(object):

    def __new__(cls, *args, **kwargs):
        return super().__new__(cls)

    @classmethod
    def register(cls, event, callback):
        log.debug("registering {} to {}".format(event, callback))
        if not hasattr(cls, "observers"):
            cls.observers = {}
        if event not in cls.observers:
            cls.observers[event] = []
        cls.observers[event].append(callback)

    @classmethod
    def unregister(cls, event, callback):
        try:
            cls.observers[event].remove(callback)
        except (ValueError, KeyError):
            pass

    @classmethod
    def emit(cls, event, data):
        log.debug("emiting event {} with {}".format(event, repr(data)))
        if not hasattr(cls, "observers"):
            cls.observers = {}
        [cbk(data)
         for evt, cbks in cls.observers.items()
         for cbk in cbks
         if evt == event]


class APIFlavour(_PosixFlavour):

    def parse_parts(self, parts):
        # Handle non ascii chars for python2
        parts = [p.encode('ascii', errors='replace').decode('ascii')
                 for p in parts]
        return super().parse_parts(parts)


class Path(PurePosixPath):
    _flavour = APIFlavour()

    @classmethod
    def _from_parsed_parts(cls, drv, root, parts):
        if parts:
            parts = [root] + os.path.relpath(os.path.join(*parts),
                                             start=root).split(os.path.sep)
            parts = [p for p in parts if p not in (".", "")]
        return super(cls, Path)._from_parsed_parts(drv, root, parts)

    def __init__(self, *args):
        self.meta = {}

    @property
    def base(self):
        try:
            return self.parts[1]
        except IndexError:
            pass
        return ''

    @property
    def is_root(self):
        return len(self.parts) == 1 and self.root == "/"

    @property
    def is_fq_name(self):
        return not self.is_uuid

    @property
    def is_uuid(self):
        try:
            UUID(self.name, version=4)
        except (ValueError, IndexError):
            return False
        return True

    @property
    def is_resource(self):
        """The path is a resource if it is not a Collection

        :raises AbsPathRequired: path doesn't start with '/'
        """
        return not self.is_collection

    @property
    def is_collection(self):
        """The path is a Collection if there is only one part in the path. Be
        careful, the root is a Collection.

        :raises AbsPathRequired: path doesn't start with '/'
        """
        if not self.is_absolute():
            raise AbsPathRequiredError("Path is not absolute: {!r}", self)
        return self.base == self.name

    def relative_to(self, path):
        try:
            return PurePosixPath.relative_to(self, path)
        except ValueError:
            return self


def to_json(resource_dict, cls=None):
    return json.dumps(resource_dict,
                      indent=2,
                      sort_keys=True,
                      skipkeys=True,
                      cls=cls)


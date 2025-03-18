# coding: utf-8
from collections import namedtuple

from ..utils import import_object


Stamp = namedtuple('Stamp', ['timestamp', 'name', 'host', 'group', 'data'])


class StampDoesNotExist(Exception):
    pass


class Stamper(object):

    def set(self, name, host, group, data=None):
        raise NotImplementedError

    def get(self, name, host=None, group=None):
        raise NotImplementedError


class LazyClientStamper(Stamper):
    def __init__(self, client_path):
        self.client_path = client_path

        self._client = None

        super(LazyClientStamper, self).__init__()

    @property
    def client(self):
        if self._client is None:
            client = import_object(self.client_path)

            if callable(client):
                client = client()

            self._client = client

        return self._client

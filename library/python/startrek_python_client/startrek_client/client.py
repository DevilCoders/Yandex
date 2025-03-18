# coding: utf-8

from yandex_tracker_client import TrackerClient
from .connection import Connection
from . import collections

__all__ = ['Startrek']


class Startrek(TrackerClient):
    connector = Connection

    def __init__(self, *args, **kwargs):
        super(Startrek, self).__init__(*args, **kwargs)
        self.goals = self._get_collection(collections.Goals)
        self.applications = self._get_collection(collections.Applications)
        self.issues = self._get_collection(collections.Issues)
        self.queues = self._get_collection(collections.Queues)

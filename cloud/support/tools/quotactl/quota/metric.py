#!/usr/bin/env python3
"""This module contains Subject of quota service."""

from quota.base import Base
from quota.utils.helpers import bytes_to_human
from quota.constants import CONVERTABLE_VALUES


class Metric(Base):
    """This object represents a quota metric object.

    Attributes:
      :name: str
      :limit: int
      :value: int
      :usage: int
      :cloud_id: str
      :folder_id: str
      :client: object
      :human_size_limit: str
      :human_size_value: str
      :human_size_usage: str

    """

    def __init__(self,
                 name=None,
                 limit=None,
                 value=None,
                 usage=None,
                 cloud_id=None,
                 folder_id=None,
                 client=None,
                 **kwargs):

        super().handle_unknown_kwargs(self, **kwargs)

        self.name = name
        self.limit = int(limit) if limit is not None else None
        self.value = int(value) if value is not None else None
        self.usage = int(usage) if usage is not None else self.value

        self.cloud_id = cloud_id
        self.folder_id = folder_id
        self.client = client

    @property
    def human_size_limit(self):
        if self.name in CONVERTABLE_VALUES:
            return bytes_to_human(self.limit)
        return self.limit

    @property
    def human_size_value(self):
        if self.name in CONVERTABLE_VALUES:
            return bytes_to_human(self.value)
        return self.value

    @property
    def human_size_usage(self):
        if self.name in CONVERTABLE_VALUES:
            return bytes_to_human(self.usage)
        return self.usage

    @classmethod
    def de_json(cls, data: dict, client: object):
        if not data:
            return None

        data = super(Metric, cls).de_json(data, client)
        return cls(client=client, **data)

    @classmethod
    def de_list(cls, data: list, client: object):
        if not data:
            return []

        metrics = list()
        for metric in data:
            metrics.append(cls.de_json(metric, client))

        return metrics

    def update(self, value: int):
        """Update metric value. Shortcut for service.update_metric()."""
        subject_id = self.folder_id if self.client.name == 'resource-manager' else self.cloud_id
        return self.client.update_metric(subject_id, self.name, value)

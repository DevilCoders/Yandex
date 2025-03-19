#!/usr/bin/env python3
"""This module contains Subject class."""

from app.quota.base import Base
from app.quota.metric import Metric
from app.quota.error import LogicError


class Subject(Base):
    """This object represents a subject of Yandex.Cloud.

    Attributes:
      :cloud_id: str
      :folder_id: str
      :metrics: list
      :client: object

    """

    def __init__(self,
                 cloud_id=None,
                 folder_id=None,
                 metrics=None,
                 client=None,
                 **kwargs):

        super().handle_unknown_kwargs(self, **kwargs)

        self.cloud_id = cloud_id
        self.folder_id = folder_id
        self.metrics = metrics
        self.client = client

    @classmethod
    def de_json(cls, data: dict, client: object):
        if not data:
            return None

        for metric in data.get('metrics', []):
            if metric is not None:
                metric.update({'cloud_id': data.get('cloud_id', None)})
                metric.update({'folder_id': data.get('folder_id', None)})

        data = super(Subject, cls).de_json(data, client)

        # Packing metrics as a sorted list of objects
        metrics = data.get('metrics')
        sorted_metrics = sorted(metrics, key=lambda k: k['name']) if isinstance(metrics, list) else metrics
        data['metrics'] = Metric.de_list(sorted_metrics, client)

        return cls(client=client, **data)

    def refresh(self):
        """Force pull actual quota metrics for subject."""
        if self.cloud_id and self.folder_id:
            raise LogicError('Folder ID and Cloud ID cannot be together')

        if self.cloud_id is not None:
            return self.client.get_metrics(self.cloud_id)

        if self.folder_id is not None:
            return self.client.get_metrics(self.folder_id)

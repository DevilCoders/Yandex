# coding: utf-8
from __future__ import unicode_literals, absolute_import

import json

from .base import Stamper, Stamp, StampDoesNotExist

from ..models import StampRecord


class DBStamper(Stamper):
    def __init__(self, database=None):
        self.queryset = StampRecord.objects.all()
        self.database = database
        if self.database:
            self.queryset = self.queryset.using(database)

    def set(self, name, host, group, data=None):
        data = json.dumps(data) if data else ''

        try:
            record = self.queryset.get(name=name, host=host, group=group)
        except StampRecord.DoesNotExist:
            record = StampRecord(name=name, host=host,
                                 group=group, data=data)
        else:
            record.data = data

        record.save(using=self.database)

        return self._convert_record(record)

    def get(self, name, host=None, group=None):
        lookup = {
            'name': name,
        }

        if host is not None:
            lookup['host'] = host

        if group is not None:
            lookup['group'] = group

        return [
            self._convert_record(r)
            for r in self.queryset.filter(**lookup)
        ]

    def _convert_record(self, record):
        return Stamp(
            timestamp=record.timestamp,
            name=record.name,
            host=record.host,
            group=record.group,
            data=json.loads(record.data) if record.data else None
        )

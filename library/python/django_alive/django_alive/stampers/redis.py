# coding: utf-8
from __future__ import unicode_literals, absolute_import

import json
import calendar
import datetime

from django.utils import timezone

from .base import LazyClientStamper, Stamp, StampDoesNotExist


class RedisStamper(LazyClientStamper):

    def __init__(self, client_path, prefix='alive'):
        self.prefix = prefix

        super(RedisStamper, self).__init__(client_path)

    def set(self, name, host, group, data=None):
        data = {'host': host, 'name': name, 'group': group, 'data': data,
                'timestamp': calendar.timegm(timezone.now().timetuple())}

        self.client.set(self._create_key(*[self.prefix, name, host, group]), json.dumps(data))

        return self._convert_record(data)

    def get(self, name, host=None, group=None):
        tail = []

        if host is not None and group is not None:
            tail += [host, group]
        elif host is not None:
            tail += [host, '*']
        elif group is not None:
            tail += ['*', group]

        keys = self.client.keys(self._create_key(*[self.prefix, name] + tail))

        return [
            self._convert_record(json.loads(r))
            for r in self.client.mget(keys)
        ] if keys else []

    def _convert_record(self, record):
        return Stamp(
            timestamp=timezone.make_aware(datetime.datetime.utcfromtimestamp(record['timestamp']),
                                          timezone.utc),
            name=record['name'],
            host=record['host'],
            group=record['group'],
            data=record['data']
        )

    def _create_key(self, *bits):
        return '-'.join(bits)

# coding: utf-8

from __future__ import unicode_literals, absolute_import

from datetime import datetime

import pytz
import tzlocal

from .base import LazyClientStamper, Stamp


class MongoDBStamper(LazyClientStamper):

    def __init__(self, client_path, collection_name='StampRecord'):
        self.collection_name = collection_name

        super(MongoDBStamper, self).__init__(client_path)

    def set(self, name, host, group, data=None):
        spec = {'host': host, 'name': name}
        data = {
            'group': group,
            'data': data,
            'timestamp': datetime.utcnow(),
        }
        data.update(spec)

        try:
            self.client[self.collection_name].update(
                spec, data, upsert=True
            )
            record = self.client[self.collection_name].find_one(spec)
        except Exception:  # FIXME
            raise

        return self._convert_record(record)

    def get(self, name, host=None, group=None):
        lookup = {'name': name}

        if host is not None:
            lookup['host'] = host

        if group is not None:
            lookup['group'] = group

        records = self.client[self.collection_name].find(lookup)
        return list(map(self._convert_record, records))

    def _convert_record(self, record):
        utc_timestamp = pytz.utc.localize(record['timestamp'])
        tz = tzlocal.get_localzone()
        local_timestamp = utc_timestamp.astimezone(tz)
        return Stamp(
            timestamp=local_timestamp,
            name=record['name'],
            host=record['host'],
            group=record['group'],
            data=record['data']
        )

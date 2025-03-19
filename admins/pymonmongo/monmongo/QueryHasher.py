import re
import json
import datetime
import bson
import logging


class QueryHasher:
    KEY_RX = re.compile(r'(\w{20,24})')

    @staticmethod
    def uniqualize_key(key: str):
        """ Hack for dealing with non-unique keys """
        return QueryHasher.KEY_RX.sub(lambda x: 'X' * len(x.group(1)), key)

    @staticmethod
    def uniqualize(data):
        """ Convert all scalar values to one """
        if isinstance(data, bool):
            return False
        if isinstance(data, int):
            return 0
        if isinstance(data, str):
            return '?'
        if isinstance(data, bson.min_key.MinKey):
            return '[minkey]'
        if isinstance(data, bson.max_key.MaxKey):
            return '[maxkey]'
        if isinstance(data, bson.uuid.UUID):
            return '[uuid]'
        if data is None:
            return None
        if isinstance(data, datetime.datetime):
            return '[datetime]'
        if isinstance(data, bson.timestamp.Timestamp):
            return '[timestamp]'
        if isinstance(data, bson.objectid.ObjectId):
            return '[objectid]'
        if isinstance(data, list):
            result = [QueryHasher.uniqualize(x) for x in data]

            # extract duplicates
            hasher = {}
            for r in result:
                hasher[QueryHasher.normalize(r)] = r
            result = []
            for name in sorted(hasher):
                result.append(hasher[name])

            return result
        if isinstance(data, dict):
            return dict([(QueryHasher.uniqualize_key(k), QueryHasher.uniqualize(data[k])) for k in data])
        logging.getLogger('QueryHasher').error('Unhandled class %s', str(data.__class__))
        return data

    @staticmethod
    def normalize(data):
        """ Uniqualize and convert to string """
        return json.dumps(QueryHasher.uniqualize(data), sort_keys=True, ensure_ascii=False)

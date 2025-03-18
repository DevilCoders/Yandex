import logging
import time
import random
import json
import hashlib

logger = logging.getLogger(__name__)


def calc_hash(d):
    return hashlib.md5(json.dumps(d, sort_keys=True)).hexdigest()


def iter_tout(initial=1.0, mult=1.5, max=20.0):
    tout = initial

    while True:
        yield tout * (random.random() + 0.5)

        tout = min(max, tout * mult)


class FakeDB(object):
    def __init__(self):
        self._ids = set()

    def have_id(self, id):
        if id in self._ids:
            return True

        self._ids.add(id)

        return False


class KiparisDB(object):
    def __init__(self, client, path, encode=None, decode=None):
        self._encode = encode or (lambda x: x)
        self._decode = decode or (lambda x: x)
        self._client = client
        self._path = path

    def have_id(self, id):
        def func(ids):
            logger.debug('ids: %s', ids)

            if not ids:
                return [id]

            if id in ids:
                return None

            return (ids + [id])[-100:]

        return self.apply(func, 'processed_ids') is None

    def repr(self, path):
        res = {}

        def func(data):
            res['res'] = data

        self.apply(func, path)

        return res['res']

    def apply(self, func, path):
        path = self._path + '/' + path

        def do():
            with self._client.Transaction():
                self._client.create('document', path, recursive=True, ignore_existing=True)
                self._client.lock(path)

                prev = self._client.get(path)

                if not prev:
                    prev = None

                if prev is None:
                    logger.info('initialize db')
                else:
                    try:
                        prev = self._decode(prev)
                    except Exception:
                        logger.exception('can not decode')

                        prev = None

                prev_hash = calc_hash(prev)
                data = func(json.loads(json.dumps(prev)))

                if data:
                    data_hash = calc_hash(data)

                    if data_hash == prev_hash:
                        logger.info('data not changed: %s', prev_hash)
                    else:
                        logger.info('will store %s, %s', data_hash, path)

                        self._client.set(path, self._encode(data))
                else:
                    logger.info('no data')

                    return prev

                return data

        for tout in iter_tout(max=60):
            try:
                return do()
            except Exception as e:
                if 'lock is taken by concurrent transaction' not in str(e):
                    if 'is locked by concurrent transaction' not in str(e):
                        raise

            logger.debug('will retry %s, after %s seconds', e, tout)
            time.sleep(tout)

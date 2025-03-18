import json
import logging

from tornado import gen

from sentinel import RedisSentinelInterface

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)


class FakeRedisSentinel(RedisSentinelInterface):

    def __init__(self, *args, **kwargs):
        self.cache = 'temp-cahe-file-for-tests'
        self.connected = False

    def is_connected(self):
        return self.connected

    def connect(self):
        logger.debug('New connection to FakeRedisSentinel')
        open(self.cache, 'w').close()
        self.connected = True

    def get_cb(self, key, callback):
        logger.debug('Get value from FakeRedisSentinel key={}'.format(key))
        value = None
        with open(self.cache, 'r') as f:
            for line in f:
                stored_key = json.loads(line)['key']
                if stored_key == key:
                    value = str(json.loads(line)['value'])
        callback(value)

    @gen.coroutine
    def get(self, key):
        response = yield gen.Task(self.get_cb, key)
        raise gen.Return(response)

    def setex_cb(self, key, seconds, value, callback):
        logger.debug('Set value in FakeRedisSentinel key={} seconds={} value={}'.format(key, seconds, value))
        with open(self.cache, 'a') as f:
            f.write(json.dumps(dict(key=str(key), value=str(value))) + '\n')  # Dont planning to test EXPIRE, so just SET
        if callback:
            callback('OK')

    @gen.coroutine
    def setex(self, key, seconds, value):
        response = yield gen.Task(self.setex_cb, key, seconds, value)
        raise gen.Return(response)

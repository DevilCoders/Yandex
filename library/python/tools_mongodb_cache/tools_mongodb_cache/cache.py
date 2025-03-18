# coding: utf-8

from __future__ import unicode_literals

import base64
import logging
import six
from datetime import timedelta

from django.utils import timezone
from pymongo import errors as pymongo_errors
from pymongo import uri_parser
from django.core.exceptions import ImproperlyConfigured
from django.core.cache.backends.base import BaseCache
from six.moves import cPickle as pickle

from .mongodb import LoggingCollection, MongoDBWrapper, ensure_indexes

logger = logging.getLogger(__name__)


def serialize_base64(value):
    return base64.b64encode(
        pickle.dumps(value, protocol=pickle.HIGHEST_PROTOCOL)
    )


def deserialize_base64(serialized):
    return pickle.loads(base64.b64decode(serialized))


class BadDataInCache(RuntimeError):
    pass


_ensured_ttl_indexes = set()  # Список коллекций, у которых точно висит TTL индекс в монге.
                              # Нужен здесь для того, чтобы делать ensure_indexes один раз для коллекции
                              # во время запуска воркера, а не на каждый запрос


UNKNOWN_MONGO_ERROR_MESSAGE = 'Unknown error from pymongo client: "%s"'


def convert_to_capped(cache, size_limit=500 * 1024 * 1024):
    """
    Convert normal collection to capped collection

    @type cache: Collection
    @param cache: collection to convert
    @type size_limit: int
    @param size_limit: capped collection size limit. default = 500 MB
    @raise: OperationFailure
    """
    db = cache.database
    collection_name = cache.collection_name

    db.command("convertToCapped", collection_name, size=size_limit, check=True)


class MongoDBCache(BaseCache):
    """
    Позволяет хранить джанго-кеши в монге.

    Если вы хотите класть не только json-сериализуемые данные,
    пропишите в CACHES в джанго-настройках.

    'OPTIONS': {
      'VALUES_ARE_JSON_SERIALIZEABLE': False,
    }

    Это отрицательно скажется на производительности.
    """
    _collection = None
    _database_name = None

    # Атрибут класса! Не одного объекта.
    # запоминаем, что монга была мертва и раз в N секунд забываем про это.
    mongodb_is_dead_since = {}
    # через какое время пробовать работать с монгой
    _give_mongo_chance_every = 1  # seconds

    # допускаются не-json сериализуемые значения в этом кеше.
    json_serializeable_values = None

    @property
    def give_mongodb_chance(self):
        """
        Вернуть True, если можно снова пробовать работать с монгой.
        """
        if not self.mongodb_is_dead_since:
            return True
        dead_since = self.mongodb_is_dead_since['dead_since']
        if dead_since + timedelta(seconds=self._give_mongo_chance_every) < timezone.now():
            del self.mongodb_is_dead_since['dead_since']
            return True
        return False

    def mongodb_became_dead(self):
        """
        Пометить монгу как совсем мертвую.
        """
        self.mongodb_is_dead_since['dead_since'] = timezone.now()

    def __init__(self, location, params, mongodb=None):
        """
        @type mongodb: инстанс подключения к монго
        """
        self.collection_name = params['collection']
        options = params.get('OPTIONS', {})
        self.write_concern = options.get('WRITE_CONCERN', 1)
        self.json_serializeable_values = options.get(
            'VALUES_ARE_JSON_SERIALIZEABLE', True)
        strategy = options.get('STRATEGY', 'NEAREST')
        assert self.write_concern > -1
        assert isinstance(self.collection_name, six.string_types)
        if not location.startswith('mongodb://'):
            raise ImproperlyConfigured('connection to mongo should start with mongodb://')
        database = uri_parser.parse_uri(location)['database']
        if not database:
            raise ImproperlyConfigured('Specify DB like that mongodb://hosts/database_name')
        self.mongodb = mongodb or MongoDBWrapper(
            hosts=location,
            strategy=strategy,
            replica_set=options['replica_set'],
            database_name=database
        )
        self.logger = logging.getLogger('mongo_requests')
        super(MongoDBCache, self).__init__(params)

    def collection(self):
        self._ensure_ttl_collection()
        return self._get_collection()

    def set(self, key, value, timeout=None, version=None):
        """
        Положить значение по ключу.

        @rtype: bool
        """
        assert isinstance(key, six.string_types)

        if not self.give_mongodb_chance:
            return False

        # кастомный таймаут записи не должен превышать таймаут коллекции, в
        # таком случае используем дефолтный таймаут коллекции
        if timeout and timeout > self.default_timeout:
            logger.warning('%s custom timeout can\'t be bigger than default %s',
                           self.collection_name, self.default_timeout)
            timeout = None

        if version is not None:
            key = self.make_key(key, version)

        try:
            self.write_in_mongo(
                key,
                value if self.json_serializeable_values else serialize_base64(value),
                timeout)
        except pymongo_errors.DocumentTooLarge as e:
            logger.exception('DocumentTooLarge: %s', repr(e))
        except pymongo_errors.PyMongoError as e:
            self.logger.exception(UNKNOWN_MONGO_ERROR_MESSAGE, repr(e))
            self.mongodb_became_dead()
            return False
        except pymongo_errors.BSONError as e:
            self.logger.exception(UNKNOWN_MONGO_ERROR_MESSAGE, repr(e))
            return False
        return True

    add = set

    def write_in_mongo(self, key, value, timeout=None):
        """
        Записать в монго значение.

        @param key: ключ, по которому пишется запись. Хранится в поле "key"
        @param value: значение, которое должно быть json-сериализуемым (!)
        @type timeout: int
        @param timeout: опциональный отдельный таймаут на запись, используемый вместо дефолтного
                        таймаута коллекции, в секундах.
        """
        # в cPickle unicode лучше не слать, он дает ошибки для high unicode символов,
        # к примеру для \xfc
        self.collection().update(
            {'key': key},
            {
                'key': key,
                'value': value,
                'status': timezone.now(),
                'record_timeout': timeout
            },
            True,
            w=self.write_concern,
        )

    def get(self, key, default=None, version=None):
        """
        Получить объект из кеша.

        Бросает BadDataInCache, если данные из кеша невозможно использовать.
        """
        assert isinstance(key, six.string_types)

        if not self.give_mongodb_chance:
            return default

        if version is not None:
            key = self.make_key(key, version)

        result = None
        try:
            result = self.read_from_mongo(key)
        except pymongo_errors.PyMongoError as e:
            self.logger.exception(UNKNOWN_MONGO_ERROR_MESSAGE, repr(e))
            self.mongodb_became_dead()
        except pymongo_errors.BSONError as e:
            self.logger.exception(UNKNOWN_MONGO_ERROR_MESSAGE, repr(e))

        if result is None:
            return default
        if not isinstance(result, dict):
            raise BadDataInCache('Result is not a dict, but "{0}"'.format(
                type(result))
            )

        if result.get('record_timeout', None):
            rtimeout = int(result['record_timeout'])

            # таймаут записи не должен быть больше таймаута коллекции, в
            # этом случае приводим его к дефолтному
            if rtimeout > self.default_timeout:
                rtimeout = self.default_timeout

            if timezone.now() > result['status'] + timedelta(seconds=rtimeout):
                return default

        result = result['value']

        try:
            if not self.json_serializeable_values:
                result = deserialize_base64(result)
        except:
            self.logger.exception('Bad data in cache')
            raise BadDataInCache('Failed to use data from cache')

        return result

    def read_from_mongo(self, key):
        return self.collection().find_one(
            {'key': key}
        )

    def delete(self, key, version=None):
        self.collection().remove({
            'key': self.make_key(key, version=version),
        })

    def delete_many(self, keys, version=None):
        self.collection().remove({
            '$or': [
                {'key': self.make_key(key, version=version)} for key in keys
            ],
        })

    def clear(self):
        self.collection().remove()

    def is_capped(self):
        """
        Return True if collection is capped

        @rtype: bool
        """
        return self.collection().options().get('capped', False)

    def _get_collection(self):
        """

        @rtype: Collection
        """
        if self._collection is None:
            self._collection = LoggingCollection(
                getattr(self.mongodb.database, self.collection_name), logger
            )
        return self._collection

    def _ensure_ttl_collection(self):
        """
        ensure collection has TTL index
        """
        if self.collection_name not in _ensured_ttl_indexes:
            try:
                ensure_indexes(self._get_collection(), self.default_timeout)
            except pymongo_errors.PyMongoError as exc:
                logger.error(
                    'I failed to ensure indexes on "%s": "%s"',
                    self.collection_name, repr(exc)
                )
            else:
                _ensured_ttl_indexes.add(self.collection_name)


class MongoFailSafeSessionCache(MongoDBCache):
    """
    Работает с монгой, не обращая внимания на ошибки монги.
    """
    MONGODB_ERROR_MESSAGE = 'Sessions unavailable because of mongodb error'

    def add(self, key, value, timeout=None, version=None):
        """
        Создать сессию, но не создавать пустую.

        Не создает пустую сессию, чтобы лишний раз не ходить в хранилище.

        """
        if not value:
            self.logger.warning('Skipped creating empty session with value "%s"', repr(value))
            return True
        try:
            return super(MongoFailSafeSessionCache, self).add(key, value, timeout, version)
        except pymongo_errors.PyMongoError:
            self.logger.exception(self.MONGODB_ERROR_MESSAGE)
            return True

    def set(self, key, value, timeout=None, version=None):
        try:
            return super(MongoFailSafeSessionCache, self).set(
                key, value, timeout, version
            )
        except pymongo_errors.PyMongoError:
            self.logger.exception(self.MONGODB_ERROR_MESSAGE)
            return True

    def delete(self, key, version=None):
        try:
            return super(MongoFailSafeSessionCache, self).delete(
                key, version
            )
        except pymongo_errors.PyMongoError:
            self.logger.exception(self.MONGODB_ERROR_MESSAGE)
            return True

    def get(self, key, default=None, version=None):
        try:
            return super(MongoFailSafeSessionCache, self).get(
                key, default, version
            )
        except pymongo_errors.PyMongoError:
            self.logger.exception(self.MONGODB_ERROR_MESSAGE)
            return default

    def _ensure_ttl_collection(self):
        """
        Обертка для подавления ошибок, если монга отвалилась
        """
        try:
            super(MongoFailSafeSessionCache, self)._ensure_ttl_collection()
        except pymongo_errors.PyMongoError:
            self.logger.exception(self.MONGODB_ERROR_MESSAGE)

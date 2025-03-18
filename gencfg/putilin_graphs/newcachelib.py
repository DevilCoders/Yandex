import hashlib
import cPickle as pickle
import json
import time
import logging
import inspect
from copy import deepcopy

from redis.sentinel import Sentinel


class SimpleRedisLease(object):
    """
    Simple redis lease/lock
    Doesn't release it in __exit__, so the same lease can be easily reacquired
    """
    def __init__(self, master, redis_key, redis_key_ttl, instance_id):
        self.master = master
        self.redis_key = redis_key
        self.redis_key_ttl = redis_key_ttl
        self.instance_id = instance_id

    def lease(self):
        while True:
            self.master.set(self.redis_key, self.instance_id, nx=True, ex=self.redis_key_ttl)

            runner_instance = self.master.get(self.redis_key)
            if runner_instance == self.instance_id:
                self.master.expire(self.redis_key, self.redis_key_ttl)
                logging.debug("Runner instance is %s -- Lease is acquired", runner_instance)
                if runner_instance == self.instance_id:
                    break

            remaining_ttl = self.master.ttl(self.redis_key)
            logging.debug("Runner instance is %s. Sleeping for %s seconds", runner_instance, remaining_ttl)
            time.sleep(remaining_ttl)

        return self

    def __enter__(self):
        return self.lease()

    def __exit__(self, exc_type, exc_value, traceback):
        return

    @staticmethod
    def get_active_instance(master, redis_key):
        return master.get(redis_key)


class CacheManager(object):
    @classmethod
    def create_from_sentinels_string(cls, comma_separated_sentinels_string, redis_service_name="mymaster", redis_db=0):
        hostports = comma_separated_sentinels_string.split(',')
        sentinels_list = []
        for hostport in hostports:
            host, port = hostport.split(':')
            port = int(port)

            sentinels_list.append((host, port))

        return cls.create_from_sentinels_list(sentinels_list, redis_service_name, redis_db)

    @classmethod
    def create_from_sentinels_list(cls, sentinels_list, redis_service_name="mymaster", redis_db=0):
        redis_sentinel = Sentinel(
            [('man1-8872.search.yandex.net', 21750),
             ('myt1-2531.search.yandex.net', 21750),
             ('sas1-3116.search.yandex.net', 21750)],
            socket_timeout=3.5
        )

        return CacheManager(redis_sentinel, redis_service_name, redis_db)

    def __init__(self, redis_sentinel, redis_service_name="mymaster", redis_db=0):
        self.redis_sentinel = redis_sentinel
        self.redis_service_name = redis_service_name
        self.redis_db = redis_db
        self.global_forced_refresh_mode = False

    def is_refresh_forced(self):
        # TODO: it's possible to implement local refresh using thread local storage
        #      (the api would change: add recursive_refresh=True to parCachedFunction.refresh_cache() )
        #       For now leaving only global refresh mode, since it's simpler and we don't really need local refresh mode
        #       (and explicit > implicit after all)
        return self.global_forced_refresh_mode  # TODO: or local_forced_refresh_mode

    def enable_global_forced_refresh_mode(self):
        self.global_forced_refresh_mode = True

    def disable_forced_refresh_mode(self):
        self.global_forced_refresh_mode = False

    def get_redis_master(self):
        return self.redis_sentinel.master_for(self.redis_service_name, db=self.redis_db)

    def _run_redis_command(self, command, *command_args, **command_kwargs):
        logging.debug("CacheManager._run_redis_command(): start")
        master = self.get_redis_master()
        logging.debug("CacheManager._run_redis_command(): redis master is %s", master)
        res = getattr(master, command)(*command_args, **command_kwargs)
        logging.debug("CacheManager._run_redis_command(): got result")

        return res

    def memoize(self, key_prefix, timeout, exclude_from_automatic_refresh=False):
        def memoize_decorator(f):
            return CachedFunction(self, key_prefix, f, timeout, exclude_from_automatic_refresh)

        return memoize_decorator

    def _get_raw_cached_data(self, redis_key):
        return self._run_redis_command('hmget', redis_key, "sha1", "data")

    def get_unpickled_cached_data_if_exists(self, redis_key):
        sha1_hash, raw_data = self._get_raw_cached_data(redis_key)
        if sha1_hash is not None:
            logging.debug("Found key %s in Redis (sha1 = %s)", redis_key, sha1_hash)
            unpickled_object = pickle.loads(raw_data)
            return True, unpickled_object
        else:
            logging.debug("Cannot find key %s in Redis", redis_key)
            return False, None

    def get_sha1(self, redis_key):
        return self._run_redis_command('hget', redis_key, "sha1")

    def get_key_creation_timestamp(self, redis_key):
        return int(self._run_redis_command('hget', redis_key, "creation_timestamp"))

    def has_redis_key(self, redis_key):
        return self._run_redis_command('exists', redis_key)

    def cached_data_size(self, redis_key):
        return self._run_redis_command('hstrlen', redis_key, 'data')

    def pickle_and_set_cached_data(self, redis_key, value, timeout):
        serialized_data = pickle.dumps(value)
        sha1_hash = hashlib.sha1(serialized_data).hexdigest()

        master = self.get_redis_master()

        def transaction(pipe):
            pipe.delete(redis_key)
            pipe.hmset(redis_key, {
                "data": serialized_data,
                "sha1": sha1_hash,
                "creation_timestamp": int(time.time())  # TODO: maybe use TIME redis command?
            })
            if timeout is not None:
                pipe.expire(redis_key, timeout)

        master.transaction(transaction)

    def clear_cache(self, redis_key):
        master = self.get_redis_master()
        master.delete(redis_key)

    def get_records_by_prefix(self, redis_key_prefix):
        res = []
        # TODO: SCAN / scan_iter
        keys = self._run_redis_command('keys', "{}*".format(redis_key_prefix))
        master = self.get_redis_master()
        pipe = master.pipeline()
        for key in keys:
            pipe.hget(key, 'data')

        for key, data in zip(keys, pipe.execute()):
            if data is not None:
                res.append((key, pickle.loads(data)))

        return res

    def get(self, key):
        return self.get_unpickled_cached_data_if_exists(key)[1]

    def set(self, key, value, timeout):
        return self.pickle_and_set_cached_data(key, value, timeout)


class CachedFunction(object):
    def __init__(self, cache_manager, redis_key_prefix, original_function, timeout, exclude_from_automatic_refresh):
        self.redis_key_prefix = "newcachelib_{}/".format(redis_key_prefix)
        self.cache_manager = cache_manager
        self.original_function = original_function
        self.timeout = timeout
        self.exclude_from_automatic_refresh = exclude_from_automatic_refresh

    def __call__(self, *args, **kwargs):
        logging.debug("Calling decorated function %s(*%s, **%s)", self.redis_key_prefix, args, kwargs)

        allowed_types = (list, int, str, unicode)

        if not all(isinstance(arg, allowed_types) for arg in list(args) + kwargs.values()):
            raise Exception("Encountered value of disallowed type. The only allowed types are {}".format(allowed_types))

        if not self.cache_manager.is_refresh_forced() or self.exclude_from_automatic_refresh:
            has_key, cached_value = self.get_cached_value_if_exists(*args, **kwargs)
            if not has_key:
                logging.debug("Refreshing cached function %s(*%s, **%s)", self.redis_key_prefix, args, kwargs)
                return self.refresh_cache(*args, **kwargs)
            else:
                logging.debug("Returning cached value of function %s(*%s, **%s)", self.redis_key_prefix, args, kwargs)
                return cached_value
        else:
            logging.debug("FORCE refreshing cached function %s(*%s, **%s)", self.redis_key_prefix, args, kwargs)
            return self.refresh_cache(*args, **kwargs)

    def _make_key(self, args, kwargs):
        remaining_kwargs = deepcopy(kwargs)
        argspec = inspect.getargspec(self.original_function)
        full_arg_list = []
        for i in xrange(len(argspec.args)):
            argname = argspec.args[i]
            if i < len(args):
                full_arg_list.append(args[i])
            else:
                if argname in kwargs:
                    full_arg_list.append(kwargs[argname])
                    del remaining_kwargs[argname]
                else:
                    full_arg_list.append(argspec.defaults[i - len(args)])

        full_arg_list += args[len(full_arg_list):]

        return "{}{}".format(self.redis_key_prefix, json.dumps([full_arg_list, sorted(remaining_kwargs.items())]))

    def _parse_key(self, key):
        fields = key.split('/', 1)
        if len(fields) != 2:
            raise Exception("Invalid key format (wrong number of fields): {}".format(key))
        key_prefix, args_str = fields
        args, kwargs_list = json.loads(args_str)

        return key_prefix, args, dict(kwargs_list)

    def is_cached(self, *args, **kwargs):
        redis_key = self._make_key(args, kwargs)
        return self.cache_manager.has_redis_key(redis_key)

    def get_cached_size(self, *args, **kwargs):
        redis_key = self._make_key(args, kwargs)
        return self.cache_manager.cached_data_size(redis_key)

    def cached_timestamp(self, *args, **kwargs):
        redis_key = self._make_key(args, kwargs)
        return self.cache_manager.get_key_creation_timestamp(redis_key)

    def call_bypassing_cache(self, *args, **kwargs):
        return self.original_function(*args, **kwargs)

    def refresh_cache(self, *args, **kwargs):
        redis_key = self._make_key(args, kwargs)

        res = self.original_function(*args, **kwargs)
        self.cache_manager.pickle_and_set_cached_data(redis_key, res, self.timeout)

        return res

    def clear_cache(self, *args, **kwargs):
        redis_key = self._make_key(args, kwargs)
        self.cache_manager.clear_cache(redis_key)

    def get_cached_value_if_exists(self, *args, **kwargs):
        redis_key = self._make_key(args, kwargs)
        return self.cache_manager.get_unpickled_cached_data_if_exists(redis_key)

    def set_cached_value(self, args, kwargs, value):
        redis_key = self._make_key(args, kwargs)
        return self.cache_manager.pickle_and_set_cached_data(redis_key, value, self.timeout)

    def get_all_cached_single_arg(self):
        def short_key_version(args, kwargs):
            if len(args) + len(kwargs) > 1:
                raise Exception(
                    "Function with too many arguments. Cannot return results as dict."
                    " Use get_all_cached_extended() which returns list of pairs"
                )
            if len(args) == 1:
                return args[0]
            else:
                return kwargs.values()[0] if kwargs else None

        args_to_value = {}

        records = self.cache_manager.get_records_by_prefix(self.redis_key_prefix)
        for key, val in records:
            key_prefix, args, kwargs = self._parse_key(key)
            args_to_value[short_key_version(args, kwargs)] = val

        return args_to_value

    def get_all_cached(self):
        res = []

        records = self.cache_manager.get_records_by_prefix(self.redis_key_prefix)
        for key, val in records:
            key_prefix, args, kwargs = self._parse_key(key)
            res.append(((args, kwargs), val))

        return res

    def refresh_all_cached(self):
        res = []

        records = self.cache_manager.get_records_by_prefix(self.redis_key_prefix)
        for key, val in records:
            key_prefix, args, kwargs = self._parse_key(key)

            val = self.refresh_cache(*args, **kwargs)

            res.append(((args, kwargs), val))

        return res

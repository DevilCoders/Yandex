# -*- coding: utf-8 -*-

import cPickle
import redis

from src.transports.base_db_transport import InstanceDbTransport
from src.utils import DummyInstance


class RedisCachingTransport(InstanceDbTransport):
    def __init__(self, slave, host='localhost', port=6379, ttl_seconds=60 * 10, trunk_fallback=True):
        super(RedisCachingTransport, self).__init__(
            trunk_fallback=trunk_fallback,
        )
        self._slave = slave
        self._cache = redis.StrictRedis(host=host, port=port)
        self.ttl_seconds = ttl_seconds

    def _cache_get(self, key):
        ret = self._cache.get(key)
        if ret is not None:
            return cPickle.loads(ret)
        return ret

    def _cache_put(self, key, value):
        dumped = cPickle.dumps(value, -1)
        self._cache.set(key, dumped, ex=self.ttl_seconds)

    def _cache_key_group_instances(self, group_name, gencfg_version):
        return 'gi:{}:{}'.format(group_name, gencfg_version)

    def _cache_get_group_instances(self, group_name, gencfg_version):
        key = self._cache_key_group_instances(group_name, gencfg_version)
        return self._cache_get(key)

    def _cache_put_group_instances(self, group_name, gencfg_version, instances):
        key = self._cache_key_group_instances(group_name, gencfg_version)
        return self._cache_put(key, instances)

    def get_group_instances(self, group_name, gencfg_version=None):
        # return self._slave.get_group_instances(group_name, gencfg_version)
        cached = self._cache_get_group_instances(group_name, gencfg_version)
        if cached is not None:
            return cached
        ret = self._slave.get_group_instances(group_name, gencfg_version)
        if ret is not None:
            self._cache_put_group_instances(group_name, gencfg_version, ret)
        return ret

    def _cache_key_intlookup_instances(self, intlookup_name, instance_type, gencfg_version):
        return 'il:{}:{}:{}'.format(intlookup_name, instance_type, gencfg_version)

    def _cache_get_intlookup_instances(self, intlookup_name, instance_type, gencfg_version):
        key = self._cache_key_intlookup_instances(intlookup_name, instance_type, gencfg_version)
        return self._cache_get(key)

    def _cache_put_intlookup_instances(self, intlookup_name, instance_type, gencfg_version, ret):
        key = self._cache_key_intlookup_instances(intlookup_name, instance_type, gencfg_version)
        return self._cache_put(key, ret)

    def get_intlookup_instances(self, intlookup_name, instance_type=None, gencfg_version=None):
        cached = self._cache_get_intlookup_instances(intlookup_name, instance_type, gencfg_version)
        if cached is not None:
            return cached
        ret = self._slave.get_intlookup_instances(
            intlookup_name, instance_type=instance_type, gencfg_version=gencfg_version,
        )
        if ret is not None:
            self._cache_put_intlookup_instances(intlookup_name, instance_type, gencfg_version, ret)
        return ret

    def describe_gencfg_version(self, gencfg_version=None):
        return self._slave.describe_gencfg_version(gencfg_version=gencfg_version)

    def _cache_key_resolve_host(self, host_name, families, use_curdb, gencfg_version):
        if isinstance(host_name, DummyInstance):
            return 'rh:{}:{}:{}:{}:{}'.format(host_name.hostname, host_name.port, families, use_curdb, gencfg_version)

        return 'rh:{}:{}:{}:{}'.format(host_name, families, use_curdb, gencfg_version)

    def _cache_get_resolve_host(self, host_name, families, use_curdb, gencfg_version):
        key = self._cache_key_resolve_host(host_name, families, use_curdb, gencfg_version)
        return self._cache_get(key)

    def _cache_put_resolve_host(self, host_name, families, use_curdb, gencfg_version, result):
        key = self._cache_key_resolve_host(host_name, families, use_curdb, gencfg_version)
        self._cache_put(key, result)

    def resolve_host(self, host_name, families, use_curdb=True, gencfg_version=None):
        cached = self._cache_get_resolve_host(host_name, families, use_curdb, gencfg_version)
        if cached is not None:
            return cached
        ret = self._slave.resolve_host(host_name, families, use_curdb=use_curdb, gencfg_version=gencfg_version)
        if ret is not None:
            self._cache_put_resolve_host(host_name, families, use_curdb, gencfg_version, ret)
        return ret

    def close_session(self):
        return self._slave.close_session()

    def name(self):
        return self._slave.name()

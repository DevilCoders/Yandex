# -*- coding: utf-8 -*-
import logging
import random
import re
import os
import socket

from collections import defaultdict

from redis.sentinel import (
    MasterNotFoundError,
    SentinelConnectionPool,
    SlaveNotFoundError,
    Sentinel,
)

logger = logging.getLogger(__name__)


class MdbSortedSlaves:
    PREFERRED_DCS = {
        'iva': ('iva', 'myt', 'vla', 'sas', 'man'),
        'myt': ('myt', 'iva', 'vla', 'sas', 'man'),
        'sas': ('sas', 'vla', 'iva', 'myt', 'man'),
        'vla': ('vla', 'iva', 'myt', 'sas', 'man'),
        'man': ('man', 'myt', 'iva', 'vla', 'sas'),
    }

    def __init__(self, current_dc=None):
        self.dc_pattern = re.compile(r'(\w{3})-.*')
        self.current_dc = current_dc or os.environ.get('DEPLOY_NODE_DC')
        self.slave_to_dc = {}
        try:
            self.preferred_dcs = self.PREFERRED_DCS[self.current_dc]
        except KeyError:
            self.preferred_dcs = list(self.PREFERRED_DCS.keys())
            random.shuffle(self.preferred_dcs)

    def get_dc(self, ip):
        fqdn = socket.getfqdn(ip)
        m = self.dc_pattern.match(fqdn)
        if not m:
            return None
        return m.group(1)

    def get_sorted_slaves(self, slaves):
        dcs = defaultdict(list)
        for slave in slaves:
            if slave not in self.slave_to_dc:
                dc = self.get_dc(slave[0])
                self.slave_to_dc[slave] = dc

            dc = self.slave_to_dc[slave]
            dcs[dc].append(slave)
        logger.debug('Redis: dcs %r', dcs)

        sorted_slaves = []
        for dc in self.preferred_dcs:
            dc_slaves = dcs.get(dc)
            if dc_slaves:
                if len(dc_slaves) > 1:
                    random.shuffle(dc_slaves)
                sorted_slaves += dc_slaves
        logger.debug('Redis: sorted_slaves %r', sorted_slaves)

        return sorted_slaves


class MdbSentinelConnectionPool(SentinelConnectionPool):
    def __init__(self, *args, **kwargs):
        self.mdb_slaves = MdbSortedSlaves()
        super().__init__(*args, **kwargs)

    def get_slaves(self):
        return self.sentinel_manager.discover_slaves(self.service_name)

    def rotate_slaves(self):
        slaves = self.get_slaves()
        if slaves:
            for slave in self.mdb_slaves.get_sorted_slaves(slaves):
                logger.debug('Redis: slave %r', slave)
                yield slave

        # Fallback to the master connection
        try:
            yield self.get_master_address()
        except MasterNotFoundError:
            pass
        raise SlaveNotFoundError('Redis: no slave found for %s' % self.service_name)


class MdbSentinel(Sentinel):
    def master_for(self, *args, connection_pool_class=MdbSentinelConnectionPool, **kwargs):
        return super().master_for(*args, connection_pool_class=connection_pool_class, **kwargs)

    def slave_for(self, *args, connection_pool_class=MdbSentinelConnectionPool, **kwargs):
        return super().slave_for(*args, connection_pool_class=connection_pool_class, **kwargs)

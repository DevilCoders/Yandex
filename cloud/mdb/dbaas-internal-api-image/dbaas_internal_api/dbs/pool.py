# -*- coding: utf-8 -*-
"""
DBaaS Internal API Base PoolGovernor
"""

import logging
import socket
import threading


class GetPoolError(Exception):
    """
    Generic get pool error
    """


class BasePoolGovernor(threading.Thread):
    """
    Simple helper for several pools management
    """

    def __init__(self, name, config, logger_name):
        threading.Thread.__init__(self, name=name)
        self.daemon = True
        self.logger = logging.getLogger(logger_name)
        self.config = config
        self.should_run = True
        self.pools = {}
        self.dc_map = {
            'e': 'iva',
            'f': 'myt',
            'h': 'sas',
            'i': 'man',
            'k': 'vla',
        }
        self.my_dc = self._get_host_dc(socket.getfqdn())

    def _get_host_dc(self, host):
        """
        Get host dc (None if not found by dc map)
        """
        prefix = host.split('-')[0]
        if prefix in self.dc_map.values():
            return prefix
        return self.dc_map.get(host.split('.')[0][-1], None)

    def _get_local_host(self, hosts):
        """
        Get hostname with same dc as our host
        (returns first host if no match found)
        """
        if self.my_dc:
            for host in hosts:
                if self._get_host_dc(host) == self.my_dc:
                    return host

        return hosts[0]

    def stop(self):
        """
        Properly stop governor
        """
        self.should_run = False

    # pylint: disable=unused-argument
    def getpool(self, master=False):
        """
        Get connection pool
        """
        raise RuntimeError('Not implemented')

    def run(self):
        raise RuntimeError('Not implemented')

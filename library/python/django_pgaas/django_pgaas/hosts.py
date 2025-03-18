# coding=utf-8

import os
from random import random, randrange, shuffle


class HostManager(object):
    # Maps datacentres to their `nearest neighbours` in a preferred order.
    # Default ordering is by geodesic distance.
    # See github.yandex-team.ru/gist/mkznts/8f25c19950c0e2a959694e4e66eedec7
    PREFERRED_DCS = {
        'iva': ('iva', 'myt', 'vla', 'sas', 'man'),
        'myt': ('myt', 'iva', 'vla', 'sas', 'man'),
        'sas': ('sas', 'vla', 'iva', 'myt', 'man'),
        'vla': ('vla', 'iva', 'myt', 'sas', 'man'),
        'man': ('man', 'myt', 'iva', 'vla', 'sas'),
    }

    def __init__(self, hosts, current_dc=None, preferred_dcs=None, randomize_same_dc=False):
        self._hosts = hosts
        self._current_dc = current_dc
        self._randomize_same_dc = randomize_same_dc

        dcs = preferred_dcs or self.PREFERRED_DCS

        try:
            preferred = dcs[self.current_dc]

        except KeyError:
            # Use random order when the current DC is unknown or undefined
            preferred = list(dcs.keys())
            shuffle(preferred)

        # Map DCs to ranks. The less the more preferred.
        self.ranks = {dc: rank for rank, dc in enumerate(preferred)}

    @property
    def current_dc(self):
        if self._current_dc is not None:
            return self._current_dc
        dc_env_value = os.getenv('QLOUD_DATACENTER') or os.getenv('DEPLOY_NODE_DC') or ''
        return dc_env_value.lower()

    @property
    def host_string(self):
        return ','.join(self.sorted_hosts)

    @property
    def sorted_hosts(self):
        def get_sort_key(x):
            if self._randomize_same_dc:
                return (self.get_rank(x[1]), random())
            return self.get_rank(x[1])

        s = sorted(self._hosts, key=get_sort_key)
        return [host for host, dc in s]

    def get_rank(self, dc):
        return self.ranks.get(dc, randrange(0, len(self.ranks) + 1))

    @classmethod
    def create_from_yc(cls, yc_hosts, current_dc=None, preferred_dcs=None, randomize_same_dc=False):
        """
        Create using list of hosts as returned by `yc mdb cluster ListHosts`:
        {
            "name": <hostname>,
            "options": {"geo": "sas|vla|man|..."}
        }
        """

        host_geo = [
            (host['name'], host.get('options', {}).get('geo'))
            for host in yc_hosts
        ]

        return cls(
            host_geo, current_dc, preferred_dcs=preferred_dcs, randomize_same_dc=randomize_same_dc,
        )

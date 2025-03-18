import re
import socket
from enum import IntEnum
from collections import defaultdict


class DataCenters(IntEnum):
    man = 1
    sas = 2
    myt = 3
    vla = 4
    iva = 5

    def __str__(self):
        return str(self.value)

    def __repr__(self):
        return str(self.value)


HOSTNAME = socket.gethostname()
PREFERABLE_DC_MAP = {
    DataCenters.sas: [DataCenters.sas, DataCenters.vla],
    DataCenters.vla: [DataCenters.vla, DataCenters.sas],
    DataCenters.myt: [DataCenters.myt, DataCenters.sas, DataCenters.vla],
    DataCenters.iva: [DataCenters.iva, DataCenters.vla, DataCenters.sas],
    DataCenters.man: [DataCenters.man, DataCenters.vla],
}
DC_RE = re.compile(r'\b(?:{})\b'.format('|'.join(dc.name for dc in DataCenters)))


def get_host_dc(host):
    if isinstance(host, (tuple, list)):
        host = host[0]
    s = DC_RE.search(host)
    return getattr(DataCenters, s.group(), None) if s else None


def group_by_preferred_dc(hosts, host):
    """
    >>> hosts = ['man-ad48abw8zimwyiyv.db.yandex.net', 'man-01.yandex.net', 'sas-eg5ai7thhldbhokv.db.yandex.net', 'vla-kwuq43ehqnqu5tq6.db.yandex.net']
    >>> group_by_preferred_dc(hosts, 'cryproxtest-sas-4.aab.yandex.net')
    [['sas-eg5ai7thhldbhokv.db.yandex.net'], ['vla-kwuq43ehqnqu5tq6.db.yandex.net'], ['man-ad48abw8zimwyiyv.db.yandex.net', 'man-01.yandex.net']]
    >>> group_by_preferred_dc(hosts, 'cryprox-vla-02.aab.yandex.net')
    [['vla-kwuq43ehqnqu5tq6.db.yandex.net'], ['sas-eg5ai7thhldbhokv.db.yandex.net'], ['man-ad48abw8zimwyiyv.db.yandex.net', 'man-01.yandex.net']]
    >>> group_by_preferred_dc(hosts, 'cryprox-man-12.aab.yandex.net')
    [['man-ad48abw8zimwyiyv.db.yandex.net', 'man-01.yandex.net'], ['vla-kwuq43ehqnqu5tq6.db.yandex.net'], ['sas-eg5ai7thhldbhokv.db.yandex.net']]

    >>> hosts = ['sas.yp-c.yandex.net', 'yandex.ru', '5.255.255.70', '2a02:6b8:a::a']
    >>> group_by_preferred_dc(hosts, 'cryprox-sas-12.aab.yandex.net')
    [['sas.yp-c.yandex.net'], ['yandex.ru', '5.255.255.70', '2a02:6b8:a::a']]
    >>> group_by_preferred_dc(hosts, 'dev-osx')
    [['sas.yp-c.yandex.net', 'yandex.ru', '5.255.255.70', '2a02:6b8:a::a']]

    >>> hosts = [('man-1.db.yandex.net', 6379), ('sas-2.db.yandex.net', 6379), ('vla-3.db.yandex.net', 6379)]
    >>> group_by_preferred_dc(hosts, 'cryprox-iva-10.aab.yandex.net')
    [[('vla-3.db.yandex.net', 6379)], [('sas-2.db.yandex.net', 6379)], [('man-1.db.yandex.net', 6379)]]
    """
    host_dc = get_host_dc(host)
    if host_dc is None or host_dc not in PREFERABLE_DC_MAP:
        return [hosts]
    preferable_dcs = PREFERABLE_DC_MAP[host_dc]

    sorted_hosts = []
    preferable_dc_to_hosts_map = defaultdict(list)
    other_dc_hosts = []
    for h in hosts:
        dc = get_host_dc(h)
        if dc is not None and dc in preferable_dcs:
            preferable_dc_to_hosts_map[dc].append(h)
        else:
            other_dc_hosts.append(h)
    for dc in preferable_dcs:
        if preferable_dc_to_hosts_map[dc]:
            sorted_hosts.append(preferable_dc_to_hosts_map[dc])
    if other_dc_hosts:
        sorted_hosts.append(other_dc_hosts)
    return sorted_hosts


def group_by_preferred_dc_for_current_host(hosts):
    return group_by_preferred_dc(hosts, HOSTNAME)

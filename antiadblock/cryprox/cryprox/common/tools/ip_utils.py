import os
import logging
import socket
from collections import namedtuple

from netaddr import IPAddress
from netaddr.ip import IPV4_LOOPBACK, IPV6_LOOPBACK
from tornado.httpclient import HTTPClient

from antiadblock.cryprox.cryprox.config import service

URL = "https://racktables.yandex.net/export/expand-fw-macro.php?macro=_YANDEXNETS_"
HARDCODED_YANDEX_NETS = ('5.45.192.0/18', '5.255.192.0/18', '37.9.64.0/18', '37.140.128.0/18', '77.88.0.0/18', '87.250.224.0/19', '93.158.128.0/18', '95.108.128.0/17',
                         '100.43.64.0/19', '141.8.128.0/18', '178.154.128.0/17', '199.36.240.0/22', '213.180.192.0/19', '2620:10f:d000::/44', '2a02:6b8::/32')
CACHE_FILENAME = 'cached_yandex_nets.txt'
CACHE_RESOURCE = os.path.join(service.PERSISTENT_VOLUME_PATH, CACHE_FILENAME)

HeaderStatusType = namedtuple('HeaderStatusType', 'absent internal external')
HeaderStatus = HeaderStatusType('absent', 'internal', 'external')


def get_yandex_nets(url=URL, cache=CACHE_RESOURCE, default_result=HARDCODED_YANDEX_NETS):
    def get_from_racktables():
        client = HTTPClient()
        try:
            result = client.fetch(url, connect_timeout=1, request_timeout=2)
        except Exception:
            logging.exception("Exception while trying to download yandex networks list")
            return
        try:
            if not os.path.exists(os.path.dirname(cache)):
                os.makedirs(os.path.dirname(cache))
            with open(cache, "w+") as f:
                f.write(result.body)
        except Exception:
            logging.exception("Error while trying to write the list of yandex networks")
        return result.body.split()

    def get_from_cache():
        try:
            with open(cache, "r") as f:
                return [s.rstrip() for s in f.readlines()]
        except Exception:
            logging.exception("Exception while trying to read cached yandex networks list")

    if service.ENV_TYPE == 'load_testing':  # Since we have nginx-with-stubs on a same machine as cryprox and are testing non-internal partner
        return []
    else:
        return get_from_racktables() or get_from_cache() or default_result


def is_ip_in_net(nets, ip_addr):
    if not ip_addr:
        return False
    for net in nets:
        if ip_addr in net:
            return True
    return False


def check_ip_headers_are_internal(internal_nets, headers, header_names=('x-real-ip', 'x-forwarded-for')):
    """
    :param internal_nets: list of internal nets
    :param headers: request headers
    :param header_names: headers with ip you need to check
    :return: dict { header_name: value}
    value = -1 if header is not present, 1/0 - present and is/not Yandex
    """
    result = dict()
    for header_name in header_names:
        header_value = headers.get(header_name, HeaderStatus.absent)
        if header_value != HeaderStatus.absent:
            header_value = HeaderStatus.internal if is_ip_in_net(internal_nets, header_value.split(',', 1)[0].strip()) \
                else HeaderStatus.external

        result.update({header_name: header_value})
    return result


def ip_headers_is_internal(checked_ip_headers):
    """
    :param checked_ip_headers: dict { header_name: value}
    value = -1 if header is not present, 1/0 - present and is/not Yandex
    :return: bool value is internal or not
    """
    for header_status_value in checked_ip_headers.values():
        if header_status_value == HeaderStatus.internal:
            return True
    return False


def is_local_net(sock, local_nets):
    is_local = False
    try:
        if sock:
            peername = sock.getpeername()[0]
            ip_addr = IPAddress(peername)
            if sock.family == socket.AF_INET:
                if ip_addr in IPV4_LOOPBACK or is_ip_in_net(filter(lambda n: n.version == 4, local_nets), ip_addr):
                    is_local = True
            elif sock.family == socket.AF_INET6:
                if ip_addr == IPV6_LOOPBACK or is_ip_in_net(filter(lambda n: n.version == 6, local_nets), ip_addr):
                    is_local = True
    except Exception:
        logging.exception("failed to detect net")
    return is_local

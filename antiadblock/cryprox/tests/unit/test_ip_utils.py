import socket

import pytest
from netaddr import IPNetwork

from antiadblock.cryprox.cryprox.common.tools.ip_utils import is_local_net, check_ip_headers_are_internal, \
    HARDCODED_YANDEX_NETS, HeaderStatus, ip_headers_is_internal

LOCAL_NETS = map(lambda n: IPNetwork(n), HARDCODED_YANDEX_NETS)


class SocketMock:
    def __init__(self, family, ip):
        self.family = family
        self.peername = (ip, 0)

    def getpeername(self):
        return self.peername


@pytest.mark.parametrize('sock, expected', [
    (SocketMock(socket.AF_INET, "5.45.192.1"), True),
    (SocketMock(socket.AF_INET, "6.45.192.1"), False),
    (SocketMock(socket.AF_INET6, "2a02:6b8::2:242"), True),
    (SocketMock(socket.AF_INET, None), False)
])
def test_is_local_net(sock, expected):
    assert expected == is_local_net(sock, LOCAL_NETS)


@pytest.mark.parametrize('x_real_ip, x_forwarded_for, result, is_internal', [
    ('', '', {'x-real-ip': HeaderStatus.absent, 'x-forwarded-for': HeaderStatus.absent}, False),
    ('64.233.165.102', '64.233.165.113', {'x-real-ip': HeaderStatus.external, 'x-forwarded-for': HeaderStatus.external}, False),
    ('5.45.192.1', '2a02:6b8::2:242', {'x-real-ip': HeaderStatus.internal, 'x-forwarded-for': HeaderStatus.internal}, True),
    ('2a00:1450:4010:c08::8a', '2a02:6b8::2:242', {'x-real-ip': HeaderStatus.external, 'x-forwarded-for': HeaderStatus.internal}, True),
    ('', '5.45.192.1', {'x-real-ip': HeaderStatus.absent, 'x-forwarded-for': HeaderStatus.internal}, True),
    ('', '64.233.165.113,5.45.192.1', {'x-real-ip': HeaderStatus.absent, 'x-forwarded-for': HeaderStatus.external}, False),
    ('', '5.45.192.1,64.233.165.113', {'x-real-ip': HeaderStatus.absent, 'x-forwarded-for': HeaderStatus.internal}, True),
])
def test_ip_headers(x_real_ip, x_forwarded_for, result, is_internal):
    headers = {'x-real-ip': x_real_ip,
               'x-forwarded-for': x_forwarded_for}
    checked_ip_headers = check_ip_headers_are_internal(LOCAL_NETS, headers)
    assert checked_ip_headers
    assert ip_headers_is_internal(checked_ip_headers) == is_internal

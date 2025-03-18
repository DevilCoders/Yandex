import pytest

from netaddr import IPNetwork, IPAddress
from tornado.concurrent import Future

from antiadblock.cryprox.cryprox.service.resolver import BlacklistingResolver


class MockResolver(object):
    def __init__(self, resolved_addresses):
        self.resolved = resolved_addresses

    def resolve(self, host, port, *args, **kwargs):
        res = Future()
        res.set_result(self.resolved)
        return res


@pytest.mark.parametrize('blacklisted_nets, resolved_addresses', [([IPNetwork('192.168.1.2')], [(2, ('192.168.1.1', 1024)), (2, ('192.168.1.2', 1024))]),
                                                                  ([IPNetwork('2001:db8:a0b:12f0::2')], [(2, ('2001:db8:a0b:12f0::1', 1024)), (2, ('2001:db8:a0b:12f0::2', 1024))]),
                                                                  ])
def test_resolver_filtering_matching_host(blacklisted_nets, resolved_addresses):
    resolver = BlacklistingResolver(resolver=MockResolver(resolved_addresses), blacklisted_networks=blacklisted_nets)
    result = resolver.resolve('test.local', 0)
    assert len(result.result()) == 1
    assert not any([IPAddress(result.result()[0][1][0]) in net for net in blacklisted_nets])


@pytest.mark.parametrize('blacklisted_nets, resolved_addresses', [([IPNetwork('192.168.1.2')], [(2, ('192.168.1.2', 1024))]),
                                                                  ([IPNetwork('2001:db8:a0b:12f0::2')], [(2, ('2001:db8:a0b:12f0::2', 1024))]),
                                                                  ])
def test_resolver_no_allowed_addresses(blacklisted_nets, resolved_addresses):
    resolver = BlacklistingResolver(resolver=MockResolver(resolved_addresses), blacklisted_networks=blacklisted_nets)
    result = resolver.resolve('test.local', 0)

    with pytest.raises(IOError):
        result.result()

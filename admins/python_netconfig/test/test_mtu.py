from utils import get_interface, has_up


HOST_DATA = {
    'test.host': {
        'addresses': {
            'ip6': {
                'address': '2a02:6b8:0:1435::2',
                'family': 6,
            }
        },
        'host': {
            'groups': [],
            'tags': [],
        },
    }
}

DNS_DATA = {
    'test.host': {
        4: [],
        6: ['2a02:6b8:0:1435::2'],
    }
}


def test_ipv6_only_mtu():
    params = dict(
        fqdn='test.host',
        host_data=HOST_DATA,
        dns_data=DNS_DATA,
    )
    iface = get_interface('eth0', params)
    assert iface.mtu is None
    assert has_up(iface, 'ifconfig eth0 up mtu 8950')

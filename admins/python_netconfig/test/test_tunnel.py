from utils import get_interface, has_postup, has_postdown


HOST_DATA = {
    'test.host': {
        'addresses': {
            'ip6': {
                'address': '2a02:6b8:0:1435::2',
                'family': 6,
                'gateway': '2a02:6b8:0:1435::1',
                'tunnel_vips': ['5.255.240.211'],
            },
            'ip4': {
                'address': '37.9.69.65',
                'family': 4,
                'tunnel_vips': [],
                'cdn_vips': [],
            },
        },
        'host': {
            'groups': [],
            'tags': ['ipvs_tun', 'ipvs_tun_communal'],
        },
    }
}

DNS_DATA = {
    'test.host': {
        4: ['37.9.69.65'],
        6: ['2a02:6b8:0:1435::2'],
    }
}


def test_ipv6_tunnel_has_ipv4_loopback():
    params = dict(
        fqdn='test.host',
        host_data=HOST_DATA,
        dns_data=DNS_DATA,
    )
    iface = get_interface('lo:200', params)
    assert has_postup(iface, 'route add 5.255.240.211 via 2a02:6b8:0:1435::1 dev eth0 src 37.9.69.65')
    assert has_postdown(iface, 'route del 5.255.240.211 via 2a02:6b8:0:1435::1 dev eth0 src 37.9.69.65')

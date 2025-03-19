# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt.salt._modules import mdb_firewall, dbaas, mdb_network
from cloud.mdb.salt_tests.common.mocks import mock_grains, mock_pillar
from cloud.mdb.internal.python.pytest.utils import parametrize


@parametrize(
    {
        'id': 'default',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f801:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'compute',
                    },
                },
                'firewall': {
                    'ports': {'external': [6379, 26379, 6380, 26380]},
                },
            },
            'testing': False,
            'result': '''domain ip6 {
    table filter {
        chain INPUT {

        }
    }
}''',
        },
    },
    {
        'id': 'default, ports=None',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f801:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'compute',
                    },
                },
            },
            'testing': False,
            'result': None,
        },
    },
    {
        'id': 'default, firewall:reject_enabled=True',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f801:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'compute',
                    },
                },
                'firewall': {
                    'ports': {'external': [6379, 26379, 6380, 26380]},
                    'reject_enabled': True,
                },
            },
            'testing': False,
            'result': '''domain ip6 {
    table filter {
        chain INPUT {

        }
    }
}''',
        },
    },
    {
        'id': 'default, firewall:reject_enabled=True, data:dbaas:vtype=porto',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f801:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'porto',
                    },
                },
                'firewall': {
                    'ports': {'external': [6379, 26379, 6380, 26380]},
                    'reject_enabled': True,
                },
            },
            'testing': False,
            'result': '''domain ip6 {
    table filter {
        chain INPUT {

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x453d"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x453e"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x4b9c"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x660"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf80a"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf826"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf82f"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf83d"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf889"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf898"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc11"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc15"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc1f"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc58"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc72"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

        }
    }
}''',
        },
    },
    {
        'id': 'enabled access flags, prod env',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f801:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'access': {
                        'web_sql': True,
                        'data_lens': True,
                        'metrika': True,
                        'serverless': True,
                        'yandex_query': True,
                        'data_transfer': True,
                    },
                    'dbaas': {
                        'vtype': 'compute',
                    },
                },
                'firewall': {
                    'ports': {'external': [6379, 26379, 6380, 26380]},
                },
            },
            'testing': False,
            'result': '''domain ip6 {
    table filter {
        chain INPUT {

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x453e"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x4b9c"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x660"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf80a"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf826"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf82f"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf83d"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf889"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf898"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

        }
    }
}''',
        },
    },
    {
        'id': 'enabled access flags, prod env, firewall:reject_enabled=True, data:dbaas:vtype=porto',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f801:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'access': {
                        'web_sql': True,
                        'data_lens': True,
                        'metrika': True,
                        'serverless': True,
                        'yandex_query': True,
                        'data_transfer': True,
                    },
                    'dbaas': {
                        'vtype': 'porto',
                    },
                },
                'firewall': {
                    'ports': {'external': [6379, 26379, 6380, 26380]},
                    'reject_enabled': True,
                },
            },
            'testing': False,
            'result': '''domain ip6 {
    table filter {
        chain INPUT {

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x453d"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x453e"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x4b9c"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x660"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf80a"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf826"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf82f"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf83d"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf889"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf898"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc11"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc15"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc1f"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc58"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc72"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

        }
    }
}''',
        },
    },
    {
        'id': 'enabled access flags, testing env',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f801:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'access': {
                        'web_sql': True,
                        'data_lens': True,
                        'metrika': True,
                        'serverless': True,
                        'yandex_query': True,
                        'data_transfer': True,
                    },
                    'dbaas': {
                        'vtype': 'compute',
                    },
                },
                'firewall': {
                    'ports': {'external': [6379, 26379, 6380, 26380]},
                },
            },
            'testing': True,
            'result': '''domain ip6 {
    table filter {
        chain INPUT {

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x453d"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x453e"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x4b9c"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x660"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf80a"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf826"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf82f"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf83d"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf889"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf898"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc11"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc15"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc1f"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc58"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc72"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

        }
    }
}''',
        },
    },
    {
        'id': 'enabled access flags, testing env, firewall:reject_enabled=True, data:dbaas:vtype=porto',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f801:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'access': {
                        'web_sql': True,
                        'data_lens': True,
                        'metrika': True,
                        'serverless': True,
                        'yandex_query': True,
                        'data_transfer': True,
                    },
                    'dbaas': {
                        'vtype': 'porto',
                    },
                },
                'firewall': {
                    'ports': {'external': [6379, 26379, 6380, 26380]},
                    'reject_enabled': True,
                },
            },
            'testing': True,
            'result': '''domain ip6 {
    table filter {
        chain INPUT {

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x453d"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x453e"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x4b9c"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x660"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf80a"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf826"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf82f"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf83d"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf889"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf898"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc11"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc15"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc1f"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc58"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc72"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

        }
    }
}''',
        },
    },
    {
        'id': 'custom external projects',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f801:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'access': {
                        'web_sql': True,
                        'data_lens': True,
                        'metrika': True,
                        'serverless': True,
                    },
                    'dbaas': {
                        'vtype': 'compute',
                    },
                    'external_project_ids': ['0xf101', '0xf102'],
                },
                'firewall': {
                    'ports': {'external': [6379, 26379, 6380, 26380]},
                },
            },
            'testing': False,
            'result': '''domain ip6 {
    table filter {
        chain INPUT {

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x453e"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x4b9c"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x660"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf101"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf102"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf80a"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf82f"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf83d"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf898"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

        }
    }
}''',
        },
    },
    {
        'id': 'custom external projects, firewall:reject_enabled=True, data:dbaas:vtype=porto',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f801:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'access': {
                        'web_sql': True,
                        'data_lens': True,
                        'metrika': True,
                        'serverless': True,
                    },
                    'external_project_ids': ['0xf101', '0xf102'],
                    'dbaas': {
                        'vtype': 'porto',
                    },
                },
                'firewall': {
                    'ports': {'external': [6379, 26379, 6380, 26380]},
                    'reject_enabled': True,
                },
            },
            'testing': False,
            'result': '''domain ip6 {
    table filter {
        chain INPUT {

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x453d"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x453e"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x4b9c"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x660"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf101"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf102"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf80a"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf826"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf82f"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf83d"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf889"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf898"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc11"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc15"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc1f"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc58"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc72"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

        }
    }
}''',
        },
    },
    {
        'id': 'user project matches one of external projects',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f83d:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'access': {
                        'web_sql': True,
                        'data_lens': True,
                        'metrika': True,
                        'serverless': True,
                    },
                    'dbaas': {
                        'vtype': 'compute',
                    },
                },
                'firewall': {
                    'ports': {'external': [6379, 26379, 6380, 26380]},
                },
            },
            'testing': False,
            'result': '''domain ip6 {
    table filter {
        chain INPUT {

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x453e"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x4b9c"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x660"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf80a"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf82f"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf83d"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf898"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

        }
    }
}''',
        },
    },
    {
        'id': 'user project matches one of external projects, firewall:reject_enabled=True, data:dbaas:vtype=porto',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f83d:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'access': {
                        'web_sql': True,
                        'data_lens': True,
                        'metrika': True,
                        'serverless': True,
                    },
                    'dbaas': {
                        'vtype': 'porto',
                    },
                },
                'firewall': {
                    'ports': {'external': [6379, 26379, 6380, 26380]},
                    'reject_enabled': True,
                },
            },
            'testing': False,
            'result': '''domain ip6 {
    table filter {
        chain INPUT {

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x453d"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x453e"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x4b9c"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x660"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf80a"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf826"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf82f"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf83d"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf889"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf898"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc11"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc15"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc1f"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc58"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc72"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

        }
    }
}''',
        },
    },
    {
        'id': 'user project matches no one of external projects',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:faaa:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'access': {
                        'web_sql': True,
                        'data_lens': True,
                        'metrika': True,
                        'serverless': True,
                    },
                    'dbaas': {
                        'vtype': 'compute',
                    },
                },
                'firewall': {
                    'ports': {'external': [6379, 26379, 6380, 26380]},
                },
            },
            'testing': False,
            'result': '''domain ip6 {
    table filter {
        chain INPUT {

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x453e"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x4b9c"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x660"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf80a"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf82f"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf83d"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf898"
            interface eth1 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

        }
    }
}''',
        },
    },
    {
        'id': 'user project matches no one of external projects, firewall:reject_enabled=True, data:dbaas:vtype=porto',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:faaa:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'pillar': {
                'data': {
                    'access': {
                        'web_sql': True,
                        'data_lens': True,
                        'metrika': True,
                        'serverless': True,
                    },
                    'dbaas': {
                        'vtype': 'porto',
                    },
                },
                'firewall': {
                    'ports': {'external': [6379, 26379, 6380, 26380]},
                    'reject_enabled': True,
                },
            },
            'testing': False,
            'result': '''domain ip6 {
    table filter {
        chain INPUT {

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x453d"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x453e"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x4b9c"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0x660"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf80a"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf826"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf82f"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf83d"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf889"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xf898"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) ACCEPT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc11"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc15"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc1f"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc58"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

            proto tcp
            mod u32 u32 "0x10&0xffffffff=0xfc72"
            interface eth0 mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( 6379 26379 6380 26380 ) REJECT;

        }
    }
}''',
        },
    },
)
def test_render_external_access_config(grains, pillar, testing, result):
    mock_grains(mdb_firewall.__salt__, grains)
    mock_pillar(mdb_firewall.__salt__, pillar)
    mock_grains(dbaas.__salt__, grains)
    mock_pillar(dbaas.__salt__, pillar)
    mock_grains(mdb_network.__salt__, grains)
    mock_pillar(mdb_network.__salt__, pillar)
    mdb_firewall.__salt__['dbaas.is_porto'] = lambda: dbaas.is_porto()
    mdb_firewall.__salt__['dbaas.is_compute'] = lambda: dbaas.is_compute()
    mdb_firewall.__salt__[
        'mdb_network.get_external_project_ids_with_action'
    ] = lambda: mdb_network.get_external_project_ids_with_action()
    mdb_network.__salt__['dbaas.is_testing'] = lambda: testing
    assert mdb_firewall.render_external_access_config() == result

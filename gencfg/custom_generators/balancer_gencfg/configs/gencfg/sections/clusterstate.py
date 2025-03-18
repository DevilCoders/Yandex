from collections import OrderedDict

import src.modules as Modules


CLUSTERSTATE_HOST = 'clusterstate.yandex-team.ru(:\\\\d+)?'
CLUSTERSTATE_BETA_HOST = 'clusterstate-beta.yandex-team.ru(:\\\\d+)?'

HOSTINFO_BACKENDS = [
    'MAN_CLUSTERSTATE_HOSTINFO',
    'SAS_CLUSTERSTATE_HOSTINFO',
    'VLA_CLUSTERSTATE_HOSTINFO'
]

CLUSTERSTATE_BACKENDS = [
    'MAN_CLUSTERSTATE',
    'SAS_CLUSTERSTATE',
    'VLA_CLUSTERSTATE',
]


def get_modules():
    proxy_options = OrderedDict([
        ('backend_timeout', '320s'),
    ])
    return [
        ('clusterstate', {'match_fsm': OrderedDict([('host', CLUSTERSTATE_HOST)])}, [
            (Modules.Regexp, [
                ('searchmap', {'match_fsm': OrderedDict([('URI', '/search_map(/.*)?')])}, [
                    (Modules.Report, {'uuid': 'clusterstate'}),
                    (Modules.Balancer, {
                        'balancer_type': 'hashing',
                        'backends': HOSTINFO_BACKENDS,
                        'proxy_options': proxy_options,
                        'attempts': 3,
                    }),
                ]),
                ('groupalive', {'match_fsm': OrderedDict([('URI', '/group/[A-Z0-9_]+/alive(/.*)?')])}, [
                    (Modules.Report, {'uuid': 'clusterstate'}),
                    (Modules.Balancer, {
                        'balancer_type': 'hashing',
                        'backends': HOSTINFO_BACKENDS,
                        'proxy_options': proxy_options,
                        'attempts': 3,
                    }),
                ]),
                ('default', {}, [
                    (Modules.Report, {'uuid': 'clusterstate'}),
                    (Modules.Balancer, {
                        'balancer_type': 'rr',
                        'backends': CLUSTERSTATE_BACKENDS,
                        'proxy_options': proxy_options,
                        'attempts': 3,
                    }),
                ]),
            ]),
        ]),
    ]

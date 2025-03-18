#!/skynet/python/bin/python

from collections import OrderedDict

import src.constants as Constants
import src.macroses as Macroses
import src.modules as Modules
from src.generator import generate
from src.optionschecker import OptionsChecker


@OptionsChecker([
    ('addrs_backends', None, OrderedDict, True),
    ('attempts', 5, int, False),
    ('config_tag', 'trunk', str, False),
    ('geo', None, str, False),
    ('need_resolve', False, bool, False),
    ('connect_timeout', '0.08s', str, False),
    ('backend_timeout', '5s', str, False),
])
def templateAddrsReport(options):

    if options['geo'] in ('man'):
        to_upstream_modules = (Modules.Balancer2, {
            'attempts': options['attempts'],
            'attempts_file': './controls/attempts.count',
            'backends': options['addrs_backends'][options['geo']],
            'policies': OrderedDict([
                ('unique_policy', {})
            ]),
            'proxy_options': OrderedDict([
                ('connect_timeout', options['connect_timeout']),
                ('backend_timeout', options['backend_timeout']),
                ('need_resolve', options['need_resolve']),
                ('keepalive_count', 10),
            ])
        })

    else:
        to_upstream_modules = (Modules.Balancer2, {
            'balancer_type': 'rr',
            'balancer_options': OrderedDict([
                ('weights_file', './controls/traffic_control.weights')
            ]),
            'attempts': len(options['addrs_backends']),
            'policies': OrderedDict([
                ('unique_policy', {})
            ]),
            'custom_backends': [
                (1, 'addrs_{}'.format(key), [
                    (Modules.Report, {
                        'uuid': 'requests_to_{}'.format(key)
                    }),
                    (Modules.Balancer2, {
                        'backends': value,
                        'attempts': options['attempts'],
                        'attempts_file': './controls/attempts.count',
                        'policies': OrderedDict([
                            ('unique_policy', {})
                        ]),
                        'proxy_options': OrderedDict([
                            ('connect_timeout', options['connect_timeout']),
                            ('backend_timeout', options['backend_timeout']),
                            ('need_resolve', options['need_resolve']),
                            ('keepalive_count', 10),
                        ])
                    })
                ]) for key, value in options['addrs_backends'].iteritems()
            ]
        })

    submodules = [
        (Macroses.ExtendedHttpV2, {'port': 15160}),
        (Macroses.WebHeaders, {'disable_response_headers': True}),
        (Modules.Regexp, [
            ('slb_ping', {'match_fsm': OrderedDict([('url', '/yandsearch\\\\?info=getstatus')])}, [
                (Modules.Balancer, {
                    'balancer_type': 'rr',
                    'attempts': 1,
                    'balancer_options': OrderedDict([
                        ('weights_file', './controls/slb_check.weights')
                    ]),
                    'custom_backends': [
                        (1, 'to_upstream', [to_upstream_modules]),
                        (-1, 'switch_off', [
                            (Modules.ErrorDocument, {
                                'status': 503,
                            }),
                        ])
                    ]
                })
            ]),
            ('default', {}, [to_upstream_modules])
        ])
    ]

    result = [
        (Modules.Main, {
            'workers': 3,
            'maxconn': 7000,
            'port': 15160,
            'admin_port': 15160,
            'config_tag': options['config_tag'],
            'enable_reuse_port': True,
            'logs_path': '/place/db/www/logs',
        }),
        (Modules.Ipdispatch, [
            (Macroses.BindAndListen, {
                'port': 15160,
                'outerport': 17140,
                'domain': 'addrs.yandex.ru',
                'modules': submodules
            })
        ])
    ]

    return result


def process(options):
    params = options.params
    output_file = options.output_file
    transport = options.transport
    config_tag = transport.describe_gencfg_version()
    prjs = ('sas', 'man', 'vla')
    assert len(params) == 1 and params[0] in prjs, "Wrong parameters"

    addrs_backends = OrderedDict([
        ('sas', ['SAS_ADDRS_NMETA_NEW']),
        ('man', ['MAN_ADDRS_NMETA_NEW']),
        ('vla', ['VLA_ADDRS_NMETA_NEW'])
    ])

    cfg = templateAddrsReport({
        'addrs_backends': addrs_backends,
        'attempts': 2 if params[0] in ('vla', 'sas') else 5,
        'config_tag': config_tag,
        'geo': params[0]
    })

    tun_prefix = Constants.GetIpByIproute
    generate(cfg, output_file, instance_db_transport=transport, user_config_prefix=tun_prefix)

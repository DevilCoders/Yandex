#!/skynet/python/bin/python
# -*- coding: utf-8 -*-
from collections import OrderedDict

import src.constants as Constants
import src.macroses as Macroses
import src.modules as Modules
import src.utils as Utils
from .optional_sni import get_optional_sni
from .redirects import gen_redirects
from src.generator import generate
from src.lua_globals import LuaFuncCall, LuaGlobal, LuaAnonymousKey
from src.optionschecker import OptionsChecker


class AnyRedirects(Macroses.IMacro):
    @staticmethod
    @OptionsChecker([
        ('name', None, str, True),
        ('port', 0, int, True),
        ('any_backends', None, OrderedDict, True),
        ('any_weight_matrix', None, OrderedDict, True),
        ('antirobot_backends', None, list, True),
        ('ssl_enabled', False, bool, False),
    ])
    def generate(options):
        custom_backends = []

        for dc_name, dc_backends in options['any_backends'].iteritems():
            backend = [
                (Modules.Report, {
                    'disable_robotness': True,
                    'disable_sslness': True,
                    'uuid': 'any_requests_to_' + dc_name,
                }),
                (Modules.Balancer2, {
                    'backends': dc_backends,
                    'check_backends': OrderedDict([
                        ('name', Modules.gen_check_backends_name(dc_backends)),
                        ('quorum', 0.35),
                    ]),
                    'attempts': 1,
                    'connection_attempts': 5,
                    'rate_limiter_coeff': 0.99,
                    'attempts_limit': 0.1,
                    'balancer_type': 'rr',
                    'policies': OrderedDict([
                        ('unique_policy', {}),
                    ]),
                    'proxy_options': OrderedDict([
                        ('connect_timeout', '0.05s'),
                        ('backend_timeout', '1s'),
                    ]),
                }),
            ]

            custom_backends.append((options['any_weight_matrix'][dc_name], 'any_' + dc_name, backend))

        regexp_default = [
            (Macroses.WebHeaders, {
                'X-Forwarded-For-Y_weak': True,
                'X-Yandex-HTTPS': True if options['ssl_enabled'] else False,
                'X-Antirobot-Service-Y': 'any',
                'log_reqid': True,
                'log_yandexuid': True,
                'log_set_cookie_meta': True,
            }),
            (Modules.H100, {}),
            (Modules.Antirobot, {
                'backends': options['antirobot_backends'],
                'proxy_options': OrderedDict([
                    ('keepalive_count', 1),
                    ('keepalive_timeout', '60s')
                ]),
            }),
            (Modules.Redirects, {
                'actions': gen_redirects()
            }),
            (Modules.CookiePolicy, {
                'uuid': 'service_total',
                'default_yandex_policies': 'unstable'
            }),
            # start code
            (Modules.Balancer2, {
                'balancer_options': OrderedDict([
                    ('weights_file', LuaGlobal('WeightsDir', './controls/') + '/any.weights'),
                ]),
                'balancer_type': 'rr',
                'attempts': 3,
                'policies': OrderedDict([(
                    'unique_policy', {},
                )]),
                'custom_backends': custom_backends,
            }),
        ]

        optional_sni = get_optional_sni()

        modules = [
            (Macroses.ExtendedHttp, {
                'port': options['port'],
                'ssl_enabled': options['ssl_enabled'],
                'report_enabled': True,
                'report_uuid': 'service_total,http%s' % ('s' if options['ssl_enabled'] else ''),
                'report_robotness': True,
                'report_sslness': False,
                'ban_requests_file': './controls/ban_requests',
                'enable_cycles_protection': True,
                'max_cycles': 8,
                'cycles_header_len_alert': 4096,
                'disable_cycles_protection_file': './controls/disable_cycles_protection',
                'ssl_cert': LuaGlobal(
                    'public_any_custom_other_rsa',
                    '/dev/shm/certs/allCAs-any_custom_other_rsa.pem'
                ),
                'ssl_priv': LuaGlobal(
                    'private_any_custom_other_rsa',
                    '/dev/shm/certs/priv/any_custom_other_rsa.pem'
                ),
                'ssllib': 'openssl_extended_sni',
                'openssl_ciphers': Constants.SSL_CIPHERS_SUITES_SHA2,
                'ticket_keys_list': OrderedDict([
                    ('tls_1stkey', OrderedDict([
                        ('keyfile', '/dev/shm/certs/priv/1st.yandex_sha2ecc.key'),
                        ('priority', 1000)
                    ])),
                    ('tls_2ndkey', OrderedDict([
                        ('keyfile', '/dev/shm/certs/priv/2nd.yandex_sha2ecc.key'),
                        ('priority', 999)
                    ])),
                    ('tls_3rdkey', OrderedDict([
                        ('keyfile', '/dev/shm/certs/priv/3rd.yandex_sha2ecc.key'),
                        ('priority', 998)
                    ])),
                ]),
                'events': OrderedDict([
                    ('reload_ticket_keys', 'reload_ticket'),
                    ('force_reload_ticket_keys', 'force_reload_ticket'),
                ]),
                'sni_contexts': OrderedDict(optional_sni),
                'timeout': '100800s'  # 28 hours
            }),
            (
                Modules.RegexpPath,
                Macroses.MacroCheckReply().generate({'uri': '/ok.any.html', 'zero_weight_at_shutdown': True}) + [('default', {}, regexp_default)]
            ),
        ]

        local_name = 'localips_%s' % ('ssl' if options['ssl_enabled'] else '')
        # end
        result = [
            (
                options['name'],
                {
                    'ips': Utils.get_ip_from_rt('any.yandex.ru'),
                    'name': options['name'],
                    'port': 443 if options['ssl_enabled'] else 80,
                    'stats_attr': 'http%s' % ('s' if options['ssl_enabled'] else ''),
                    'slb': 'any.yandex.ru',  # service name
                    'disabled': LuaGlobal('SkipBind', False),
                },
                modules
            ),
            (
                local_name,
                {
                    'ips': [
                        LuaFuncCall('GetIpByIproute', {'key': 'v4'}),
                        LuaFuncCall('GetIpByIproute', {'key': 'v6'})
                    ],
                    'name': local_name,
                    'port': options['port'],
                    'stats_attr': 'http%s' % ('s' if options['ssl_enabled'] else ''),
                },
                modules
            )
        ]

        return result


def process(options):
    params = options.params
    output_file = options.output_file
    transport = options.transport
    assert len(params) == 1
    assert params[0] in ['man', 'sas', 'vla'], "Wrong parameters"
    dc = params[0]
    port = 5080
    unistat_port = 5082

    any_backends = OrderedDict([
        ('sas', [
            OrderedDict([
                ('cluster_name', 'sas'),
                ('endpoint_set_id', 'stable-morda-any-sas-yp'),
            ])
        ]),
        ('man', [
            OrderedDict([
                ('cluster_name', 'man'),
                ('endpoint_set_id', 'stable-morda-any-man-yp'),
            ])
        ]),
        ('vla', [
            OrderedDict([
                ('cluster_name', 'vla'),
                ('endpoint_set_id', 'stable-morda-any-vla-yp'),
            ])
        ]),
    ])

    any_weight_matrix = OrderedDict([
        ('man', -1),
        ('sas', 1),
        ('vla', 1),
    ])

    antirobot_backends = {
        'man':
            [OrderedDict([
                ('cluster_name', 'man'),
                ('endpoint_set_id', 'prod-antirobot-yp-man'),
            ]),
            OrderedDict([
                ('cluster_name', 'man'),
                ('endpoint_set_id', 'prod-antirobot-yp-prestable-man'),
            ])],
        'sas':
            [OrderedDict([
                ('cluster_name', 'sas'),
                ('endpoint_set_id', 'prod-antirobot-yp-sas'),
            ]),
            OrderedDict([
                ('cluster_name', 'sas'),
                ('endpoint_set_id', 'prod-antirobot-yp-prestable-sas'),
            ])],
        'vla':
            [OrderedDict([
                ('cluster_name', 'vla'),
                ('endpoint_set_id', 'prod-antirobot-yp-vla'),
            ]),
            OrderedDict([
                ('cluster_name', 'vla'),
                ('endpoint_set_id', 'prod-antirobot-yp-prestable-vla'),
            ])],
    }

    unistat_addrs = OrderedDict((
        (LuaAnonymousKey(), OrderedDict([('ip', '127.0.0.1'), ('port', unistat_port)])),
        (LuaAnonymousKey(), OrderedDict([('ip', '::1'), ('port', unistat_port)])),
        (LuaAnonymousKey(), OrderedDict([('ip', LuaFuncCall('GetIpByIproute', {'key': 'v4'})), ('port', unistat_port)])),
        (LuaAnonymousKey(), OrderedDict([('ip', LuaFuncCall('GetIpByIproute', {'key': 'v6'})), ('port', unistat_port)])),
    ))

    cfg = [
        (Modules.Main, {
            'workers': 2,
            'maxconn': 10000,
            'port': port,
            'admin_port': port,
            'config_tag': transport.describe_gencfg_version(),
            'dns_async_resolve': LuaGlobal('dns_async_resolve', True),
            'sd_client_name': 'balancer_any.yandex.ru',
            'unistat': OrderedDict([
                ('addrs', LuaGlobal('instance_unistat_addrs', unistat_addrs)),
            ]),
        }),
        (Modules.Ipdispatch, [
            (AnyRedirects, {
                'name': 'remote',
                'port': port,
                'any_backends': any_backends,
                'any_weight_matrix': any_weight_matrix,
                'antirobot_backends': antirobot_backends[dc],
            }),
            (AnyRedirects, {
                'name': 'remote_ssl',
                'port': port + 1,
                'any_backends': any_backends,
                'any_weight_matrix': any_weight_matrix,
                'antirobot_backends': antirobot_backends[dc],
                'ssl_enabled': True,
            }),
        ]),
    ]

    tun_prefix = Constants.GetIpByIproute
    generate(cfg, output_file, instance_db_transport=transport, user_config_prefix=tun_prefix)

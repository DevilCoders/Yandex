#!/skynet/python/bin/python
# -*- coding: utf8 -*-
from collections import OrderedDict
import src.constants as Constants
import src.macroses as Macroses
import src.modules as Modules
from src.generator import generate
from src.optionschecker import OptionsChecker, Helper
from src.lua_globals import LuaGlobal, LuaAnonymousKey
import sections


class GuiSubModule(Macroses.IMacro):
    @staticmethod
    @OptionsChecker([
        ('port', None, int, True),
        ('ssl_enabled', False, bool, True),
    ])
    def generate(options):
        https_postfix = 's' if options['ssl_enabled'] else ''

        modules = [
            (Macroses.ExtendedHttpV2, {
                'port': options['port'],
                'report_uuid': 'http%s' % https_postfix,
                'report_refers': 'service_total',
                'ssl_enabled': options['ssl_enabled'],
                'maxlen': 1024 * 1024,
                'maxreq': 1024 * 1024,
                'force_ssl': True,
                'fqdn': 'gencfg.yandex-team.ru',
                'ticket_keys_enabled': True,
            }),
            (Modules.Headers, {
                'create_func_weak': OrderedDict([
                    ('X-Forwarded-For', 'realip'),
                ]),
            }),
            (Modules.Hasher, {
                'mode': 'subnet',
                'subnet_v4_mask': 32,
                'subnet_v6_mask': 128,
            }),
            (Modules.Regexp, (
                sections.other.get_modules(https_postfix)
                + sections.clusterstate.get_modules()
                + sections.gencfg.get_modules(https_postfix)
                + sections.callisto.get_modules()
            ) + [
                ('default', {}, [
                    (Modules.ErrorDocument, {
                        'status': 404,
                        'content': '404. Unknown host'
                    }),
                ]),
            ]),
        ]

        return [
            (Macroses.BindAndListen, {
                'port': options['port'],
                'outerport': 443 if options['ssl_enabled'] else 80,
                'stats_attr': 'http%s' % https_postfix,
                'domain': 'gencfg.yandex-team.ru',
                'modules': modules
            })
        ]


class StatsStorage(Macroses.IMacro):

    def __init__(self):
        Macroses.IMacro.__init__(self)

    @staticmethod
    @Helper(
        'Macroses.StatsStorage',
        '''Секция ipdispatch для хранения статистики от других секций''',
        [
            ('port', None, int, True, 'Порт инстанса'),
        ]
    )
    def generate(options):
        return [(
            'stats_storage',
            {
                'ip': '127.0.0.4',
                'port': options['port'],
                'stats_attr': 'stats_storage',
                'slb': 'stats_storage'
            },
            [
                (Modules.Report, {
                    'uuid': 'service_total',
                    'just_storage': True,
                    'disable_robotness': True,
                    'disable_sslness': True,
                }),
                (Modules.Http, {}),
                (Modules.ErrorDocument, {
                    'status': 204,
                }),
            ]
        )]


def process(options):
    output_file = options.output_file
    transport = options.transport

    port = 7020
    unistat_port = port + 2

    unistat_addrs = OrderedDict((
        (LuaAnonymousKey(), OrderedDict([('port', unistat_port)])),
    ))

    cfg = [
        (Modules.Main, {
            'workers': 2,
            'maxconn': 1000,
            'port': port,
            'admin_port': port,
            'config_tag': transport.describe_gencfg_version(),
            'enable_reuse_port': True,
            'logs_path': '/place/db/www/logs',
            'sd_client_name': 'gencfg-lb.yandex-team.ru',
            'unistat': OrderedDict([
                ('addrs', LuaGlobal('instance_unistat_addrs', unistat_addrs)),
            ]),
        }),
        (Modules.Ipdispatch, [
            (StatsStorage, {'port': port}),
            (GuiSubModule, {
                'port': port,
                'ssl_enabled': False,
            }),
            (GuiSubModule, {
                'port': port + 1,
                'ssl_enabled': True,
            })
        ])
    ]

    man_prefix = Constants.GetIpByIproute
    generate(cfg, output_file, instance_db_transport=transport, user_config_prefix=man_prefix)

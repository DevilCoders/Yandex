#!/skynet/python/bin/python
# -*- coding: utf-8 -*-
import itertools

from collections import OrderedDict

import src.constants as Constants
import src.macroses as Macroses
import src.modules as Modules

from src.lua_globals import LuaGlobal


class ProxiesL7Macro(Macroses.IMacro):
    def __init__(self):
        pass

    @staticmethod
    @Macroses.Helper(
        'Macroses.PassportInRegexp',
        '''
            Обрабатывает запросы на passport.y.com.tr в поиске.
            Должен находиться прямо внутри Modules.Regexp.
        ''',
        [
            ('backends', None, list, True, 'Список backend-ов suggest-а.'),
            ('host_match', None, str, True, 'Регексп по хосту'),
            ('service_name', None, str, True, 'Имя сервиса'),
            ('https_backend', False, bool, False, 'Ходить в https бекенды'),
            ('backend_timeout', '20s', str, False, 'Backend timeout'),
            ('root_ca', LuaGlobal('passport_backend_cert', './RootPassPort.pem'), str, False, 'Путь до RootCa'),
            ('ciphers', Constants.SSL_CIPHERS_SUITES, str, False, 'Шифры'),
            ('websockets', False, bool, False, 'Разрешены ли вебсокеты'),
            ('backend_read_timeout', '15s', str, False, None),
            ('backend_write_timeout', '2s', str, False, None),
            ('client_read_timeout', '15s', str, False, None),
            ('client_write_timeout', '5s', str, False, None),
        ]
    )
    def generate(options):
        threshold = not options['websockets']
        modules = [
            (Modules.Report, {
                'uuid': options['service_name'],
                'disable_robotness': True,
                'disable_sslness': True,
            }),
            (Modules.Meta, {
                'id': 'upstream-info',
                'fields': OrderedDict([
                    ('upstream', options['service_name']),
                ]),
            }),
        ]
        if threshold:
            modules.append(
                (Modules.Threshold, {
                    'lo_bytes': 1024 * 1024,
                    'hi_bytes': 4 * 1024 * 1024,
                    'pass_timeout': '15s',
                    'recv_timeout': '1s',
                })
            )
        modules.append((Modules.Headers, {'create': OrderedDict([('Y-Service', options['service_name'])])}))

        proxy_options = [
            ('need_resolve', False),
            ('fail_on_5xx', False),
            ('backend_timeout', options['backend_timeout']),
        ]
        if options['websockets']:
            proxy_options += [
                ('allow_connection_upgrade', True),
            ]
            ws_timeout_opts = [
                '{}_{}_timeout'.format(a, b)
                for a, b in itertools.product(
                    ['client', 'backend'], ['read', 'write'])
            ]
            proxy_options += [
                (opt, options[opt])
                for opt in ws_timeout_opts
            ]
        if options['https_backend']:
            proxy_options += [
                ('https_settings', OrderedDict([
                    ('ciphers', options['ciphers']),
                    ('ca_file', options['root_ca']),
                    ('sni_on', False),
                    ('verify_depth', 3),
                ]))
            ]

        proxy_options = OrderedDict(proxy_options)

        if not options['https_backend']:
            modules.append((Modules.Balancer2, {
                'backends': options['backends'],
                'resolve_protocols': [6],
                'balancer_type': 'rr',
                'attempts': 2,
                'policies': OrderedDict([
                    ('unique_policy', {})
                ]),
                'proxy_options': proxy_options,
            }))
        else:
            modules.append((Modules.Regexp, [
                ('to_upstream', {'match_fsm': OrderedDict([('header', {'name': 'X-Yandex-HTTPS', 'value': 'yes'})])}, [
                    (Modules.Balancer2, {
                        'backends': options['backends'],
                        'resolve_protocols': [6],
                        'balancer_type': 'rr',
                        'attempts': 2,
                        'proxy_options': proxy_options,
                        'policies': OrderedDict([('unique_policy', {})]),
                    })
                ]),
                ('default', {}, [
                    (Modules.ErrorDocument, {
                        'status': 403,
                        'force_conn_close': True,
                    }),
                ])
            ]))

        return [
            ('upstream_' + options['service_name'], {'match_fsm': OrderedDict([('host', options['host_match'])])}, modules)]


def template_proxies(config_data):
    return [
        (ProxiesL7Macro, {
            'service_name': 'pass_yandex_tld',
            'backends': ['pass.yandex.ua:443'],
            'host_match': 'pass\\\\.yandex\\\\.[\\\\w.]+',
            'https_backend': True,
        }),
        (ProxiesL7Macro, {
            'service_name': 'passport_yandex_tld',
            'backends': ['passport.yandex.ua:443'],
            'host_match': '(sso\\\\.)?passport\\\\.yandex\\\\.[\\\\w.]+|sso\\\\.kinopoisk\\\\.ru',
            'https_backend': True,
        }),
        (ProxiesL7Macro, {
            'service_name': 'social_yandex_tld',
            'backends': ['social.yandex.ua:443'],
            'host_match': 'social\\\\.yandex\\\\.[\\\\w.]+',
            'root_ca': LuaGlobal('proxies_root_ca', './Certum_Root_CA.crt'),
            'ciphers': Constants.SSL_CIPHERS_SUITES_SHA2,
            'https_backend': True,
        }),
        (ProxiesL7Macro, {
            'service_name': 'oauth_yandex_tld',
            'backends': ['oauth.yandex.ua:443'],
            'host_match': '(oauth|login)\\\\.yandex\\\\.[\\\\w.]+',
            'root_ca': LuaGlobal('proxies_root_ca', './Certum_Root_CA.crt'),
            'ciphers': Constants.SSL_CIPHERS_SUITES_SHA2,
            'https_backend': True,
            # 'https_backend': True,
        }),
        # (ProxiesL7Macro, {
        #    'service_name': 'mc_yandex_tld',
        #    'backends': ['mc.yandex.ru:80'],
        #    'host_match': 'mc\\\\.yandex\\\\.(ru|com|ua)',
        # }),
        # (ProxiesL7Macro, {
        #    'service_name': 'an_yandex_tld',
        #    'backends': ['an.yandex.ru:80'],
        #    'host_match': '(bs|an)\\\\.yandex\\\\.ru',
        # }),
        (ProxiesL7Macro, {
            'service_name': 'brozen_yandex_tld',
            'backends': ['brozen.yandex.ru:80'],
            'host_match': 'brozen\\\\.yandex\\\\.ru',
        }),
        (ProxiesL7Macro, {
            'service_name': 'zen_yandex_ru',
            'backends': ['zen.yandex.ru:80'],
            'host_match': 'zen\\\\.yandex\\\\.[\\\\w.]+',
        }),
        (ProxiesL7Macro, {
            'service_name': 'support_yandex_ru',
            'backends': ['support.yandex.ru:80'],
            'host_match': 'support\\\\.yandex\\\\.[\\\\w.]+',
        }),
        (ProxiesL7Macro, {
            'service_name': 'mobile_yandex_net',
            'backends': ['mobile.yandex.net:80'],
            'host_match': 'mobile\\\\.yandex\\\\.net',
        }),
        (ProxiesL7Macro, {
            'service_name': 'redirect_appmetrica_ru_com',
            'backends': ['redirect.appmetrica.yandex.com:80'],
            'host_match': '(.*\\\\.)?redirect\\\\.appmetrica\\\\.yandex\\\\.[\\\\w.]+',
        }),
        (ProxiesL7Macro, {
            'service_name': 'rosenberg_appmetrica_yandex_net',
            'backends': ['rosenberg.appmetrica.yandex.net:80'],
            'host_match': 'rosenberg\\\\.appmetrica\\\\.yandex\\\\.net',
        }),
        (ProxiesL7Macro, {
            'service_name': 'report_appmetrica_yandex_net',
            'backends': ['report.appmetrica.yandex.net:443'],
            'host_match': 'report\\\\.appmetrica\\\\.yandex\\\\.net',
            'root_ca': LuaGlobal('proxies_root_ca', './Certum_Root_CA.crt'),
            'ciphers': Constants.SSL_CIPHERS_SUITES_SHA2,
            'https_backend': True,
        }),
        (ProxiesL7Macro, {
            'service_name': 'redirect_appmetrica_webvisor_com',
            'backends': ['redirect.appmetrica.yandex.com:80'],
            'host_match': 'redirect\\\\.appmetrica\\\\.webvisor\\\\.com',
        }),
        (ProxiesL7Macro, {
            'service_name': 'report_appmetrica_webvisor_com',
            'backends': ['report.appmetrica.webvisor.com:80'],
            'host_match': 'report\\\\.appmetrica\\\\.webvisor\\\\.com',
        }),
        (ProxiesL7Macro, {
            'service_name': 'report_n_appmetrica_webvisor_com',
            'backends': ['report.appmetrica.webvisor.com:80'],
            'host_match': 'report-(1|2)\\\\.appmetrica\\\\.webvisor\\\\.com',
        }),
        (ProxiesL7Macro, {
            'service_name': 'rosenberg_appmetrica_webvisor_com',
            'backends': ['rosenberg.appmetrica.webvisor.com:80'],
            'host_match': 'rosenberg\\\\.appmetrica\\\\.webvisor\\\\.com',
        }),
        (ProxiesL7Macro, {
            'service_name': 'api_browser_yandex_tld',
            'backends': ['api.browser.yandex.ru:80'],
            'host_match': '(.*\\\\.)?browser\\\\.yandex\\\\.[\\\\w.]+',
        }),
        (ProxiesL7Macro, {
            'service_name': 'soft_export_yandex_net',
            'backends': ['api.browser.yandex.ru:80'],
            'host_match': 'soft\\\\.export\\\\.yandex\\\\.[\\\\w.]+',
        }),
        (ProxiesL7Macro, {
            'service_name': 'mc_webvisor_com',
            'backends': ['mc.webvisor.com:80'],
            'host_match': 'mc\\\\.webvisor\\\\.com',
        }),
        (ProxiesL7Macro, {
            'service_name': 'dr_yandex_net',
            'backends': ['dr.yandex.net:80'],
            'host_match': '(dr|dr2)\\\\.yandex\\\\.net',
        }),
        (ProxiesL7Macro, {
            'service_name': 'favicon_yandex_net',
            'backends': ['favicon.yandex.net:80'],
            'host_match': 'favicon\\\\.yandex\\\\.net',
        }),
        (ProxiesL7Macro, {
            'service_name': 'p_ya_ru',
            'backends': ['p.ya.ru:80'],
            'host_match': 'p\\\\.ya\\\\.ru',
        }),
        (ProxiesL7Macro, {
            'service_name': 'news_yandex_tld',
            'backends': ['news-internal.yandex.ru:80'],
            'host_match': '(news|news-clck|m\\\\.news)\\\\.yandex\\\\.(ru|ua|fr)',
        }),
        (ProxiesL7Macro, {
            'service_name': 'avatars_mds_yandex_net',
            'backends': ['avatars.mds.yandex.net:80'],
            'host_match': 'avatars\\\\.mds\\\\.yandex\\\\.net',
        }),
        (ProxiesL7Macro, {
            'service_name': 'weather_yandex_ru',
            'backends': ['api.weather.yandex.ru:80'],
            'host_match': '(api|info)\\\\.weather\\\\.yandex\\\.(ru|net)',
        }),
        (ProxiesL7Macro, {
            'service_name': 'api_sport_yandex_ru',
            'backends': ['api.sport.yandex.ru:80'],
            'host_match': 'api\\\\.sport\\\\.yandex\\\\.ru',
        }),
        (ProxiesL7Macro, {
            'service_name': 'push_yandex_ua',
            'backends': ['push.yandex.ua:80'],
            'host_match': 'push\\\\.yandex\\\\.(ua|ru|com)',
            'websockets': True,
            'client_read_timeout': '8640000s',  # 100 days
            'backend_read_timeout': '65s',
        }),
        (ProxiesL7Macro, {
            'service_name': 'pdd_yandex_ua',
            'backends': ['pdd.yandex.ua:80'],
            'host_match': 'pdd\\\\.yandex\\\\.(ua|ru|com)',
        }),
        (ProxiesL7Macro, {
            'service_name': 'resize_yandex_net',
            'backends': ['resize.yandex.net:80'],
            'host_match': 'resize\\\\.yandex\\\\.net',
        }),
        (ProxiesL7Macro, {
            'service_name': 'launcher_cache_mobile_yandex_net',
            'backends': ['launcher-cache.mobile.yandex.net:80'],
            'host_match': 'launcher-cache\\\\.mobile\\\\.yandex\\\\.net',
        }),
        (ProxiesL7Macro, {
            'service_name': 'addappter_api_mobile_yandex_net',
            'backends': ['common-public.stable.qloud-b.yandex.net:80'],
            'host_match': 'addappter-api\\\\.mobile\\\\.yandex\\\\.net',
        }),
        (ProxiesL7Macro, {
            'service_name': 'updater_mobile_yandex_net',
            'backends': ['updater.mobile.yandex.net:80'],
            'host_match': 'updater\\\\.mobile\\\\.yandex\\\\.net',
        }),
        (ProxiesL7Macro, {
            'service_name': 'tv_yandex_ru',
            'backends': ['2a02:6b8::1:154@80'],
            'host_match': '(.*\\\\.)?tv\\\\.yandex\\\\.[\\\\w.]+',
        }),
        (ProxiesL7Macro, {
            'service_name': 'translate_yandex_net',
            'backends': ['translate.yandex.net:80'],
            'host_match': 'translate\\\\.yandex\\\\.net',
        }),
        (ProxiesL7Macro, {
            'service_name': 'dictionary_yandex_net',
            'backends': ['dictionary.yandex.net:80'],
            'host_match': 'dictionary\\\\.yandex\\\\.net',
        }),
        (ProxiesL7Macro, {
            'service_name': 'predictor_yandex_net',
            'backends': ['predictor.yandex.net:80'],
            'host_match': 'predictor\\\\.yandex\\\\.net',
        }),
        (ProxiesL7Macro, {
            'service_name': 'ext_captcha_yandex_net',
            'backends': ['ext.captcha.yandex.net:80'],
            'host_match': 'ext\\\\.captcha\\\\.yandex\\\\.net',
        }),
        (ProxiesL7Macro, {
            'service_name': 'z5h64q92x9_net',
            'backends': ['z5h64q92x9.net:80'],
            'host_match': 'z5h64q92x9\\\\.net',
        }),
    ]

#  service_name без точек, а uuid  сдохнет

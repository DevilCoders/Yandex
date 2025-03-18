#!/skynet/python/bin/python
# -*- coding: utf-8 -*-

# nonce

import os
import sys

sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', 'custom_generators', 'src', 'src')))

from src.transports import InstanceDbTransportHolder  # noqa
import src.modules as Modules  # noqa
import src.macroses as Macroses  # noqa
import src.constants as Constants  # noqa
import src.trusted_nets as TrustedNets  # noqa
from src.generator import generate  # noqa
from collections import OrderedDict  # noqa
from src.lua_globals import LuaAnonymousKey, LuaGlobal, LuaFuncCall  # noqa

import globaldata as gd  # noqa
from l7macro import is_knoss_balancer, is_term_balancer

ticket_key_list = OrderedDict([
    ('tls_1stkey', OrderedDict([('keyfile', '/dev/shm/certs/priv/1st.yandex_sha2ecc.key'), ('priority', 1000)])),
    ('tls_2ndkey', OrderedDict([('keyfile', '/dev/shm/certs/priv/2nd.yandex_sha2ecc.key'), ('priority', 999)])),
    ('tls_3rdkey', OrderedDict([('keyfile', '/dev/shm/certs/priv/3rd.yandex_sha2ecc.key'), ('priority', 998)])),
])


def _gen_host_regexp(host_re):
    return OrderedDict(
        match_fsm=OrderedDict(
            host="(.*[.])?" + host_re,
            case_insensitive=True
        )
    )


def _gen_path_regexp(path_re):
    return OrderedDict(
        match_fsm=OrderedDict(
            path=path_re,
        )
    )


def _gen_not_path(path_re):
    return OrderedDict(
        match_not=_gen_path_regexp(path_re)
    )


def _gen_host_and_not_path(host_re, path_re):
    return OrderedDict(
        match_and=OrderedDict([
            (LuaAnonymousKey(), _gen_host_regexp(host_re)),
            (LuaAnonymousKey(), _gen_not_path(path_re))
        ])
    )


def _gen_proto_matcher(proto):
    return OrderedDict(
        match_proto=OrderedDict(
            proto=proto
        )
    )


_SERVICE_MATCHER_MAP = OrderedDict([
    ('health_check', _gen_path_regexp(Constants.BALANCER_HEALTH_CHECK)),
    ('total', _gen_not_path(Constants.BALANCER_HEALTH_CHECK)),
    ('total_com', _gen_host_and_not_path('yandex[.]com', Constants.BALANCER_HEALTH_CHECK)),
    ('total_comtr', _gen_host_and_not_path('yandex[.]com[.]tr', Constants.BALANCER_HEALTH_CHECK)),
    ('total_ua', _gen_host_and_not_path('yandex[.]ua', Constants.BALANCER_HEALTH_CHECK)),
    ('total_ru', _gen_host_and_not_path('yandex[.](ru|by|kz)', Constants.BALANCER_HEALTH_CHECK)),
    ('total_yaru', _gen_host_and_not_path('ya[.]ru', Constants.BALANCER_HEALTH_CHECK)),
])

_DEFAULT_TERM_MATCHER_MAP = OrderedDict([
    ('com', _gen_host_regexp('yandex[.]com')),
    ('comtr', _gen_host_regexp('yandex[.]com[.]tr')),
    ('ua', _gen_host_regexp('yandex[.]ua')),
    ('ru', _gen_host_regexp('yandex[.](ru|by|kz)')),
    ('yaru', _gen_host_regexp('ya[.](ru)')),
    ('h1', _gen_proto_matcher("http1x")),
    ('h2', _gen_proto_matcher("http2")),
])


def build_modules(sect_list, config_data):
    result = list()
    for sect_func in sect_list:
        result.extend(sect_func(config_data))
    return result


def _is_yandex_tld_prod(config_data):
    return config_data.domain == gd.Domain.RKUB and config_data.conftype != gd.ConfigType.TUNNEL_TESTING and not config_data.mms


def _is_yandex_tld_only_prod(config_data):
    return config_data.domain == gd.Domain.RKUB and config_data.conftype != gd.ConfigType.TUNNEL_TESTING and config_data.mms


def _is_yaru_tld_prod(config_data):
    return config_data.domain == gd.Domain.L7_YARU and config_data.conftype != gd.ConfigType.TUNNEL_TESTING and not config_data.mms


def _generate_optional_tld_sni(tld, priority=999):
    return (
        'yandex_{0}'.format(tld),
        OrderedDict([
            ('ciphers', Constants.SSL_CIPHERS_SUITES_CHACHA_SHA2),
            ('cert', LuaGlobal(
                'yandex_{0}_public'.format(tld),
                '/dev/shm/certs/allCAs-yandex.{0}_sha2_rsa_certum.pem'.format(tld),
            )),
            ('priv', LuaGlobal(
                'yandex_{0}_private'.format(tld),
                '/dev/shm/certs/priv/yandex.{0}_sha2_rsa_certum.pem'.format(tld),
            )),
            ('servername', {
                'servername_regexp':
                    '([^.]+[.])?yandex[.]{0}'.format(tld),
                'case_insensitive': True,
            }),
            ('events', OrderedDict([
                ('reload_ticket_keys', 'reload_ticket'),
                ('force_reload_ticket_keys', 'force_reload_ticket'),
            ])),
            ('priority', priority),
            ('ticket_keys_list', ticket_key_list),
        ])
    )


def _gen_extended_http_params(port, ssl_enabled, ja3_enabled, http3, config_data):
    domain = config_data.domain

    ch_restore_enabled = False
    if _is_yandex_tld_prod(config_data)               \
            or _is_yandex_tld_only_prod(config_data)  \
            or _is_yaru_tld_prod(config_data):
        ch_restore_enabled = True

    extended_http_params = {
        'port': port,
        'maxlen': 64 * 1024,
        'maxreq': 64 * 1024,
        'ssl_enabled': ssl_enabled,
        'ja3_enabled': ja3_enabled,
        'secrets_log': 'Enabled',
        'ssllib': 'openssl_extended_sni',
        'openssl_ciphers': Constants.SSL_CIPHERS_SUITES_CHACHA_SHA2,
        'no_keepalive_file': './controls/keepalive_disabled',
        'timeout': '100800s',  # 28 hours
        'ticket_keys_list': ticket_key_list,
        'report_enabled': True,
        'ban_requests_file': './controls/ban_requests',
        # 'additional_ip_header': 'X-Forwarded-For-Y',  # empty in real life
        'allow_client_hints_restore': ch_restore_enabled,
        'client_hints_ua_header': 'Y-User-Agent',
        'enable_cycles_protection': True,
        'max_cycles': 8,
        'cycles_header_len_alert': 4096,
        'disable_cycles_protection_file': './controls/disable_cycles_protection',
        'events': OrderedDict([
            ('reload_ticket_keys', 'reload_ticket'),
            ('force_reload_ticket_keys', 'force_reload_ticket'),
            ('reload_ocsp_response', 'reload_ocsp')
        ]),
    }

    # all domains
    extended_http_params['secrets_log_freq_file'] = './controls/secrets_log_enable.ratefile'

    extended_http_params['ssl_cert'] = LuaGlobal(
        'yandex_public_cert',
        '/dev/shm/certs/allCAs-any.yandex.tld.ecc.pem',
    )
    extended_http_params['ssl_priv'] = LuaGlobal(
        'yandex_private_cert',
        '/dev/shm/certs/priv/any.yandex.tld.ecc.pem',
    )
    extended_http_params['ssl_cert_secondary'] = LuaGlobal(
        'yandex_secondary_cert_certum',
        '/dev/shm/certs/allCAs-any.yandex.tld.rsa.pem',
    )
    extended_http_params['ssl_priv_secondary'] = LuaGlobal(
        'yandex_secondary_priv_certum',
        '/dev/shm/certs/priv/any.yandex.tld.rsa.pem',
    )

    optional_tlds = []

    for idx, tld in enumerate(['eu', 'fi', 'pl']):
        optional_tlds.append(_generate_optional_tld_sni(tld, priority=999 - idx - 1))

    extended_http_params['sni_contexts'] = OrderedDict(optional_tlds)

    if domain == gd.Domain.FUKRAINE:
        extended_http_params['sni_contexts'] = OrderedDict([
            ('appmetrica_yandex_com', OrderedDict([
                ('cert', LuaGlobal(
                    'appmetrica_public_cert',
                    '/dev/shm/certs/allCAs-star.mobile.yandex.net.pem'
                )),
                ('priv', LuaGlobal(
                    'appmetrica_private_cert',
                    '/dev/shm/certs/priv/priv.star.mobile.yandex.net.pem'
                )),
                ('events', OrderedDict([
                    ('reload_ticket_keys', 'reload_ticket'),
                    ('force_reload_ticket_keys', 'force_reload_ticket'),
                ])),
                ('priority', 996),
                ('servername', {
                    'servername_regexp': '(.*\\\\.)?appmetrica\\\\.yandex\\\\.(com|ru|net)',
                    'case_insensitive': True,
                }),
                ('ticket_keys_list', ticket_key_list),
            ])),
            ('api_browser_yandex_ru', OrderedDict([
                ('cert', LuaGlobal(
                    'api_browser_yandex_ru_public_cert',
                    '/dev/shm/certs/allCAs-api.browser.yandex.ru.pem'
                )),
                ('priv', LuaGlobal(
                    'api_browser_yandex_ru_private_cert',
                    '/dev/shm/certs/priv/priv-api.browser.yandex.ru.pem'
                )),
                ('events', OrderedDict([
                    ('reload_ticket_keys', 'reload_ticket'),
                    ('force_reload_ticket_keys', 'force_reload_ticket'),
                ])),
                ('priority', 995),
                ('servername', {
                    'servername_regexp': '(.*\\\\.)?browser\\\\.yandex\\\\.(ru|ua|kz|by|com|com.tr|net)',
                    'case_insensitive': True,
                }),
                ('ticket_keys_list', ticket_key_list),
            ])),
            ('adfox_yandex_ru', OrderedDict([
                ('cert', LuaGlobal(
                    'adfox_yandex_ru_public_cert',
                    '/dev/shm/certs/allCAs-ads.adfox.ru.pem'
                )),
                ('priv', LuaGlobal(
                    'adfox_yandex_ru_private_cert',
                    '/dev/shm/certs/priv/priv-ads.adfox.ru.pem'
                )),
                ('events', OrderedDict([
                    ('reload_ticket_keys', 'reload_ticket'),
                    ('force_reload_ticket_keys', 'force_reload_ticket'),
                ])),
                ('priority', 994),
                ('servername', {
                    'servername_regexp': '(content|login|ads)\\\\.adfox\\\\.ru|matchid\\\\.adfox\\\\.yandex\\\\.ru',
                    'case_insensitive': True,
                }),
                ('ticket_keys_list', ticket_key_list),
            ])),
            ('webvisor_com', OrderedDict([
                ('cert', LuaGlobal(
                    'webwisor_com_public_cert',
                    '/dev/shm/certs/allCAs-appmetrica.webvisor.com.pem'
                )),
                ('priv', LuaGlobal(
                    'webwisor_com_private_cert',
                    '/dev/shm/certs/priv/priv-appmetrica.webvisor.com.pem'
                )),
                ('events', OrderedDict([
                    ('reload_ticket_keys', 'reload_ticket'),
                    ('force_reload_ticket_keys', 'force_reload_ticket'),
                ])),
                ('priority', 993),
                ('servername', {
                    'servername_regexp': '(.*)\\\\.webvisor\\\\.com',
                    'case_insensitive': True,
                }),
                ('ticket_keys_list', ticket_key_list),
            ])),
            ('news_avatars_icons_pogoda', OrderedDict([
                ('cert', LuaGlobal(
                    'news_avatars_icons_pogoda_public_cert',
                    '/dev/shm/certs/allCAs-api.weather.yandex.ru.pem'
                )),
                ('priv', LuaGlobal(
                    'news_avatars_icons_pogoda_private_cert',
                    '/dev/shm/certs/priv/priv-api.weather.yandex.ru.pem'
                )),
                ('events', OrderedDict([
                    ('reload_ticket_keys', 'reload_ticket'),
                    ('force_reload_ticket_keys', 'force_reload_ticket'),
                ])),
                ('priority', 990),
                ('servername', {
                    'servername_regexp': (
                        '(favicon|info\\\\.weather|avatars\\\\.mds)\\\\.yandex.net|p\\\\.ya\\\\.ru|'
                        'api\\\\.sport\\\\.yandex\\\\.ru|api\\\\.weather\\\\.yandex\\\\.ru|'
                        '(news|news-clck)\\\\.yandex\\\\.fr|'
                        'm\\\\.news\\\\.yandex\\\\.(ua|ru|fr)'
                    ),
                    'case_insensitive': True,
                }),
                ('ticket_keys_list', ticket_key_list),
            ])),
            ('launcher_cache_mobile_yandex_net', OrderedDict([
                ('cert', LuaGlobal(
                    'launcher_cache_mobile_yandex_net_public_cert',
                    '/dev/shm/certs/allCAs-star.mobile.yandex.net.pem'
                )),
                ('priv', LuaGlobal(
                    'launcher_cache_mobile_yandex_net_private_cert',
                    '/dev/shm/certs/priv/priv.star.mobile.yandex.net.pem'
                )),
                ('events', OrderedDict([
                    ('reload_ticket_keys', 'reload_ticket'),
                    ('force_reload_ticket_keys', 'force_reload_ticket'),
                ])),
                ('priority', 989),
                ('servername', {
                    'servername_regexp': 'launcher-cache\\\\.mobile\\\\.yandex\\\\.net',
                    'case_insensitive': True,
                }),
                ('ticket_keys_list', ticket_key_list),
            ])),
            ('tv_yandex_ru_pem', OrderedDict([
                ('cert', LuaGlobal(
                    'tv_yandex_ru_pem_public_cert',
                    '/dev/shm/certs/allCAs-tv.yandex.ru.pem'
                )),
                ('priv', LuaGlobal(
                    'tv_yandex_ru_pem_private_cert',
                    '/dev/shm/certs/priv/priv-tv.yandex.ru.pem'
                )),
                ('events', OrderedDict([
                    ('reload_ticket_keys', 'reload_ticket'),
                    ('force_reload_ticket_keys', 'force_reload_ticket'),
                ])),
                ('priority', 988),
                ('servername', {
                    'servername_regexp': '(.*\\\\.)?tv\\\\.yandex\\\\.(ru|ua|by|kz)',
                    'case_insensitive': True,
                }),
                ('ticket_keys_list', ticket_key_list),
            ])),
            ('sso_passport_yandex_ru_pem', OrderedDict([
                ('cert', LuaGlobal(
                    'sso_passport_yandex_ru_pem_public_cert',
                    '/dev/shm/certs/allCAs-sso.passport.yandex.ru.pem'
                )),
                ('priv', LuaGlobal(
                    'sso_passport_yandex_ru_pem_private_cert',
                    '/dev/shm/certs/priv/priv-sso.passport.yandex.ru.pem'
                )),
                ('events', OrderedDict([
                    ('reload_ticket_keys', 'reload_ticket'),
                    ('force_reload_ticket_keys', 'force_reload_ticket'),
                ])),
                ('priority', 987),
                ('servername', {
                    'servername_regexp': 'sso\\\\.passport\\\\.yandex\\\\.(ua|ru|kz|by)|sso\\\\.kinopoisk\\\\.ru',
                    'case_insensitive': True,
                }),
                ('ticket_keys_list', ticket_key_list),
            ])),
        ])

    if is_term_balancer(config_data):
        extended_http_params['http2_alpn_file'] = './controls/http2_enable.ratefile'
        extended_http_params['http2_alpn_rand_mode'] = 'ip_hash'
        extended_http_params['http2_alpn_freq'] = 0.0
        if domain == gd.Domain.RKUB:
            extended_http_params['http2_refused_stream_file_switch'] = './controls/http2_refused_stream_off'

    if config_data.mms:
        extended_http_params['http2_alpn_freq'] = 1.0

    return extended_http_params


def generate_section(config_data, port, modules, ssl_enabled=False, familysearch=False, http3=False):
    if port is None:
        return []

    extended_http_params = _gen_extended_http_params(
        port,
        ssl_enabled=ssl_enabled,
        ja3_enabled=True,
        http3=http3,
        config_data=config_data
    )

    submodules = [(Modules.H100, {})]

    # if ssl_enabled and config_data.hsts:  # HSTS for COM enabled here
    if config_data.hsts:  # HSTS for COM enabled here
        submodules.append(
            (Modules.ResponseHeadersIf, {
                'if_has_header': 'X-Yandex-STS-Plus',
                'create_header': OrderedDict([
                    ('Strict-Transport-Security', 'max-age=31536000; includeSubDomains'),  # 1 year with sub-domains
                ]),
                'erase_if_has_header': True  # erase comes first and then comes create
            })
        )

    if config_data.conftype in [gd.ConfigType.TUNNEL_TESTING, gd.ConfigType.L7_YARU_TESTING]:
        extended_http_params['http2_alpn_file'] = './controls/http2_enable.ratefile'
        extended_http_params['http2_alpn_freq'] = 0.0
        extended_http_params['http2_alpn_rand_mode'] = 'ip_hash'

        extended_http_params['sni_contexts'] = OrderedDict([
            _generate_optional_tld_sni(tld, priority=999 + idx + 1)
            for idx, tld in enumerate(['eu', 'fi', 'pl'])
        ] + [
            ('wildcard_l7test_yandex_ru', OrderedDict([
                ('cert', LuaGlobal(
                    'wildcard_l7test_yandex_ru_public',
                    '/dev/shm/certs/allCAs-l7test.yandex.ru.pem',
                )),
                ('priv', LuaGlobal(
                    'wildcard_l7test_yandex_ru_private',
                    '/dev/shm/certs/priv/l7test.yandex.ru.pem',
                )),
                ('servername', {
                    'servername_regexp': (
                        '([^.]+)[.]l7test[.](yandex[.](.+)|ya[.]ru|xn--d1acpjx3f[.]xn--p1ai)'
                    ),
                    'case_insensitive': True,
                }),
                ('events', OrderedDict([
                    ('reload_ticket_keys', 'reload_ticket'),
                    ('force_reload_ticket_keys', 'force_reload_ticket'),
                ])),
                ('priority', 999),
                ('ticket_keys_list', ticket_key_list),
                ('secrets_log', LuaFuncCall('GetPathToSecretLog', {})),
            ])),
        ])

    inner_modules = []

    if not is_knoss_balancer(config_data):
        inner_modules.append((Modules.Icookie, {
            'use_default_keys': True,
            'domains': config_data.icookie_domains,
            'trust_children': True,
            'force_equal_to_yandexuid': True,
            'enable_parse_searchapp_uuid': True,
            'enable_guess_searchapp': True,
            'force_generate_from_searchapp_uuid': True,
            'encrypted_header': 'X-Yandex-ICookie-Encrypted',
            'exp_salt': 'fd70bae9980eaa08e8c6fb66c2f60d26',
            'exp_A_testid': 447804,
            'exp_B_testid': 447805,
            'exp_A_slots': 3,
            'exp_B_slots': 4,
        }))

    if http3:
        modules = list(modules)
        for i in xrange(len(modules)):
            if modules[i][0] == Macroses.MacroCheckReply:
                modules[i][1]['zero_weight_at_shutdown'] = True
                modules[i][1]['force_conn_close'] = False
                modules[i][1]['push_checker_address'] = False

    if config_data.domain in [gd.Domain.FUKRAINE, gd.Domain.RKUB]:
        inner_modules.append((Modules.Regexp, modules))
    else:
        inner_modules.append((Modules.RegexpPath, modules))

    def gen_balancer_hint_options(opt):
        if config_data.mms:
            opt.update({'X-Yandex-Balancing-Hint': config_data.location})
        else:
            opt.update({'remove_X-Yandex-Balancing-Hint': True})

        return opt

    def gen_cookie_policy(uuid):
        return {
            'uuid': uuid,
            'default_yandex_policies': 'unstable',
        }

    if config_data.trusted_networks and not http3:
        trusted_headers = gen_balancer_hint_options({
            'X-Forwarded-For-Y_weak': True,
            'X-Forwarded-For_weak': True,
            'X-Req-Id_weak': True,
            'X-Yandex-IP_weak': True,
            'X-Yandex-RandomUID': True,
            'X-Forwarded-Proto': True,
            'remove_from_resp_HSTS-Report': True,
            'remove_X-Yandex-Internal-Flags': True,
            'remove_X-Yandex-Report-Type': True,
            'remove_Yandex-Sovetnik-Cookie': True,
            'X-Yandex-HTTPS': ssl_enabled,
            'X-HTTPS-Request': ssl_enabled,
            'X-Yandex-Family-Search': familysearch,
            'X-Yandex-HTTP-Version': True,
            'X-Yandex-HTTPS-Info': True,
            'X-Yandex-TCP-Info': True,
            'X-Yandex-Ja3': True,
            'X-Yandex-Ja4': True,
            'X-Yandex-P0f': True,
            'Y-Balancer-Experiments': True,
            'add_nel_headers': True,
            'log_reqid': True,
            'log_location': True,
            'log_yandexuid': True,
            'log_tcp-info': True,
            'log_balancer-experiments': True,
            'log_user-agent': True,
            'log_yandex-ip': True,
            'log_gdpr': True,
            'log_cookie_meta': True,
            'log_set_cookie_meta': True,
            'enable_shadow_headers': True,
            'reqid_type': 'search_reqid',
        })

        pre_trusted_modules = [
            (Modules.Report, {
                'uuid': 'service_total_trusted_networks',
                'all_default_ranges': False,
            }),
            (Macroses.WebHeaders, trusted_headers),
        ]

        default_headers = gen_balancer_hint_options({
            'X-Forwarded-For-Y_weak': False,
            'X-Forwarded-For_weak': True,
            'X-Yandex-IP': True,
            'X-Yandex-RandomUID': True,
            'X-Forwarded-Proto': True,
            'remove_from_resp_HSTS-Report': True,
            'remove_X-Yandex-Internal-Flags': True,
            'remove_X-Yandex-Report-Type': True,
            'remove_Laas-Headers': True,
            'remove_Yandex-Sovetnik-Cookie': True,
            'X-Yandex-HTTPS': ssl_enabled,
            'X-HTTPS-Request': ssl_enabled,
            'X-Yandex-Family-Search': familysearch,
            'remove_X-Yandex-HTTPS': not ssl_enabled,
            'X-Yandex-HTTP-Version': True,
            'X-Yandex-HTTPS-Info': True,
            'X-Yandex-TCP-Info': True,
            'X-Yandex-Ja3': True,
            'X-Yandex-Ja4': True,
            'X-Yandex-P0f': True,
            'Y-Balancer-Experiments': True,
            'add_nel_headers': True,
            'log_reqid': True,
            'log_location': True,
            'log_yandexuid': True,
            'log_tcp-info': True,
            'log_balancer-experiments': True,
            'log_user-agent': True,
            'log_yandex-ip': True,
            'log_XFF': True,
            'log_gdpr': True,
            'log_cookie_meta': True,
            'log_set_cookie_meta': True,
            'remove_blacklisted_headers': True,
            'enable_shadow_headers': True,
            'reqid_type': 'search_reqid',
        })

        pre_default_modules = [
            (Macroses.WebHeaders, default_headers),
        ]

        submodules.append((Modules.Regexp, [
            (
                'trusted_networks',
                {
                    'match_source_ip': OrderedDict(
                        [('source_mask', TrustedNets.TRUSTED_NETS)]
                    )
                },
                pre_trusted_modules + inner_modules
            ),
            ('default', {}, pre_default_modules + inner_modules),
        ]))

    else:
        if http3:
            pre_modules = [
                (Modules.Report, {
                    'uuid': 'service_total_proxy',
                    'all_default_ranges': False,
                    'matcher_map': OrderedDict([
                        ('ping', _gen_path_regexp(Constants.NOC_STATIC_CHECK)),
                        ('http3', _gen_not_path(Constants.NOC_STATIC_CHECK)),
                    ]),
                }),
                (Macroses.WebHeaders, gen_balancer_hint_options({
                    'X-Forwarded-For-Y_weak': True,
                    'X-Forwarded-For_weak': True,
                    'X-Source-Port-Y_weak': True,
                    'X-Yandex-RandomUID': True,
                    'remove_from_resp_HSTS-Report': True,
                    'remove_X-Yandex-Internal-Flags': True,
                    'remove_X-Yandex-Report-Type': True,
                    'remove_Laas-Headers': True,
                    'remove_Yandex-Sovetnik-Cookie': True,
                    'Y-Balancer-Experiments': True,
                    'add_nel_headers': True,
                    'log_reqid': True,
                    'log_location': True,
                    'log_yandexuid': True,
                    'log_tcp-info': True,
                    'log_balancer-experiments': True,
                    'log_user-agent': True,
                    'log_yandex-ip': True,
                    'log_XFFY': True,
                    'log_gdpr': True,
                    'log_cookie_meta': True,
                    'log_set_cookie_meta': True,
                    'remove_blacklisted_headers_http3': True,
                    'reqid_type': 'search_reqid',
                    'fix_http3_headers': True,
                })),
            ]
        elif is_knoss_balancer(config_data):
            pre_modules = [
                (Macroses.WebHeaders, {
                    'X-Start-Time_weak': True,
                    'X-Forwarded-For-Y_weak': True,
                    'X-Source-Port-Y_weak': True,
                    'log_reqid': True,
                    'log_location': True,
                    'log_yandexuid': True,
                    'log_XFFY': True,
                    'log_yandex-ip': True,
                    'log_shadow_XFF': True,
                    'disable_response_headers': True,
                    'X-Req-Id_weak': True,
                    'reqid_type': 'search_reqid',
                }),
            ]
        else:
            pre_modules = [
                (Macroses.WebHeaders, gen_balancer_hint_options({
                    'X-Forwarded-For-Y_weak': False,
                    'X-Forwarded-For_weak': True,
                    'X-Yandex-IP': True,
                    'X-Yandex-RandomUID': True,
                    'X-Forwarded-Proto': True,
                    'X-Yandex-HTTPS': ssl_enabled,
                    'X-HTTPS-Request': ssl_enabled,
                    'X-Yandex-HTTP-Version': True,
                    'remove_Laas-Headers': True,
                    'remove_from_resp_HSTS-Report': True,
                    'remove_X-Yandex-Internal-Flags': True,
                    'remove_X-Yandex-Report-Type': True,
                    'remove_Antirobot-Headers': True,
                    'remove_Yandex-Sovetnik-Cookie': True,
                    'X-Yandex-Family-Search': familysearch,
                    'X-Yandex-HTTPS-Info': True,
                    'X-Yandex-TCP-Info': True,
                    # Enable Ja3 only on yaru
                    'X-Yandex-Ja3': config_data.domain == gd.Domain.L7_YARU,
                    'X-Yandex-Ja4': config_data.domain == gd.Domain.L7_YARU,
                    'X-Yandex-P0f': config_data.domain == gd.Domain.L7_YARU,
                    'Y-Balancer-Experiments': True,
                    'log_reqid': True,
                    'log_location': True,
                    'log_yandexuid': True,
                    'log_tcp-info': True,
                    'log_balancer-experiments': True,
                    'log_user-agent': True,
                    'log_yandex-ip': True,
                    'log_XFF': True,
                    'log_gdpr': True,
                    'log_cookie_meta': True,
                    'remove_blacklisted_headers': True,
                    'enable_shadow_headers': True,
                    'reqid_type': 'search_reqid',
                })),
            ]

        submodules.extend(pre_modules + inner_modules)

    if config_data.conftype in (
            gd.ConfigType.SERVICE_L7_SEARCH,
            gd.ConfigType.SERVICE_L7_MORDA,
    ):
        extended_http_params['report_uuid'] = 'service'
        extended_http_params['report_matcher_map'] = _SERVICE_MATCHER_MAP
    elif is_term_balancer(config_data):
        extended_http_params['report_matcher_map'] = _DEFAULT_TERM_MATCHER_MAP

    if config_data.domain == gd.Domain.FUKRAINE:
        extended_http_params['report_enabled'] = True
        result = [
            (Macroses.ExtendedHttp, extended_http_params),
            (Modules.Hasher, {'mode': 'subnet'}),
            (Modules.Regexp, [
                ('yandex', {'match_fsm': OrderedDict([
                    ('host', '(.*)(:\\\\d+|\\\\.)?'),
                    ('case_insensitive', 'true')
                ])}, submodules),
                ('default', {}, [
                    (Modules.ErrorDocument, {'status': 406, 'force_conn_close': True}),
                ]),
            ]),
        ]
    else:
        result = []

        result += [
            ((Macroses.ExtendedHttp, extended_http_params)),
            ((Modules.Hasher, {'mode': 'subnet'})),
        ]

        match_yandex_kg_host = OrderedDict([
            ('host', 'yandex.kg.*'),
            ('case_insensitive', 'true')
        ])

        match_well_known_path = OrderedDict([
            ('URI', '/[.]well-known.*'),
            ('case_insensitive', 'true')
        ])


        def gen_yandex_kg_exclusion_matcher():
            return {'match_and': OrderedDict([
                (LuaAnonymousKey(), {'match_fsm': match_yandex_kg_host}),
                (LuaAnonymousKey(), {'match_not': {'match_fsm': match_well_known_path}})
            ])}
        

        def gen_yaru_matcher():
            return {'match_fsm': OrderedDict([
                    ('host', Constants.YARU_HOST),
                    ('case_insensitive', 'true')
                ])
            }

        response_headers_for_yandex_kg = {'create': OrderedDict([
            ('content-type', 'text/plain'),
        ])}

        error_modules_for_yandex_kg = [
            (Modules.Report, {'uuid': 'yandex_kg'}),
            (Modules.ResponseHeaders, response_headers_for_yandex_kg),
            (Modules.ErrorDocument, {'status': 404}),
        ]

        if config_data.conftype in [gd.ConfigType.SERVICE_L7_SEARCH, gd.ConfigType.SERVICE_L7_MORDA]:
            result.append((Modules.Regexp, [
                ('yandex_kg', gen_yandex_kg_exclusion_matcher(), error_modules_for_yandex_kg),
                ('default', {}, submodules),
            ]))
        elif config_data.conftype in [gd.ConfigType.L7_YARU, gd.ConfigType.L7_YARU_TESTING]:
            result.append((Modules.Regexp, [
                ('yaru', gen_yaru_matcher(), submodules),	
                ('default', {}, [	
                    (Modules.ErrorDocument, {'status': 406, 'force_conn_close': True}),	
                ]),	
            ]))
        else:
            result += submodules

    return result


def add_sections(config_data, iplist, port, ssl_port, section, ssl_section, skip_bind_var=True,
                 name=None):
    result = list()
    ips = iplist
    val = {
        'ips': ips,
        'port': port,
        'name': name if name else config_data.name,
    }
    if val['name']:
        val['port'] = LuaGlobal('port_' + val['name'], port)
    if skip_bind_var:
        val['disabled'] = LuaGlobal('SkipBind', False) if skip_bind_var else False
    result.append((name, val, section))
    if ssl_port is not None:
        ssl_val = {
            'ips': ips,
            'port': ssl_port,
            'name': name if name else config_data.name,
        }
        if ssl_val['name']:
            ssl_val['port'] = LuaGlobal('port_ssl_' + ssl_val['name'], ssl_port)
        if skip_bind_var:
            ssl_val['disabled'] = LuaGlobal('SkipBind', False) if skip_bind_var else False
        result.append((name + '_ssl', ssl_val, ssl_section))
    return result


def create_config(config_data, project, transport):
    submodules = build_modules(config_data.sect_list, config_data)
    http = generate_section(
        config_data,
        config_data.http_instance_port,
        submodules
    )
    https = generate_section(
        config_data,
        config_data.https_instance_port,
        submodules,
        ssl_enabled=True
    )

    modules = []

    if config_data.main_iplist:
        modules.extend(add_sections(
            config_data=config_data,
            iplist=config_data.main_iplist,
            port=80,
            ssl_port=443,
            section=http,
            ssl_section=https,
            name=config_data.section_name,
        ))

    if config_data.familysearch_iplist:
        section = generate_section(
            config_data,
            config_data.http_instance_port,
            submodules,
            familysearch=True,
        )
        ssl_section = generate_section(
            config_data,
            config_data.https_instance_port,
            submodules,
            ssl_enabled=True,
            familysearch=True,
        )
        modules.extend(add_sections(
            config_data=config_data,
            iplist=config_data.familysearch_iplist,
            port=80,
            ssl_port=443,
            section=section,
            ssl_section=ssl_section,
            name='familysearch_%s' % config_data.section_name,
        ))

    if _is_yandex_tld_prod(config_data):
        section = generate_section(
            config_data,
            config_data.http_instance_port,
            submodules,
            http3=True,
        )
        modules.extend(add_sections(
            config_data=config_data,
            iplist=['127.0.0.16'],
            port=80,
            ssl_port=None,
            section=section,
            ssl_section=None,
            name='http3_upstream_%s' % config_data.section_name,
        ))

    modules.extend(add_sections(
        config_data=config_data,
        iplist=config_data.instance_iplist,
        port=config_data.http_instance_port,
        ssl_port=config_data.https_instance_port,
        section=http,
        ssl_section=https,
        skip_bind_var=False,
        name='localips',
    ))

    if is_term_balancer(config_data):
        cpu_limiter_options = OrderedDict([
            ('enable_conn_reject', True),
            ('enable_keepalive_close', True),
            ('enable_http2_drop', True),
            ('disable_file', './controls/cpu_limiter_disabled'),
            ('disable_http2_file', './controls/cpu_limiter_http2_disabled'),
        ])
    else:
        cpu_limiter_options = OrderedDict([
            ('enable_conn_reject', True),
            ('enable_keepalive_close', True),
            ('disable_file', './controls/cpu_limiter_disabled'),
        ])

    main_options = {
        'use_port_admin_as_log_suffix': config_data.custom.get('is_service_balancer', False),
        'workers': LuaGlobal('workers', config_data.workers),
        'config_check': {
            'quorums_file': './controls/backend_check_quorums'
        },
        'tcp_listen_queue': (
            128 if config_data.custom.get('is_service_balancer')
            else 0
        ),
        'temp_buf_prealloc': LuaGlobal('temp_buf_prealloc', config_data.temp_buf_prealloc),
        'maxconn': config_data.maxconn,
        'port': config_data.http_instance_port,
        'admin_port': LuaGlobal('port_admin', config_data.http_instance_port),
        'buffer': 2 * 1024 * 1024,
        'config_tag': transport.describe_gencfg_version(),
        'cpu_limiter': cpu_limiter_options,
        'enable_reuse_port': True,
    }

    if is_term_balancer(config_data):
        main_options['pinger_required'] = True
        main_options['tcp_keep_idle'] = 60
        main_options['tcp_keep_intvl'] = 10

    if is_knoss_balancer(config_data) or config_data.domain in (gd.Domain.L7_YARU, gd.Domain.RKUB):
        main_options['sd_client_name'] = project

    main_options['state_directory'] = LuaGlobal('instance_state_directory', '/dev/shm/balancer-state-{}/'.format(config_data.http_instance_port))
    main_options['enable_dynamic_balancing_log'] = True
    main_options['enable_pinger_log'] = True

    unistat_port = LuaGlobal('port_localips', config_data.http_instance_port) + 2
    unistat_addrs = OrderedDict((
        (LuaAnonymousKey(), OrderedDict([('ip', '127.0.0.1'), ('port', unistat_port)])),
        (LuaAnonymousKey(), OrderedDict([('ip', '::1'), ('port', unistat_port)])),
        (LuaAnonymousKey(), OrderedDict([('ip', LuaFuncCall('GetIpByIproute', {'key': 'v4'})), ('port', unistat_port)])),
        (LuaAnonymousKey(), OrderedDict([('ip', LuaFuncCall('GetIpByIproute', {'key': 'v6'})), ('port', unistat_port)])),
    ))

    main_options['unistat'] = OrderedDict([
        ('addrs', LuaGlobal('instance_unistat_addrs', unistat_addrs)),
        ('hide_legacy_signals', True),
    ])

    main_options['backends_blacklist'] = './controls/backends_blacklist'

    if _is_yandex_tld_prod(config_data):
        main_options['tcp_congestion_control'] = LuaFuncCall('InstallBbrQdisc', {})

    if is_knoss_balancer(config_data):
        main_options['shutdown_close_using_bpf'] = True

    if _is_yandex_tld_prod(config_data) or _is_yaru_tld_prod(config_data):
        main_options['p0f_enabled'] = True

    main_options['shutdown_accept_connections'] = LuaGlobal('shutdown_accept_connections', _is_yandex_tld_prod(config_data))
    main_options['dns_async_resolve'] = LuaGlobal('dns_async_resolve', True)

    # enable coro pool
    main_options['coro_pool_allocator'] = LuaGlobal('coro_pool_allocator', True)
    if is_term_balancer(config_data):
        main_options['coro_pool_stacks_per_chunk'] = LuaGlobal('coro_pool_stacks_per_chunk', 4096)
        main_options['coro_pool_rss_keep'] = LuaGlobal('coro_pool_rss_keep', 3)
        main_options['coro_pool_small_rss_keep'] = LuaGlobal('coro_pool_small_rss_keep', 3)
        main_options['coro_pool_release_rate'] = LuaGlobal('coro_pool_release_rate', 8)
    else:
        main_options['coro_pool_stacks_per_chunk'] = LuaGlobal('coro_pool_stacks_per_chunk', 512)
        main_options['coro_pool_rss_keep'] = LuaGlobal('coro_pool_rss_keep', 1)
        main_options['coro_pool_small_rss_keep'] = LuaGlobal('coro_pool_small_rss_keep', 1)
        main_options['coro_pool_release_rate'] = LuaGlobal('coro_pool_release_rate', 1)

    # enable connection manager
    main_options['connection_manager'] = OrderedDict([
        ('capacity', main_options['maxconn']),
        ('ignore_per_proxy_limit', True),
    ])

    t_config = [
        (Modules.Main, main_options),
        (Modules.Ipdispatch, modules),
    ]
    return t_config


PROJECT_PARAMS = {
    # RKUB

    "l7heavy_production_tun_sas": gd.ConfigData('sas', 'rkub', gd.ConfigType.TUNNEL_COMMON),
    # Should be the same as l7heavy_production_tun_sas but with MMS=True option (sas.yandex.ru)
    "l7heavy_production_tun_sas_only": gd.ConfigData('sas', 'rkub', gd.ConfigType.TUNNEL_MMS),

    "l7heavy_production_tun_man": gd.ConfigData('man', 'rkub', gd.ConfigType.TUNNEL_COMMON),
    # Should be the same as l7heavy_production_tun_man but with MMS=True option
    "l7heavy_production_tun_man_only": gd.ConfigData('man', 'rkub', gd.ConfigType.TUNNEL_MMS),

    "l7heavy_production_tun_vla": gd.ConfigData('vla', 'rkub', gd.ConfigType.TUNNEL_COMMON),
    # Should be the same as l7heavy_production_tun_vla but with MMS=True option
    "l7heavy_production_tun_vla_only": gd.ConfigData('vla', 'rkub', gd.ConfigType.TUNNEL_MMS),

    "l7heavy_testing_tun_man": gd.ConfigData('man', 'rkub', gd.ConfigType.TUNNEL_TESTING),  # l7test.yandex.ru
    
    "l7heavy_testing_tun_sas": gd.ConfigData('sas', 'rkub', gd.ConfigType.TUNNEL_TESTING),  # l7test.yandex.ru

    "l7heavy_testing_tun_vla": gd.ConfigData('vla', 'rkub', gd.ConfigType.TUNNEL_TESTING),  # l7test.yandex.ru

    "l7heavy_experiments_vla": gd.ConfigData('vla', 'rkub', gd.ConfigType.EXPERIMENTS, custom={'is_exp_balancer': True}),

    # FUKRAINE
    "l7heavy_production_fukraine_man": gd.ConfigData('man', 'fukraine', gd.ConfigType.TYPE_FUKRAINE),
    "l7heavy_production_fukraine_sas": gd.ConfigData('sas', 'fukraine', gd.ConfigType.TYPE_FUKRAINE),
    "l7heavy_production_fukraine_vla": gd.ConfigData('vla', 'fukraine', gd.ConfigType.TYPE_FUKRAINE),

    # YARU
    "l7heavy_production_yaru_man": gd.ConfigData('man', 'yaru', gd.ConfigType.L7_YARU),
    "l7heavy_production_yaru_sas": gd.ConfigData('sas', 'yaru', gd.ConfigType.L7_YARU),
    "l7heavy_production_yaru_vla": gd.ConfigData('vla', 'yaru', gd.ConfigType.L7_YARU),
    "l7heavy_production_yaru_testing_sas": gd.ConfigData('sas', 'yaru', gd.ConfigType.L7_YARU_TESTING),

    "l7heavy_service_search_man": gd.ConfigData(
        'man', 'service_search', gd.ConfigType.SERVICE_L7_SEARCH,
        custom={'is_service_balancer': True}
    ),
    "l7heavy_service_search_sas": gd.ConfigData(
        'sas', 'service_search', gd.ConfigType.SERVICE_L7_SEARCH,
        custom={'is_service_balancer': True}
    ),
    "l7heavy_service_search_vla": gd.ConfigData(
        'vla', 'service_search', gd.ConfigType.SERVICE_L7_SEARCH,
        custom={'is_service_balancer': True}
    ),

    "l7heavy_service_morda_man": gd.ConfigData(
        'man', 'service_morda', gd.ConfigType.SERVICE_L7_MORDA,
        custom={'is_service_balancer': True}
    ),
    "l7heavy_service_morda_sas": gd.ConfigData(
        'sas', 'service_morda', gd.ConfigType.SERVICE_L7_MORDA,
        custom={'is_service_balancer': True}
    ),
    "l7heavy_service_morda_vla": gd.ConfigData(
        'vla', 'service_morda', gd.ConfigType.SERVICE_L7_MORDA,
        custom={'is_service_balancer': True}
    ),
}


def process(options):
    project = options.params[0]

    if project in PROJECT_PARAMS:
        InstanceDbTransportHolder.set_transport(options.transport)
        config_data = PROJECT_PARAMS[project]
        config_data.verify(options.transport)
        cfg = create_config(config_data, project, options.transport)
    else:
        raise SystemExit('Unknown subproject %s' % project)

    man_prefix = Constants.GetIpByIproute

    create_testing_config = config_data.conftype in [gd.ConfigType.TUNNEL_TESTING, gd.ConfigType.L7_YARU_TESTING]
    generate(
        cfg, options.output_file,
        create_testing_config=create_testing_config,
        instance_db_transport=options.transport,
        user_config_prefix=man_prefix,
        multiple_uuid_allowed=True,
    )

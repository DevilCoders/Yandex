#!/skynet/python/bin/python
# -*- coding: utf-8 -*-

import src.constants as Constants
import src.modules as Modules

from l7macro import KnossOnly, L7ActiveCheckReply, MetaL7WeightedBackendGroups, Redirect
from collections import OrderedDict
from src.lua_globals import LuaAnonymousKey


def generate_sections(config_data):
    return [
        {
            'service_name': 'blogs',
            'match_uri': '/blogs(/.*)?',
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'in_dc_connection_attempts': 6,
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691,
        },
        {
            'service_name': 'chat',
            'match_uri': '/chat(/.*)?',
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'in_dc_connection_attempts': 6,
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
            'cookie_policy_modes': {
                'yandexuid_new': 'fix'
            }
        },
        {
            'service_name': 'reportmarket',
            'match_uri': '/search/report_market(/.*)?',
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'in_dc_connection_attempts': 6,
            'weights_key': 'search',
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
            'exp_service_name': 'web',
        },
        {
            'service_name': 'searchpre',
            'match_uri': '/search/(pre|pad/pre|touch/pre)(/.*)?',
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'in_dc_attempts': 2,
            'in_dc_connection_attempts': 6,
            'cross_dc_attempts': 2,
            'exp_service_name': 'web',
            'report_uuid': 'searchpre',
            'weights_key': 'searchpre',
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
        },
        {
            'match_uri': '/search/(ads|touch/ads|pad/ads|direct|touch/direct|pad/direct)(/.*)?',
            'service_name': 'searchads',
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'in_dc_connection_attempts': 6,
            'exp_service_name': 'web',
            'report_uuid': 'searchads',
            'weights_key': 'searchads',
            'knoss_backend_timeout': '41s',
        },
        {
            'service_name': 'padsearch',
            'match_uri': '/(padsearch|search/pad)(/)?',
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'in_dc_connection_attempts': 6,
            'exp_service_name': 'web',
            'onerr_backends': config_data.yalite_backends,
            'pumpkin_prefetch': True,
            'weights_key': 'search',
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
            'no_uaas_laas': True,
        },
        {
            'service_name': 'padsearch_last',
            'match_uri': '/(padsearch|search/pad)(/.*)?',
            'backends_groups': config_data.search_backends_split_sas,
            'in_dc_connection_attempts': 6,
            'exp_service_name': 'web',
            'report_uuid': 'padsearch',
            'weights_key': 'search',
            'knoss_backend_timeout': '41s',  # https://st.yandex-team.ru/MINOTAUR-691
        },
        {
            'service_name': 'searchapp_other',
            'match_uri': '/searchapp/(meta|sdch)(/.*)?',
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'backend_timeout': '3s',
            'in_dc_attempts': 2,
            'in_dc_connection_attempts': 6,
            'weights_key': 'searchapp',
            'exp_service_name': 'searchapp',
            'cross_dc_attempts': 2,
            'prefetch_switch': True,
            'knoss_backend_timeout': '11s',  # https://st.yandex-team.ru/MINOTAUR-691
        },
        {
            'service_name': 'searchapp',
            'match_uri': '/searchapp(/.*)?',
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'backend_timeout': '3s',
            'in_dc_attempts': 2,
            'in_dc_connection_attempts': 6,
            'cross_dc_attempts': 2,
            'prefetch_switch': True,
            'knoss_backend_timeout': '11s',  # https://st.yandex-team.ru/MINOTAUR-691
            'no_uaas_laas': True,
        },
        {
            'service_name': 'searchapi',
            'match_uri': '/(searchapi|search/searchapi)(/.*)?',
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'in_dc_attempts': 2,
            'in_dc_connection_attempts': 6,
            'cross_dc_attempts': 2,
            'exp_service_name': 'web',
            'weights_key': 'searchapi',
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
        },
        {
            'service_name': 'touchsearch',
            'match_uri': '/(touchsearch|search/touch)(/)?',
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'in_dc_attempts': 2,
            'in_dc_connection_attempts': 6,
            'cross_dc_attempts': 2,
            'exp_service_name': 'web',
            'onerr_backends': config_data.yalite_backends,
            'pumpkin_prefetch': True,
            'prefetch_switch': True,
            'chromium_prefetch_switch': True,
            'weights_key': 'search',
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
            'no_uaas_laas': True,
        },
        {
            'service_name': 'touchsearch_last',
            'match_uri': '/(touchsearch|search/touch|brosearch)(/.*)?',
            'backends_groups': config_data.search_backends_split_sas,
            'in_dc_attempts': 2,
            'in_dc_connection_attempts': 6,
            'cross_dc_attempts': 2,
            'exp_service_name': 'web',
            'weights_key': 'search',
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
        },
        {
            'service_name': 'jsonproxy',
            'match_uri': '/(jsonproxy)(/)?',
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'in_dc_attempts': 2,
            'in_dc_connection_attempts': 6,
            'cross_dc_attempts': 2,
            'onerr_backends': config_data.yalite_backends,
            'pumpkin_prefetch': True,
            'weights_key': 'search',
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
        },
        {
            'service_name': 'jsonproxy_last',
            'match_uri': '/(jsonproxy)(/.*)?',
            'backends_groups': config_data.search_backends_split_sas,
            'in_dc_attempts': 2,
            'in_dc_connection_attempts': 6,
            'cross_dc_attempts': 2,
            'exp_service_name': 'jsonproxy',
            'report_uuid': 'jsonproxy',
            'weights_key': 'search',
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
        },
        {
            'service_name': 'search-xml',
            'match_uri': '/(search/xml|xmlsearch)(/.*)?',
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'in_dc_connection_attempts': 6,
            'exp_service_name': 'web',
            'weights_key': 'searchxml',
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
            'restore_headers_for_request': {'X-Real-Ip':'X-Laas-User-IP'},
            'no_uaas_laas': True,
        },
        {
            'service_name': 'search-adonly',
            'match_uri': '/search/adonly(/.*)?',  # MINOTAUR-2058
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'in_dc_connection_attempts': 6,
            'exp_service_name': 'web',
            'weights_key': 'searchadonly',
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
            'restore_shadow_headers': ['X-Forwarded-For'],
            'restore_headers_for_request': {'X-Forwarded-For-Y': 'X-Forwarded-For'},
        },
        {
            'service_name': 'sitesearch',
            'match_uri': '/(sitesearch|search/site)(/.*)?',  # MINOTAUR-1342
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'exp_service_name': 'web',
            'knoss_backend_timeout': '41s',  # https://st.yandex-team.ru/MINOTAUR-691
        },
        {
            'service_name': 'searchapphost_shinydiscovery',
            'match_uri': '/search/shiny-discovery(/.*)?', # MINOTAUR-2819
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'weights_key': 'searchapphost',
            'exp_service_name': 'web',
            'knoss_backend_timeout': '41s',  # https://st.yandex-team.ru/MINOTAUR-691
            'no_uaas_laas': True,
        },
        {
            'service_name': 'searchapphost',
            # MINOTAUR-1481 MINOTAUR-2604 BALANCERSUPPORT-1315
            'match_uri': '/search/({})(/.*)?'.format("|".join([
                str('film-catalog'),
                str('afisha-schedule'),
                str('ugc2/desktop-digest'),
                str('ugc2/discussions'),
                str('ugc2/sideblock'),
                str('catalogsearch'),
                str('entity'),
                str('suggest-history'),
                str('vwizdoc2doc'),
                str('zero'),
            ])),
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'weights_key': 'searchapphost',
            'exp_service_name': 'web',
            'knoss_backend_timeout': '41s',  # https://st.yandex-team.ru/MINOTAUR-691
            'no_uaas_laas': True,
        },
        {
            'service_name': 'recommendation',
            'match_uri': '/search/recommendation(/.*)?',  # MINOTAUR-1458
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'exp_service_name': 'web',
            'knoss_backend_timeout': '41s',  # https://st.yandex-team.ru/MINOTAUR-691
        },
        {
            'service_name': 'itditp',
            'match_uri': '/search/itditp(/.*)?',  # MINOTAUR-1986
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'exp_service_name': 'web',
            'knoss_backend_timeout': '41s',  # https://st.yandex-team.ru/MINOTAUR-691
        },
        {
            'service_name': 'searchresult',
            'match_uri': '/search/result(/.*)?',  # MINOTAUR-1816
            'backends_groups': config_data.http_adapter_search_split_sas_backends,  # https://st.yandex-team.ru/MINOTAUR-1878
            'exp_service_name': 'web',
            'knoss_backend_timeout': '41s',  # https://st.yandex-team.ru/MINOTAUR-691
            'no_uaas_laas': True,
        },
        {
            'service_name': 'searchwizardsjson',
            'match_uri': '/search/wizardsjson(/.*)?',  # MINOTAUR-1897
            'backends_groups': config_data.http_adapter_search_split_sas_backends,  # https://st.yandex-team.ru/MINOTAUR-1878
            'exp_service_name': 'web',
            'knoss_backend_timeout': '41s',  # https://st.yandex-team.ru/MINOTAUR-691
            'no_uaas_laas': True,
        },
        {
            'service_name': 'search_direct_preview_touch',
            'report_uuid': 'search_direct_preview',
            'match_uri': '/search/direct-preview/touch(/.*)?',
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'in_dc_connection_attempts': 6,
            'exp_service_name': 'web',
            'onerr_backends': config_data.yalite_backends,
            'pumpkin_prefetch': True,
            'weights_key': 'search',
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
            'no_uaas_laas': True,
            'create_headers': OrderedDict([
                ('user-agent', 'Mozilla/5.0 (Linux; Android 11; SM-A705FN) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.77 Mobile Safari/537.36')  # MINOTAUR-2886
            ]),
        },
        {
            'service_name': 'search_direct_preview',
            'match_uri': '/search/direct-preview(/.*)?',  # MINOTAUR-1680
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'in_dc_connection_attempts': 6,
            'exp_service_name': 'web',
            'onerr_backends': config_data.yalite_backends,
            'pumpkin_prefetch': True,
            'weights_key': 'search',
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
            'no_uaas_laas': True,
        },
        {
            'service_name': 'search_fronted_entity',
            'match_uri': '/search/frontend-entity(/.*)?',  # MINOTAUR-2301
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'in_dc_connection_attempts': 6,
            'weights_key': 'search',
            'exp_service_name': 'web',
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
        },
        {
            'service_name': 'searchabuse',
            'match_uri': '/search/abuse(/.*)?',  # MINOTAUR-3126
            'backends_groups': config_data.search_apphost_migration_backends,
            'in_dc_connection_attempts': 6,
            'exp_service_name': 'web',
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
        },
        {
            'service_name': 'search',
            'match_uri': '/(search|msearch|search/smart|yandsearch)(/)?',
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'in_dc_connection_attempts': 6,
            'exp_service_name': 'web',
            'onerr_backends': config_data.yalite_backends,
            'pumpkin_prefetch': True,
            'chromium_prefetch_switch': True,
            'weights_key': 'search',
            'report_uuid': 'search',
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
            'no_uaas_laas': True,
        },
        {
            'service_name': 'searchsdch',
            'match_uri': '/search/sdch/(.*)',
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'in_dc_connection_attempts': 6,
            'exp_service_name': 'web',
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
        },
        {
            'service_name': 'opensearch',
            'match_uri': '/(search/opensearch\\\\.xml|opensearch\\\\.xml)',
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'in_dc_connection_attempts': 6,
            'exp_service_name': 'web',
            'weights_key': 'opensearch',
            'onerr_backends': config_data.yalite_backends,
            'pumpkin_prefetch': True,
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
        },
        {
            'service_name': 'search_other',
            'match_uri': '/(familysearch|schoolsearch|telsearch)(/)?',
            'backends_groups': config_data.search_backends_split_sas,
            'in_dc_connection_attempts': 6,
            'exp_service_name': 'web',
            'weights_key': 'search',
            'onerr_backends': config_data.yalite_backends,
            'pumpkin_prefetch': True,
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
        },
        {
            'service_name': 'search_static_prefetch',
            'match_uri': r'(/search/(prefetch\\.txt|yandcache\\.js|padcache\\.js|touchcache\\.js))|(/prefetch\\.txt.*)',
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'in_dc_connection_attempts': 6,
            'exp_service_name': 'web',
            'weights_key': 'searchstaticprefetch',
            'knoss_backend_timeout': '41s',  # https://st.yandex-team.ru/MINOTAUR-691
        },
        {
            'service_name': 'search_prefetch',
            'match_uri': '/prefetch(/.*)?',
            'backends_groups': config_data.http_adapter_search_split_sas_backends,
            'in_dc_connection_attempts': 6,
            'weights_key': 'search',
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
        },
        {
            'service_name': 'tutor',
            'match_uri': '/tutor/search(/.*)?',
            'backends_groups': config_data.http_adapter_search_split_sas_backends_sas_vla,
            'in_dc_connection_attempts': 6,
            'weights_key': 'search',
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
        },
        {
            'service_name': 'search_last',
            'match_uri': Constants.YANDSEARCH_REQUEST_URI_WEIGHTED + '|' + Constants.L7_SEARCH_JUNK_URI,
            'backends_groups': config_data.search_backends_split_sas,
            'in_dc_connection_attempts': 6,
            'exp_service_name': 'web',
            'weights_key': 'search',
            'knoss_backend_timeout': '41s',  # https://st.yandex-team.ru/MINOTAUR-691
        },
    ]


def tmpl_search_service(config_data):
    prefetch_matcher = {
        'match_fsm': OrderedDict([
            ('header', {'name': 'Yandex-Preload', 'value': 'prerender'}),
        ]),
    }

    prefetch_switch_file = Constants.L7_DEFAULT_WEIGHTS_DIR + '/prefetch_switch'

    chromium_prefetch_matcher = {
        'match_and': OrderedDict([
            (LuaAnonymousKey(), {
                'match_fsm': OrderedDict([
                    ('header', {'name': 'purpose', 'value': 'prefetch'}),
                ]),
            }),
            (LuaAnonymousKey(), {'match_not': prefetch_matcher}),
        ]),
    }

    chromium_prefetch_switch_file = Constants.L7_DEFAULT_WEIGHTS_DIR + '/chromium_prefetch_switch'

    def _update(section):
        section.pop('knoss_backend_timeout', None)
        section.update({
            'generator': 'hashing',
            'hashing_by': 'ICookie',
            'weight_matrix': config_data.search_weight_matrix,
            'rate_limiter_limit': 0.2,
            'rate_limiter_max_budget': 50,
            'add_geo_only_matcher': True,
            'enable_dynamic_balancing': True,
            'service_name_to_backend_header': 'X-Yandex-ExpServiceName',
            'backends_check_quorum': 0.35,
            'enable_cookie_policy': True,
            'synthetics_load': True,
            'rps_limiter_backends': config_data.rps_limiter_web_backends,
            'rps_limiter_location': config_data.location,
            'by_dc_rps_limiter': OrderedDict({
                'namespace': 'knoss_search',
                'disable_file': Constants.L7_DEFAULT_WEIGHTS_DIR + '/disable_rpslimiter',
            }),


        })

        if section.get('pumpkin_prefetch'):
            section['pumpkin_prefetch_timeout'] = '400ms'
            section['sink_backends'] = section['onerr_backends']
            section['sink_indicator_header'] = 'X-Yandex-Pumpkin-Mirroring'

        if not section.get('no_uaas_laas'):
            section.update({
                'uaas_backends': config_data.uaas_backends,
                'uaas_new_backends': config_data.uaas_new_backends,
                'remote_log_backends': config_data.remote_log_backends,
                'laas_backends': config_data.laas_backends,
                'laas_backends_onerror': config_data.laas_backends_onerror,
                'laas_uaas_processing_time_header': True,
            })

        if section.get('prefetch_switch'):
            section.update({
                'prefetch_switch_matcher': prefetch_matcher,
                'prefetch_switch_per_platform': True,
                'prefetch_switch_file': prefetch_switch_file,
            })

        if section.get('chromium_prefetch_switch'):
            section.update({
                'chromium_prefetch_switch_matcher': chromium_prefetch_matcher,
                'chromium_prefetch_switch_file': chromium_prefetch_switch_file,
            })

        return (MetaL7WeightedBackendGroups, section)

    return [
        (Redirect, {
            'match_uri': '/(app-host|search/app-host)(/.*)?',
            'service_name': 'emptyapphost',
            'location': '/search/',
        }),
        (Redirect, {
            'match_uri': '/search/customize(/.*)?',
            'service_name': 'customize2tune',
            'location': '/tune/search/',
        }),
        (Redirect, {
            'match_uri': '/search/infected(/.*)?',
            'service_name': 'infected2safety',
            'location': '/safety/',
        }),
        (Redirect, {  # https://st.yandex-team.ru/MINOTAUR-2140
            'match_uri': '/search/family(/.*)?',
            'service_name': 'familysearch',
            'location': '/support/search/additional-features/#adult-filter__adult-filter-on',
        }),
        (L7ActiveCheckReply, {
            'match_uri': Constants.BALANCER_HEALTH_CHECK,
            'case_insensitive': False,
            'service_name': 'balancer_health_check',
            'weight_file': './controls/manual_weight_file',
            'zero_weight_at_shutdown': True,
        })
    ] + [_update(section) for section in generate_sections(config_data)]


def tmpl_search(config_data):
    def _update(section):
        section.pop('pumpkin_prefetch', None)
        section.pop('override_balancing_hints', None)
        section.pop('cookie_policy_modes', None)

        knoss_section = {
            'service_name': section['service_name'],
            'match_uri': section['match_uri'],
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'onerr_backends': section.get('onerr_backends') if not config_data.mms else None,
            'knoss_backends': config_data.search_knoss_backends,
            'knoss_backend_timeout': section['knoss_backend_timeout'],
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_balancer_options': OrderedDict([
                ('request', Constants.BALANCER_HEALTH_CHECK_REQUEST),
                ('delay', '20s'),
                ('use_backend_weight', True),
                ('backend_weight_disable_file', './controls/disable_backend_weight_in_active'),
            ]),
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': OrderedDict([
                ('max_pessimized_share', 0.2)
            ]),
            'enable_cookie_policy': False,  # Have it in knoss already
        }

        return (KnossOnly, knoss_section)

    return [_update(section) for section in generate_sections(config_data) if section['service_name']]


def tmpl_turbo(config_data):
    turbo_on_error_module = [
        (Modules.Report, {'uuid': 'turbo_requests_to_knoss_onerror'}),
        (Modules.ErrorDocument, {
            'status': 504
        })
    ]
    return [
        (KnossOnly, {  # MINOTAUR-276
            'match_uri': '/turbo(/.*)?',
            'service_name': 'turbo',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_on_error_modules': turbo_on_error_module,
            'knoss_backends': config_data.turbo_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '5s',  # https://st.yandex-team.ru/MINOTAUR-691
            'knoss_balancer_options': OrderedDict([
                ('request', Constants.AWACS_BALANCER_HEALTH_CHECK_REQUEST),
                ('delay', '20s'),
                ('use_backend_weight', True),
                ('backend_weight_disable_file', './controls/disable_backend_weight_in_active'),
            ]),
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
        }),
        (KnossOnly, {  # BALANCERSUPPORT-394
            'match_uri': '/turboforms(/.*)?',
            'service_name': 'turboforms',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_on_error_modules': turbo_on_error_module,
            'knoss_backends': config_data.turbo_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '5s',  # https://st.yandex-team.ru/MINOTAUR-691
            'knoss_balancer_options': OrderedDict([
                ('request', Constants.AWACS_BALANCER_HEALTH_CHECK_REQUEST),
                ('delay', '20s'),
                ('use_backend_weight', True),
                ('backend_weight_disable_file', './controls/disable_backend_weight_in_active'),
            ]),
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
        }),
    ]

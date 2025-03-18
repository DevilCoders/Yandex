#!/skynet/python/bin/python
# -*- coding: utf8 -*-
# import src.macroses as Macroses

from l7macro import KnossOnly, L7ActiveCheckReply, MetaL7WeightedBackendGroups
from collections import OrderedDict
from src.constants import L7_DEFAULT_WEIGHTS_DIR, BALANCER_HEALTH_CHECK, BALANCER_HEALTH_CHECK_REQUEST
from src.lua_globals import LuaAnonymousKey
import src.macroses as Macroses


MORDA_FILE = '/({})(.*)?'.format('|'.join([
    str("my-cookie[.]js"),
    str("wsave[.]html"),
    str("form[.]html"),
    str("mailinfo[.]js"),
    str("dropcounters[.]js"),
    str("all[.]xml"),
    str("original[.]xml"),
    str("services[.]xml"),
    str("hello[.]html"),
    str("hellom[.]html"),
    str("hellot[.]html"),
    str("hellotel[.]html"),
    str("ok[.]html"),
    str("index[.]html"),
    str("index[.]htm"),
    str("google([a-zA-Z0-9]+)[.]html"),
    str("application[.]xml"),
    str("black[.]html"),
    str("white[.]html"),
    str("empty[.]html"),
    str("crossdomain[.]xml"),
    str("i-social__closer[.]html"),
    str("login-status[.]html"),
    str("mda[.]html"),
    str("mdaxframeproxy[.]html"),
    str("xframeproxy[.]html"),
    str("apple-touch-icon[.]png"),
    str("embeded[.]min[.]js"),
    str("htc"),
    str("HTC"),
    str("mdaiframe[.]html"),
    str("apple-app-site-association"),
    str("apple-touch-icon-76x76[.]png"),
    str("apple-touch-icon-120x120[.]png"),
    str("apple-touch-icon-152x152[.]png"),
]))

MORDA_DIR = "/|/({})(/.*)?".format('|'.join([
    str("wsave"),
    str("autopattern"),
    str("all"),
    str("myclean"),
    str("screenx"),
    str("remotes-status"),
    str("setmy"),
    str("adddata"),
    str("wcomplain"),
    str("route"),
    str("clean_route"),
    str("save_route"),
    str("drop_route"),
    str("m"),
    str("d"),
    str("mdae"),
    str("gpsave"),
    str("mgpsave"),
    str("jcb"),
    str("gs"),
    str("bjk"),
    str("fb"),
    str("sade"),
    str("all"),
    str("themes"),
    str("skins"),
    str("rapid"),
    str("instant"),
    str("postcard"),
    str("y"),
    str("json"),
    str("data"),
    str("test"),
    str("banner"),
    str("portal"),
    str("log"),
    str("black"),
    str("white"),
    str("map_router"),
    str("ua"),
    str("ru"),
    str("kz"),
    str("by"),
    str("safari"),
    str("ios7se"),
    str("inline"),
    str("jsb"),
    str("i"),
    str("dform"),
    str("chrome"),
    str("[.]well-known"),
    str("1tv"),
    str("1tv[.]ru"),
    str("matchtv"),
    str("ntv"),
    str("5tv"),
    str("ru24"),
    str("vesti"),
    str("rentv"),
    str("tnt"),
    str("m24"),
    str("a2"),
    str("sovsekretno"),
    str("izvestia"),
    str("echotv"),
    str("probusiness"),
    str("uspeh"),
    str("globalstar"),
    str("tochkatv"),
    str("hardlife"),
    str("oceantv"),
    str("gamanoid"),
    str("hitv"),
    str("rutv"),
    str("topspb"),
    str("tdk"),
    str("oirtv"),
    str("rt"),
    str("rtdoc"),
    str("hdmedia"),
    str("wfc"),
    str("sibir"),
    str("ntvpravo"),
    str("ntvserial"),
    str("ntvstyle"),
    str("ctc"),
    str("samsung-bixby"),

    # Temporary workaround, should be deleted. See for details MINOTAUR-2643.
    str("bUb37PWT2MmDsHF"),
    str("6RJwj93etLgYaLQ"),
    str("BdE8qfhjSTqeke8"),
    str("xHP0CWq9OQ9BcKk"),
    str("d6WhMSmeM7aR58t"),
    str("H1GnexzAm8nhZza"),
    str("mThAztkM5Wa62xL"),
    str("YVhcbqk1n2EyPFb"),
    str("SfzcdSuJBNAjRA0"),
    str("TOgFmNN3UqSJJ5z"),
    str("SdoTBVrPOjckonF"),
    str("AFc1G2diUtKe6Wd"),
    str("wNWjJuAdonkd7R3"),
    str("mUacq7u3Ttju1VG"),
    str("lzee4cRGCkmJfL4"),
    str("AClTSwdfLhH3SZB"),
    str("zdjsb9WdxBMNy9l"),
    str("MK6ln5wlRkGiC9E"),
    str("zrWlgmIRzNzgew3"),
    str("Kak7FKQkQGQJQMd"),
]))

YARU_DIR = "/({})(/.*)?".format('|'.join([
    str("design"),
    str("discounts"),
    str("set"),
    str("music"),
    str("ymusic"),
    str("ydisk"),
    str("advanced"),
    str("am"),
    str("ovi"),
    str("ovi/content"),
    str("cgi-bin/vesna"),
    str("lingvo"),
    str("video"),
    str("moikrug"),
    str("mobi"),
    str("tabs"),
    str("vse"),
    str("services"),
    str("classic"),
    str("e"),
    str("adresa"),
    str("afisha"),
    str("auto"),
    str("bar"),
    str("blogs"),
    str("calendar"),
    str("cards"),
    str("collection"),
    str("direct"),
    str("encycl"),
    str("fotki"),
    str("guest"),
    str("images"),
    str("large"),
    str("lenta"),
    str("local"),
    str("mail"),
    str("maps"),
    str("market"),
    str("money"),
    str("my"),
    str("nahodki"),
    str("narod"),
    str("news"),
    str("play"),
    str("probki"),
    str("rasp"),
    str("referats"),
    str("slovari"),
    str("time"),
    str("transport"),
    str("tv"),
    str("yaca"),
    str("zakladki"),
    str("yandsearch"),
]))

YARU_FILE = '/({})(.*)?'.format('|'.join([
    str("advanced[.]html"),
    str("images[.]html"),
    str("vesna[.]html"),
    str("advanced[.]html"),
    str("all_services[.]html"),
    str("robots[.]txt"),
    str("favicon[.]ico"),
]))


def knoss_morda_defaults(config_data):
    return {
        'antirobot_backends': config_data.antirobot_backends,
        'antirobot_sink_backends': config_data.antirobot_sink_backends,
        'knoss_backend_timeout': '21s',  # https://st.yandex-team.ru/MINOTAUR-691
        'knoss_backends': config_data.morda_knoss_backends,
        'knoss_location': config_data.location,
        'knoss_is_geo_only': config_data.mms,
        'knoss_balancer_options': OrderedDict([
            ('request', BALANCER_HEALTH_CHECK_REQUEST),
            ('delay', '20s'),
            ('use_backend_weight', True),
        ]),
        'knoss_dynamic_balancing_enabled': True,
        'knoss_dynamic_balancing_options': OrderedDict([
            ('max_pessimized_share', 0.2)
        ]),
        'enable_cookie_policy': False,
    }


def template_morda_service(config_data):
    result = template_portal(config_data, morda_service=True) + \
        template_android_widget_api(config_data, morda_service=True) + \
        template_partner_strmchannels(config_data, morda_service=True) + \
        template_wy(config_data, morda_service=True) + \
        morda_health_check()

    return result


def morda_health_check():
    return [(L7ActiveCheckReply, {
        'match_uri': BALANCER_HEALTH_CHECK,
        'case_insensitive': False,
        'service_name': 'balancer_health_check',
        'weight_file': './controls/manual_weight_file',
        'zero_weight_at_shutdown': True,
    })]


def template_portal(config_data, morda_service=False):
    if morda_service:
        # BALANCER-2237
        switch_file = L7_DEFAULT_WEIGHTS_DIR + '/morda_prefetch_switch'
        matcher = {
            'match_or': OrderedDict([
                (LuaAnonymousKey(), {'match_fsm': OrderedDict([
                    ('header', {'name': 'Yandex-Preload', 'value': '(prerender|prefetch)'})
                ])}),
                (LuaAnonymousKey(), {'match_fsm': OrderedDict([
                    ('cgi', '.*yandex_prefetch=(prerender|prefetch).*')
                ])}),
            ]),
        }

        prefetch_matcher = {
            'match_and': OrderedDict([
                (LuaAnonymousKey(), {'match_if_file_exists': {'path': switch_file}}),
                (LuaAnonymousKey(), matcher)
            ])
        }

        def _common(custom_options):
            options = {
                'onerr_backend_timeout': '4s',
                'onerr_connection_attempts': 5,
                'onerr_attempts': 2,
                'onerr_backends': config_data.morda_yalite_backends,  # HOME-48123
                'uaas_backends': config_data.uaas_backends,
                'uaas_new_backends': config_data.uaas_new_backends,
                'remote_log_backends': config_data.remote_log_backends,
                'laas_backends': config_data.laas_backends,
                'laas_backends_onerror': config_data.laas_backends_onerror,
                'in_dc_attempts': 2,
                'rate_limiter_limit': 0.2,
                'rate_limiter_max_budget': 50,
                'in_dc_connection_attempts': 5,
                'cross_dc_attempts': 2,
                'cross_dc_connection_attempts': 5,
                'generator': 'b2standart',
                'connect_timeout': '50ms',
                'backend_timeout': '1.5s',
                'threshold_enable': True,
                'enable_dynamic_balancing': True,
                'dynamic_balancing_options': OrderedDict([
                    ('max_pessimized_share', 0.2)
                ]),
                'backends_check_quorum': 0.35,
            }
            options.update(custom_options)
            return options

        return [
            (MetaL7WeightedBackendGroups, _common({
                'service_name': 'portal_station',
                'match_uri': '/portal/station(/.*)?',
                'exp_service_name': 'uniproxy',
                'weights_key': 'morda',
                'backends_groups': config_data.portal_station_backends,
                'weight_matrix': config_data.morda_weight_matrix,
                'enable_cookie_policy': True,
                'cookie_policy_modes': {
                    'yandexuid_val': 'off',
                    'yandex_perm': 'off',
                    'yandex_root': 'off',
                    'yandex_sess': 'off',
                    'yandex_js': 'off',
                },
            })),

            (MetaL7WeightedBackendGroups, _common({
                'service_name': 'portal_front',
                'match_uri': '/portal/front(/.*)?',
                'exp_service_name': 'morda',
                'weights_key': 'morda',
                'backends_groups': config_data.portal_front_backends,
                'weight_matrix': config_data.morda_weight_matrix,
                'backend_experiments': [],
                'backend_timeout': '3s',
                'enable_cookie_policy': True,
                'cookie_policy_modes': {
                    'yandexuid_val': 'off',
                    'yandex_perm': 'off',
                    'yandex_root': 'off',
                    'yandex_sess': 'off',
                    'yandex_js': 'off',
                },
                'rps_limiter_backends': config_data.rps_limiter_backends,
                'rps_limiter_location': config_data.location,
                'rps_limiter': OrderedDict({
                    'namespace': 'morda',
                    'disable_file': L7_DEFAULT_WEIGHTS_DIR + '/disable_rpslimiter',
                    'skip_on_error': True,
                }),
            })),

            (MetaL7WeightedBackendGroups, _common({
                'service_name': 'morda_browser_config',
                'match_uri': '/portal/mobilesearch/config/browser(/.*)?',
                'exp_service_name': 'browser',
                'weights_key': 'mordabrowserconf',
                'backends_groups': config_data.morda_app_backends,
                'weight_matrix': config_data.morda_app_weight_matrix,
                'backend_experiments': [],
                'enable_cookie_policy': True,
                'cookie_policy_modes': {
                    'yandexuid_val': 'off',
                    'yandex_perm': 'off',
                    'yandex_root': 'off',
                    'yandex_sess': 'off',
                    'yandex_js': 'off',
                },
                'rps_limiter_backends': config_data.rps_limiter_backends,
                'rps_limiter_location': config_data.location,
                'rps_limiter': OrderedDict({
                    'namespace': 'morda',
                    'disable_file': L7_DEFAULT_WEIGHTS_DIR + '/disable_rpslimiter',
                    'skip_on_error': True,
                }),
            })),

            (MetaL7WeightedBackendGroups, _common({
                'service_name': 'morda_ibrowser_config',
                'match_uri': '/portal/mobilesearch/config/ibrowser(/.*)?',
                'exp_service_name': 'ibrowser',
                'weights_key': 'mordabrowserconf',
                'backends_groups': config_data.morda_app_backends,
                'weight_matrix': config_data.morda_app_weight_matrix,
                'backend_experiments': [],
                'enable_cookie_policy': True,
                'cookie_policy_modes': {
                    'yandexuid_val': 'off',
                    'yandex_perm': 'off',
                    'yandex_root': 'off',
                    'yandex_sess': 'off',
                    'yandex_js': 'off',
                },
                'rps_limiter_backends': config_data.rps_limiter_backends,
                'rps_limiter_location': config_data.location,
                'rps_limiter': OrderedDict({
                    'namespace': 'morda',
                    'disable_file': L7_DEFAULT_WEIGHTS_DIR + '/disable_rpslimiter',
                    'skip_on_error': True,
                }),
            })),

            (MetaL7WeightedBackendGroups, _common({
                'service_name': 'morda_searchapp_config',
                'match_uri': '/(portal/mobilesearch/config/searchapp|portal/subs/config/0)(/.*)?',
                'exp_service_name': 'morda',
                'weights_key': 'mordasearchappconf',
                'backends_groups': config_data.morda_app_backends,
                'weight_matrix': config_data.morda_app_weight_matrix,
                'backend_experiments': [],
                'enable_cookie_policy': True,
                'cookie_policy_modes': {
                    'yandexuid_val': 'off',
                    'yandex_perm': 'off',
                    'yandex_root': 'off',
                    'yandex_sess': 'off',
                    'yandex_js': 'off',
                },
                'rps_limiter_backends': config_data.rps_limiter_backends,
                'rps_limiter_location': config_data.location,
                'rps_limiter': OrderedDict({
                    'namespace': 'morda',
                    'disable_file': L7_DEFAULT_WEIGHTS_DIR + '/disable_rpslimiter',
                    'skip_on_error': True,
                }),
            })),

            (MetaL7WeightedBackendGroups, _common({
                'service_name': 'morda_app',
                'match_uri': '/(portal/api/search|portal/api/yabrowser|portal/mobilesearch|portal/geo)(/.*)?',
                'exp_service_name': 'morda',
                'weights_key': 'mordaapp',
                'backends_groups': config_data.morda_app_backends,
                'weight_matrix': config_data.morda_app_weight_matrix,
                'backend_experiments': [],
                'backend_timeout': '3s',
                'enable_cookie_policy': True,
                'cookie_policy_modes': {
                    'yandexuid_val': 'off',
                    'yandex_perm': 'off',
                    'yandex_root': 'off',
                    'yandex_sess': 'off',
                    'yandex_js': 'off',
                },
                'threshold_pass_timeout': '10s',
            })),

            (MetaL7WeightedBackendGroups, _common({
                'service_name': 'morda_ntp',
                'match_uri': '/(portal/api/data/1|portal/ntp/notifications|portal/ntp/informers|portal/ntp/banner|portal/ntp/refresh_data)(/.*)?',
                'exp_service_name': 'morda',
                'weights_key': 'mordantp',
                'backends_groups': config_data.morda_app_backends,
                'weight_matrix': config_data.morda_app_weight_matrix,
                'backend_experiments': [],
                'enable_cookie_policy': True,
                'cookie_policy_modes': {
                    'yandexuid_val': 'off',
                    'yandex_perm': 'off',
                    'yandex_root': 'off',
                    'yandex_sess': 'off',
                    'yandex_js': 'off',
                },
                'rps_limiter_backends': config_data.rps_limiter_backends,
                'rps_limiter_location': config_data.location,
                'rps_limiter': OrderedDict({
                    'namespace': 'morda',
                    'disable_file': L7_DEFAULT_WEIGHTS_DIR + '/disable_rpslimiter',
                    'skip_on_error': True,
                }),
            })),

            (MetaL7WeightedBackendGroups, _common({
                'service_name': 'morda',
                'match_uri': MORDA_FILE + '|' + MORDA_DIR,
                'exp_service_name': 'morda',
                'prefetch_switch': True,
                'prefetch_switch_matcher': prefetch_matcher,
                'prefetch_switch_file': switch_file,
                'backends_groups': config_data.morda_backends,
                'weight_matrix': config_data.morda_weight_matrix,
                'backend_experiments': [],
                'backend_timeout': '3s',
                'enable_cookie_policy': True,
                'cookie_policy_modes': {
                    'yandexuid_val': 'off',
                    'yandex_perm': 'off',
                    'yandex_root': 'off',
                    'yandex_sess': 'off',
                    'yandex_js': 'off',
                },
            })),

            (MetaL7WeightedBackendGroups, _common({
                'service_name': 'yaruecoo',
                'exp_service_name': 'morda',
                'match_uri': '/ecoo(/.*)?',
                'backends_groups': config_data.ecoo_backends,
                'weight_matrix': config_data.morda_weight_matrix,
                'backend_experiments': [],
                'onerr_backends': config_data.yalite_backends,
                'onerr_backend_timeout': '10s',
                'enable_cookie_policy': True,
                'cookie_policy_modes': {
                    'yandexuid_val': 'off',
                    'yandex_perm': 'off',
                    'yandex_root': 'off',
                    'yandex_sess': 'off',
                    'yandex_js': 'off',
                },
            })),

            (MetaL7WeightedBackendGroups, _common({
                'service_name': 'yaru',
                'exp_service_name': 'morda',
                'match_uri': YARU_FILE + '|' + YARU_DIR,
                'backends_groups': config_data.morda_backends,
                'weight_matrix': config_data.morda_weight_matrix,
                'backend_experiments': [],
                'in_dc_attempts': 1,
                'onerr_backends': config_data.yalite_backends,
                'onerr_backend_timeout': '10s',
                'enable_cookie_policy': True,
                'cookie_policy_modes': {
                    'yandexuid_val': 'off',
                    'yandex_perm': 'off',
                    'yandex_root': 'off',
                    'yandex_sess': 'off',
                    'yandex_js': 'off',
                },
            })),

            (MetaL7WeightedBackendGroups, _common({
                'service_name': 'default',
                'exp_service_name': 'morda',
                'backends_groups': config_data.any_backends,
                'weight_matrix': config_data.morda_weight_matrix,
                'backend_experiments': [],
                'onerr_backends': config_data.yalite_backends,
                'onerr_backend_timeout': '10s',
                'in_dc_attempts': 1,
                'enable_cookie_policy': True,
                'cookie_policy_modes': {
                    'yandexuid_val': 'off',
                    'yandex_perm': 'off',
                    'yandex_root': 'off',
                    'yandex_sess': 'off',
                    'yandex_js': 'off',
                },
            })),
        ]
    else:
        def _common(options):
            options.update(knoss_morda_defaults(config_data))
            options.update({
                'knoss_create_headers': OrderedDict([
                    ('X-Antirobot-Service-Y', 'morda')
                ]),
                'onerr_backends': config_data.yalite_backends,
                'knoss_migration_name': 'morda',
            })
            return options

        return [
            (KnossOnly, _common({
                'service_name': 'portal_station',
                'match_uri': '/portal/station(/.*)?',
            })),

            (KnossOnly, _common({
                'service_name': 'morda_app',
                'match_uri': '/(portal/api/search|portal/api/yabrowser|portal/mobilesearch|portal/geo)(/.*)?',
            })),

            (KnossOnly, _common({
                'service_name': 'morda_browser_config',
                'match_uri': '/portal/mobilesearch/config/browser(/.*)?',
            })),

            (KnossOnly, _common({
                'service_name': 'morda_ibrowser_config',
                'match_uri': '/portal/mobilesearch/config/ibrowser(/.*)?',
            })),

            (KnossOnly, _common({
                'service_name': 'morda_searchapp_config',
                'match_uri': '/(portal/mobilesearch/config/searchapp|portal/subs/config/0)(/.*)?',
            })),

            (KnossOnly, _common({
                'service_name': 'morda_ntp',
                'match_uri': '/(portal/api/data/1|portal/ntp/notifications|portal/ntp/informers|portal/ntp/banner|portal/ntp/refresh_data)(/.*)?',
            })),

            (KnossOnly, _common({
                'service_name': 'portal_front',
                'match_uri': '/portal/front(/.*)?',
            })),

            (KnossOnly, _common({
                'service_name': 'morda',
                'match_uri': MORDA_FILE + '|' + MORDA_DIR,
            })),
        ]


def template_partner_strmchannels(config_data, morda_service=False):
    properties = {
        'service_name': 'partner_strmchannels',
        'match_uri': '/partner-strmchannels(/.*)?',
        'backends_check_quorum': 0.35,
    }

    if morda_service:
        properties.update({
            'backends_groups': config_data.yaru_backends,
            'backend_timeout': '1.5s',
            'laas_backends': config_data.laas_backends,
            'laas_backends_onerror': config_data.laas_backends_onerror,
            'uaas_backends': config_data.uaas_backends,
            'uaas_new_backends': config_data.uaas_backends,
            'remote_log_backends': config_data.remote_log_backends,
            'in_dc_attempts': 2,
            'rate_limiter_limit': 0.2,
            'in_dc_connection_attempts': 5,
            'cross_dc_attempts': 2,
            'cross_dc_connection_attempts': 5,
            'connect_timeout': '50ms',
            'generator': 'b2standart',
            'weight_matrix': config_data.morda_weight_matrix,
            'weights_key': 'yaru',
            'exp_service_name': 'morda',
            'threshold_enable': True,
            'enable_dynamic_balancing': True,
            'dynamic_balancing_options': OrderedDict([
                ('max_pessimized_share', 0.2)
            ]),
            'enable_cookie_policy': True,
            'cookie_policy_modes': {
                'yandexuid_val': 'off',
                'yandex_perm': 'off',
                'yandex_root': 'off',
                'yandex_sess': 'off',
                'yandex_js': 'off',
            },
        })

        return [(MetaL7WeightedBackendGroups, properties)]
    else:
        properties.update(knoss_morda_defaults(config_data))
        properties.update({
            'knoss_backend_timeout': '5.5s',  # https://st.yandex-team.ru/MINOTAUR-691
            'knoss_migration_name': 'morda',
        })

        return [(KnossOnly, properties)]


def template_yaru(config_data):
    knoss_options = knoss_morda_defaults(config_data)
    knoss_options.update({
        'knoss_create_headers': OrderedDict([
            ('X-Antirobot-Service-Y', 'yaru')
        ]),
        'service_name': 'yaru',
        'match_uri': '(.*)',
        'onerr_backends': config_data.yalite_backends,
    })

    return [
        (Macroses.CaptchaInRegexp, {
            'backends': config_data.antirobot_backends,
            'connection_attempts': 3,
            'report_stats': True,
            'proxy_options': OrderedDict([
                ('connect_timeout', '0.03s'),
                ('backend_timeout', '10s'),
            ]),
        }),
        (KnossOnly, knoss_options)
    ]


def template_android_widget_api(config_data, morda_service=False):
    properties = {
        'service_name': 'android_widget_api',
        'match_uri': '/android_widget_api(/.*)?',
        'backends_check_quorum': 0.35,
    }

    if morda_service:
        properties.update({
            'weights_key': 'androidwidgetapi',
            'backends_groups': config_data.morda_androidwidget_backends,
            'backend_timeout': '1.5s',
            'laas_backends': config_data.laas_backends,
            'laas_backends_onerror': config_data.laas_backends_onerror,
            'uaas_backends': config_data.uaas_backends,
            'uaas_new_backends': config_data.uaas_new_backends,
            'remote_log_backends': config_data.remote_log_backends,
            'in_dc_attempts': 2,
            'rate_limiter_limit': 0.2,
            'in_dc_connection_attempts': 5,
            'cross_dc_attempts': 2,
            'cross_dc_connection_attempts': 5,
            'connect_timeout': '50ms',
            'generator': 'b2standart',
            'weight_matrix': config_data.morda_weight_matrix,
            'threshold_enable': True,
            'onerr_backend_timeout': '4s',
            'onerr_attempts': 2,
            'onerr_connection_attempts': 5,
            'onerr_backends': config_data.morda_yalite_backends,
            'enable_dynamic_balancing': True,
            'dynamic_balancing_options': OrderedDict([
                ('max_pessimized_share', 0.2)
            ]),
            'enable_cookie_policy': True,
            'cookie_policy_modes': {
                'yandexuid_val': 'off',
                'yandex_perm': 'off',
                'yandex_root': 'off',
                'yandex_sess': 'off',
                'yandex_js': 'off',
            },
        })

        return [(MetaL7WeightedBackendGroups, properties)]
    else:
        properties.update(knoss_morda_defaults(config_data))
        properties.update({
            'knoss_backend_timeout': '5.5s',  # https://st.yandex-team.ru/MINOTAUR-691
            'knoss_migration_name': 'morda',
        })

        return [(KnossOnly, properties)]


# TRAFFIC-1559
def template_wy(config_data, morda_service=False):
    properties = {
        'service_name': 'wy',
        'match_uri': '/(wy)(/.*)?',
        'backends_check_quorum': 0.35,
    }

    if morda_service:
        properties.update({
            'backends_groups': config_data.wy_backends,
            'in_dc_attempts': 2,
            'in_dc_connection_attempts': 2,
            'weight_matrix': config_data.wy_weight_matrix,
            'balancer_type': 'rr',
            'backend_timeout': '5s',
            'generator': 'b2standart',
            'threshold_enable': True,
            'enable_cookie_policy': True,
            'cookie_policy_modes': {
                'yandexuid_val': 'off',
                'yandex_perm': 'off',
                'yandex_root': 'off',
                'yandex_sess': 'off',
                'yandex_js': 'off',
            },
        })

        return [(MetaL7WeightedBackendGroups, properties)]
    else:
        properties.update(knoss_morda_defaults(config_data))
        properties.update({
            'knoss_migration_name': 'morda',
        })

        return [(KnossOnly, properties)]

#!/skynet/python/bin/python
# -*- coding: utf8 -*-
from l7macro import KnossOnly
from collections import OrderedDict


def tmpl_geohelper(config_data):
    return [
        (KnossOnly, {
            'service_name': 'geohelper',
            'match_uri': '/(geohelper)(/.*)?',
            'knoss_backends': config_data.geohelper_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '41s',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
        }),
    ]


def tmpl_ick(config_data):
    return [
        (KnossOnly, {
            'service_name': 'ick',
            'match_uri': '/ick(/.*)?',
            'knoss_backends': config_data.ick_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '30s',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'cookie_policy_modes': {
                'yandexuid_val': 'off',
            },
        }),
    ]


def tmpl_ebell(config_data):
    return [
        (KnossOnly, {
            'service_name': 'bell',
            'match_uri': '/bell(/.*)?',
            'knoss_backends': config_data.ebell_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '5s',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
        }),
    ]


def tmpl_conflagexp(config_data):
    return [
        (KnossOnly, {
            'service_name': 'conflagexp',
            'match_uri': '/conflagexp(/.*)?',
            'knoss_backends': config_data.conflagexp_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '3s',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
        }),
    ]


def tmpl_uslugi(config_data):
    return [
        (KnossOnly, {
            'service_name': 'uslugi',
            'match_uri': '/uslugi(/.*)?',
            'knoss_backends': config_data.uslugi_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '31s',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
        }),
    ]


def tmpl_sport_live(config_data):
    return [
        (KnossOnly, {
            'service_name': 'sport_live',
            'match_uri': '/sport/live(/.*)?',
            'knoss_backends': config_data.sport_live_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '5s',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
        }),
    ]


def tmpl_news(config_data):
    return [
        (KnossOnly, {
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_backends': config_data.news_knoss_backends_yp,
            'knoss_backend_timeout': '5s',
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_balancer_options': OrderedDict([
                ('request', 'GET /slb_ping HTTP/1.1\\n\\n'),
                ('delay', '20s'),
            ]),
            'match_uri': '/({})(/.*)?'.format('|'.join([
                str("news"),
                str("mirror"),
                str("sport"),
                str("hightechnews"),
                str("sciencenews"),
                str("autonews"),
                str("realtynews"),
                str("culturenews"),
                str("ecologynews"),
                str("travelnews"),
                str("showbusinessnews"),
                str("religionnews"),
            ])),
            'service_name': 'news',
        }),
    ]


def tmpl_service_workers(config_data):
    return [
        (KnossOnly, {
            'knoss_backends': config_data.service_workers_knoss_backends,
            'knoss_backend_timeout': '5s',
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'match_uri': '/service-workers(/.*)?',
            'service_name': 'serviceworkers',
        }),
    ]


# MINOTAUR-1606
def tmpl_comments(config_data):
    return [
        (KnossOnly, {
            'knoss_backends': config_data.comments_knoss_backends,
            'knoss_backend_timeout': '7s',
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'match_uri': '/comments(/.*)?',
            'service_name': 'comments',
        }),
    ]


def tmpl_sprav(config_data):
    return [
        (KnossOnly, {
            'service_name': 'spravapi',
            'match_uri': '/spravapi(/.*)?',
            'knoss_backends': config_data.spravapi_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '30s',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
        })
    ]


def tmpl_gnc(config_data):
    return [
        (KnossOnly, {
            'service_name': 'gnc',
            'match_uri': '/gnc(/.*)?',
            'knoss_backends': config_data.gnc_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '31s',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
        }),
    ]


def tmpl_znatoki(config_data):
    return [
        (KnossOnly, {
            'service_name': 'znatoki',
            'match_uri': '/(q|znatoki)(/.*)?',
            'knoss_backends': config_data.znatoki_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '20s',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
        }),
        (KnossOnly, {
            'service_name': 'znatoki-landing',
            'match_uri': '/thequestion(/.*)?',
            'knoss_backends': config_data.znatoki_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '8s',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
        }),
    ]


def tmpl_appcry(config_data):
    return [
        (KnossOnly, {
            'service_name': 'appcry',
            'match_uri': '/appcry(/.*)?',
            'knoss_backends': config_data.cryprox_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '5s',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
        }),
    ]


def tmpl_games(config_data):
    return [
        (KnossOnly, {
            'service_name': 'games',
            'match_uri': '/games(/.*)?',
            'knoss_backends': config_data.games_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '29s',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
        }),
    ]


def tmpl_weather(config_data):
    return [
        (KnossOnly, {
            'service_name': 'weather',
            'match_uri': '/(hava|pogoda|weather)(/.*)?',
            'knoss_backends': config_data.fast_knoss_backends,
            'knoss_migration_name': 'weather',
            'knoss_backends_new': config_data.weather_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '7s',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
        }),
    ]


def tmpl_an(config_data):
    return [
        (KnossOnly, {
            'service_name': 'an',
            'match_uri': '/an(/.*)?',
            'knoss_backends': config_data.an_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '5s',
            'knoss_balancer_options': OrderedDict([
                ('request', 'GET /ping HTTP/1.1\\n\\n'),
                ('delay', '20s'),
            ]),
        }),
    ]

def tmpl_ads(config_data):
    return [
        (KnossOnly, {
            'service_name': 'ads',
            'match_uri': '/ads(/.*)?',
            'knoss_backends': config_data.ads_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '5s',
            'knoss_balancer_options': OrderedDict([
                ('request', 'GET /ping HTTP/1.1\\n\\n'),
                ('delay', '20s'),
            ]),
        }),
    ]

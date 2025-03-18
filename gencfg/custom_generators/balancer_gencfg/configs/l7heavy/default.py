#!/skynet/python/bin/python
# -*- coding: utf8 -*-

from l7macro import KnossOnly, is_term_balancer
from collections import OrderedDict
import src.modules as Modules
import src.constants as Constants
from src.lua_globals import LuaAnonymousKey

# For L7_HEAVY
def template_default(config_data):
    return [
        (KnossOnly, {
            'service_name': 'default',
            'knoss_create_headers': OrderedDict([
                ('X-Antirobot-Default-Path', 'true')  # MINOTAUR-2873
            ]),
            'knoss_backends': config_data.fast_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '30s',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_balancer_options': OrderedDict([
                ('request', Constants.AWACS_BALANCER_HEALTH_CHECK_REQUEST),
                ('delay', '20s'),
            ]),
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
            'host_rewrite': [('xn--d1acpjx3f.xn--p1ai', 'yandex.ru')],
        }),
    ]


def gen_yandex_matcher(config_data):
    match_yandex = OrderedDict([
            ('host', Constants.YANDEX_HOST),
            ('case_insensitive', True)
    ])

    match_geo_only = OrderedDict([
        ('host', Constants.YANDEX_GEO_ONLY_HOST),
        ('case_insensitive', True)
    ])

    if is_term_balancer(config_data) and not config_data.mms:
        return {'match_and': OrderedDict([
            (LuaAnonymousKey(), {'match_fsm': match_yandex}),
            (LuaAnonymousKey(), {'match_not': {'match_fsm': match_geo_only}})
        ])}
    else:
        return {'match_fsm': match_yandex}


def gen_yaru_matcher(_):
    match_yaru = OrderedDict([
            ('host', Constants.YARU_HOST),
            ('case_insensitive', True)
    ])

    return {'match_fsm': match_yaru}


def template_error_default(_):
    return [('default', {}, [
                (Modules.ErrorDocument, {'status': 406, 'force_conn_close': True}),
            ])]


def template_gen_regexp(service_name, match_func, sect_list, config_data):
    modules = list()
    for sect_func in sect_list:
        modules.extend(sect_func(config_data))

    return [(service_name, match_func(config_data) if match_func else {}, [(Modules.RegexpPath, modules)])]

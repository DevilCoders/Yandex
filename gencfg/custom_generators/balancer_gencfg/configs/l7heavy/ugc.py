#!/skynet/python/bin/python
# -*- coding: utf-8 -*-

from l7macro import KnossOnly
from collections import OrderedDict
from src.constants import AWACS_BALANCER_HEALTH_CHECK_REQUEST


def _make_common_options(config_data, args):
    common = {
        'antirobot_backends': config_data.antirobot_backends,
        'antirobot_sink_backends': config_data.antirobot_sink_backends,
        'knoss_backend_timeout': '180s',  # https://st.yandex-team.ru/MINOTAUR-1420
        'knoss_backends': config_data.ugc_knoss_backends,
        'knoss_location': config_data.location,
        'knoss_is_geo_only': config_data.mms,
        'knoss_balancer_options': OrderedDict([
            ('request', AWACS_BALANCER_HEALTH_CHECK_REQUEST),
            ('delay', '20s'),
        ]),
        'knoss_dynamic_balancing_enabled': True,
        'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
    }
    common.update(args)
    return common


def tmpl_ugcpub(config_data):
    return [
        (KnossOnly, _make_common_options(config_data, {
            'service_name': 'ugcpub',
            'match_uri': '/ugcpub(/.*)?',
        }))
    ]


def tmpl_user(config_data):
    return [
        (KnossOnly, _make_common_options(config_data, {
            'service_name': 'user',
            'match_uri': '/user(/.*)?',
        }))
    ]


def tmpl_ugc_my(config_data):
    return [
        (KnossOnly, {
            'service_name': 'ugc_my',
            'match_uri': '/my(/.*)?',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_backend_timeout': '3s',  # https://st.yandex-team.ru/MINOTAUR-691
            'knoss_backends': config_data.ugc_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_balancer_options': OrderedDict([
                ('request', AWACS_BALANCER_HEALTH_CHECK_REQUEST),
                ('delay', '20s'),
            ]),
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
        })
    ]

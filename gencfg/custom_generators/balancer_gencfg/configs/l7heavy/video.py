#!/skynet/python/bin/python
# -*- coding: utf-8 -*-

from l7macro import KnossOnly
from collections import OrderedDict
from src.constants import AWACS_BALANCER_HEALTH_CHECK_REQUEST


def tmpl_video(config_data):
    return [
        (KnossOnly, {
            'service_name': 'video_api',
            'match_uri': '/video/(api|quasar)(/.*)?',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
            'knoss_backends': config_data.video_yp_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_balancer_options': OrderedDict([
                ('request', AWACS_BALANCER_HEALTH_CHECK_REQUEST),
                ('delay', '20s'),
            ]),
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
        }),
        (KnossOnly, {
            'service_name': 'video-xml',
            'match_uri': '/video-xml(/.*)?',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'onerr_backends': config_data.yalite_backends,
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
            'knoss_backends': config_data.video_yp_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_balancer_options': OrderedDict([
                ('request', AWACS_BALANCER_HEALTH_CHECK_REQUEST),
                ('delay', '20s'),
            ]),
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
        }),
        (KnossOnly, {
            'service_name': 'video',
            'match_uri': '/video(/.*)?',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'onerr_backends': config_data.yalite_backends,
            'knoss_backend_timeout': '51s',  # https://st.yandex-team.ru/MINOTAUR-691
            'knoss_backends': config_data.video_yp_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_balancer_options': OrderedDict([
                ('request', AWACS_BALANCER_HEALTH_CHECK_REQUEST),
                ('delay', '20s'),
            ]),
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
        }),
    ]

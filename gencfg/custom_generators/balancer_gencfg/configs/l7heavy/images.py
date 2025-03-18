#!/skynet/python/bin/python
# -*- coding: utf-8 -*-

import src.constants as Constants

from l7macro import KnossOnly
from collections import OrderedDict


def tmpl_images(config_data):
    return [
        (KnossOnly, {
            'service_name': 'images-xml',
            'match_uri': '/images-xml(/.*)?',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_backends': config_data.images_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '41s',  # https://st.yandex-team.ru/MINOTAUR-691
            'knoss_balancer_options': OrderedDict([
                ('request', Constants.AWACS_BALANCER_HEALTH_CHECK_REQUEST),
                ('delay', '20s'),
            ]),
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
        }),
        (KnossOnly, {
            'service_name': 'images-apphost',
            'match_uri': '/images-apphost(/.*)?',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_backends': config_data.images_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '41s',  # https://st.yandex-team.ru/MINOTAUR-691
            'knoss_balancer_options': OrderedDict([
                ('request', Constants.AWACS_BALANCER_HEALTH_CHECK_REQUEST),
                ('delay', '20s'),
            ]),
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
        }),
        (KnossOnly, {
            'service_name': 'images',
            'match_uri': Constants.IMAGES_URI + '|' + Constants.IMAGES_TODAY_URI,  # может объеденить в один?
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_backends': config_data.images_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '121s',  # https://st.yandex-team.ru/MINOTAUR-691
            'knoss_balancer_options': OrderedDict([
                ('request', Constants.AWACS_BALANCER_HEALTH_CHECK_REQUEST),
                ('delay', '20s'),
            ]),
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
        })
    ]


def tmpl_collections(config_data):
    return [
        (KnossOnly, {
            'service_name': 'collections_api',
            'match_uri': '/collections/api(/.*)?',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_backend_timeout': '16s',  # https://st.yandex-team.ru/MINOTAUR-691
            'knoss_backends': config_data.collections_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
        }),
        (KnossOnly, {
            'service_name': 'collections',
            'match_uri': '/collections?(/.*)?',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_backend_timeout': '11s',  # https://st.yandex-team.ru/MINOTAUR-691
            'knoss_backends': config_data.collections_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
        }),
        (KnossOnly, {
            'service_name': 'feed',
            'match_uri': '/feed(/.*)?',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_backends': config_data.collections_knoss_backends,
            'knoss_backend_timeout': '11s',
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
        }),
    ]

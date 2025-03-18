#!/skynet/python/bin/python
# -*- coding: utf-8 -*-

from l7macro import KnossOnly
from collections import OrderedDict


def tmpl_messenger_api(config_data):
    return [
        (KnossOnly, {
            'service_name': 'messenger_api',
            'match_uri': '/messenger/api(/.*)?',
            'knoss_backends': config_data.messenger_api_knoss_backends,
            'knoss_backend_timeout': '5s',
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_balancer_options': OrderedDict([
                ('request', 'GET /ping HTTP/1.1\\n\\n'),
                ('delay', '20s'),
            ]),



        }),
    ]


def tmpl_messenger_api_alpha(config_data):
    return [
        (KnossOnly, {
            'service_name': 'messenger_api_alpha',
            'match_uri': '/messenger/api/alpha(/.*)?',
            'knoss_backends': config_data.messenger_api_alpha_knoss_backends,
            'knoss_backend_timeout': '5s',
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_balancer_options': OrderedDict([
                ('request', 'GET /ping HTTP/1.1\\n\\n'),
                ('delay', '20s'),
            ]),
        }),
    ]

#!/skynet/python/bin/python
# -*- coding: utf8 -*-
from l7macro import KnossOnly


def tmpl_maps(config_data):
    return [
        (KnossOnly, {
            'service_name': 'maps',
            'match_uri': '/(maps|harita|web-maps|navi|metro|profile)(/.*)?',  # SEPE-14566
            'knoss_backends': config_data.maps_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_backend_timeout': '11s',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
        }),
    ]


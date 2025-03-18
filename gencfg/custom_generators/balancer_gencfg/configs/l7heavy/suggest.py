#!/skynet/python/bin/python
# -*- coding: utf8 -*-

import src.constants as Constants
from l7macro import KnossOnly


def template_suggest(config_data):
    def _common(config_data, options):
        ret = {
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_backends': config_data.suggest_knoss_backends,
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
            'cookie_policy_modes': {'eprivacy_client': 'off'},
        }
        ret.update(options)
        return ret

    return [
        (KnossOnly, _common(config_data, {
            'service_name': 'suggest-images',
            'match_uri': '/suggest-images(/.*)?',
            'knoss_backend_timeout': '3s',  # https://st.yandex-team.ru/MINOTAUR-691
        })),
        (KnossOnly, _common(config_data, {
            'service_name': 'suggest-video',
            'match_uri': '/suggest-video(/.*)?',
            'knoss_backend_timeout': '3s',  # https://st.yandex-team.ru/MINOTAUR-691
        })),
        (KnossOnly, _common(config_data, {
            'service_name': 'suggest_market',
            'match_uri': Constants.SUGGEST_MARKET_URI,
            'knoss_backend_timeout': '1s',  # https://st.yandex-team.ru/MINOTAUR-691
        })),
        (KnossOnly, _common(config_data, {
            'service_name': 'suggest_history',
            'match_uri': Constants.SUGGEST_HISTORY_URI,
            'knoss_backend_timeout': '1s',  # https://st.yandex-team.ru/MINOTAUR-691
        })),
        (KnossOnly, _common(config_data, {
            'service_name': 'suggest_exprt',
            'match_uri': Constants.SUGGEST_EXPRT_URI,
            'knoss_backend_timeout': '1s',  # https://st.yandex-team.ru/MINOTAUR-691
        })),
        (KnossOnly, _common(config_data, {
            'service_name': 'suggest_spok',
            'match_uri': Constants.SUGGEST_SPOK_URI,
            'knoss_backend_timeout': '1s',  # https://st.yandex-team.ru/MINOTAUR-691
        })),
        (KnossOnly, _common(config_data, {
            'service_name': 'suggest',
            'match_uri': Constants.SUGGEST_COMMON_URI,
            'knoss_backend_timeout': '1s',  # https://st.yandex-team.ru/MINOTAUR-691
        })),
    ]

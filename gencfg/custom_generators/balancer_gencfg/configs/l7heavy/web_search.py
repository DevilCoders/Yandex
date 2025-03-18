#!/skynet/python/bin/python
# -*- coding: utf8 -*-

import src.macroses as Macroses
import src.modules as Modules

from l7macro import KnossOnly
from collections import OrderedDict
from src.lua_globals import LuaGlobal
from src.constants import AWACS_BALANCER_HEALTH_CHECK_REQUEST


def template_web_search(config_data):
    clck_on_error_module = [
        (Modules.Report, {'uuid': 'clck_requests_to_knoss_onerror'}),
        (Modules.Balancer2, {
            'policies': OrderedDict([('unique_policy', {})]),
            'balancer_type': 'rr',
            'attempts': 1,
            'balancer_options': OrderedDict([('weights_file', './controls/click_switch.txt')]),
            'custom_backends': [
                (1., 'clcklib', [
                    (Modules.Click, {
                        'keys': LuaGlobal('ClickDaemonKeys', '/dev/shm/certs/priv/clickdaemon.keys'),
                        'json_keys': LuaGlobal('ClickDaemonJsonKeys', '/dev/shm/certs/priv/clickdaemon.json_keys'),
                    }),
                    (Modules.ErrorDocument, {'status': 200}),
                ]),
                (-1, 'clckflbck', [
                    (Modules.ErrorDocument, {'status': 200})
                ]),
            ]
        })
    ]

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
        (KnossOnly, {
            'service_name': 'clck',
            'match_uri': '/clck(/.*)',
            'antirobot_backends': config_data.antirobot_backends,
            'antirobot_sink_backends': config_data.antirobot_sink_backends,
            'knoss_on_error_modules': clck_on_error_module,
            'knoss_backends': config_data.clck_knoss_backends,
            'knoss_backends_new': config_data.clck_knoss_backends,  # TODO(velavokr): keeping to debug BALANCER-3085
            'knoss_backend_timeout': '6s',  # https://st.yandex-team.ru/MINOTAUR-691
            'knoss_location': config_data.location,
            'knoss_is_geo_only': config_data.mms,
            'knoss_local_proxy_options': OrderedDict([
                ('keepalive_count', 1)
            ]),
            'knoss_balancer_options': OrderedDict([
                ('request', AWACS_BALANCER_HEALTH_CHECK_REQUEST),
                ('delay', '20s'),
                ('use_backend_weight', True),
                ('backend_weight_disable_file', './controls/disable_backend_weight_in_active'),
            ]),
            'knoss_dynamic_balancing_enabled': True,
            'knoss_dynamic_balancing_options': config_data.dynamic_balancing_options,
            'cookie_policy_modes': {'eprivacy_client': 'off', 'is_gdpr': 'off', 'is_gdpr_b': 'off'},
        }),
        ('postedit', {'pattern': '/edit'}, [
            (Modules.ErrorDocument, {'status': 403}),
        ]),
    ]

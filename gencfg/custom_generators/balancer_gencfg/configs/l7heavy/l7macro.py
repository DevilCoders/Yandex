#!/skynet/python/bin/python
# -*- coding: utf-8 -*-

import copy
import re
from collections import OrderedDict

import src.macroses as Macroses
import src.constants as Constants
import src.modules as Modules

from urllib import urlencode
from src.lua_globals import LuaGlobal, LuaAnonymousKey, LuaProdOrTesting


"""
Модуль куда  ожидается свести все лишние макросы и уменьшить в l7 copy-paste.
"""

def _uaas_headers_size_limit(headers_size_limit, exp_service_name):
    if headers_size_limit is not None:
        return headers_size_limit
    default_values = {
        "web": 61440,
        "images": 61440,
        "video": 61440,
        "turbo": 61440,
        "news": 61440,
        "uslugi": 61440,
        "chat": 61440,
        "blogs": 61440,
        "people": 61440,
        "profile": 61440,
        "searchapp": 61440,
        "dbrowser": 61440,
        "morda": 51200,
        "browser": 51200,
        "disk": 30720,
        "mail": 30720,
        "mail-mobile-apps": 30720,
        "auto_ru": 20480,
        "autoru_app": 20480,
        "market": 15360,
        "marketapps": 15360,
        "blue-market": 15360,
        "bluemarketapps": 15360,
        "passport": 14336,
        "uniproxy": 14336,
        "answers": 8192,
        "realty": 8192,
        "realty_app": 8192,
        "collections": 8192,
        "pogoda": 8192,
        "hava": 8192,
        "weather": 8192,
        "maps": 8192,
    }
    return default_values.get(exp_service_name, None)


def is_knoss_balancer(config_data):
    return config_data.custom.get('is_service_balancer', False)


def is_term_balancer(config_data):
    return not is_knoss_balancer(config_data)


def is_exp_balancer(config_data):
    return config_data.custom.get('is_exp_balancer', False)


def raise_for_l7heavy_incompatible_section_id(section_id):
    """
    Проверяет, что @section_id может быть использован как имя секции в няня-интерфейса управления весами l7heavy.
    """
    pattern = '^[a-z][a-z0-9]+$'

    if not re.match(pattern, section_id):
        raise Exception(
            "{!r} can't be used as a section id in nanny/l7heavy weights control interface. "
            "Section id must match {!r}.".format(
                section_id, pattern)
        )


def get_report_service_name(options):
    return options['report_uuid'] if options.get('report_uuid') is not None else options['service_name']


def get_report_options(pattern, options):
    uuid = get_report_service_name(options)
    report_uuid = pattern % uuid

    if options.get('report_dc_uuid_suffix') is not None:
        report_uuid = '%s_%s' % (report_uuid, options['report_dc_uuid_suffix'])

    return {'uuid': report_uuid}


class Laas(Macroses.IMacro):
    def __init__(self):
        Macroses.IMacro.__init__(self)

    @staticmethod
    @Macroses.Helper(
        'Laas',
        '''
            Location-as-a-Service
        ''',
        [
            ('laas_backends', None, list, True, ''),
            ('laas_backends_onerror', None, list, True, ''),
            ('processing_time_header', False, bool, False, ''),
        ]
    )
    def generate(options):
        modules_onerror = [
            (Modules.Report, {'uuid': 'l7heavygeobasemodule_fallback'}),
            (Modules.Balancer2, {
                'backends': options['laas_backends_onerror'],
                'balancer_type': 'rr',
                'attempts': 1,
                'policies': OrderedDict([
                    ('unique_policy', {})
                ]),
                'check_backends': OrderedDict([
                    ('name', Modules.gen_check_backends_name(options['laas_backends_onerror'])),
                    ('quorum', 0.35),
                ]),
                'proxy_options': OrderedDict([
                    ('connect_timeout', '0.015s'),
                    ('backend_timeout', '0.02s'),
                    ('keepalive_count', 0),
                ]),
            })
        ]

        backends = (Modules.Balancer2, {
            'backends': options['laas_backends'],
            'balancer_type': 'hashing',
            'attempts': 2,
            'connection_attempts': 3,
            'policies': OrderedDict([('unique_policy', {})]),
            'attempts_limit': 0.3,
            'proxy_options': OrderedDict([
                ('connect_timeout', '0.015s'),
                ('backend_timeout', LuaGlobal('LaasTimeout', '0.03s')),
                ('keepalive_count', 2),
            ]),
            'check_backends': OrderedDict([
                ('name', Modules.gen_check_backends_name(options['laas_backends'])),
                ('quorum', 0.35),
            ]),
            'on_error': modules_onerror
        })

        modules = [
            (Modules.Report, {'uuid': 'l7heavygeobasemodule'}),
            (Modules.Regexp, [
                ('bigb-uid', {'match_fsm': OrderedDict(cgi='[?](.*&)?bigb-uid=.*')}, [
                    (Modules.CgiHasher, {'mode': 'priority', 'parameters': [
                        'bigb-uid',
                    ]}),
                    backends
                ]),
                ('yandexuid', {'match_fsm': OrderedDict(cookie='yandexuid=[0-9]+')}, [
                    (Modules.CookieHasher, {'cookie': 'yandexuid'}),
                    backends
                ]),
                ('default', {}, [
                    (Modules.CgiHasher, {'mode': 'priority', 'parameters': [
                        'puid', 'idfa', 'gaid', 'oaid', 'uuid', 'duid',
                    ]}),
                    backends
                ])
            ]),
        ]

        result = [
            (Modules.Geobase, {
                'file_switch': './controls/disable_geobase.switch',
                'geo': modules,
                'processing_time_header': options['processing_time_header']
            })
        ]

        return result


_default_rate_limiter_options = {
    'rate_limiter': (True, bool, False, 'Включить/выключить ограничитель перезапросов'),
    'rate_limiter_limit': (0.1, float, False, 'Максимально допустимое отношение количества перезапросов к запросам'),
    'rate_limiter_coeff': (None, float, False, 'Коэффициент забвения для ограничителя перезапросов'),
    'rate_limiter_max_budget': (None, int, False, 'Максимальный бюджет ограничителя презапросов')
}


def add_rate_limiter_options(options):
    """
    Adds rate limiter options to a list of options for macro options checker helper,
    if an option is not present.
    """
    default_options = dict(_default_rate_limiter_options)

    for item in options:
        option_name = item[0]
        if option_name in default_options:
            default_options.pop(option_name)

    for (option, params) in default_options.iteritems():
        options.append((option, params[0], params[1], params[2], params[3]))

    return options


def copy_rate_limiter_options(new_options, options):
    for opt in _default_rate_limiter_options.keys():
        old_opt = copy.deepcopy(options[opt])
        if old_opt is not None:
            new_options[opt] = old_opt


def update_rate_limiter_options(balancer2_options, options):
    """
    Updates balancer2 options dictinary with rate limiter options.

    :param balancer2_options: balancer2 options to update
    :param options: options for rate limiter
    """

    max_attempts = balancer2_options['attempts']

    if max_attempts == 1 or not options['rate_limiter']:
        return

    balancer2_options['attempts_limit'] = options['rate_limiter_limit']
    balancer2_options['rate_limiter_max_budget'] = options['rate_limiter_max_budget']
    balancer2_options['rate_limiter_coeff'] = options['rate_limiter_coeff']


class BackendsRegexp(object):
    def __init__(self, regexp):
        self.regexp = regexp


class BackendsLocation:
    def __init__(self, backends, balancer2_options):
        self.backends = backends
        self.balancer2_options = balancer2_options


def generate_backend_modules(backends, balancer2_generator, report_generator=None):
    if type(backends) is BackendsRegexp:
        regexp_list = []
        for name, matcher, inner_backends in backends.regexp:
            regexp_list.append(
                (name, matcher, generate_backend_modules(inner_backends, balancer2_generator, report_generator))
            )
        return [(Modules.Regexp, regexp_list)]
    else:
        modules = [] if report_generator is None else report_generator()
        modules.append((Modules.Balancer2, balancer2_generator(backends)))
        return modules


def generate_on_error(options, on_error_attempts=None, report_options=None, connection_attempts=None, backend_timeout=None, balancer_type=None):
    if on_error_attempts is None:
        on_error_attempts = 1

    def balancer2_options(backends):
        if isinstance(backends, BackendsLocation):
            backends = backends.backends
        res = {
            'backends': backends,
            'balancer_type': balancer_type,
            'policies': OrderedDict([
                ('retry_policy', {
                    'unique_policy': {}
                })]),
            'attempts': on_error_attempts,
            'connection_attempts': connection_attempts,
            'proxy_options': OrderedDict([
                ('connect_timeout', '0.1s'),
                ('backend_timeout', backend_timeout if backend_timeout else options['backend_timeout']),
            ]),
        }
        if options['backends_check_quorum']:
            res['check_backends'] = OrderedDict([
                ('quorum', options['backends_check_quorum']),
                ('name', Modules.gen_check_backends_name(backends)),
            ])
            if options.get('backends_check_hysteresis'):
                res['check_backends']['hysteresis'] = options['backends_check_hysteresis']
        return res

    def report_generator():
        modules = [
            (Modules.Report, report_options if report_options else get_report_options('%s_requests_to_onerror', options)),
        ]

        return modules

    if not options.get('onerr_backends'):
        return

    onerr_backends = options['onerr_backends']
    if options.get('add_geo_only_matcher'):
        onerr_backends = BackendsRegexp([
            (
                'not_geo_only',
                {
                    'match_not': OrderedDict([
                        ('match_fsm', OrderedDict([
                            ('header', {'name': 'X-Yandex-Balancing-Hint', 'value': '.*'})
                        ])),
                    ]),
                },
                onerr_backends,
            ),
        ])

        return generate_backend_modules(onerr_backends, balancer2_options, report_generator)
    else:
        return report_generator() + generate_backend_modules(onerr_backends, balancer2_options)


def add_geo_only_matcher(policies, backends_groups, weights_key, override_balancing_hints):
    hints = []

    override_balancing_hints = override_balancing_hints or {}

    for dcname in backends_groups.keys():
        target_dc = override_balancing_hints.get(dcname)
        if target_dc is None:
            target_dc = dcname

        assert target_dc in backends_groups.keys()

        hints.append(OrderedDict([
            ('backend', '%s_%s' % (weights_key, target_dc)),
            ('hint', dcname),
        ]))

    return [
        ('by_name_from_header_policy', OrderedDict([
            ('hints', hints),
            ('allow_zero_weights', True),
            ('strict', True),
        ] + policies))
    ]


def add_rps_limiter_backends(rps_limiter_options, backends, location):
    balancer2 = {
        'backends': backends,
        'attempts': 1,
        'connection_attempts': 1,
        'policies': OrderedDict([
            ('unique_policy', {}),
        ]),
        'proxy_options': OrderedDict([
            ('connect_timeout', '50ms'),
            ('backend_timeout', '100ms'),
        ]),
        'check_backends': OrderedDict([
            ('name', Modules.gen_check_backends_name(backends)),
            ('quorum', 0.35),
            ('hysteresis', 0.05),
        ])
    }

    balancer2 = _add_dynamic_balancing(balancer2)

    report_uuids = ['rpslimiter', 'rpslimiter_' + location]
    if rps_limiter_options.get('register_only', False):
        report_uuids.append('rpslimiter_register_only')

    rps_limiter_options['checker'] = [
        (Modules.Report, {'uuid': ','.join(report_uuids)}),
        (Modules.Balancer2, balancer2)
    ]


class GenB2WeightedHashing(Macroses.IMacro):
    def __init__(self):
        Macroses.IMacro.__init__(self)

    @staticmethod
    @Macroses.Helper(
        'GenB2WeightedHashing',
        '''
            Балансировка по локациям для сервисов с учетом весов и хеша.
        ''',
        add_rate_limiter_options([
            ('backends_groups', None, OrderedDict, False, 'Бекенды сервиса'),
            ('onerr_backends', None, list, False, 'Бекенды fallback'),
            ('on_error_modules', None, list, False, 'Кастомный fallback'),
            ('connect_timeout', None, str, False, 'Таймаут на соединение с бекендами сервиса'),
            ('backend_timeout', None, str, False, 'Таймаут на обмен данными с бекендами сервиса'),
            ('cross_dc_attempts', None, int, False, 'Количество попыток между датацентрами'),
            ('cross_dc_connection_attempts', None, int, False, 'Количество попыток между датацентрами'),
            ('in_dc_attempts', None, int, False, 'Количество попыток на обмен данными внутри датацентра'),
            ('in_dc_connection_attempts', None, int, False, 'Количество попыток на соединение c бэкендами сервиса'),
            ('weight_matrix', None, OrderedDict, True, 'Матрица статических весов'),
            ('weights_file', Constants.L7_WEIGHT_SWITCH_FILE, str, False, 'Файл с весами'),
            (
                'weights_key', None, str, False,
                'Ключ секции с весами, нужен управления разными сервисами из одного окна админки',
            ),
            (
                'buffering', False, bool, False,
                'Параметр buffering для модуля proxy. Если включен, '
                'то балансер полностью получит ответ от бэкэнда, прежде чем начнет передавать его клиенту.',
            ),
            ('service_name', None, str, True, 'Имя сервиса'),
            ('balancer_type', None, str, True, 'Тип балансировки'),
            ('active_request', None, str, False, 'Url активной проверки'),
            ('active_delay', None, str, False, 'время между активными проверками'),
            ('hashing_by', None, str, False, 'выбор метода хэширования'),
            ('report_uuid', None, str, False, 'Идентификатор модуля Report, в который отправляется статистика.'),
            ('report_dc_uuid_suffix', None, str, False, 'Суффикс для полокационных модулей Report.'),
            ('local_balancer_type', 'rr', str, False, 'Тип балансировки на конечных бекендах(по дефолту rr'),
            ('add_geo_only_matcher', None, bool, False, 'Добавить матчер по X-Yandex-Balancing-Hint'),
            ('enable_dynamic_balancing', False, bool, False, 'Включить динамическую балансировку'),
            ('dynamic_balancing_options', None, OrderedDict, False, 'Дополнительные опции для динамической балансировки'),
            ('rps_limiter', None, OrderedDict, False, 'Опции модуля rpslimiter перед каждым запросом'),
            ('by_dc_rps_limiter', None, OrderedDict, False, 'Опции модуля rpslimiter перед попыткой в каждый ДЦ'),
            ('rps_limiter_location', None, str, False, 'Локация, в которую пойдет первая попытка для rpslimiter'),
            ('rps_limiter_namespace', None, str, False, 'Секция в rpslimiter'),
            ('rps_limiter_backends', None, list, False, 'Бэкенды rpslimiter'),
            ('rps_limiter_disable_file', None, str, False, 'Файл для отключения rpslimiter'),
            ('override_balancing_hints', None, dict, False, 'Принудительно перезаписать подсказки балансировки'),
            ('pumpkin_prefetch', False, bool, False, 'Включить рубильник для походов в тыкву'),
            ('pumpkin_prefetch_timeout', None, str, False, 'Таймаут для префетча в тыкву'),
            ('backends_check_quorum', None, float, False, 'Кворум для проверки бэкендов на живость в prepare'),
        ])
    )
    def generate(options):
        custom_backends = list()

        if options['weights_key']:
            weights_key = options['weights_key']
        else:
            weights_key = options['service_name']
        raise_for_l7heavy_incompatible_section_id(weights_key)

        pumpkin_prefetch_modules = None
        if options['pumpkin_prefetch']:
            assert options['pumpkin_prefetch_timeout'] is not None
            report_options = get_report_options('%s_requests_to_onerror', options)
            report_options['uuid'] += ',pumpkin_prefetch,{}_pumpkin_prefetch'.format(get_report_service_name(options))

            # For details see: MINOTAUR-2492
            on_error_options = copy.deepcopy(options)
            on_error_options['add_geo_only_matcher'] = False

            pumpkin_prefetch_modules = generate_on_error(
                on_error_options,
                report_options=report_options,
                backend_timeout=options['pumpkin_prefetch_timeout'],
            )

        for dcname in options['backends_groups'].keys():
            assert dcname != 'devnull'

            backend_name = '%s_%s' % (weights_key, dcname)
            current_weight = options['weight_matrix'][dcname]

            def balancer2_generator(backends):
                balancer2_backends_options = {}

                if isinstance(backends, BackendsLocation):
                    balancer2_backends_options = copy.deepcopy(backends.balancer2_options) or {}
                    backends = backends.backends

                balancer2_options = {
                    'backends': backends,
                    'policies': OrderedDict([('unique_policy', {})]),
                    'attempts': options['in_dc_attempts'],
                    'connection_attempts': options['in_dc_connection_attempts'],
                    'proxy_options': OrderedDict([
                        ('connect_timeout', options['connect_timeout']),
                        ('backend_timeout', options['backend_timeout']),
                    ]),
                }

                if options['backends_check_quorum']:
                    balancer2_options['check_backends'] = OrderedDict([
                        ('name', Modules.gen_check_backends_name(backends)),
                        ('quorum', options['backends_check_quorum']),
                    ])

                balancer_type = options["local_balancer_type"]

                if 'balancer_type' in balancer2_backends_options:
                    balancer_type = balancer2_backends_options['balancer_type']

                if balancer_type == "rr":
                    balancer2_options['balancer_type'] = 'rr'
                elif balancer_type == "weighted2":
                    balancer2_options['balancer_type'] = 'weighted2'
                    balancer2_options['balancer_options'] = OrderedDict([
                        ('slow_reply_time', '1.0s'),
                        ('correction_params', OrderedDict([
                            ('min_weight', '0.05'),
                            ('max_weight', '5.0'),
                            ('plus_diff_per_sec', '0.05'),
                            ('minus_diff_per_sec', '0.2'),
                            ('history_time', '60s'),
                            ('feedback_time', '300s'),
                        ])),
                    ])

                balancer2_options.update(balancer2_backends_options)

                if 'pumpkin' not in dcname:
                    update_rate_limiter_options(balancer2_options, options)

                if options['enable_dynamic_balancing']:
                    balancer2_options = _add_dynamic_balancing(balancer2_options, options['dynamic_balancing_options'])

                if 'pumpkin' not in dcname and pumpkin_prefetch_modules is not None:
                    balancer2_options = _add_pumpkin_prefetch(balancer2_options, pumpkin_prefetch_modules)

                return balancer2_options

            current_modules = [
                (Modules.Report, get_report_options('%s_requests_to_' + dcname, options)),
            ] + generate_backend_modules(options['backends_groups'][dcname], balancer2_generator)

            if options['by_dc_rps_limiter']:
                rps_limiter_options = copy.deepcopy(options['by_dc_rps_limiter'])

                add_rps_limiter_backends(rps_limiter_options, options['rps_limiter_backends'], options['rps_limiter_location'])

                rps_limiter_options['module'] = copy.deepcopy(current_modules)
                rps_limiter_options['skip_on_error'] = True

                if 'pumpkin' not in dcname:
                    report_options = get_report_options('%s_requests_to_onerror', options)
                    report_options['uuid'] += ',limited_request,{}_limited_request'.format(get_report_service_name(options))

                    on_error_options = copy.deepcopy(options)
                    on_error_options['add_geo_only_matcher'] = False
                    rps_limiter_options['on_limited_request'] = generate_on_error(
                        on_error_options,
                        report_options=report_options,
                        connection_attempts=3
                    )

                current_modules = [
                    (Modules.Headers, {'create': OrderedDict([
                        ('X-Rpslimiter-DC', dcname),
                        ('X-Knoss-Upstream', options['service_name']),
                    ])}),
                    (Modules.RpsLimiter, rps_limiter_options),
                ]

            custom_backends.append(
                (current_weight, backend_name, current_modules)
            )

        custom_backends.append((-1, '%s_devnull' % weights_key, [
            (Modules.Report, get_report_options('%s_requests_to_devnull', options)),
            (Modules.ErrorDocument, {'status': 204}),
        ]))

        top_balancer_policies = [
            ('by_hash_policy', {
                'unique_policy': {}
            })
        ]

        if options['add_geo_only_matcher']:
            top_balancer_policies = add_geo_only_matcher(
                top_balancer_policies,
                options['backends_groups'],
                weights_key,
                options['override_balancing_hints']
            )

        top_balancer_options = {
            'attempts': options['cross_dc_attempts'],
            'attempts_file': './controls/l7_cross_dc_attempts',
            'balancer_type': 'rr',
            'balancer_options': OrderedDict([
                ('weights_file', options['weights_file'])
            ]),
            'policies': OrderedDict(top_balancer_policies),
            'custom_backends': custom_backends
        }

        if options['cross_dc_connection_attempts']:
            top_balancer_options['connection_attempts'] = options['cross_dc_connection_attempts']

        on_error = generate_on_error(options, connection_attempts=3)
        if on_error is not None:
            top_balancer_options['on_error'] = on_error

        # overriding the on_error section generated above
        if options['on_error_modules'] is not None:
            top_balancer_options['on_error'] = options['on_error_modules']

        modules = list()

        if options['hashing_by'] is not None:
            hashing_by = options['hashing_by']
            if hashing_by == 'text':
                modules.append((Modules.Hasher, {
                    'mode': 'text',
                }))
            elif hashing_by == 'UID':
                modules.append((Modules.HeadersHasher, {
                    'header_name': 'X-Yandex-LogstatUID',
                    'randomize_empty_match': True,
                }))
            elif hashing_by == 'ICookie':
                modules.append((Modules.HeadersHasher, {
                    'header_name': 'X-Yandex-ICookie',
                    'randomize_empty_match': True,
                }))
            else:
                raise NotImplementedError
        else:
            raise Exception('Using hashing generator type, but "hashing_by" option is not set')

        modules.append((Modules.Balancer2, top_balancer_options))

        return modules


class GenB2Standart(Macroses.IMacro):
    def __init__(self):
        Macroses.IMacro.__init__(self)

    @staticmethod
    @Macroses.Helper(
        'GenB2Standart',
        '''
        Full Mesh по локация с B2  и unique_policy
        ''',
        add_rate_limiter_options([
            ('backends_groups', None, OrderedDict, False, 'Бекенды сервиса'),
            ('onerr_backends', None, list, False, 'Бекенды fallback'),
            ('on_error_modules', None, list, False, 'Кастомный fallback'),
            ('connect_timeout', None, str, False, 'Таймаут на соединение с бекендами сервиса'),
            ('backend_timeout', None, str, False, 'Таймаут на обмен данными с бекендами сервиса'),
            ('cross_dc_attempts', None, int, False, 'Количество попыток между датацентрами'),
            ('cross_dc_connection_attempts', None, int, False, 'Количество попыток на соединение между датацентрами'),
            ('in_dc_attempts', None, int, False, 'Количество попыток на обмен данными внутри датацентра'),
            ('in_dc_connection_attempts', None, int, False, 'Количество попыток на соединение c бэкендами сервиса'),
            ('weight_matrix', None, OrderedDict, True, 'Матрица статических весов'),
            ('weights_file', Constants.L7_WEIGHT_SWITCH_FILE, str, False, 'Файл с весами'),
            ('weights_key', None, str, False,
                'ключ секции с весами, нужен управления разными сервисами из одного окна админки'),
            ('buffering', False, bool, False,
                'Параметр buffering для модуля proxy. Если включен, то балансер полностью получит ответ от бэкэнда, '
                'прежде чем начнет передавать его клиенту.'),
            ('balancer_type', None, str, True, 'Тип балансировки'),
            ('active_request', None, str, False, 'Url активной проверки'),
            ('active_delay', None, str, False, 'время между активными проверками'),
            ('service_name', None, str, True, 'Имя директории в няне для кондуктора'),
            ('report_uuid', None, str, False, 'Идентификатор модуля Report, в который отправляется статистика'),
            ('retry_non_idempotent', None, bool, False, 'Делать перепопытки для неидемпотентных запросов'),
            ('onerr_backend_timeout', None, str, False, 'Таймаут на обмен данными с бекендами fallback'),
            ('onerr_connection_attempts', None, int, False, 'Количество попыток на соединение c бэкендами fallback'),
            ('onerr_attempts', None, int, False, 'Количество попыток на обмен данными c бэкендами fallback'),
            ('add_geo_only_matcher', None, bool, False, 'Добавить матчер по X-Yandex-Balancing-Hint'),
            ('enable_dynamic_balancing', False, bool, False, 'Включить динамическую балансировку'),
            ('dynamic_balancing_options', None, OrderedDict, False, 'Дополнительные опции для динамической балансировки'),
            ('by_dc_rps_limiter', None, OrderedDict, False, 'Опции модуля rps_limiter перед попыткой в каждый ДЦ'),
            ('rps_limiter', None, OrderedDict, False, 'Параметры для хождения в rpslimiter перед каждым запросом в балансер'),
            ('rps_limiter_namespace', None, str, False, 'Название секции в rps_limiter'),
            ('rps_limiter_location', None, str, False, 'ДЦ для первой попытки в rpslimiter'),
            ('rps_limiter_disable_file', None, str, False, 'Файл для отключения rpslimiter'),
            ('rps_limiter_backends', None, list, False, ''),
            ('pumpkin_prefetch', False, bool, False, 'Включить рубильник для походов в тыкву'),
            ('pumpkin_prefetch_timeout', None, str, False, 'Таймаут для префетча в тыкву'),
            ('backends_check_quorum', None, float, False, 'Кворум для проверки бэкендов на живость в prepare'),
            ('report_dc_uuid_suffix', None, str, False, 'Суффикс для полокационных модулей Report.'),
        ])
    )
    def generate(options):
        modules = list()
        custom_backends = list()

        if options['weights_key']:
            weights_key = options['weights_key']
        else:
            weights_key = options['service_name']
        raise_for_l7heavy_incompatible_section_id(weights_key)

        for dcname, backends in options['backends_groups'].items():
            assert dcname != 'devnull'

            balancer_type = options['balancer_type']
            local_balancer_options = OrderedDict()
            balancer2_backends_options = {}

            if isinstance(backends, BackendsLocation):
                balancer2_backends_options = copy.deepcopy(backends.balancer2_options) or {}
                backends = backends.backends

            if balancer2_backends_options:
                balancer_type = balancer2_backends_options['balancer_type']

            if balancer_type == 'active':
                local_balancer_options['request'] = options['active_request']
                local_balancer_options['delay'] = options['active_delay']

            backend_name = '%s_%s' % (weights_key, dcname)
            current_weight = options['weight_matrix'][dcname]

            current_balancer_options = {
                'backends': backends,
                'policies': OrderedDict([('unique_policy', {})]),
                'attempts': options['in_dc_attempts'],
                'connection_attempts': options['in_dc_connection_attempts'],
                'attempts_file': './controls/%s.attempts' % options['service_name'],
                'balancer_type': options['balancer_type'],
                'retry_non_idempotent': options['retry_non_idempotent'],
                'proxy_options': OrderedDict([
                    ('connect_timeout', options['connect_timeout']),
                    ('backend_timeout', options['backend_timeout']),
                    ('buffering', options['buffering']),
                ]),
            }

            current_balancer_options.update(balancer2_backends_options)

            if 'pumpkin' not in dcname:
                update_rate_limiter_options(current_balancer_options, options)

            if local_balancer_options:
                current_balancer_options['balancer_options'] = local_balancer_options

            if options['enable_dynamic_balancing']:
                current_balancer_options = _add_dynamic_balancing(current_balancer_options, options['dynamic_balancing_options'])

            if options['backends_check_quorum']:
                current_balancer_options['check_backends'] = OrderedDict([
                    ('name', Modules.gen_check_backends_name(current_balancer_options['backends'])),
                    ('quorum', options['backends_check_quorum']),
                ])

            custom_backends.append(
                (current_weight, backend_name, [
                    (Modules.Report, get_report_options('%s_requests_to_' + dcname, options)),
                    (Modules.Balancer2, current_balancer_options),
                ])
            )

        custom_backends.append(
            (-1, "%s_%s" % (weights_key, 'devnull'), [
                (Modules.Report, {
                    'uuid': '%s_requests_to_devnull' % (options['service_name']),
                }),
                (Modules.ErrorDocument, {'status': 204}),
            ])
        )

        top_balancer_policies = [('unique_policy', {})]

        if options['add_geo_only_matcher']:
            top_balancer_policies = add_geo_only_matcher(top_balancer_policies, options['backends_groups'], weights_key)

        top_balancer_options = {
            'balancer_type': 'rr',
            'policies': OrderedDict(top_balancer_policies),
            'attempts': options['cross_dc_attempts'],
            'attempts_file': './controls/l7_cross_dc_attempts',
            'retry_non_idempotent': options['retry_non_idempotent'],
            'balancer_options': OrderedDict([
                ('weights_file', options['weights_file']),
            ]),
            'custom_backends': custom_backends,
        }

        if options['cross_dc_connection_attempts']:
            top_balancer_options['connection_attempts'] = options['cross_dc_connection_attempts']

        on_error = generate_on_error(
            options,
            balancer_type=options['balancer_type'],
            backend_timeout=options['onerr_backend_timeout'],
            connection_attempts=options['onerr_connection_attempts'],
            on_error_attempts=options['onerr_attempts'],
        )

        if on_error is not None:
            top_balancer_options['on_error'] = on_error

        # overriding the on_error section generated above
        if options['on_error_modules'] is not None:
            top_balancer_options['on_error'] = options['on_error_modules']

        modules.append((Modules.Balancer2, top_balancer_options))

        return modules


def _add_dynamic_balancing(balancer2_options, balancing_options=None):
    assert balancer2_options['backends'] is not None
    dynamic_balancer2_balancer_options = OrderedDict([
        ('max_pessimized_share', 0.1),
        ('backends_name', Modules.gen_dyn_backends_name(balancer2_options['backends'])),
    ])

    if balancer2_options.get('balancer_type', None) == 'active':
        balancer_options = balancer2_options.get('balancer_options')
        active = {}
        if balancer_options:
            for key, value in balancer_options.iteritems():
                if key in ['request', 'delay', 'use_backend_weight', 'weight_normalization_coeff']:
                    active[key] = value
        assert len(active) > 0
        if active.get('weight_normalization_coeff') is None:
            active['weight_normalization_coeff'] = 10
        dynamic_balancer2_balancer_options['active'] = active

    if balancing_options:
        if isinstance(balancing_options, LuaProdOrTesting):
            result = copy.deepcopy(balancing_options)

            for key, value in dynamic_balancer2_balancer_options.items():
                if key not in result.prod_groups:
                    result.prod_groups[key] = value

                if key not in result.testing_groups:
                    result.testing_groups[key] = value

            dynamic_balancer2_balancer_options = result

        elif isinstance(balancing_options, dict):
            dynamic_balancer2_balancer_options.update(balancing_options)

    balancer2_options = copy.deepcopy(balancer2_options)
    balancer2_options['balancer_type'] = 'dynamic'
    balancer2_options['balancer_options'] = dynamic_balancer2_balancer_options

    return balancer2_options


_PUMKIN_GOOD_QUERY_HEADER = 'X-Yandex-Pumpkin-Good-Query'


def _add_pumpkin_prefetch(balancer2_options, pumpkin_modules):
    assert pumpkin_modules is not None

    # Weird thing here: response_matcher will pass control to the matched item,
    # so matcher should evaluate to true in case 'there's no query in pumpkin', i.e.:
    # NOT (X-Yandex-Pumpkin-Good-Query == 1 AND response status code == 200)
    no_pumpkin_matcher = {
        'match_not': OrderedDict([
            ('match_and', OrderedDict([
                (LuaAnonymousKey(), {'match_header': {'name': _PUMKIN_GOOD_QUERY_HEADER, 'value': '1'}}),
                (LuaAnonymousKey(), {'match_response_codes': {'codes': OrderedDict([(LuaAnonymousKey(), 200)])}}),
            ]))
        ])
    }

    with_prefetch = [
        (Modules.ResponseHeaders, {'delete': _PUMKIN_GOOD_QUERY_HEADER}),
        (Modules.ResponseMatcher, [
            # If no query found in pumpkin, pass query by the normal way
            ('no_query_in_pumpkin', no_pumpkin_matcher, [
                (Modules.Report, {'uuid': 'pumpkin_prefetch_misses'}),
                (Modules.Balancer2, balancer2_options),
            ]),
        ]),

        # This balancer2 is used to catch errors from pumpkin, because response_matcher doesn't support on_error
        (Modules.Balancer2, {
            'balancer_type': 'rr',
            'attempts': 1,
            'policies': OrderedDict([('unique_policy', {})]),
            'custom_backends': [
                (1, 'default', pumpkin_modules),
            ],

            # if pumpkin doesn't answer, return invalid response
            'on_error': [(Modules.ErrorDocument, {'status': 404})],
        }),
    ]

    # Standard ITS switch
    return {
        'balancer_type': 'rr',
        'balancer_options': OrderedDict([
            ('weights_file', LuaGlobal('WeightsDir', './controls/') + '/pumpkin_prefetch_switch'),
        ]),
        'attempts': 1,
        'policies': OrderedDict([('unique_policy', {})]),
        'custom_backends': [
            (-1, 'enabled', with_prefetch),
            (1, 'disabled', [(Modules.Balancer2, balancer2_options)]),
        ]
    }


def generate_knoss_b2(options, backends, backends_new, migration_prefix, suffix, custom_proxy_options):
    service_name = options['service_name']
    localdc = suffix == 'knoss_localdc'
    modules = [
        (Modules.Report, {
            'uuid': service_name + '_requests_to_' + suffix,
            'ranges': Constants.NORMAL_REPORT_TIMELINES,
        }),
    ]
    proxy_options = OrderedDict([
        ('connect_timeout', options['knoss_connect_timeout']),
        ('backend_timeout', options['knoss_backend_timeout']),
        ('fail_on_5xx', False),
    ])

    if custom_proxy_options is not None:
        proxy_options.update(custom_proxy_options)

    def gen_balancer2(backends):
        balancer2 = {
            'backends': backends,
            'attempts': 1,
            'connection_attempts': 2,
            'balancer_type': 'rr',
            'policies': OrderedDict([
                ('unique_policy', {}),
            ]),
            'proxy_options': proxy_options,
        }
        if options['backends_check_quorum']:
            balancer2['check_backends'] = OrderedDict([
                ('quorum', options['backends_check_quorum']),
                ('name', Modules.gen_check_backends_name(backends)),
            ])
            if options['backends_check_hysteresis']:
                balancer2['check_backends']['hysteresis'] = options['backends_check_hysteresis']
            if options['use_by_location'] and not localdc:
                balancer2['check_backends']['skip'] = True
        if options['knoss_balancer_options'].get('request') is not None:
            balancer2['balancer_type'] = 'active'
            balancer2['balancer_options'] = options['knoss_balancer_options']
        if options['knoss_dynamic_balancing_enabled']:
            balancer2 = _add_dynamic_balancing(balancer2, options['knoss_dynamic_balancing_options'])
        return balancer2

    if backends_new:
        balancer2 = {
            'attempts': 1,
            'balancer_type': 'rr',
            'policies': OrderedDict([
                ('unique_policy', {}),
            ]),
            'balancer_options': OrderedDict([
                ('weights_file', LuaGlobal('WeightsDir', './controls/') + '/knoss_migration')
            ]),
            'custom_backends': [
                (1, migration_prefix + '_old', [
                    (Modules.Report, {'uuid': service_name + '_old,' + migration_prefix + '_old'}),
                    (Modules.Balancer2, gen_balancer2(backends)),
                ]),
                (-1, migration_prefix + '_new', [
                    (Modules.Report, {'uuid': service_name + '_new,' + migration_prefix + '_new'}),
                    (Modules.Balancer2, gen_balancer2(backends_new)),
                ]),
            ]
        }
        if options['use_by_location']:
            balancer2['balancer_type'] = 'by_location'
            balancer2['check_backends'] = OrderedDict([
                ('skip', True),
                ('amount_quorum', 2),
                ('name', migration_prefix),
            ])
    else:
        balancer2 = gen_balancer2(backends)

    modules.append((Modules.Balancer2, balancer2))
    return modules


def use_by_location_algorithm(config_data):
    return is_exp_balancer(config_data) or is_term_balancer(config_data)


class KnossOnly(Macroses.IMacro):
    @staticmethod
    @Macroses.Helper(
        '''KnossOnly''',
        '''
        Служебный макрос для генерации knoss only секции
        Мы ходим только в сервисные балансеры, но не ходим сразу в бэкенды
        ''',
        [
            ('service_name', None, str, True, 'Имя сервиса'),
            ('match_uri', None, str, False, 'Матч uri, если не определено, то делать default'),
            ('knoss_create_headers', None, OrderedDict, False, 'Заголовки, которые нужно создать для данной секции'),
            ('antirobot_backends', None, list, False, 'Список backend-ов антиробота'),
            ('antirobot_sink_backends', None, list, False, 'Список backend-ов антиробота для request replier'),
            ('onerr_backends', None, list, False, 'Бекенды fallback'),
            ('case_insensitive', False, bool, False, 'Нечуствительность к регистру url'),
            ('knoss_backends', None, OrderedDict, True, 'Список бекендов кносс балансера'),
            ('knoss_backends_new', None, OrderedDict, False, 'Доп.список бекендов кносс балансера во время миграции для переключения при помощи ITS'),
            ('knoss_migration_name', None, str, False, 'Имя сервиса в ITS ручке'),
            ('knoss_location', None, str, True, 'В какую локацию запрос будет приходить в первую очередь'),
            ('knoss_local_proxy_options', None, OrderedDict, False, 'Опции прокси для хождения в локальный дц'),
            ('knoss_connect_timeout', '50ms', str, False, 'Таймаут на коннект в knoss'),
            ('knoss_backend_timeout', '2s', str, False, 'Таймаут на запрос-ответ в knoss'),
            ('knoss_is_geo_only', None, bool, True, 'Cекция для geo only балансера'),
            ('knoss_on_error_modules', None, list, False, 'модули на случай onerror в кнос части терма'),
            ('knoss_balancer_options', OrderedDict(), OrderedDict, False, 'URL для эктив чека и частота эктив чека'),
            ('knoss_dynamic_balancing_enabled', False, bool, False, 'Включение динамической балансировки'),
            ('knoss_dynamic_balancing_options', OrderedDict(), OrderedDict, False, 'Дополнительные опции динамической балансировки'),
            ('host_rewrite', None, list, False, 'rewrite для host заголовка'),
            ('backends_check_quorum', 0.35, float, False, 'Кворум для проверки бэкендов на живость (задаётся как доля от общего числа бекендов)'),
            ('backends_check_hysteresis', 0.05, float, False, 'Гистерезис для проверки бэкендов на живость (задаётся как доля от общего числа бекендов)'),
            ('enable_cookie_policy', True, bool, False, 'Включить фильтрацию кук'),
            ('cookie_policy_modes', None, dict, False,
                'Кастомизация правил cookie_policy для апстрима (rule_name -> "off/dry_run/fix"'),
            ('use_by_location', True, bool, False, 'Использовать алгоритм полокационной балансировки'),
        ]
    )
    def generate(options):
        service_name = options['service_name']

        modules = [
            (Modules.Report, {
                'uuid': service_name,
                'ranges': Constants.NORMAL_REPORT_TIMELINES,
            }),
            (Modules.Meta, {
                'id': 'upstream-info',
                'fields': OrderedDict([
                    ('upstream', service_name),
                ]),
            }),
        ]

        if options['knoss_create_headers']:
            modules.append((Modules.Headers, {'create': options['knoss_create_headers']}))

        if options['antirobot_backends']:
            antirobot_options = {
                'backends': options['antirobot_backends'],
                'sink_backends': options['antirobot_sink_backends'],
                'proxy_options': OrderedDict([
                    ('keepalive_count', 1),
                    ('keepalive_timeout', '60s'),
                ]),
            }

            modules.append((Modules.Antirobot, antirobot_options))

        if options['enable_cookie_policy']:
            modules.append((Modules.CookiePolicy, {
                'uuid': 'service_total',
                'default_yandex_policies': 'unstable',
                'mode_overrides': options['cookie_policy_modes'],
            }))

        if options['host_rewrite']:
            modules.append((Modules.Rewrite, {
                'actions': [{
                    'header_name': 'host',
                    'case_insensitive': True,
                    'literal': True,
                    'regexp': h[0],
                    'rewrite': h[1],
                } for h in options['host_rewrite']]
            }))

        if options['use_by_location']:
            knoss_cfg = {
                'attempts': 1,
                'connection_attempts': 1 if not options['knoss_is_geo_only'] else 0,
                'balancer_options': OrderedDict([
                    ('weights_file', LuaGlobal('WeightsDir', './controls/') + '/knoss_locations'),
                    ('preferred_location', '{}_knoss_{}'.format(service_name, options['knoss_location'])),
                ]),
                'balancer_type': 'by_location',
                'policies': OrderedDict([
                    ('unique_policy', {}),
                ]),
            }
        else:
            knoss_cfg = {
                'attempts': 1,
                'connection_attempts': 1 if not options['knoss_is_geo_only'] else 0,
                'balancer_options': OrderedDict([
                    ('weights_file', LuaGlobal('WeightsDir', './controls/') + '/knoss_locations'),
                ]),
                'balancer_type': 'rr',
                'policies': OrderedDict([
                    ('by_name_policy', {
                        'name': '{}_knoss_{}'.format(service_name, options['knoss_location']),
                        'unique_policy': {},
                    })
                ]),
            }

        backends = []
        for dc_name, dc_backends in options['knoss_backends'].iteritems():
            if dc_name == options['knoss_location']:
                report_suffix = 'knoss_localdc'
                custom_proxy_options = options['knoss_local_proxy_options']
                weight = 1
            else:
                report_suffix = 'knoss_fallback'
                custom_proxy_options = None
                weight = 1 if not options['knoss_is_geo_only'] and dc_name != 'man' else -1

            name = '{}_knoss_{}'.format(service_name, dc_name)

            backends_new = migration_prefix = None
            if options['knoss_backends_new']:
                backends_new = options['knoss_backends_new'].get(dc_name)
                migration_prefix = '{}_{}'.format(options['knoss_migration_name'] or service_name, dc_name)

            backends.append((
                weight, name, generate_knoss_b2(options, dc_backends, backends_new, migration_prefix, report_suffix, custom_proxy_options)
            ))

        knoss_cfg['custom_backends'] = backends
        if options['use_by_location']:
            knoss_cfg['check_backends'] = OrderedDict([
                ('name', service_name),
                ('amount_quorum', len(backends)-1),
            ])

        onerr_report_options = {
            'uuid': service_name + '_requests_to_knoss_onerror',
            'ranges': Constants.NORMAL_REPORT_TIMELINES,
        }

        on_error = generate_on_error(options, on_error_attempts=2, report_options=onerr_report_options, backend_timeout='1s')
        if on_error is not None:
            knoss_cfg['on_error'] = on_error

        # overriding the on_error section generated above
        if options['knoss_on_error_modules'] is not None:
            knoss_cfg['on_error'] = options['knoss_on_error_modules']

        modules.append((Modules.Balancer2, knoss_cfg))

        return [(service_name, {
            'pattern': options['match_uri'],
            'case_insensitive': options['case_insensitive']
        }, modules)]


def generate_l7_weighted_regular_section(options):
    def balancer_generate(generator_options):
        # -- Balacing type macroses options
        if options['generator'] == 'b2standart':
            """
            Внутри группы использует B2 unique_policy rr
            Возможно не подойдет для поиска.
            """
            return (GenB2Standart, generator_options)

        elif options['generator'] == 'hashing':  # когда qcookie недостаточно хорош
            """
            Блансировка между дц с весами и прибиванием пользователей по хэшу
            """
            return (GenB2WeightedHashing, generator_options)

        raise ValueError('no Value for options["generator"]')
        # -- end


    modules = []

    copy_headers = OrderedDict()
    create_headers = OrderedDict()
    delete_headers = "Shadow.*"

    if options['create_headers']:
        create_headers = options['create_headers']

    if options['xff_disable']:
        delete_headers += '|X-Forwarded-For'

    if options['restore_shadow_headers']:
        for i in options['restore_shadow_headers']:
            copy_headers['Shadow-' + str(i)] = str(i)

    exp_service_name = options['service_name']
    if options['exp_service_name']:
        exp_service_name = options['exp_service_name']

    if options['no_uaas_laas']:
        assert not options['uaas_backends']
        assert not options['uaas_new_backends']
        assert not options['remote_log_backends']
        assert not options['laas_backends']
        assert not options['laas_backends_onerror']
        assert not options['laas_uaas_processing_time_header']
        assert not options['uaas_headers_size_limit']
        create_headers['X-Yandex-ExpServiceName'] = exp_service_name
        create_headers['X-Yandex-LaaS-UaaS-Disabled'] = 1

    modules.append((Modules.Headers, {
        'copy': copy_headers,
        'delete': delete_headers,
        'create': create_headers,
    }))

    if options['antirobot_backends'] is not None:
        antirobot_options = {
            'backends': options['antirobot_backends'],
            'proxy_options': OrderedDict([
                ('keepalive_count', 1),
                ('keepalive_timeout', '60s'),
            ]),
        }

        modules.append((Modules.Antirobot, antirobot_options))

    # TRAFFIC-1096, TRAFFIC-1113, TRAFFIC-2123
    if options['threshold_enable']:
        modules.append(
            (Modules.Threshold, {
                'lo_bytes': options['threshold_lo_bytes'],
                'hi_bytes': options['threshold_hi_bytes'],
                'pass_timeout': options['threshold_pass_timeout'],
                'recv_timeout': options['threshold_recv_timeout'],
                'on_pass_timeout_failure': [(Modules.ErrorDocument, {'status': 413})],
            })
        )

    if options['laas_backends'] is not None and options['laas_backends_onerror'] is not None:
        modules.append((Laas, {
            'laas_backends': options['laas_backends'],
            'laas_backends_onerror': options['laas_backends_onerror'],
            'processing_time_header': options['laas_uaas_processing_time_header'],
        }))

    if options['enable_cookie_policy']:
        modules.append((Modules.CookiePolicy, {
            'uuid': 'service_total',
            'default_yandex_policies': 'unstable',
            'mode_overrides': options['cookie_policy_modes'],
        }))

    if options['uaas_backends'] is not None:
        exp_getter_options = {
            'service_name': exp_service_name,
            'service_name_to_backend_header': options['service_name_to_backend_header'],
            'uaas_backends': options['uaas_backends'],
            'uaas_new_backends': options['uaas_new_backends'],
            'remote_log_backends': options['remote_log_backends'],
            'processing_time_header': options['laas_uaas_processing_time_header'],
            'headers_size_limit': _uaas_headers_size_limit(options['uaas_headers_size_limit'], exp_service_name),
        }

        modules.append((ExpGetterAndRLog, exp_getter_options))

    if options['restore_headers_for_request']:
        copy_headers = {}

        for key in options['restore_headers_for_request'].keys():
            copy_headers[key] = options['restore_headers_for_request'][key]

        modules.append((Modules.Headers, {
            'copy': OrderedDict(copy_headers),
        }))

    generator_options = {}
    for name in [
        'service_name',
        'backends_groups',
        'cross_dc_attempts',
        'cross_dc_connection_attempts',
        'in_dc_attempts',
        'in_dc_connection_attempts',
        'connect_timeout',
        'backend_timeout',
        'weight_matrix',
        'weights_key',
        'onerr_backends',
        'on_error_modules',
        'hashing_by',
        'override_balancing_hints',
        'balancer_type',
        'active_request',
        'active_delay',
        'report_uuid',
        'retry_non_idempotent',
        'onerr_backend_timeout',
        'onerr_connection_attempts',
        'onerr_attempts',
        'add_geo_only_matcher',
        'enable_dynamic_balancing',
        'dynamic_balancing_options',
        'by_dc_rps_limiter',
        'rps_limiter_location',
        'rps_limiter_namespace',
        'rps_limiter_backends',
        'rps_limiter_disable_file',
        'pumpkin_prefetch',
        'pumpkin_prefetch_timeout',
        'backends_check_quorum',
    ]:
        generator_options[name] = options[name]

    copy_rate_limiter_options(generator_options, options)

    sink_header_name = options['sink_indicator_header']
    if sink_header_name is not None:
        modules.append((Modules.Headers, {'delete': sink_header_name}))

    if options['sink_backends'] is not None:
        sink_modules = []
        if sink_header_name is not None:
            sink_modules.append((Modules.Headers, {
                'create': OrderedDict([
                    (sink_header_name, '1'),
                ])
            }))

        sink_modules.append((Modules.Balancer2, {
            'backends': options['sink_backends'],
            'attempts': 1,
            'policies': OrderedDict([('unique_policy', {})]),
        }))

        modules.append((Modules.RequestReplier, {
            'rate_file': './controls/request_replier_rate',
            'sink': sink_modules,
        }))

    if options['synthetics_load']:
        synthetics_modules = []
        synthetics_modules.append((Modules.Headers, {
            'create': OrderedDict([
                ('X-Yandex-Internal-Request', '1'),
            ])
        }))

        skip_apphost_backends = [
            'MARKET',
            'WEB__YABS_SEARCH_NOAPPHOST_TEST',
            'YABS_NOAPPHOST__WEB',
            'YABS_SEARCH_NOAPPHOST',
        ]
        cgi = {
            # TODO remove srcrwr once APPHOST-4423 is released
            'srcrwr': [
                backend + ':devnull.yandex.net'
                for backend in skip_apphost_backends
            ],
            'backendskip': skip_apphost_backends,
            'srcskip': ['MARKET_CART'],
            'exp_flags':'degrade_logs_drop=1',
            'reqinfo':'web-drills-mirror',
        }
        cgi = urlencode(cgi, True).replace('%', '%%')

        synthetics_modules.append((
            Modules.Rewrite, {
                'actions': [
                    {
                        'split': 'cgi',
                        'regexp': r'^\\?(.*)',  # does start with `?`
                        'rewrite': '?%1' + '&' + cgi,
                    },
                    {
                        'split': 'url',
                        'regexp': r'^([^?]*)$',  # does not contain `?`
                        'rewrite': '%1?' + cgi,
                    },
                ]
            }
        ))

        synthetics_options = copy.deepcopy(generator_options)
        synthetics_options['weights_file'] = Constants.L7_DEFAULT_WEIGHTS_DIR + '/synthetics.weights'
        synthetics_modules.append(balancer_generate(synthetics_options))

        modules.append((Modules.RequestReplier, {
            'rate_file': Constants.L7_DEFAULT_WEIGHTS_DIR + '/synthetics.replier.rate',
            'sink': synthetics_modules,
            'rate': 0.0,
            'enable_failed_requests_replication': True
        }))

    if options['backend_experiments'] is not None:
        sections = []
        for exp_id, exps_header_name, backends in options['backend_experiments']:
            experiment_options = copy.deepcopy(generator_options)
            experiment_options['backends_groups'] = backends
            experiment_options['report_dc_uuid_suffix'] = 'exp_id_{}'.format(exp_id)
            experiment_matcher = {
                'match_fsm': OrderedDict([
                    ('header', {
                        'name': exps_header_name,
                        'value': r'.*{}.*'.format(exp_id)
                    })
                ])}
            sections.append(('exp_backends_{}'.format(exp_id), experiment_matcher, [balancer_generate(experiment_options)]))

        sections.append(('default', {}, [balancer_generate(generator_options)]))

        modules.append((Modules.Regexp, sections))
    else:
        modules.append(balancer_generate(generator_options))

    if options['rps_limiter']:
        assert not options['by_dc_rps_limiter']

        rps_limiter_options = copy.deepcopy(options['rps_limiter'])

        rps_limiter_options['module'] = copy.deepcopy(modules)
        add_rps_limiter_backends(rps_limiter_options, options['rps_limiter_backends'], options['rps_limiter_location'])

        modules = [(Modules.RpsLimiter, rps_limiter_options)]

    modules = [
        (Modules.Report, {
            'uuid': options['report_uuid'] if options['report_uuid'] is not None else options['service_name'],
            'ranges': Constants.NORMAL_REPORT_TIMELINES,
        }),
        (Modules.Meta, {
            'id': 'upstream-info',
            'fields': OrderedDict([
                ('upstream', options['service_name']),
            ]),
        }),
    ] + modules

    return modules


def generate_l7_weighted_prefetch_section(options, prefix=''):
    if not options[prefix + 'prefetch_switch_file']:
        raise Exception('no ' + prefix + 'prefetch_switch_file provided')

    options[prefix + 'prefetch_switch'] = False

    regular_options = copy.deepcopy(options)
    default_section = generate_l7_weighted_no_chromium_prefetch_section(regular_options)

    def gen_prefetch_branch(uuid='', weights_file=options[prefix + 'prefetch_switch_file'], prefix=prefix):
        prefetch_uuid = options['service_name'] + '_' + prefix + 'prefetch' + uuid
        prefetch_options = copy.deepcopy(options)
        prefetch_options['in_dc_attempts'] = 1
        prefetch_options['cross_dc_attempts'] = 1
        prefetch_options['onerr_backends'] = None
        prefetch_options['on_error_modules'] = [
            (Modules.Report, {
                'uuid': prefetch_uuid + '_requests_to_onerror',
                'all_default_ranges': False
            }),
            (Modules.ErrorDocument, {'status': 429})
        ]

        if not prefetch_options['by_dc_rps_limiter']:
            prefetch_options['by_dc_rps_limiter'] = OrderedDict([
                ('namespace', options['rps_limiter_namespace']),
                ('disable_file', options['rps_limiter_disable_file']),
            ])
        prefetch_section = generate_l7_weighted_regular_section(prefetch_options)

        return [
            (Modules.Report, {'uuid': prefetch_uuid + ',' + prefix + 'prefetch', 'outgoing_codes': '429,404'}),
            (Modules.Balancer2, {
                'attempts': 1,
                'balancer_type': 'rr',
                'balancer_options': OrderedDict([
                    ('weights_file', weights_file),
                ]),
                'policies': OrderedDict([('unique_policy', {})]),
                'custom_backends': [
                    (1, 'prefetch_enabled', prefetch_section),
                    (0, 'prefetch_disabled', [(Modules.ErrorDocument, {'status': 429})]),
                ],
            })
        ]

    prefetch_matcher = options[prefix + 'prefetch_switch_matcher']
    if not prefetch_matcher:
        raise Exception(prefix + 'prefetch_switch_matcher required for section with ' + prefix + 'prefetch switch')

    prefetch_matcher = copy.deepcopy(prefetch_matcher)

    regexp_sections = []

    if prefix == '' and options['prefetch_switch_per_platform']:
        # MINOTAUR-1835

        def add_platform(uuid, platform_matcher):
            modules = gen_prefetch_branch('_' + uuid, weights_file=options['prefetch_switch_file'] + ('_' + uuid))
            section = ('prefetch_' + uuid, {
                'match_and': OrderedDict([
                    (LuaAnonymousKey(), prefetch_matcher),
                    (LuaAnonymousKey(), platform_matcher),
                ])
            }, modules)

            regexp_sections.append(section)

        add_platform('disable_all', OrderedDict([
            ('match_if_file_exists', {'path': options['prefetch_switch_file'] + '_disable_all'})
        ]))

        add_platform('pp', {
            'match_fsm': OrderedDict([('cgi', r'.*(\\?|&)ui=webmobileapp.yandex.*')])
        })

        add_platform('touch_yabrowser', {
            'match_fsm': OrderedDict([('header', {'name': 'User-Agent', 'value': r'.*(Android|iPhone).*YaBrowser.*'})])
        })

        add_platform('desktop_yabrowser', {
            'match_fsm': OrderedDict([('header', {'name': 'User-Agent', 'value': r'.*(Windows|Mac|Linux).*YaBrowser.*'})])
        })
    else:
        regexp_sections.append((prefix + 'prefetch', prefetch_matcher, gen_prefetch_branch()))

    regexp_sections.append(('default', {}, default_section))

    return [
        (Modules.Regexp, regexp_sections)
    ]


def generate_l7_weighted_no_chromium_prefetch_section(options):
    if options['prefetch_switch']:
        return generate_l7_weighted_prefetch_section(options)
    return generate_l7_weighted_regular_section(options)


def generate_l7_weighted_chromium_prefetch_section(options):
    return generate_l7_weighted_prefetch_section(options, prefix='chromium_')


class MetaL7WeightedBackendGroups(Macroses.IMacro):
    @staticmethod
    @Macroses.Helper(
        'MetaL7WeightedBackendGroups',
        '''
            Мета макрос для fullmesh балансировок с весами.
        ''',
        add_rate_limiter_options([
            ('service_name', None, str, True, 'Имя сервиса'),
            ('match_uri', None, str, False, 'Матч uri, если не определено то делать default '),

            ('report_uuid', None, str, False, 'Идентификатор модуля Report, в который отправляется статистика.'),
            ('case_insensitive', False, bool, False, 'Нечуствительность к регистру url'),
            ('xff_disable', False, bool, False, 'Удалить заголовок X-Forwarded-For'),
            ('antirobot_backends', None, list, False, 'Список backend-ов антиробота'),

            ('threshold_enable', False, bool, False, 'Флаг включения Threshold'),
            ('threshold_lo_bytes', 150 * 1024, int, False, 'Threshold lo_bytes от клиента'),
            ('threshold_hi_bytes', 300 * 1024, int, False, 'Threshold hi_bytes от клиента'),
            ('threshold_pass_timeout', '2s', str, False, 'Threshold pass timeout'),
            ('threshold_recv_timeout', '0.05s', str, False, 'Threshold recv timeout'),

            ('laas_backends', None, list, False, ''),
            ('laas_backends_onerror', None, list, False, ''),

            ('exp_service_name', None, str, False, 'exp service name'),
            ('service_name_to_backend_header', None, str, False, 'Имя заголовка, в котором на бекенд будет отправлено название сервиса'),

            ('uaas_backends', None, list, False, 'Список backend-ов экспериментов'),
            ('uaas_new_backends', None, list, False, 'Список новых backend-ов экспериментов'),
            ('remote_log_backends', None, list, False, 'Список backend-ов для удаленного логирования'),
            ('restore_shadow_headers', None, list, False, 'Восстановить скопированные заголовки'),
            ('restore_headers_for_request', None, dict, False, 'Восстановить заголовки после сервисных операций и до обращения к бекенду'),

            ('rps_limiter_backends', None, list, False, ''),

            ('balancer2', True, bool, False, 'Версия модуля балансер'),
            ('balancer_type', 'rr', str, False, 'тип балансировки'),
            ('active_request', 'HEAD /ok.html HTTP/1.1\\r\\n\\r\\n', str, False, 'Url активной проверки'),
            ('active_delay', '5s', str, False, 'Время между активным проверками'),
            ('backends_groups', None, OrderedDict, False, 'Бекенды сервиса'),
            ('onerr_backends', None, list, False, 'Бекенды fallback'),
            ('on_error_modules', None, int, False, 'fallback в errordocument'),
            ('connect_timeout', '0.15s', str, False, 'Таймаут на соединение с бекендами сервиса'),
            ('backend_timeout', '10s', str, False, 'Таймаут на обмен данными с бекендами сервиса'),
            ('cross_dc_attempts', 2, int, False, 'Количество попыток между датацентрами'),
            ('cross_dc_connection_attempts', None, int, False, 'Количество попыток между датацентрами'),
            ('in_dc_attempts', 2, int, False, 'Количество попыток на обмен данными внутри датацентра'),
            ('in_dc_connection_attempts', None, int, False, 'Количество попыток на соединение c бэкендами сервиса'),
            ('weight_matrix', None, OrderedDict, False, 'Матрица статических весов'),
            ('generator', None, str, False, 'Тип генератора для балансировки в custom backeds'),
            ('sink_backends', None, list, False, 'Список бекендов для зеркалирования трафика в приемку'),
            ('sink_indicator_header', None, str, False, 'Заголовок-индикатор для зеркалированного трафика'),
            ('synthetics_load', None, bool, False, 'Включить зеркалирование трафика в основной набор backeds'),
            ('weights_key', None, str, False, 'Ключ секции с весами, нужен управления разными сервисами из одного окна админки'),
            ('hashing_by', None, str, False, 'Используемый метод для хэширования'),
            ('retry_non_idempotent', None, bool, False,
                'Ретраить неидемпотентные запросы (POST, PATCH), '
                'работает только в модуле balancer2, в balancer всегда True INFRAINCIDENTS-375'),
            ('prefetch_switch', False, bool, False, 'Включить регулирование префетч-запросов через ITS'),
            ('prefetch_switch_matcher', None, dict, False, 'Матчер для prefetch-запросов'),
            ('prefetch_switch_file', None, str, False, 'Название секции для отключения'),
            ('prefetch_switch_per_platform', False, bool, False, 'Разделение сигналов на ПП/не-ПП'),
            ('chromium_prefetch_switch', False, bool, False, 'Включить регулирование префетч-запросов chromium через ITS'),
            ('chromium_prefetch_switch_matcher', None, dict, False, 'Матчер для chromium prefetch-запросов'),
            ('chromium_prefetch_switch_file', None, str, False, 'Название секции для отключения'),
            ('onerr_backend_timeout', None, str, False, 'Таймаут на обмен данными с бекендами fallback'),
            ('onerr_connection_attempts', None, int, False, 'Количество попыток на соединение c бэкендами fallback'),
            ('onerr_attempts', None, int, False, 'Количество попыток на обмен данными c бэкендами fallback'),
            ('add_geo_only_matcher', None, bool, False, 'Добавить матчер по X-Yandex-Balancing-Hint'),
            ('enable_dynamic_balancing', False, bool, False, 'Включить динамическую балансировку'),
            ('dynamic_balancing_options', None, OrderedDict, False, 'Дополнительные опции для динамической балансировки'),
            ('backend_experiments', None, list, False, 'Эксперименты на бекенды'),
            ('by_dc_rps_limiter', None, OrderedDict, False, 'Параметры для хождения в rpslimiter перед каждой попыткой в локацию'),
            ('rps_limiter', None, OrderedDict, False, 'Параметры для хождения в rpslimiter перед каждым запросом в балансер'),
            ('rps_limiter_namespace', None, str, False, 'Название секции в rps_limiter'),
            ('rps_limiter_location', None, str, False, 'ДЦ для первой попытки в rpslimiter'),
            ('rps_limiter_disable_file', None, str, False, 'Файл для отключения rpslimiter'),
            ('override_balancing_hints', None, dict, False, 'Принудительно перезаписать заголовочные подсказки балансировки'),
            ('pumpkin_prefetch', False, bool, False, 'Включить рубильник для походов в тыкву'),
            ('pumpkin_prefetch_timeout', None, str, False, 'Таймаут для префетча в тыкву'),
            ('backends_check_quorum', None, float, False, 'Кворум для проверки бэкендов на живость в prepare'),
            # To be deleted after search experiment. For details, refer to MINOTAUR-2484.
            ('laas_uaas_processing_time_header', False, bool, False, 'Хедеры с processing_time для UaaS/LaaS'),
            ('enable_cookie_policy', False, bool, False, 'Включить фильтрацию кук'),
            ('cookie_policy_modes', None, dict, False,
             'Кастомизация правил cookie_policy для апстрима (rule_name -> "off/dry_run/fix"'),
            ('no_uaas_laas', None, bool, False, 'Режим работы без uaas и laas для поиска'),
            ('uaas_headers_size_limit', None, int, False, 'Ограничение на суммарный размер заголовков, берущихся из ответа UaaS'),
            ('create_headers', None, OrderedDict, False, 'Заголовки, которые нужно создать для данной секции'),
        ])
    )
    def generate(options):
        if options['match_uri'] == 'default':
            raise Exception('No default allowed')

        if options['chromium_prefetch_switch']:
            modules = generate_l7_weighted_chromium_prefetch_section(options)
        else:
            modules = generate_l7_weighted_no_chromium_prefetch_section(options)

        return [(options['service_name'], {
            'pattern': options['match_uri'],
            'case_insensitive': options['case_insensitive']
        }, modules)]


class ExpGetterAndRLog(Macroses.IMacro):
    @staticmethod
    @Macroses.Helper(
        'l7macro.ExpGetterAndRLog',
        '''Макрос для хождения в эксперименты и удаленного логирования MINOTAUR-17''',
        [
            ('service_name', '', str, False, 'Имя сервиса'),
            ('service_name_header', 'Y-Service', str, False, 'Имя сервиса'),
            ('service_name_to_backend_header', None, str, False, 'Имя заголовка, в котором на бекенд будет отправлено название сервиса'),
            ('balancer2', False, bool, False, 'Использовать модуль Balancer2 вместо Balancer.'),
            ('header_name', 'X-L7-EXP', str, False, 'Хедер для определения продакшн или тестинг'),
            ('uaas_backends', None, list, False, 'Список backend-ов экспериментов'),
            ('uaas_new_backends', None, list, False, 'Список новых backend-ов экспериментов'),
            (
                'trusted', False, bool, False,
                '''Булев флажок, который определяет, доверяем ли мы запросу.
                Если доверяем и в запросе есть хоть один заголовок из exp_headers - то не ходим в uaas.
                Иначе - удаляем exp_headers из запроса'''
            ),
            ('remote_log_backends', None, list, False, 'Список backend-ов для удаленного логирования'),
            ('remote_log_connect_timeout', '0.15s', str, False,
                'Таймаут на подключение к backend-ам удаленного логирования'),
            ('remote_log_backend_timeout', '1s', str, False, 'Таймаут на запрос к backend-ам удаленного логирования'),
            ('remote_log_attempts', 1, int, False, 'Число попыток запроса в backend-ы удаленного логирования'),
            (
                'remote_log_connection_attempts', 2, int, False,
                'Дополнительное число попыток на подключение к backend-ам удаленного логирования'
            ),
            # To be deleted after search experiment. For details, refer to MINOTAUR-2484.
            ('processing_time_header', False, bool, False, ''),
            ('headers_size_limit', None, int, False, 'Ограничение на суммарный размер заголовков, берущихся из ответа UaaS'),
        ]
    )
    def generate(options):
        exp_getter_options = dict(options)

        remote_log_options = [
            'remote_log_backends',
            'remote_log_connect_timeout',
            'remote_log_backend_timeout',
            'remote_log_attempts',
            'remote_log_connection_attempts',
        ]
        remote_log_options = {k: exp_getter_options.pop(k) for k in remote_log_options}
        exp_getter_options['processing_time_header'] = options['processing_time_header']

        results = [(WebExpGetter, exp_getter_options)]

        if remote_log_options['remote_log_backends'] is not None:
            remote_log_balancer = [
                (Modules.Report, {'uuid': 'remotelog'}),
                (Modules.Balancer2, {
                    'backends': remote_log_options['remote_log_backends'],
                    'policies': OrderedDict([
                        ('unique_policy', {}),
                    ]),
                    'attempts': remote_log_options['remote_log_attempts'],
                    'connection_attempts': remote_log_options['remote_log_connection_attempts'],
                    'balancer_type': 'rr',
                    'proxy_options': OrderedDict([
                        ('connect_timeout', remote_log_options['remote_log_connect_timeout']),
                        ('backend_timeout', remote_log_options['remote_log_backend_timeout']),
                        ('keepalive_count', 1),
                        ('keepalive_timeout', '60s'),
                    ]),
                    'check_backends': OrderedDict([
                        ('name', Modules.gen_check_backends_name(remote_log_options['remote_log_backends'])),
                        ('quorum', 0.35),
                    ]),
                }),
            ]

            results.append((Modules.RemoteLog, {
                'no_remote_log_file': './controls/remote_log.switch',
                'remote_log_storage': remote_log_balancer,
                'uaas_mode': False,
            }))

        return results


class WebExpGetter(Macroses.IMacro):
    @staticmethod
    @Macroses.Helper(
        'l7macro.ExpGetter',
        ''' Ходилка в эксперименты''',
        [
            ('service_name', '', str, False, 'Имя сервиса'),
            ('service_name_header', 'Y-Service', str, False, 'Имя сервиса'),
            ('service_name_to_backend_header', None, str, False, 'Имя заголовка, в котором на бекенд будет отправлено название сервиса'),
            ('balancer2', False, bool, False, 'Использовать модуль Balancer2 вместо Balancer.'),
            ('header_name', 'X-L7-EXP', str, False, 'Хедер для определения продакшн или тестинг'),
            ('uaas_backends', None, list, True, 'Список backend-ов экспериментов'),
            ('uaas_new_backends', None, list, True, 'Список новых backend-ов экспериментов'),
            (
                'trusted', False, bool, False,
                '''Булев флажок, который определяет, доверяем ли мы запросу.
                Если доверяем и в запросе есть хоть один заголовок из exp_headers - то не ходим в uaas.
                Иначе - удаляем exp_headers из запроса'''
            ),
            ('exp_static_header', 'Y-Balancer-Experiments', str, False, 'Имя заголовка для exp_static.'),
            ('processing_time_header', False, bool, False, ''),
            ('headers_size_limit', None, int, False, 'Ограничение на суммарный размер заголовков, берущихся из ответа UaaS'),
        ]
    )
    def generate(options):
        uaas_balancer = [
            (Modules.Balancer2, {
                'backends': options['uaas_backends'],
                'attempts': 2,
                'connection_attempts': 2,
                'balancer_type': 'rr',
                'policies': OrderedDict([
                    ('unique_policy', {})
                ]),
                'proxy_options': OrderedDict([
                    ('connect_timeout', '15ms'),
                    ('backend_timeout', '20ms'),
                    ('keepalive_count', 1),
                    ('keepalive_timeout', '60s'),
                ]),
            }),
        ]

        if options['uaas_new_backends'] is not None:
            uaas_old_balancer = [(Modules.Report, {
                'uuid': 'expgetter_old',
                'disable_robotness': True,
                'disable_sslness': True,
            })] + uaas_balancer
            uaas_balancer_options = {
                'backends': options['uaas_new_backends'],
                'attempts': 2,
                'connection_attempts': 2,
                'balancer_type': 'rr',
                'policies': OrderedDict([
                    ('unique_policy', {})
                ]),
                'proxy_options': OrderedDict([
                    ('connect_timeout', '15ms'),
                    ('backend_timeout', '20ms'),
                    ('keepalive_count', 1),
                    ('keepalive_timeout', '60s'),
                ]),
            }

            uaas_new_balancer = [
                (Modules.Report, {
                    'uuid': 'expgetter_new',
                    'disable_robotness': True,
                    'disable_sslness': True,
                }),
                (Modules.Balancer2, {
                    'backends': options['uaas_new_backends'],
                    'attempts': 2,
                    'connection_attempts': 2,
                    'balancer_type': 'rr',
                    'policies': OrderedDict([
                        ('unique_policy', {})
                    ]),
                    'proxy_options': OrderedDict([
                        ('connect_timeout', '15ms'),
                        ('backend_timeout', '20ms'),
                        ('keepalive_count', 1),
                        ('keepalive_timeout', '60s'),
                    ]),
                }),
            ]

            uaas_balancer = [(Modules.Balancer2, {
                                'custom_backends': [
                                    (1, 'old', uaas_old_balancer),
                                    (-1, 'new', uaas_new_balancer),
                                ],
                                'attempts': 1,
                                'balancer_type': 'rr',
                                'policies': OrderedDict([
                                    ('simple_policy', {})
                                ]),
                                'balancer_options': OrderedDict([
                                    ('weights_file', LuaGlobal('WeightsDir', './controls/') + '/uaas_migration.weights')
                                ]),
                            })]

        exp_getter_params = {
            'uaas': [(Modules.Report, {
                'uuid': 'expgetter',
                'disable_robotness': True,
                'disable_sslness': True,
            })] + uaas_balancer,
            'file_switch': './controls/expgetter.switch'
        }

        exp_getter_params['processing_time_header'] = options['processing_time_header']

        if options['service_name'] is not None and options['service_name_header'] is not None:
            exp_getter_params['service_name'] = options['service_name']
            exp_getter_params['service_name_header'] = options['service_name_header']
            exp_getter_params['service_name_to_backend_header'] = options['service_name_to_backend_header']

        if options['trusted']:
            exp_getter_params['trusted'] = True

        if options['headers_size_limit'] is not None:
            exp_getter_params['headers_size_limit'] = options['headers_size_limit']

        modules = [
            (Modules.Headers, {
                'create': OrderedDict([
                    (options['header_name'], 'true'),
                ]),
            }),
            (Modules.ExpGetter, exp_getter_params),
        ]
        return modules


class HttpToHttpsRedirect(Macroses.IMacro):
    @staticmethod
    @Macroses.Helper(
        'l7macro.HttpToHttpsRedirect',
        '''
            Выполняет редирект на https
        ''',
        [
            ('permanent', False, bool, False, 'Возвращать 301 (permanent redirect)'),
        ],
    )
    def generate(options):
        code = 302
        if options['permanent']:
            code = 301
        modules = [
            (Modules.Headers, {
                'create': OrderedDict([
                    ('Location', '1'),
                ])
            }),
            (Modules.Rewrite, {
                'actions': [{
                    'regexp': '.*',
                    'header_name': 'Location',
                    'rewrite': 'https://%{host}%{url}'
                }]
            }),
            (Modules.Regexp, [
                ('unsafe_methods', {'match_method': OrderedDict([('methods', ['delete', 'patch', 'post', 'put'])])}, [
                    (Modules.ErrorDocument, {'status': 307, 'remain_headers': 'Location'}),
                ]),
                ('default', {}, [
                    (Modules.ErrorDocument, {'status': code, 'remain_headers': 'Location'}),
                ])
            ])
        ]
        return modules


class L7ErrorDocument(Macroses.IMacro):
    @staticmethod
    @Macroses.Helper(
        'L7ErrorDocument',
        '''
            Макрос для ErrorDocument под L7
        ''',
        [
            ('match_uri', None, str, False, 'Матч uri, если не определено, то делать default'),
            ('service_name', None, str, True, 'Имя сервиса'),
            ('case_insensitive', False, bool, False, 'Нечуствительность к регистру url'),
            ('status', 200, int, False, 'Статус который возвращает ErrorDocument'),
            ('content', '', str, False, 'Тело которое возвращает ErrorDocument'),
        ]
    )
    def generate(options):
        modules = [
            (Modules.Report, {'uuid': options['service_name']}),
            (Modules.ErrorDocument, {'status': options['status'], 'content': options['content']})
        ]

        return [(options['service_name'], {
            'pattern': options['match_uri'],
            'case_insensitive': options['case_insensitive']
        }, modules)]


class L7ActiveCheckReply(Macroses.IMacro):
    @staticmethod
    @Macroses.Helper(
        'L7ActiveCheckReply',
        '''
            Макрос для ActiveCheckReply под L7
        ''',
        [
            ('match_uri', None, str, False, 'Матч uri, если не определено, то делать default'),
            ('service_name', None, str, True, 'Имя сервиса'),
            ('case_insensitive', False, bool, False, 'Нечуствительность к регистру url'),
            ('weight_file', None, str, False, 'Путь к файлу для переопределения веса'),
            ('use_header', None, bool, False, 'Отдавать вес в хедере'),
            ('zero_weight_at_shutdown', None, bool, False, 'Понижать вес до нуля при shutdown'),
        ]
    )
    def generate(options):
        modules = [
            (Modules.Report, {'uuid': options['service_name']}),
            (Modules.Meta, {
                'id': 'upstream-info',
                'fields': OrderedDict([
                    ('upstream', options['service_name']),
                ]),
            }),
            (Modules.ActiveCheckReply, {
                'use_header': options['use_header'],
                'weight_file': options['weight_file'],
                'zero_weight_at_shutdown': options['zero_weight_at_shutdown'],
            })
        ]

        return [(options['service_name'], {
            'pattern': options['match_uri'],
            'case_insensitive': options['case_insensitive']
        }, modules)]


class Redirect(Macroses.IMacro):
    @staticmethod
    @Macroses.Helper(
        'l7macro.Redirect',
        '''
            Принимает регэксп и эмулирует поведение репорта в случае, если URL не существует.
            Нормальный редирект отдает дополнительные CGI параметры ?lr=213&redircnt=UNIX_TIME,
            которые невозможно проэмулировать в ErrorDocument. Так же отсутствуют все set-cookie.
        ''',
        [
            ('match_uri', None, str, False, 'Матч uri, если не определено, то делать default'),
            ('service_name', None, str, True, 'Имя сервиса'),
            ('case_insensitive', False, bool, False, 'Нечуствительность к регистру url'),
            ('location', None, str, True, "Куда редиректить")
        ],
    )
    def generate(options):
        modules = [
            (Modules.ResponseHeaders, {
                'create': OrderedDict([
                    ('Location', options['location']),
                    ('Content-Type', 'text/plain')])
            }),
            (Modules.Meta, {
                'id': 'upstream-info',
                'fields': OrderedDict([
                    ('upstream', options['service_name']),
                ]),
            }),
            (Modules.ErrorDocument, {
                'status': 302,
            }),
        ]

        return [(options['service_name'], {
            'pattern': options['match_uri'],
            'case_insensitive': options['case_insensitive']
        }, modules)]

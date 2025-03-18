#!/skynet/python/bin/python
# -*- coding: utf8 -*-
# Please do not put lines longer than 120 chars
# and maintain PEP8 style

from collections import OrderedDict

import modules as Modules
import constants as Constants
import src.utils as Utils
from lua_globals import LuaGlobal, LuaAnonymousKey, LuaFuncCall
from optionschecker import Helper


class IMacro(object):

    def __init__(self):
        pass


class Errorlog(IMacro):

    def __init__(self):
        IMacro.__init__(self)

    @staticmethod
    @Helper(
        'Macroses.Errorlog',
        '''Обертка над Modules.Errorlog с логом, находящимся в /usr/local/www/logs/''',
        [
            (
                'port', None, int, True,
                '''
                Постфикс для имени log-файла.
                Как правило, соответствует порту, на котором балансер слушает запросы
                '''
            ),
            (
                'logs_path', '/usr/local/www/logs/', str, False,
                'Директория для логов'
            ),
        ],
        examples=[
            ('Errorlog для балансера, поднятого на порту 12345', '''(Errorlog, { 'port' : 12345 })'''),
        ],
    )
    def generate(options):
        return [
            (Modules.Errorlog, {
                'log': LuaGlobal('LogDir', options['logs_path']) + '/current-error_log-balancer-{}'.format(options['port'])
            })
        ]


class Accesslog(IMacro):

    def __init__(self):
        IMacro.__init__(self)

    @staticmethod
    @Helper(
        'Macroses.Accesslog',
        '''Обертка над Modules.Accesslog с логом, находящимся в /usr/local/www/logs/''',
        [
            ('port', None, int, True,
             'Постфикс для имени log-файла. Как правило, соответствует порту, на котором балансер слушает запросы'),
            ('additional_ip_header', None, str, False,
             'Добавляет в access лог дополнительное поле с значением из из поле указанного хедера'),
            ('additional_port_header', None, str, False,
             'Добавляет в access лог дополнительное поле с значением из из поле указанного хедера'),
            (
                'logs_path', '/usr/local/www/logs/', str, False,
                'Директория для логов'
            ),
        ],
        examples=[
            ('Accesslog для балансера, поднятого на порту 12345', '''(Errorlog, { 'port' : 12345 })'''),
        ],
    )
    def generate(options):
        access_log_params = {
            'log': LuaGlobal('LogDir', options['logs_path']) + '/current-access_log-balancer-{}'.format(options['port'])
        }

        if options['additional_ip_header'] is not None:
            access_log_params['additional_ip_header'] = options['additional_ip_header']

        if options['additional_port_header'] is not None:
            access_log_params['additional_port_header'] = options['additional_port_header']

        return [(Modules.Accesslog, access_log_params)]


class ExtendedHttp(IMacro):

    def __init__(self):
        IMacro.__init__(self)

    @staticmethod
    @Helper(
        'Macroses.ExtendedHttp',
        '''
            Преобразуется в последовательность модулей:
                Macroses.Errorlog > Modules.SslSni > Modules.Http > Macroses.Accesslog > Modules.Statistics
            Некоторые из модулей могут пропускаться в зависимости от параметров.
        ''',
        [
            ('port', None, int, True, 'Порт для Macroses.Errorlog и Macroses.Accesslog'),
            ('stats_attr', '', str, False,
             'Имя атрибута для статистики golovan-а. Прокидывается в Modules.Http, Modules.Statistics, и так далее'),
            ('maxlen', 65536, int, False,
             'Максимальная длина данных, пришедших от клиента. Прокидывается в Modules.Http'),
            ('maxreq', 65536, int, False, 'Максимальная длина тела запроса. Прокидывается в Modules.Http'),
            ('keepalive', 1, int, False, 'Включить keep-alive. Прокидывается в Modules.Http'),
            ('ssl_enabled', False, bool, False, 'Enable ssl on balancer'),
            ('ja3_enabled', False, bool, False, 'Включает ja3 fingerprint'),
            ('force_ssl', False, bool, False, 'Запрет обработки запросов пришедших по http в ssl секциях.'),
            ('disable_logs', False, bool, False, 'Disable both access and error log on balancers'),
            ('disable_sslv3', None, bool, False, 'Disable sslv3 on balancer'),
            ('ssl_protocols', ['tlsv1', 'tlsv1.1', 'tlsv1.2', 'tlsv1.3'], list, False, 'Список включенных протоколов'),
            ('ssllib', None, str, False,
             'openssl_sni for Modules.SslSni, openssl_sni_extended for Modules.SslSni with some openssl extensions'),
            ('openssl_ciphers', Constants.SSL_CIPHERS_SUITES, str, False, 'Ciphers for Modules.SslSni'),
            ('ssl_priv', None, str, False, 'Путь к файлу с приватным ключом. Прокидывается в Modules.SslSni'),
            ('ssl_cert', None, str, False, 'Путь к файлу с публичным ключом. Прокидывается в Modules.SslSni'),
            ('ssl_priv_secondary', None, str, False,
             'Путь к файлу с вторым приватным ключом. Прокидывается в Modules.SslSni'),
            ('ssl_cert_secondary', None, str, False,
             'Путь к файлу с вторым публичным ключом. Прокидывается в Modules.SslSni'),
            ('ocsp', None, str, False, 'Путь к файлу с DER закодированным OCSP response'),
            ('ocsp_secondary', None, str, False,
             'Путь к файлу с DER закодированным OCSP response для второго сертификата'),
            ('ticket_keys', None, str, False, 'Путь до файла, содержащего ssl ticket key'),
            ('ticket_keys_list', None, OrderedDict, False,
             'Приоритеты и пути до файлов, содержащих ssl ticket key.'),
            ('timeout', '100800s', str, False, 'Таймаут сессий'),
            ('secrets_log', None, str, False, 'Путь до дампа сессионных ключей'),
            ('secrets_log_freq', None, str, False, 'Дефолтная частота дампа сессионных ключей'),
            ('secrets_log_freq_file', None, str, False, 'Ручка управления частотой дампа сессионных ключей'),
            ('events', OrderedDict([('reload_ticket_keys', 'reload_ticket'), ('force_reload_ticket_keys', 'force_reload_ticket'), ('reload_ocsp_response', 'reload_ocsp')]),
             OrderedDict, False, 'Хэндлеры событий'),
            ('sni_contexts', None, OrderedDict, False, 'Описание контекстов не по умолчанию для SNI'),
            ('no_keepalive_file', './controls/keepalive_disabled', str, False, 'Путь к файлу отключения keepalive'),
            ('additional_ip_header', None, str, False,
             'Пробрасывается в макрос Accesslog. См. Описание в нем'),
            ('additional_port_header', None, str, False,
             'Пробрасывается в макрос Accesslog. См. Описание в нем'),
            ('report_enabled', False, bool, False, 'Включает использование модуля Report'),
            ('report_uuid', 'service_total', str, False, 'Набор префиксов сигналов, в которых будут учитываться запросы из данного модуля'),
            ('report_all_default_ranges', True, bool, False, 'Дефолтный сбор статистик по гистограммам'),
            ('report_timeranges', None, str, False, 'Временные интервалы для модуля Report'),
            ('report_client_fail_time_ranges', None, str, False, 'Временные интервалы клиентских запросов для модуля Report'),
            ('report_input_sizes', None, str, False, 'Интервалы размера запроса для модуля Report'),
            ('report_output_sizes', None, str, False, 'Интервалы размера ответа для модуля Report'),
            ('report_refers', None, str, False,
                'Идентификатор модуля Report, в который отправляется статистика модуля подключаемого модуля Report.'),
            ('report_robotness', False, bool, False,
                'Управляет наличием признаков роботности в статистике модуля report.'),
            ('report_sslness', False, bool, False,
                'Управляет наличием признаков эсэсэльности (бггг) в статистике модуля report.'),
            ('report_matcher_map', None, OrderedDict, False, 'Маппинг суффиксов uuid в матчеры для модуля report.'),
            ('http2_alpn_file', None, str, False, 'Имя файла с числом вероятности использования http2, от 0 до 1.'),
            ('http2_alpn_freq', None, float, False, 'Вероятность http2 в отсутствие http2_alpn_file, от 0 до 1.'),
            ('http2_alpn_rand_mode', None, str, False, 'Режим выбора http2.'),
            ('http2_alpn_rand_mode_file', None, str, False, 'Имя файла с режимом выбора http2.'),
            ('http2_alpn_exp_id', None, int, False, 'Идентификатор эксперимента http2'),
            ('http2_refused_stream_file_switch', None, str, False,
                'Отключение перезапросов со стороны браузера в http2'),
            ('exp_static_exps', OrderedDict(), OrderedDict, False,
                'Параметры экспериментов ({exp_name:{exp_id:NUM, cont_id:NUM, salt:STR},...}'),
            ('exp_static_rate_file', './controls/exp_static.switch', str, False, 'Путь к файлу включения экспериментов'),
            ('debug_log_probability', 0.0, float, False, "Вероятность записи debug лога"),
            ('ban_requests_file', None, str, False, 'Путь до файла с правилами бана запросов'),
            ('allow_client_hints_restore', False, bool, False, "Включение восстановления user-agent по client hints"),
            ('client_hints_ua_header', None, str, False, 'Заголовок куда будет выложен восстановленный user-agent'),
            ('client_hints_ua_proto_header', None, str, False, 'Заголовок куда будет выложен протобуф с client hints'),
            ('enable_cycles_protection', True, bool, False, "Включение защиты от зациклившихся запросов"),
            ('max_cycles', 8, int, False, 'Максимальное число прохождений запроса через одну и ту же группу балансеров'),
            ('cycles_header_len_alert', 4096, int, False, 'Максимальная длина загововка защиты от циклов не вызывающая алерты'),
            ('disable_cycles_protection_file', None, str, False, 'Путь до файла отключающего защиту от циклов'),
        ],
        examples=[(
            'ExtendedHttp в адресах',
            '''
                (Macroses.ExtendedHttp, {
                    'port': 17145,
                    'stats_attr': 'addrsbalancer'
                })
            '''
        )]
    )
    def generate(options):
        result = []
        if not options['disable_logs']:
            result.append((Errorlog, {
                'port': options['port'],
            }))

        logs_path = LuaGlobal('LogDir', '/usr/local/www/logs/')

        for v in options.get('exp_static_exps', {}).values():
            opt = v.copy()
            opt['rate_file'] = options['exp_static_rate_file']
            result.append((Modules.ExpStatic, opt))

        if options['ssl_enabled']:
            sslparams = OrderedDict([
                ('cert', options['ssl_cert']),
                ('priv', options['ssl_priv']),
                ('ciphers', options['openssl_ciphers']),
                ('timeout', options['timeout']),
                ('force_ssl', options['force_ssl']),
                ('ja3_enabled', options['ja3_enabled']),
            ])

            for opt in (
                'http2_alpn_file',
                'http2_alpn_freq',
                'http2_alpn_rand_mode',
                'http2_alpn_rand_mode_file',
                'http2_alpn_exp_id',
                'sni_contexts',
                'disable_sslv3',
                'ocsp',
            ):
                if options[opt] is not None:
                    sslparams[opt] = options[opt]

            if options['secrets_log_freq_file'] is not None:
                sslparams['secrets_log'] = logs_path + 'current-secrets_log-{}.log'.format(options['port'])
                sslparams['secrets_log_freq'] = '0'
                sslparams['secrets_log_freq_file'] = options['secrets_log_freq_file']
            elif options['secrets_log'] is not None:
                sslparams['secrets_log'] = LuaFuncCall('GetPathToSecretLog', {})

            if options['ssl_protocols'] is not None:
                sslparams['ssl_protocols'] = OrderedDict(map(lambda x: (LuaAnonymousKey(), x), options['ssl_protocols']))

            if options['ssl_priv_secondary'] is not None and options['ssl_cert_secondary'] is not None:
                secondary = OrderedDict([
                    ('cert', options['ssl_cert_secondary']),
                    ('priv', options['ssl_priv_secondary']),
                ])
                if options['ocsp_secondary']:
                    secondary['ocsp'] = options['ocsp_secondary']

                reload_secondary_ocsp = sslparams.get('events', {}).get('reload_secondary_ocsp')
                if reload_secondary_ocsp is not None:
                    del sslparams['events']['reload_secondary_ocsp']
                    secondary['events']['reload_ocsp_response'] = reload_secondary_ocsp

                sslparams['secondary'] = secondary

            if options['ticket_keys_list'] is not None and options['ticket_keys'] is not None:
                raise SystemExit('Using ticket_keys_list and ticket_keys at same time. Choose one.')
            elif options['ticket_keys_list'] is not None:
                sslparams['ticket_keys_list'] = options['ticket_keys_list']
            elif options['ticket_keys'] is not None:
                sslparams['ticket_keys'] = options['ticket_keys']

            result.append((Modules.SslSni, sslparams))

        http_options = OrderedDict()
        for k in ['maxlen', 'maxreq', 'keepalive', 'stats_attr', 'no_keepalive_file', 'ban_requests_file',
                  'allow_client_hints_restore', 'client_hints_ua_header', 'client_hints_ua_proto_header',
                  'enable_cycles_protection', 'max_cycles', 'cycles_header_len_alert', 'disable_cycles_protection_file']:
            http_options[k] = options[k]

        if options['ssl_enabled'] and (
            options['http2_alpn_file'] is not None or
            options['http2_alpn_exp_id'] is not None or
            options['http2_alpn_freq'] is not None
        ):
            http2_options = OrderedDict([])
            http2_options['debug_log_name'] = logs_path + 'current-http2_debug-{}.log'.format(options['port'])
            http2_options['refused_stream_file_switch'] = options['http2_refused_stream_file_switch']
            result.append((Modules.Http2, http2_options))

        result.append((Modules.Http, http_options))

        if not options['disable_logs']:
            access_log_params = {'port': options['port']}
            if options['additional_ip_header'] is not None:
                access_log_params['additional_ip_header'] = options['additional_ip_header']

            if options['additional_port_header'] is not None:
                access_log_params['additional_port_header'] = options['additional_port_header']

            result.append((Accesslog, access_log_params))

        if options.get('report_enabled', False):
            report_module_params = OrderedDict()
            report_module_params['uuid'] = options['report_uuid']
            report_module_params['all_default_ranges'] = options['report_all_default_ranges']

            if options.get('report_timeranges') is not None:
                report_module_params['ranges'] = options['report_timeranges']

            if options.get('report_client_fail_time_ranges') is not None:
                report_module_params['client_fail_time_ranges'] = options['report_client_fail_time_ranges']

            if options.get('report_input_sizes') is not None:
                report_module_params['input_size_ranges'] = options['report_input_sizes']

            if options.get('report_output_sizes') is not None:
                report_module_params['output_size_ranges'] = options['report_output_sizes']

            if options['report_refers'] is not None:
                report_module_params['refers'] = options['report_refers']

            if options['report_robotness']:
                report_module_params['disable_robotness'] = options['report_robotness']

            if options['report_sslness']:
                report_module_params['disable_sslness'] = options['report_sslness']

            if options['report_matcher_map'] is not None:
                report_module_params['matcher_map'] = options['report_matcher_map']

            result.append((
                Modules.Report, report_module_params
            ))

        return result


class ExtendedHttpV2(IMacro):

    def __init__(self):
        IMacro.__init__(self)

    @staticmethod
    @Helper(
        'Macroses.ExtendedHttpV2',
        '''Копия Macroses.ExtendedHttp для переходного периода.
        Преобразуется в последовательность модулей:
        ---
        Macroses.Errorlog
            > Modules.SslSni
                > Modules.Http
                    > Macroses.Accesslog
                        > Modules.Report
        ---
        Некоторые из модулей могут пропускаться в зависимости от параметров.
        ''',
        [
            (
                'port', None, int, True,
                'Порт для Macroses.Errorlog и Macroses.Accesslog'
            ),
            (
                'logs_path', '/place/db/www/logs', str, False,
                'Директория для логов в Macroses.Errorlog и Macroses.Accesslog'
            ),
            (
                'maxlen', 65536, int, True,
                'Максимальная длина данных, пришедших от клиента. Прокидывается в Modules.Http'
            ),
            (
                'maxreq', 65536, int, True,
                'Максимальная длина тела запроса. Прокидывается в Modules.Http'
            ),
            (
                'keepalive', 1, int, False,
                'Включить keep-alive. Прокидывается в Modules.Http'
            ),
            (
                'fqdn', None, str, False,
                'Имя сервиса, которое используется в параметрах для модуля SslSni'
            ),
            (
                'no_keepalive_file', './controls/keepalive_disabled', str, False,
                'Путь к файлу отключения keepalive'
            ),
            (
                'report_enabled', True, bool, False,
                'Включает использование модуля Report'
            ),
            (
                'report_uuid', 'service_total', str, False,
                'Набор префиксов сигналов, в которых будут учитываться запросы из данного модуля'
            ),
            (
                'report_all_default_ranges', True, bool, False,
                'Включает сбор статистики для всех гистограм'
            ),
            (
                'report_timeranges', None, str, False,
                'Временные интервалы для модуля Report'
            ),
            (
                'report_client_fail_time_ranges', None, str, False,
                'Временные интервалы клиентских фейлов для модуля Report'
            ),
            (
                'report_input_sizes', None, str, False,
                'Интервалы размера запроса для модуля Report'
            ),
            (
                'report_output_sizes', None, str, False,
                'Интервалы размера ответа для модуля Report'
            ),
            (
                'report_matcher_map', None, OrderedDict, False,
                'Маппинг суффиксов uuid в матчеры для модуля Report.'
            ),
            (
                'report_refers', None, str, False,
                'Идентификатор модуля Report, в который отправляется статистика модуля подключаемого модуля Report.'
            ),
            (
                'disable_report_robotness', True, bool, False,
                'Управляет наличием признаков роботности в статистике модуля report.'
            ),
            (
                'disable_report_sslness', True, bool, False,
                'Управляет наличием признаков эсэсэльности (бггг) в статистике модуля report.'
            ),
            (
                'disable_error_log', False, bool, False,
                'Disable error log on balancers'
            ),
            (
                'disable_access_log', False, bool, False,
                'Disable access log on balancers'
            ),
            (
                'additional_ip_header', None, str, False,
                'Пробрасывается в макрос Accesslog. См. Описание в нем'
            ),
            (
                'additional_port_header', None, str, False,
                'Пробрасывается в макрос Accesslog. См. Описание в нем'
            ),
            (
                'ssl_enabled', False, bool, False,
                'Enable ssl on balancer'
            ),
            (
                'disable_sslv3', None, bool, False,
                'Disable sslv3 on balancer'
            ),
            (
                'ssl_protocols', ['tlsv1', 'tlsv1.1', 'tlsv1.2', 'tlsv1.3'], list, False,
                'Список включенных протоколов'
            ),
            (
                'openssl_ciphers', Constants.SSL_CIPHERS_SUITES, str, False,
                'Ciphers for Modules.SslSni'
            ),
            (
                'ocsp_enabled', False, bool, False,
                'Использовать или нет ocsp stapling response'
            ),
            (
                'ssl_secondary_postfix', None, str, False,
                'Постфикс имен файлов для вторых сертификатов'
            ),
            (
                'events',
                OrderedDict([
                    ('reload_ticket_keys', 'reload_ticket'),
                    ('force_reload_ticket_keys', 'force_reload_ticket'),
                    ('reload_ocsp_response', 'reload_ocsp')
                ]),
                OrderedDict,
                False,
                'Хэндлеры событий'
            ),
            (
                'ticket_keys_enabled', False, bool, False,
                'Использовать или нет сессионые ключи для tls.'
            ),
            (
                'timeout', '100800s', str, False,
                'Таймаут tls сессий'
            ),
            (
                'ssl_sni_log', True, bool, False,
                'Логгирования работы модуля ssl_sni'
            ),
            (
                'secrets_log', None, str, False,
                'Путь до файла с фокусами'
            ),
            (
                'sni_contexts', None, OrderedDict, False,
                'Описание контекстов не по умолчанию для SNI'
            ),
            (
                'ssl_files_dir', '/dev/shm/balancer', str, False,
                'Директория, в которой распложены файлы с секретами'
            ),
            (
                'ocsp_files_dir', 'ocsp', str, False,
                'Директория, в которой распложены файлы с секретами'
            ),
            (
                'force_ssl', True, bool, False,
                'Запрет обработки запросов пришедших по http в ssl секциях.'
            ),
            (
                'lua_global_postfix', '', str, False,
                'Кастомизатор имен переменных для LuaGlobal'
            ),
        ],
        examples=[(
            'ExtendedHttpV2 в адресах',
            '''
                (Macroses.ExtendedHttpV2, {
                    'port': 17145,
                })
            '''
        )]
    )
    def generate(options):
        result = []

        if not options['disable_error_log']:
            result.append((Errorlog, {
                'port': options['port'],
                'logs_path': options['logs_path']
            }))

        if options['ssl_enabled']:
            if options['fqdn'] is None:
                raise ValueError('fdqn options is obvious for correct params of SslSni module')

            sslparams = OrderedDict([
                ('cert', LuaGlobal(
                    'public_cert{}'.format(options['lua_global_postfix']),
                    '%s/allCAs-%s.pem' % (options['ssl_files_dir'], options['fqdn'])
                )),
                ('priv', LuaGlobal(
                    'private_cert{}'.format(options['lua_global_postfix']),
                    '%s/priv/%s.pem' % (options['ssl_files_dir'], options['fqdn'])
                )),
                ('ciphers', options['openssl_ciphers']),
                ('timeout', options['timeout']),
                ('force_ssl', options['force_ssl'])
            ])

            if options['sni_contexts'] is not None:
                sslparams['sni_contexts'] = options['sni_contexts']

            if options['disable_sslv3'] is not None:
                sslparams['disable_sslv3'] = options['disable_sslv3']

            if options['ssl_protocols'] is not None:
                sslparams['ssl_protocols'] = OrderedDict(map(lambda x: (LuaAnonymousKey(), x), options['ssl_protocols']))

            if options['ssl_sni_log']:
                sslparams['log'] = \
                    LuaGlobal('LogDir', options['logs_path']) + \
                    '/current-ssl_sni-balancer-{}'.format(options['port'])

            if options['secrets_log'] is not None:
                sslparams['secrets_log'] = LuaFuncCall('GetPathToSecretLog', {})

            if options['ocsp_enabled']:
                sslparams['ocsp'] = '%s/%s%s.der' % (
                    options['ocsp_files_dir'],
                    'allCAs-' if options['ocsp_files_dir'] == 'ocsp' else '',
                    options['fqdn']
                )
                sslparams['ocsp_file_switch'] = './controls/disable_ocsp'

            if options['ssl_secondary_postfix'] is not None:
                secondary = OrderedDict([
                    ('cert', LuaGlobal(
                        'public_cert_secondary',
                        '%s/allCAs-%s_%s.pem' % (
                            options['ssl_files_dir'],
                            options['fqdn'],
                            options['ssl_secondary_postfix']
                        )
                    )),
                    ('priv', LuaGlobal(
                        'private_cert_secondary',
                        '%s/priv/%s_%s.pem' % (
                            options['ssl_files_dir'],
                            options['fqdn'],
                            options['ssl_secondary_postfix']
                        )
                    )),
                ])
                if 'ocsp_enabled':
                    secondary['ocsp'] = '%s/%s%s_%s.der' % (
                        options['ocsp_files_dir'],
                        'allCAs-' if options['ocsp_files_dir'] == 'ocsp' else '',
                        options['fqdn'],
                        options['ssl_secondary_postfix']
                    )

                sslparams['secondary'] = secondary

            if options['ticket_keys_enabled']:
                sslparams['ticket_keys_list'] = OrderedDict([
                    (LuaAnonymousKey(), OrderedDict([
                        ('keyfile', '%s/priv/%s.%s.key' % (options['ssl_files_dir'], key, options['fqdn'])),
                        ('priority', 1000 - i)
                    ])) for i, key in enumerate(['1st', '2nd', '3rd'])
                ])

            result.append((Modules.SslSni, sslparams))

        http_options = OrderedDict()
        for k in ['maxlen', 'maxreq', 'keepalive', 'no_keepalive_file']:
            if options[k] is not None:
                http_options[k] = options[k]
        result.append((Modules.Http, http_options))

        if not options['disable_access_log']:
            access_log_params = {
                'port': options['port'],
                'logs_path': options['logs_path']
            }

            if options['additional_ip_header'] is not None:
                access_log_params['additional_ip_header'] = options['additional_ip_header']

            if options['additional_port_header'] is not None:
                access_log_params['additional_port_header'] = options['additional_port_header']

            result.append((Accesslog, access_log_params))

        if options.get('report_enabled', False):
            report_module_params = {
                'uuid': options['report_uuid'],
                'disable_robotness': options['disable_report_robotness'],
                'disable_sslness': options['disable_report_sslness'],
                'all_default_ranges': options['report_all_default_ranges'],
            }

            if options.get('report_timeranges') is not None:
                report_module_params['ranges'] = options['report_timeranges']

            if options.get('report_client_fail_time_ranges') is not None:
                report_module_params['client_fail_time_ranges'] = options['report_client_fail_time_ranges']

            if options.get('report_input_sizes') is not None:
                report_module_params['input_size_ranges'] = options['report_input_sizes']

            if options.get('report_output_sizes') is not None:
                report_module_params['output_size_ranges'] = options['report_output_sizes']

            if options['report_refers'] is not None:
                report_module_params['refers'] = options['report_refers']

            if options['report_matcher_map'] is not None:
                report_module_params['matcher_map'] = options['report_matcher_map']

            result.append((
                Modules.Report, report_module_params
            ))

        return result


class CaptchaInRegexp(IMacro):

    def __init__(self):
        IMacro.__init__(self)

    @staticmethod
    @Helper(
        'Macroses.CaptchaInRegexp',
        '''
            Обрабатывает капчу.
            Должен находиться прямо внутри Modules.Regexp.
            Этот модуль матчится по урлам, соотвествующим запросам к капче (Constants.CAPTCHASEARCH_URI).
            Все параметры, вроде timeout-ов и количества попыток зашиты внутрь этого модуля
        ''',
        [
            ('backends', None, list, True, 'Список backend-ов капчи'),
            ('backends_new', None, list, False, 'Список backend-ов капчи для миграции'),
            ('connection_attempts', 1, int, False, 'Количество попыток коннекта к backend'),
            (
                'report_stats', False, bool, False,
                'Подключает модуль report для сбора статистики.'
            ),
            (
                'report_robotness', False, bool, False,
                'Управляет наличием признаков роботности в статистике модуля report.'
            ),
            (
                'report_sslness', False, bool, False,
                'Управляет наличием признаков SSL в статистике модуля report.'
            ),
            ('regexp_module', False, bool, False, 'Определяет тип матчера.'),
            (
                'take_ip_from_header', 'X-Real-IP', str, False,
                'Заголовок, откуда надо брать ip для балансировки по subnet',
            ),
            ('proxy_options', None, OrderedDict, True, 'proxy_options для balancer2'),
        ],
        examples=[(
            'Капча в click/web/news балансере',
            '''
                (Macroses.CaptchaInRegexp, {
                    'backends': [
                        'MSK_ANTIROBOT_ANTIROBOT',
                        'MSK_ANTIROBOT_ANTIROBOT_PRESTABLE'
                    ]
                })
            '''
        )]
    )
    def generate(options):
        modules = []

        if options['report_stats']:
            modules.append(
                (Modules.Report, {
                    'uuid': 'captchasearch',
                    'disable_robotness': options['report_robotness'],
                    'disable_sslness': options['report_sslness'],
                })
            )

        antirobot_opts = {
            'stats_attr': 'antirobotbalancer_captcha-backend',
            'backends': options['backends'],
            'connection_attempts': options['connection_attempts'],
            'report_robotness': options['report_robotness'],
            'report_sslness': options['report_sslness'],
            'proxy_options': options['proxy_options'],
            'take_ip_from_header': options['take_ip_from_header'],
            'cut_request_bytes': 65536,
        }

        if options['backends_new']:
            antirobot_opts['backends_new'] = options['backends_new']

        modules.extend([
            (Modules.Antirobot, antirobot_opts),
            (Modules.ErrorDocument, {'status': 403}),
        ])

        return [(
            'captcha',
            {
                'pattern': Constants.CAPTCHASEARCH_URI,
                'case_insensitive': True,
            },
            modules,
        )]


class WebHeaders(IMacro):

    def __init__(self):
        IMacro.__init__(self)

    @staticmethod
    @Helper('Macroses.WebHeaders',
            '''Преобразуется в Modules.Headers.
            Внутри макроса зашит спискок хедеров, которые нужно добавить к запросу''',
            [
                (
                    'X-Start-Time_weak', False, bool, False,
                    'Добавлять X-Start-Time, если хедера не было в запросе, или добавлять всегда.'
                ),
                (
                    'X-Forwarded-For-Y_weak', False, bool, False,
                    'Добавлять X-Forwarded-For-Y, если хедера не было в запросе, или добавлять всегда.'
                ),
                (
                    'X-Source-Port-Y_weak', False, bool, False,
                    'Добавлять X-Source-Port-Y, если хедера не было в запросе, или добавлять всегда.'
                ),
                (
                    'X-Req-Id_weak', False, bool, False,
                    'Добавлять X-Req-Id, если хедера не было в запросе, или добавлять всегда.'
                ),
                (
                    'X-Yandex-IP', False, bool, False,
                    'Добавлять в запрос ip балансера, на который пришёл запрос'
                ),
                (
                    'X-Yandex-IP_weak', False, bool, False,
                    'Добавлять в запрос ip балансера, на который пришёл запрос'
                ),
                (
                    'X-Yandex-RandomUID', False, bool, False,
                    'Добавлять X-Yandex-RandomUID с полем yuid. Актуально для экспериментов.'
                ),
                (
                    'X-Forwarded-For', False, bool, False,
                    'Добавлять X-Forwarded-For с полем realip всегда.'
                ),
                (
                    'X-Forwarded-For_weak', False, bool, False,
                    'Добавлять X-Forwarded-For с полем realip, если хедера не было в запросе.'
                ),
                (
                    'X-Yandex-HTTPS', False, bool, False,
                    'Добавлять в запрос X-Yandex-HTTPS с полем yes.'
                ),
                (
                    'X-Forwarded-Proto', False, bool, False,
                    'Добавлять в запрос X-Forwarded-Proto со схемой запроса.'
                ),
                (
                    'X-Yandex-HTTP-Version', False, bool, False,
                    'Добавлять в запрос версию протокола HTTP MINOTAUR-856'
                ),
                (
                    'X-HTTPS-Request', False, bool, False,
                    'Добавлять в запрос X-HTTPS-Request с полем yes. Необходим в рамках TRAFFIC-245'
                ),
                (
                    'X-Yandex-Family-Search', False, bool, False,
                    'Добавлять в запрос X-Yandex-Family-Search с полем yes. Актуален для семейного поиска.'
                ),
                (
                    'X-Yandex-Search-Proxy', False, bool, False,
                    'Добавлять в запрос хедер X-Yandex-Search-Proxy с локальным ip. Используется в региональных прокси.'
                ),
                (
                    'X-Yandex-HTTPS-Info', False, bool, False,
                    'Добавлять заголовок X-Yandex-HTTPS-Info ssl handshake info, время ssl'
                ),
                (
                    'X-Yandex-TCP-Info', False, bool, False,
                    'Добавлять заголовок X-Yandex-TCP-Info с метриками Tcp соединения',
                ),
                (
                    'X-Yandex-Ja3', False, bool, False,
                    'Добавляет заголовок X-Yandex-Ja3 с ja3 fingerprint строкой',
                ),
                (
                    'X-Yandex-Ja4', False, bool, False,
                    'Добавляет заголовок X-Yandex-Ja4 с ja4 fingerprint строкой',
                ),
                (
                    'X-Yandex-P0f', False, bool, False,
                    'Добавляет заголовок X-Yandex-P0f с p0f fingerprint строкой',
                ),
                (
                    'Y-Balancer-Experiments', False, bool, False,
                    'Добавлять заголовок Y-Balancer-Experiments с экспериментами, заведёнными, через exp_static'
                ),
                (
                    'X-Yandex-Balancing-Hint', None, str, False,
                    'Добавить X-Yandex-Balancing-Hint.'
                ),
                (
                    'Vary', None, list, False,
                    'Добавление Vary с указанным значением'
                ),
                (
                    'X-Antirobot-Service-Y', None, str, False,
                    'Добавлять в запрос X-Antirobot-Service-Y'
                ),
                (
                    'remove_from_resp_HSTS-Report', False, bool, False,
                    'Удалять из ответа хедеров Strict-Transport-Security и X-Yandex-Report-Type'
                ),
                (
                    'remove_Laas-Headers', False, bool, False,
                    'Удалять из запроса хедеров X-Region-* и laas_answer_header(X-LaaS-Answered).'
                ),
                (
                    'remove_Antirobot-Headers', False, bool, False,
                    'Удалять из запроса хедеров X-Yandex-Internal-Request X-Yandex-Suspected-Robot.'
                ),
                (
                    'remove_X-Yandex-HTTPS', False, bool, False,
                    'Удалять из запроса хедерa X-Yandex-HTTPS'
                ),
                (
                    'remove_X-Yandex-Internal-Flags', False, bool, False,
                    'Удалять X-Yandex-Internal-Flags внутренний заголовок'
                ),
                (
                    'remove_X-Yandex-Report-Type', False, bool, False,
                    'Удалять из запроса X-Yandex-Report-Type внутренний заголовок TRAFFIC-3050'
                ),
                (
                    'remove_X-Thdb-Version', False, bool, False,
                    'Удалять X-Thdb-Version внутренний заголовок с версией тумбовой базы'
                ),
                (
                    'remove_X-Yandex-Balancing-Hint', False, bool, False,
                    'Удалять X-Yandex-Balancing-Hint'
                ),
                (
                    'remove_Yandex-Sovetnik-Cookie', False, bool, False,
                    'Удаление заголовка советинка'
                ),
                ('add_nel_headers', False, bool, False, 'Добавляет в ответ заголовки для nel'),
                ('disable_response_headers', False, bool, False, 'Исключает добавление ResponseHeaders'),
                ('remove_blacklisted_headers', False, bool, False, 'Удаляет заголовки из BLACKLISTED_HEADERS'),
                ('enable_shadow_headers', False, bool, False, 'Включить сохранение заголовков'),
                ('log_reqid', False, bool, False, 'Логировать ли reqid.'),
                ('log_yandexuid', False, bool, False, 'Логировать ли yandexuid.'),
                ('log_location', False, bool, False, 'Логировать ли Location.'),
                ('log_gdpr', False, bool, False, 'Логировать ли gdpr, is_gdpr, is_gdpr_b и is_gdpr_m.'),
                ('log_XFFY', False, bool, False, 'Логировать ли X-Forwarded-For-Y'),
                ('log_tcp-info', False, bool, False, 'Логировать ли X-Yandex-TCP-Info'),
                ('log_balancer-experiments', False, bool, False, 'Логировать ли Y-Balancer-Experiments'),
                ('log_user-agent', False, bool, False, 'Логировать ли User-Agent'),
                ('log_yandex-ip', False, bool, False, 'Логировать ли X-Yandex-IP'),
                ('log_XFF', False, bool, False, 'Логировать ли X-Forwarded-For'),
                ('log_shadow_XFF', False, bool, False, 'Логировать скопированный X-Forwarded-For'),
                ('log_cookie_meta', False, bool, False, 'Логировать ли имена кук'),
                ('log_set_cookie_meta', None, bool, False, 'Логировать метаданные кук'),
                ('reqid_type', 'reqid', str, False, 'Тип reqid, который будет использоваться'),
                ('remove_blacklisted_headers_http3', None, bool, False, 'MINOTAUR-2915'),
                ('fix_http3_headers', None, bool, False, 'MINOTAUR-2915'),
            ])
    def generate(options):
        result = []

        if options['log_XFF']:
            result.append((Modules.LogHeaders, {
                'name_re': 'X-Forwarded-For'
            }))

        headers_to_create = OrderedDict()
        headers_to_create_func = OrderedDict()
        headers_to_create_func_weak = OrderedDict()

        def parse_opt(opt):
            if opt.endswith('_weak'):
                return opt[:-5], True
            else:
                return opt, False

        assert options['reqid_type'] in ('reqid', 'search_reqid')

        for opt, func in [
            ('X-Start-Time_weak', 'starttime'),
            ('X-Req-Id_weak', options['reqid_type']),
            ('X-Forwarded-For-Y_weak', 'realip', ),
            ('X-Source-Port-Y_weak', 'realport'),
        ]:
            header, weak = parse_opt(opt)
            assert weak
            if options[opt]:
                headers_to_create_func_weak[header] = func
            else:
                headers_to_create_func[header] = func

        for opt, func in [
            ('X-Yandex-RandomUID', 'yuid'),
            ('X-Forwarded-For', 'realip'),
            ('X-Yandex-HTTPS-Info', 'ssl_handshake_info'),
            ('X-Yandex-HTTP-Version', 'proto'),
            ('X-Yandex-TCP-Info', 'tcp_info'),
            ('Y-Balancer-Experiments', 'exp_static'),
            ('X-Forwarded-For_weak', 'realip'),
            ('X-Forwarded-Proto', 'scheme'),
            ('X-Yandex-IP_weak', 'localip'),
            ('X-Yandex-IP', 'localip'),
            ('X-Yandex-Ja3', 'ja3'),
            ('X-Yandex-Ja4', 'ja4'),
            ('X-Yandex-P0f', 'p0f'),
        ]:
            header, weak = parse_opt(opt)
            if weak:
                assert not (options[opt] and options[header])
            if options[opt]:
                if weak:
                    headers_to_create_func_weak[header] = func
                else:
                    headers_to_create_func[header] = func

        for opt, val in [
            ('X-Yandex-Search-Proxy', LuaGlobal('local_ip', 'local_ip')),
            ('X-Yandex-HTTPS', 'yes'),
            ('X-HTTPS-Request', 'yes'),
            ('X-Yandex-Family-Search', 'yes'),
            ('X-Yandex-Balancing-Hint', options['X-Yandex-Balancing-Hint']),
            ('X-Antirobot-Service-Y', options['X-Antirobot-Service-Y']),
            ('Vary', ','.join(options['Vary'] or [])),
        ]:
            header, weak = parse_opt(opt)
            assert not weak
            if options[opt]:
                headers_to_create[header] = val

        headers_to_delete = []

        for opt, hrx in [
            ('remove_Yandex-Sovetnik-Cookie', 'Yandex-Sovetnik-Cookie'),
            ('remove_Laas-Headers', 'X-Region-.*|X-LaaS-Answered'),
            ('remove_X-Yandex-HTTPS', 'X-Yandex-HTTPS'),
            ('remove_X-Yandex-Internal-Flags', 'X-Yandex-Internal-Flags'),
            ('remove_X-Yandex-Report-Type', 'X-Yandex-Report-Type'),
            ('remove_X-Thdb-Version', 'X-Thdb-Version'),
            ('remove_Antirobot-Headers', 'X-Yandex-Internal-Request|X-Yandex-Suspected-Robot'),
            ('remove_X-Yandex-Balancing-Hint', 'X-Yandex-Balancing-Hint'),
        ]:
            if options[opt]:
                headers_to_delete.append(hrx)

        headers = {'fix_http3_headers': options['fix_http3_headers']}

        for dct, attr in [
            (headers_to_create, 'create'),
            (headers_to_create_func, 'create_func'),
            (headers_to_create_func_weak, 'create_func_weak')
        ]:
            if dct:
                headers[attr] = dct

        if options['remove_blacklisted_headers']:
            headers_to_delete += Constants.BLACKLISTED_HEADERS

        if options['remove_blacklisted_headers_http3']:
            headers_to_delete += Constants.BLACKLISTED_HEADERS_HTTP3

        if options['enable_shadow_headers']:
            headers['copy'] = OrderedDict(Constants.SHADOW_HEADERS)

        if headers_to_delete:
            headers['delete'] = '|'.join(headers_to_delete)

        result.append((Modules.Headers, headers))

        response_headers = {
            'create_weak': OrderedDict([
                # ('X-XSS-Protection', '1; mode=block'),  # remove SERP-58607
                ('X-Content-Type-Options', 'nosniff')
            ])
        }

        if options['remove_from_resp_HSTS-Report']:
            response_headers['delete'] = 'Strict-Transport-Security|X-Yandex-Report-Type'

        if options['add_nel_headers']:
            response_headers['create'] = OrderedDict([
                ('NEL', r'{\"report_to\": \"network-errors\", \"max_age\": 86400, \"success_fraction\": 0.001, \"failure_fraction\": 0.1}'),
                ('Report-To', r'{ \"group\": \"network-errors\", \"max_age\": 86400, \"endpoints\": [ { \"url\": \"https://dr.yandex.net/nel\"}]}')
            ])

        if not options['disable_response_headers']:
            result.append((Modules.ResponseHeaders, response_headers))

        headers_to_log = []

        for opt, header in [
            ('log_reqid', 'X-Req-Id'),
            ('log_XFFY', 'X-Forwarded-For-Y'),
            ('log_tcp-info', 'X-Yandex-TCP-Info'),
            ('log_balancer-experiments', 'Y-Balancer-Experiments'),
            ('log_user-agent', 'User-Agent'),
            ('log_yandex-ip', 'X-Yandex-IP'),
            ('log_shadow_XFF', 'Shadow-X-Forwarded-For'),
        ]:
            if options[opt]:
                headers_to_log.append(header)

        cookies_to_log = []

        for opt, cookie in [
            ('log_yandexuid', 'yandexuid'),
            ('log_gdpr', 'gdpr,gdpr_popup,is_gdpr,is_gdpr_b'),
        ]:
            if options[opt]:
                cookies_to_log.append(cookie)

        if headers_to_log or cookies_to_log or options['log_set_cookie_meta'] or options['log_cookie_meta'] or options['log_location']:
            log_headers = OrderedDict()
            if headers_to_log:
                log_headers['name_re'] = '|'.join(headers_to_log)
            if cookies_to_log:
                log_headers['cookie_fields'] = ','.join(cookies_to_log)
            if options['log_location']:
                log_headers['response_name_re'] = 'Location'
            log_headers['log_set_cookie_meta'] = options['log_set_cookie_meta']
            log_headers['log_cookie_meta'] = options['log_cookie_meta']
            result.append((Modules.LogHeaders, log_headers))

        return result


class BindAndListen(IMacro):

    def __init__(self):
        IMacro.__init__(self)

    @staticmethod
    @Helper(
        'Macroses.BindAndListen',
        '''Генератор секций для ipdispatch''',
        [
            ('port', None, int, True, 'Порт инстанса'),
            ('outerport', 80, int, True, 'Порт сервиса'),
            ('outerports', None, list, False, 'Список внешних портов сервиса'),
            ('domain', None, str, True, 'FQDN сервиса'),
            ('stats_attr', 'service_total', str, False, 'Атрибут статистики'),
            ('modules', None, list, True, 'Набор модулей, вложенных в ipdispatch секцию'),
            ('section_name', None, str, False, 'Кастомизатор имени секции ipdispatch'),
            ('no_localips', False, bool, False, 'Отбрасываем секции для локальных адесов.'),
            ('resolve_ips_via_dns', False, bool, False, 'Get ips via dns request.'),
            ('ipv6_only', False, bool, False, 'Используем только v6 адрес машины'),
        ]
    )
    def generate(options):

        if options['resolve_ips_via_dns']:
            ips = Utils.ResolveIps({'domain': options['domain']})
        else:
            ips = Utils.GetIpsFromRT({'domain': options['domain']})

        result = [
            (
                '{}_{}'.format(
                    options['section_name'] if options['section_name'] else 'remote',
                    options['outerports'][0] if options['outerports'] is not None else options['outerport']
                ),
                {
                    'ips': ips,
                    'port': options['outerport'],
                    'stats_attr': options['stats_attr'],
                    'slb': options['domain'],
                    'disabled': LuaGlobal('SkipBind', False),
                },
                options['modules']
            ),
            (
                'localips_%s' % options['port'],
                {
                    'ips': [
                        LuaFuncCall('GetIpByIproute', {'key': 'v4'}),
                        LuaFuncCall('GetIpByIproute', {'key': 'v6'})
                    ] if not options['ipv6_only'] else [LuaFuncCall('GetIpByIproute', {'key': 'v6'})],
                    'port': options['port'],
                    'stats_attr': options['stats_attr'],
                },
                options['modules']
            )
        ]

        if options['outerports'] is not None:
            result[0][1].pop('port')
            result[0][1]['ports'] = options['outerports']

        if options['no_localips']:
            result.pop(1)

        return result


class SlbPing(IMacro):

    def __init__(self):
        IMacro.__init__(self)

    @staticmethod
    @Helper(
        'Macroses.SlbPing',
        '''
            Делает секцию для чеков от слб с возможностью закрытия, должен находится внутри модуля Regexp
        ''',
        [
            ('backends', [], list, False, 'Список бекендов для проксирования.'),
            ('errordoc', False, bool, False, 'Опция отдавать чеки с балансера, по дефолту прокидываем в бекенды'),
            ('check_pattern', None, str, False, 'Паттерн для чека'),
            ('regexp_host_pattern', None, str, False, 'VirtualHost для опции pattren в модуле RegexpHost'),
            ('prefix_path_route', None, str, False, 'URI для опции route в модуле PrefixPathRouter'),
            ('balancer_type', 'weighted2', str, False, 'Тип балансировки'),
            ('connect_timeout', '0.15s', str, False, 'Опции для backend-ов'),
            ('backend_timeout', '10s', str, False, 'Опции для backend-ов'),
            ('keepalive_count', 0, int, False, 'Опции для backend-ов'),
            ('attempts', 5, int, False, 'Количество попыток задания запросов к backend-у'),
            ('need_resolve', None, bool, False, 'Всегда брать адрес бекенда из cached_ip вместо резолвинга хостнеймов'),
        ]
    )
    def generate(options):
        submodules = []

        if sum(options[name] is not None for name in ['check_pattern', 'regexp_host_pattern', 'prefix_path_route']) > 1:
            raise Exception(
                'Simultaneously using of check_pattern and regexp_host_pattern and prefix_path_route is prohibited'
            )
        if not options['backends'] and not options['errordoc']:
            raise Exception('One of `backends` or `errordoc` options must be used')

        if options['errordoc']:
            submodules.append(
                (Modules.ErrorDocument, {'status': 200})
            )
        else:
            proxy_options = OrderedDict([
                ('connect_timeout', options['connect_timeout']),
                ('backend_timeout', options['backend_timeout']),
                ('keepalive_count', options['keepalive_count']),
                ('need_resolve', options['need_resolve']),
            ])

            submodules.append(
                (Modules.Balancer2, {
                    'policies': OrderedDict([
                        ('unique_policy', {})
                    ]),
                    'backends': options['backends'],
                    'attempts': options['attempts'],
                    'resolve_protocols': [6, 4],
                    'stats_attr': 'backends',
                    'balancer_type': options['balancer_type'],
                    'proxy_options': proxy_options
                })
            )

        if options['regexp_host_pattern']:
            regexp_options = {'pattern': options['regexp_host_pattern']}
        elif options['prefix_path_route']:
            regexp_options = {'route': options['prefix_path_route']}
        else:
            regexp_options = {'match_fsm': OrderedDict([('url', options['check_pattern'])])}

        modules = [
            ('slb_ping', regexp_options, [
                (Modules.Balancer, {
                    'balancer_type': 'rr',
                    'attempts': 1,
                    'balancer_options': OrderedDict([
                        ('weights_file', './controls/slb_check.weights')
                    ]),
                    'custom_backends': [
                        (1, 'to_upstream', submodules),
                        (-1, 'switch_off', [
                            (Modules.ErrorDocument, {'status': 503}),
                        ]),
                    ],
                })
            ])
        ]

        return modules


class MacroCheckReply(IMacro):
    def __init__(self):
        IMacro.__init__(self)

    @staticmethod
    @Helper(
        'static',
        '''Отвечает на пинги, отдает вес''',
        [
            ('name', 'noc_static_check', str, False, 'Имя секции проверки'),
            ('uri', Constants.NOC_STATIC_CHECK, str, False, 'Uri  запроса'),
            ('regexp', False, bool, False, 'Использовать Regexp вместо RegexpPath'),
            ('push_checker_address', None, bool, False, 'Добавлять ip чекеров в кеш cpu limiter-а'),
            ('zero_weight_at_shutdown', None, bool, False, 'Понижать вес до нуля при shutdown'),
            ('force_conn_close', None, bool, False, 'Принудительно разрывать keepalive после ответа, нужно для L3'),
        ]
    )
    def generate(options):
        modules = []

        modules.append((Modules.Report, {
            'uuid': options['name'],
        }))
        modules.append((Modules.Meta, {
            'id': 'upstream-info',
            'fields': OrderedDict([
                ('upstream', options['name']),
            ]),
        }))
        modules.append((Modules.Balancer2, {
            'policies': OrderedDict([('unique_policy', {})]),
            'balancer_type': 'rr',
            'attempts': 1,
            'balancer_options': OrderedDict([
                ('weights_file', LuaGlobal('WeightsDir', './controls/') + '/l7_noc_check'),
            ]),
            'custom_backends': [
                (1, 'return_200_weighted', [(Modules.ActiveCheckReply, {
                    'default_weight': 10,
                    'weight_file': './controls/l7_noc_check_weight',
                    'push_checker_address': options['push_checker_address'],
                    'zero_weight_at_shutdown': options['zero_weight_at_shutdown'],
                    'force_conn_close': options['force_conn_close'],
                })]),
                (-1, 'return_503', [(Modules.ErrorDocument, {'status': 503, 'content': 'echo NO'})]),
                (-1, 'return_200', [(Modules.ErrorDocument, {'status': 200, 'content': 'echo ok'})]),
            ]
        }))
        if options['regexp']:
            return [(options['name'], {'match_fsm': OrderedDict([('URI', options['uri'])])}, modules)]
        else:
            return [(options['name'], {'pattern': options['uri']}, modules)]

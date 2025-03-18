#!/skynet/python/bin/python
# -*- coding: utf8 -*-
from __future__ import print_function
import constants as Constants
import copy
import hashlib
import random
import itertools
import operator

from collections import defaultdict, OrderedDict
from utils import get_instances, resolve_host, load_listen_addrs, DummyInstance, ValidateStatusCodes
from lua_globals import LuaComment, LuaGlobal, LuaAnonymousKey, LuaPrefixes, LuaFuncCall, LuaBackendList, LuaProdOrTesting
from optionschecker import OptionsChecker, OptionValue, Helper, apply_options_checker
from parsecontext import ParseContext

backendPortShiftLuaGlobal = LuaGlobal('BackendPortShift', 0)


def gen_backends_name(backends, sep, preproc):
    def _get_backend_name(backend):
        if isinstance(backend, OrderedDict) or isinstance(backend, dict):
            return '{}#{}'.format(backend['cluster_name'], backend['endpoint_set_id'])
        return backend

    if isinstance(backends, LuaProdOrTesting):
        return LuaProdOrTesting(
            prod_groups=gen_backends_name(backends.prod_groups, sep, preproc),
            testing_groups=gen_backends_name(backends.testing_groups, sep, preproc),
        )
    elif isinstance(backends, list):
        return sep.join(preproc(_get_backend_name(backend)) for backend in backends)
    else:
        raise Exception('Unknown type for backends: ' + backends.__class__.__name__)


def gen_dyn_backends_name(backends):
    return gen_backends_name(backends, sep=',', preproc=lambda x: x)


def gen_check_backends_name(backends):
    return gen_backends_name(backends, sep='&', preproc=lambda x: x.replace(',', '!'))


# dict with mapping some_hash: balancer backends
BALANCER_BACKENDS_BY_HASH = dict()


@Helper(
    'Modules.Hasher',
    '''Cчитает хэш запроса. Используется при балансировке по хешу.
    Есть следующие варианты хеша:
        * **barnavig** - хеш CGI-параметра uid (специально для barnavig-а)
        * **subnet** - хэш по маске IP-адреса клиента (используется в антироботе для того,
            чтобы посылать запросы "из одного места" на один и тот же backend)
        * **request** - хэш по телу запроса,
        * **text** - хэш по CGI-параметру text. Используется только первое значение.
            При отсутствии значения в качестве хэша используется случайное число
    ''',
    [
        ('mode', None, str, True, 'Вариант хеша'),
        ('take_ip_from', None, str, False, 'Название хедера, из которого нужно брать Ip-адрес (для subnet)'),
        ('subnet_v4_mask', None, int, False, 'Маска для IPv4'),
        ('subnet_v6_mask', None, int, False, 'Маска для IPv6'),
    ],
    examples=[
        ('''Hasher в web-е для антиробот-а''', '''(Modules.Hasher, { 'mode' : 'subnet' } ),'''),
    ]
)
def Hasher(options):
    if options['mode'] != 'subnet' and options['take_ip_from'] is not None:
        raise Exception("Option 'take_ip_from' disabled for mode '%s'" % options['mode'])

    result = {'hasher': options}
    return LuaPrefixes(), result, result['hasher']


@Helper(
    'Modules.HeadersHasher',
    '''
        Cчитает хэш запроса по заголовку. Используется при балансировке по хешу.
    ''',
    [
        (
            'header_name', None, str, True,
            'Регулярка, описывающая заголовки, по значению которых будет вычисляться хэш'
        ),
        (
            'surround', None, bool, False,
            'Флаг, определяющий включение PIRE опции surround (.*regexp.*) для header_name'
        ),
        (
            'randomize_empty_match', None, bool, True,
            '''Флаг, определяющий назначение случайного значения хэша в случае,
            если значения целевых заголовков отсутствуют или имеют нулевую длину'''
        )
    ],
    examples=[]
)
def HeadersHasher(options):
    result = {'headers_hasher': options}
    return LuaPrefixes(), result, result['headers_hasher']


@Helper(
    'Modules.CookieHasher',
    'Cчитает хэш от поля указанной cookie. Используется при балансировке по хешу',
    [
        ('cookie', None, str, True, 'Имя cookie'),
        (
            'file_switch', './controls/disable_cookie_hasher', str, False,
            'Файловый флаг для отключения вычисления хеша'
        ),
    ],
    examples=[]
)
def CookieHasher(options):
    result = {'cookie_hasher': options}
    return LuaPrefixes(), result, result['cookie_hasher']


@Helper(
    'Modules.CgiHasher',
    '''Cчитает хэш запроса из заданных CGI-параметров. Используется при балансировке по хешу.
    Есть следующие параметры:
        * **parameters** - список cgi параметров, от которых надо считать хэш,
            обязательно должен быть минимум один параметр
        * **randomize_empty_match** - нужно ли выставлять случайный хэш,
            если не нашли указанных параметров, по умолчанию true
        * **case_insensitive** - - считать ли параметры из parameters регистро-независимыми, по умолчанию false.
    ''',
    [
        ('parameters', None, list, True, 'Список CGI-параметров'),
        ('combine_hashes', None, bool, False, 'Наследовать хэш'),
        ('mode', None, str, False, 'priority/concatenated, default=concatenated'),
        ('randomize_empty_match', None, bool, False, 'Рандомизация при отсутствии параметра'),
        ('case_insensitive', None, bool, False, 'Учет регистра'),
    ],
    examples=[
        (
            'Hasher для хождения в кеши у тумбов Я.Картинок',
            '''
                (Modules.CgiHasher, {
                    'parameters' : ['id']
                })
            '''
        ),
    ]
)
def CgiHasher(options):
    options['parameters'] = OrderedDict(map(lambda x: (LuaAnonymousKey(), x), options['parameters']))
    result = {'cgi_hasher': options}
    return LuaPrefixes(), result, result['cgi_hasher']


@Helper(
    'Modules.ActiveCheckReply',
    '''
       Генерирует ответ на пинг c собственным весом
    ''',
    [
        ('default_weight', 10, int, False, 'Дефолтный вес'),
        ('weight_file', None, str, False, 'Путь к файлу для переопределения веса'),
        ('push_checker_address', False, bool, False, 'Добавлять ip чекеров в кеш cpu limiter-а'),
        ('zero_weight_at_shutdown', False, bool, False, 'Понижать вес до нуля при shutdown'),
        ('force_conn_close', True, bool, False, 'Принудительно разрывать keepalive после ответа, нужно для L3'),
        ('use_header', None, bool, False, 'Отдавать вес в хедере'),
        ('use_body', None, bool, False, 'Отдавать вес в теле ответа'),
    ],
    examples=[]
)
def ActiveCheckReply(options):
    return LuaPrefixes(), {"active_check_reply": options}, None


@Helper(
    'Modules.ErrorDocument',
    '''
        Отдает статический контент с указанным HTTP статусом.
        Если указан файл, из которого брать содержимое, то он читается при старте балансера;
        замена или удаление файла после старта балансера ни на что не влияет
    ''',
    [
        ('status', 404, int, True, 'Статус ответа'),
        (
            'file', None, str, False,
            '''Файл с содержимым, которое нужно отослать клиенту.
            Не может быть использован одноверменно с **content** и **base64**.'''
        ),
        (
            'content', None, str, False,
            '''Содержимое, которое нужно отослать клиенту.
            Не может быть использован одновременно с **file** и **base64**.'''
        ),
        (
            'base64', None, str, False,
            '''Содержимое, которое нужно отослать клиенту закодированное base64.
            Не может быть использован одновременно с **file** и **content**.'''
        ),
        (
            'remain_headers', None, str, False,
            '''Регулярное выражение, нужное для определения заголовков запроса,
            которые должны попасть в ответ.'''
        ),
        (
            'force_conn_close', None, bool, False,
            'Опция определяющая необходимость закрытия соединения после отправки сообщения.'
        ),
    ],
    examples=[
        (
            'Отдача статуса "Service unavailable" и файла с ошибкой',
            '''
                (Modules.ErrorDocument, {
                    'status' : 503,
                    'file' : '/wwwdata/503.html'
                })
            '''
        )
    ]
)
def ErrorDocument(options):
    if sum(options[name] is not None for name in ['file', 'content', 'base64']) > 1:
        raise Exception("You can use only one of 'file', 'content' and 'base64 options simultaneously")

    result = {'errordocument': options}

    return LuaPrefixes(), result, None


@Helper(
    'Modules.Redirects',
    '''
        Отдает редиректы
    ''',
    [
        ('actions', None, list, True, 'редиректы'),
    ],
    examples=[(
        '''
            (Modules.Redirects, {
                'actions' : [
                    OrderedDict(
                        src='//maps.yandex.ru/*',
                        redirect=OrderedDict(
                            dst='https://yandex.ru/maps/{path}{query}',
                            code=301
                        )
                    ),
                    OrderedDict(
                        src='//maps.yandex.ru/robots.txt',
                        forward=OrderedDict(
                            dst='//yastatic.net/s3/maps/robots.txt',
                            modules=[
                                (Modules.Proxy, {
                                    'host': '2a02:6b8:20::215',
                                    'port': 80,
                                    'backend_timeout': '10s'
                                })
                            ]
                        )
                    )
                ]
            })
        '''
    )]
)
def Redirects(options):
    def copy_except(dst, src, exc):
        for k in filter(lambda x: x not in exc, src.keys()):
            dst[k] = src[k]

    prefixes = LuaPrefixes()
    result = {
        'redirects': OrderedDict(
            actions=OrderedDict()
        )
    }
    res_actions = result['redirects']['actions']

    for act in options['actions']:
        res_act = OrderedDict(
            src=act['src']
        )
        res_actions[LuaAnonymousKey()] = res_act
        if bool(act.get('redirect')) == bool(act.get('forward')):
            raise Exception("Exactly one of 'forward' or 'redirect' must be in {}".format(act))
        if 'redirect' in act:
            res_act['redirect'] = act['redirect']
        elif 'forward' in act:
            act_src, act_dst = act['forward'], res_act.setdefault('forward', OrderedDict())
            sub_prefix, mods = SeqProcessor(copy.deepcopy(act_src['modules']))
            prefixes += sub_prefix
            act_dst.update(copy.deepcopy(mods))
            act_dst.update(act_src)
            act_dst.pop('modules')

    return prefixes, result, result['redirects']


@Helper(
    'Modules.Regexp',
    '''
        Направляет запрос в один из подмодулей (**dispatcher**) в зависимости от того,
        какому регулярному выражению он удовлетворяет. На соответствие можно проверять
        URI, CGI, полный запрос, хост, произвольный хедер.

        Также можно проверять IP-адрес отправителя, который задается в виде объединения
        масок подходящих IP-адресов: **77.88.3.0/8,1.2.3.4,77.88.4.0/8,77.88.5.32/4**.
        Может быть указана специальная сеция **default**, в которую отправляются
        все запросы, не удовлетворяющие ни одному из условий
    ''',
    [
        (
            'match_fsm', None, OrderedDict, False,
            '''
            Проверка одной из частей запроса на соответствие регулярному выражению.
            Обязательно должен присутствовать один из параметров: **match_fsm**, **match_source_ip**, **match_and**
            '''
        ),
        ('match_not', None, OrderedDict, False,
         'Проверка запроса на несоответствие регулярному выражению '
         '(берет единственный матчер и "переворачивает" его результат'),
        ('match_method', None, OrderedDict, False, 'Проверка запросов на соответствие методов'),
        ('match_source_ip', None, OrderedDict, False,
         'Проверка IP-адреса пользователя на соответствие регулярному выражению'),
        ('match_and', None, OrderedDict, False,
         'Проверка запроса на одновременное соответствие нескольким регулярным выражениям'),
        ('match_or', None, OrderedDict, False,
         'Проверка запроса на соответствие одному из нескольких регулярных выражений'),
        ('match_if_file_exists', None, OrderedDict, False,
         'Проверка на существование локального файла'),
        ('priority', None, int, False, 'Приоритет секции. Выставляется автоматически и не может быть изменен.'),
    ],
    examples=[
        (
            'Разделение секций в адресах',
            '''
            (Modules.Regexp, [
                ('geocoder', {'match_fsm': {'match': '(GET|POST) +(http://[^/]+)?/+maps\\\\?.*'}}, [
                    (Modules.Balancer, {
                        'balancer_type': 'hashing',
                        'backends': 'ALL_ADDRS_GEOCODER(msk)',
                        'stats_attr': 'addrsbalancer-backend',
                        'connect_timeout': '0.03s',
                        '2tier_balancer': True,
                    }),
                ]),
                ('default', {}, [
                    (Modules.Balancer, {
                        'balancer_type': 'hashing',
                        'backends': 'ALL_ADDRS_GEOCODER(msk)',
                        'stats_attr': 'addrsbalancer-backend',
                        'connect_timeout': '0.03s',
                        '2tier_balancer': True,
                    }),
                ]),
            ])
            '''
        ),
        (
            'Разделение секций в новостях',
            '''
            (Modules.Regexp, [
                ('postedit', {'match_fsm': {'URI': '/edit' }}, [
                    (Modules.ErrorDocument, { 'status' : 403 }),
                ]),
                ('sub_nginx_proxy', {'match_fsm': {'URI': Constants.NEWS_NGINX_URI}}, [
                    (Modules.Antirobot, {
                        'backends': options['antirobot_backends'],
                        'attempts': 1,
                        'connection_attempts': 3,
                        'stats_attr': 'balancer-antirobot_query'
                    }),
                    (Modules.Balancer, {
                        'backends': options['nginx_backends']
                    }),
                ]),
            ]),
            '''
        )
    ]
)
def _Regexp(options):
    matchers = ['match_fsm', 'match_source_ip', 'match_method']
    for l, r in itertools.combinations(matchers, 2):
        if options[l] is not None and options[r] is not None:
            raise Exception('At most one of [' + ', '.join(matchers) + '] options should be in options')

    def recurse_patch(name, data):
        if name == 'match_fsm':
            if 'case_insensitive' not in data:
                data['case_insensitive'] = False
            if 'surround' not in data:
                data['surround'] = False
        elif name == 'match_and' or name == 'match_or':
            for k, v in data.iteritems():
                recurse_patch(k, v)
        elif name == 'match_method':
            data['methods'] = OrderedDict(map(lambda x: (LuaAnonymousKey(), x), data['methods']))

    for k, v in options.iteritems():
        if v is not None:
            recurse_patch(k, v)

    return LuaPrefixes(), options, options


def Regexp(options):
    p = len(options) + 1
    for section, params, modules in options:
        params['priority'] = p
        p -= 1

    result_prefix, result_dict, result_next = _Dispatcher(options, _Regexp)
    return result_prefix, {'regexp': result_dict}, None


@Helper(
    'Modules.PrefixPathRouter',
    '''
    Модуль для роутинга запросов по URI
    ''',
    [
        ('route', None, str, False, 'path'),
    ],
    examples=[]
)
def _PrefixPathRouter(options):

    return LuaPrefixes(), options, options


def PrefixPathRouter(options):
    result_prefix, result_dict, result_next = _Dispatcher(options, _PrefixPathRouter)
    result_dict['case_insensitive'] = True
    return result_prefix, {'prefix_path_router': result_dict}, None


@Helper(
    'Modules.RegexpHost',
    '''
    Модуль для роутинга запросов по хедеру Host
    ''',
    [
        ('pattern', None, str, False, 'Регулярное выражение, описывающее хедр Host'),
        ('case_insensitive', True, bool, False, 'Учет регистра'),
    ],
    examples=[]
)
def _RegexpHost(options):

    return LuaPrefixes(), options, options


def RegexpHost(options):
    result_prefix, result_dict, result_next = _Dispatcher(options, _RegexpHost)
    return result_prefix, {'regexp_host': result_dict}, None


@Helper(
    'Modules.RegexpPath',
    '''
    Модуль для роутинга запросов по регулряному выражению для URI
    ''',
    [
        ('pattern', None, str, False, 'Регулярное выражение, описывающее URI'),
        ('case_insensitive', True, bool, False, 'Учет регистра'),
        ('priority', None, int, False, 'Приоритет секции. Выставляется автоматически.'),
    ],
    examples=[]
)
def _RegexpPath(options):
    return LuaPrefixes(), options, options


def RegexpPath(options):
    p = len(options) + 1
    for section, params, modules in options:
        params['priority'] = p
        p -= 1

    result_prefix, result_dict, result_next = _Dispatcher(options, _RegexpPath)
    return result_prefix, {'regexp_path': result_dict}, None


@Helper(
    'Modules.ResponseMatcher',
    '',
    [
        ('match_not', None, OrderedDict, False,
         'Проверка запроса на несоответствие регулярному выражению '
         '(берет единственный матчер и "переворачивает" его результат'),
        ('match_and', None, OrderedDict, False,
         'Проверка запроса на одновременное соответствие нескольким регулярным выражениям'),
        ('match_or', None, OrderedDict, False,
         'Проверка запроса на соответствие одному из нескольких регулярных выражений'),
        ('priority', None, int, False, 'Приоритет секции. Выставляется автоматически и не может быть изменен.'),
        ('match_header', None, OrderedDict, False, 'Матчит хедер'),
        ('match_response_codes', None, OrderedDict, False, 'Матчит статус коды'),
    ],
    examples=[]
)
def _ResponseMatcher(options):
    return LuaPrefixes(), options, options


def ResponseMatcher(options):
    p = len(options)
    for section, params, modules in options:
        params['priority'] = p
        p -= 1

    result_prefix, result, _ = _Dispatcher(options, _Regexp)
    result = {'on_response': result, 'module': {}}
    return result_prefix, {'response_matcher': result}, result['module']


@Helper(
    'Modules.Ipdispatch',
    '''
    Напраляет запрос в разные подмодули (**dispatcher**) в зависимости от адреса и порта,
    к которому присоединился клиент
    ''',
    [
        ('ip', None, str, False, 'IP-адрес, к которому присоединяется клиент.'),
        ('ips', None, list, False, 'Список IP-адресов, к которым присоединяется клиент (в случае, если их больше 1)'),
        ('port', None, int, False, 'Порт, к которому присоединяется клиент'),
        (
            'ports', None, list, False,
            'Список портов, к которым может присоединяться клиент '
            '(в случае, если их больше 1)'
        ),
        ('stats_attr', '', str, False, 'Имя атрибута для статистики golovan-а'),
        ('slb', None, str, False, 'Slb, соответствующий данной секции'),
        ('disabled', False, bool, False, 'Если опция включена, мы не слушаем на соответствующем порту'),
        ('name', None, str, False, 'Имя секции')
    ],
    examples=[(
        'Dispatcher в barnavig-е',
        '''
        (Modules.Ipdispatch, [
            ('MISC_BARNAV', {'ip': '*', 'port': 9200, 'stats_attr': 'barnavigbalancer'}, [
                ...
            ]),
            ('MISC_BARNAV_ssl', {'ip': '*', 'port': 9201, 'stats_attr': 'barnavigbalancer_ssl'}, [
                ...
            ]),
            ('MISC_BARNAV_AMS', {'ip': '*', 'port': 9202, 'stats_attr': 'barnavigbalancer'}, [
                ...
            ]),
        ])
        '''
    )],
)
def _Ipdispatch(options):
    options['slb'] = None
    options['disabled'] = None

    if options['ips'] is not None:
        if options['name'] is not None:
            options['ips'] = LuaGlobal('ipdispatch_' + options['name'], options['ips'])
            options['name'] = None
        else:
            options['ips'] = OrderedDict(map(lambda x: (LuaAnonymousKey(), x), options['ips']))
    if options['ports'] is not None:
        options['ports'] = OrderedDict(map(lambda x: (LuaAnonymousKey(), x), options['ports']))

    return LuaPrefixes(), options, options


def Ipdispatch(options):
    # check if all addresses are different
    load_listen_addrs(map(lambda x: x[1], options))

    result_prefix, result_dict, result_next = _Dispatcher(options, _Ipdispatch)
    result_dict['events'] = OrderedDict([('stats', 'report')])
    return result_prefix, {'ipdispatch': result_dict}, None


@Helper(
    'Modules.Http',
    '''
    Парсит HTTP, выделяет непосредственно сам запрос, хедеры, контент и т.д., отрезает лишние хедеры.
    Поддерживает keep-alive соединения.
    Кроме проверки на валидность в этом модуле также выполняются проверки на размер запроса
    ''',
    [
        (
            'maxlen', 65536, int, False,
            '''
            Максимальная длина запроса (без данных), пришедшего от клиента.
            Включает в себя первую строку запроса и все хедеры.
            '''
        ),
        ('maxreq', 65536, int, False, 'Максимальная длина первой строки запроса, пришедшего от клиента.'),
        ('stats_attr', '', str, False, 'Имя атрибута для статистики golovan-а'),
        ('keepalive', 1, int, False, 'Включение либо выключение поддержки keep-alive с клиентом'),
        ('allow_trace', None, int, False,
         'Разрешает обрабатывать TRACE запросы, на которые по умолчанию балансер отвечает 405.'),
        ('no_keepalive_file', './controls/keepalive_disabled', str, False,
         'Путь до файла, в случае существования которого отключается keepalive'),
        ('events', OrderedDict([('stats', 'report')]), OrderedDict, False,
         'Ивенты для сбора статистики через модуль Report'),
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
        'Http для improxy',
        '''
        (Modules.Http, {
            'maxlen': 16 * 1024,
            'maxreq': 8 * 1024,
            'stats_attr': 'improxy_balancer'
        })
        '''
    )],
)
def Http(options):
    result = {'http': options}
    return LuaPrefixes(), result, result['http']


@Helper(
    'Modules.Http2',
    """
    Включает поддержку http2, ставится выше Modules.Http
    """,
    [
        ('goaway_debug_data_enabled', 0, int, False, 'Отладочный вывод в GOAWAY'),
        ('debug_log_enabled', 1, int, False, 'Включение отладочного вывода'),
        ('debug_log_name', '/usr/local/www/logs/current-http2_debug.log', str, False, 'Имя дебаг лога'),
        ('debug_log_freq', 0., float, False, 'Частота сессий с отладочным выводом'),
        ('debug_log_freq_file', './controls/http2_debug.ratefile', str, False, 'Файл с частотой отладочного вывода'),
        ('refused_stream_file_switch', None, str, False,
            'Отключение перезапросов со стороны браузера в http2'),
        ('events', OrderedDict([('stats', 'report')]), OrderedDict, False,
            'Ивенты для сбора статистики через модуль Report'),
    ]
)
def Http2(options):
    result = {'http2': options}
    return LuaPrefixes(), result, result['http2']


@Helper(
    'Modules.Accesslog',
    '''Пишет accesslog''',
    [
        ('additional_ip_header', None, str, False,
         'Добавляет в access лог дополнительное поле с значением из из поле указанного хедера'),
        ('additional_port_header', None, str, False,
         'Добавляет в access лог дополнительное поле с значением из из поле указанного хедера'),
        ('log', None, str, True, 'Путь к лог-файлу'),
    ],
    examples=[(
        'Accesslog для improxy',
        '''
        (Modules.Accesslog, {
            'log': logDirLuaGlobal + '/imgcbir.access.log'
        }),
        '''
    )],
)
def Accesslog(options):
    result = {'accesslog': options}
    return LuaPrefixes(), result, result['accesslog']


@Helper(
    'Modules.Errorlog',
    '''Пишет errorlog''',
    [
        ('log', None, str, True, 'Путь к лог-файлу'),
        ('log_level', 'ERROR', str, False,
         'Уровень логирования. Допусистимы следующие значения: CRITICAL, ERROR, INFO, DEBUG.')
    ],
    examples=[(
        'Errorlog для improxy',
        '''
        (Modules.Errorlog, {
            'log': logDirLuaGlobal + '/imgcbir.error.log'
        }),
        '''
    )]
)
def Errorlog(options):
    result = {'errorlog': options}
    return LuaPrefixes(), result, result['errorlog']


HEADERS_PARAMS = [
    (
        'delete', '', str, False,
        'Регулярное выражение, описывающее хедеры, которые нужно удалить'
    ),
    (
        'create', OrderedDict(), OrderedDict, False,
        "Список хедеров, которые нужно добавить, в формате 'имя' : 'значение'"
    ),
    (
        'create_weak', OrderedDict(), OrderedDict, False,
        'То же, что и **create**, но добавляются только отсутствующие в запросе хедеры'
    ),
    (
        'create_func', OrderedDict(), OrderedDict, False,
        """
        Список хедеров, которые нужно добавить, в формате 'имя хедера' : 'функция',
        где в качестве функции могу выступать:
            * **realip** - IP-адрес клиента
            * **realport** - Порт клиента
            * **starttime** - Время старта выполнения запроса
            * **reqid** - Уникальный идентификатор запроса, генерящийся балансером
            * **url** - URL запроса
            * **location** - значение хедера Host + URL запроса
        """
    ),
    (
        'create_func_weak', OrderedDict(), OrderedDict, False,
        'То же, что и **create_func**, но добавляются только отсутствующие в запросе хедеры'
    ),
    (
        'copy', OrderedDict(), OrderedDict, False,
        'Скопировать заголовок в новый заголовок'
    ),
    ('fix_http3_headers', None, bool, False, 'MINOTAUR-2915'),
]

HEADERS_EXAMPLES = [(
    'Хедеры для web-а',
    '''
        (Modules.Headers, {'create_func': OrderedDict([
            ('X-Forwarded-For-Y', 'realip'),
            ('X-Source-Port-Y','realport'),
            ('X-Start-Time', 'starttime'),
            ('X-Req-Id', 'reqid')
        ])}),
    '''
)]


@Helper(
    'Modules.Headers',
    'Добавляет или удаляет хедеры в запросах.',
    HEADERS_PARAMS,
    examples=HEADERS_EXAMPLES
)
def Headers(options):
    for k in options.keys():
        if not options[k]:
            options.pop(k)

    result = {'headers': options}
    return LuaPrefixes(), result, result['headers']


@Helper(
    'Modules.ResponseHeaders',
    'Добавляет или удаляет хедеры в ответах.',
    HEADERS_PARAMS,
    examples=HEADERS_EXAMPLES
)
def ResponseHeaders(options):
    for k in options.keys():
        if not options[k]:
            options.pop(k)

    result = {'response_headers': options}
    return LuaPrefixes(), result, result['response_headers']


# BALANCER-870
@Helper(
    'Modules.ResponseHeadersIf',
    'Добавляет или удаляет хедеры в ответах если был встречен заголовок-триггер',
    [
        ('if_has_header', None, str, True, 'Регулярка по названию заголовка, игнорирует значения поля'),
        ('create_header', None, OrderedDict, True, 'таблица вида имя-заголовка = значение-заголовка'),
        ('delete_header', None, str, False, 'Регулярка для удаления произовльного заголовка'),
        ('erase_if_has_header', True, bool, False, 'Удалять ли заголовок тригер'),
    ],
    examples=[(
        'X',
        '''
            response_headers_if = {
                -- регулярка с названием заголовка,
                -- при наличии которого будем добавлять новые заголовки
                if_has_header = "Trigger";

                -- таблица вида имя-заголовка = значение-заголовка
                create_header = {
                    ["Led"] = "Zeppelin";
                };

                -- удалить хидер some_header_to_delete
                delete_header = 'some_header_to_delete';

                -- флаг, определяющий, нужно ли удалять заголовок по регулярке if_has_header
                erase_if_has_header = true;
                -- submodule here
                }
        '''
    )],
)
def ResponseHeadersIf(options):

    result = {'response_headers_if': options}
    return LuaPrefixes(), result, result['response_headers_if']


@Helper(
    'Modules.HeadersForwarder',
    'Передавать заголовки из запроса прямо в ответ, опционально изменяя их названия.',
    [(
        'actions', [], list, False,
        '''
            Список правил:
                'request_header', None, str, True,
                    Имя заголовка запроса;
                'response_header', None, str, True,
                    Имя заголовка, в которой нужно положить значение из request_header;
                'erase_from_request', False, bool, False,
                    Вырезать значение из заголовка перед запросом в бекенд;
                'erase_from_response', False, bool, False,
                    Вырезать значение из заголовка от бекенда. Если не включать,
                    то в ответе будет два заголовка ответа с разными значениями;
                'weak', False, bool, False,
                    Если не включать, то при наличии заголовка ответа в ответе бэкэнда,
                    ничего перезаписоваться не будет. Нельзя одновременно включать
                    erase_from_response и weak.
        '''
    )],
    examples=[(
        'CORS support',
        '''
            (Modules.HeadersForwarder, {
                'actions': [({
                    'request_header': 'Origin',
                    'response_header': 'Access-Control-Allow-Origin',
                    'erase_from_request': True,
                    'erase_from_response': True,
                    'weak': False
                })]
            })
        '''
    )]
)
def HeadersForwarder(options):
    result = {'headers_forwarder': OrderedDict()}
    result['headers_forwarder']['actions'] = OrderedDict()
    for action in options['actions']:
        result['headers_forwarder']['actions'][LuaAnonymousKey()] = action
    return LuaPrefixes(), result, result['headers_forwarder']


@Helper(
    'Modules.Static',
    '''
    Отдает статичиские данные.
    Работает в двух вариантах:
        * либо отдает файл из указанной директории по названию
        * либо отдает один и тот же файл на любой запрос.
    В пером случае при отсутствии файла возвращается 404
    ''',
    [
        (
            'dir', None, str, False,
            '''
            Root для директории, откуда будут отадаватья файлы.
            Не может использоваться одновременно с директовой file.
            Одна из директив dir или file должна присуствовать.
            '''
        ),
        (
            'file', None, str, False,
            '''
            Файл, содержимое которого нужно отдать в ответ на любой запрос.
            Не может использоваться одновременно с директивой dir.
            Одна из директив file или dir должна присутствовать.
            '''
        ),
        (
            'index', None, str, False,
            '''Имя файла, который балансер отдаст по запросу /.
            По умолчанию index.html
            '''
        ),
    ],
)
def Static(options):
    if options['file'] is not None and options['dir'] is not None:
        raise Exception("You must use only one of ['file', 'dir']")
    result = {'static': options}

    return LuaPrefixes(), result, None


class MetaBalancerGenerator(type):

    def __new__(cls, class_name, bases, attrs):
        res = super(MetaBalancerGenerator, cls).__new__(cls, class_name, bases, attrs)
        res.OPTIONS = res.BASE_OPTIONS + res.SPECIFIC_OPTIONS
        return res


class BalancerGenerator(object):
    __metaclass__ = MetaBalancerGenerator

    MODULE_NAME = 'balancer'

    BASE_OPTIONS = [
        ('balancer_type', 'weighted2', str, False, 'Тип балансировки'),
        ('attempts', 5, int, False, 'Количество попыток задания запросов к backend-у'),
        (
            'active_skip_attempts',
            None,
            int,
            False,
            '''
                Определяет количество попыток обращений к мертвым бекендам, которые не влияют на attempts.
                BALANCER-385.
                Используется только в балансировках типа active.
            '''
        ),
        ('balancer_options', OrderedDict(), OrderedDict, False,
         'Опции балансера, специфичные для каждого типа балансировки'),
        ('proxy_options', OrderedDict(), OrderedDict, False, 'Опции для backend-ов'),
        ('stats_attr', 'backends', str, False, 'Имя для атрибута статистики'),
        (
            '2tier_balancer', False, bool, False,
            '''
            Сделать балансер двухуровневым:
            * сначала балансируется между группами машин, находящихся в разных кластерах,
            * затем внутри этой группы
            '''
        ),
        (
            '2tier_attempts', None, int, False,
            '''
            Количество попыток у "внутреннего" балансера при двухуровневой балансировке.
            Этот параметр можно устанавливать только тогда, когда включена двухуровневая балансировка
            '''
        ),
        ('custom_backends', None, list, False,
         'Список backend-ов балансера, которые представляют собой пару (вес, модуль).'),
        (
            'backends', None, list, False,
            '''
            Список backend-ов, на которые смотрит балансер.
            Представляет собой либо группу генерилки, либо intlookup, из которого берутся инстансы
            '''
        ),
        ('no_statistics', None, bool, False, 'Не собирать статистику для backend-а'),
        ('timelines', None, str, False, 'Тайминги для статистики по времени работы'),
        (
            'on_error', None, list, False,
            '''
            Набор модулей для обработки ошибки в случае,
            когда балансер за нужное количество попыток не смог получить нормального ответа.
            '''
        ),
        (
            'buffering', None, bool, False,
            '''
            Накапливать полный ответ на балансере перед отправкой клиенту.
            Позволяет легко переключиться на другой бэкенд,
            если пришло начало ответа, а окончание долго не приходит.
            '''
        ),
        ('shift_ports', False, bool, False, 'Добавлять переменную для сдвига портов.Применяется для балансерных бет'),
        (
            'weights_file', None, str, False,
            'Файл с весами статической балансировки (только для balancer_type == rr, экспериментально для weighted2)'
        ),
        ('resolve_protocols', [6, 4], list, False, 'Список протоколов для ресловинга имен в адреса'),
        (
            'override_domain', None, str, False,
            '''
            Подменять домен у инстансов.
            Hапример .yandex.ru -> .search.yandex.net, чтобы записать в cached_ip IPv6 адрес
            '''
        ),
        ('backends_shuffle_seed', None, int, False, 'Перемешать backend-ы в соотвествии с указанным seed-ом'),
        ('rewind_limit', None, int, False, 'Размер тела, по достижении которого отключаются перезапросы и on_error'),
        ('attempts_file', None, str, False, 'Возможность переопределять количество попыток через ИТС - BALANCER-256.'),
    ]

    SPECIFIC_OPTIONS = [
        ('global_timeout', None, str, False, 'Общий таймаут на обработку запросов'),
    ]

    OPTIONS = BASE_OPTIONS + SPECIFIC_OPTIONS

    LUA_PREFIX = '''function genProxyOptions(data, backend)
    local retval = {
        host = backend['host'];
        port = backend['port'];
        cached_ip = backend['cached_ip'];
    };

    for optname, optvalue in pairs(data) do
        if optname ~= 'backends' and optname ~= 'stats_attr' and optname ~= 'balancer_options' and optname ~= 'timelines' then
            retval[optname] = optvalue
        end
    end

    return retval;
end

function genProxy(data, backend)
    local retval = {
        weight = backend['weight'];
    };

    retval.proxy = genProxyOptions(data, backend)

    return retval;
end

function genBalancer(data)
    result = {}

    for option_name, option_value in pairs(data['balancer_options']) do
        result[option_name] = option_value
    end

    backends = data['backends']
    for index, backend in pairs(backends) do
        result[backend['name'] or index] = genProxy(data, backend)
    end

    return result
end

function GetLog(data)
    return data["dir"] .. data["prefix"] .. tostring(data["port"])
end

function ReplaceGlobalVariables()
    if globals_replace_map then
        local json = require("json")
        local file, err = io.open(globals_replace_map, "r")
        if file == nil then
            error("Error opening mocked backend map: " .. err)
        end
        globals_replace_map = json.decode(file:read("*a"));
        io.close(file)
        for k in pairs(_G) do
            local is_backends = string.sub(k, 1, 9) == "backends_";
            local is_ips = string.sub(k, 1, 11) == "ipdispatch_";
            local is_instance_addrs = string.sub(k, 1, 9) == "instance_";
            local is_port = string.sub(k, 1, 5) == "port_";
            if is_backends or is_ips or is_instance_addrs or is_port then
                local new_value = globals_replace_map[k]
                if not new_value and is_backends then
                    new_value = globals_replace_map["_"]
                end
                if new_value then
                    _G[k] = new_value
                end
            end
        end
    end
end

function InstallBbrQdisc()
    local handle = io.popen("uname -r")
    local uname = handle:read("*a")
    handle:close()

    local ver = tonumber(string.sub(uname, string.find(uname, "%d+.%d+")))

    if ver < 4.19 then
        return nil
    end

    local handle = io.popen("tc qdisc show dev ip6tnl0 2>/dev/null | head -n 1 | awk '{print $2}'")
    local qdisc = string.gsub(handle:read("*a"), "[\\r|\\n]", "")
    handle:close()

    if qdisc ~= "fq" then
        return nil
    end

    local handle = io.popen("tc qdisc show dev tun0 2>/dev/null | head -n 1 | awk '{print $2}'")
    local qdisc = string.gsub(handle:read("*a"), "[\\r|\\n]", "")
    handle:close()

    if qdisc == "fq" then
        return "bbr"
    end

    if qdisc == "mq" then
        local handle_mq = io.popen("tc qdisc show dev tun0 2>/dev/null | awk '{print $2}' | grep 'fq' | wc -l")
        local handle_data = string.gsub(handle_mq:read("*a"),  "[\\r|\\n]", "")
        handle_mq:close()
        if handle_data == nil then
            return nil
        end

        local fq_count = tonumber(handle_data)

        if fq_count == nil then
            return nil
        end

        local handle_txq = io.popen("find /sys/class/net/tun0/queues/ -name 'tx-*' 2>/dev/null | awk -F 'tx-' '{print $2}' | sort -nr | head -n 1")
        local handle_data = string.gsub(handle_txq:read("*a"),  "[\\r|\\n]", "")
        handle_txq:close()
        if handle_data == nil then
            return nil
        end

        local txq_count = tonumber(handle_data)

        if txq_count == nil then
            return nil
        end

        txq_count = txq_count + 1

        if fq_count ~= txq_count then
            return nil
        end
    end

    return "bbr"
end'''

    PROXY_OPTIONS_CHECKER = OptionsChecker([
        ('backend_timeout', '10s', str, False),
        ('keepalive_count', 0, int, False),
        ('connect_timeout', '0.15s', str, False),
        ('fail_on_5xx', True, bool, False),
        ('http_backend', 1, int, False),
        ('buffering', False, bool, False),
        ('https_settings', None, OrderedDict, False),
        ('need_resolve', None, bool, False),
        # статус-коды или их семейства, которые считаются плохими
        ('status_code_blacklist', None, list, False),
        # исключения из status_code_blacklist (чтобы можно было указать "4xx" в blacklist и 404 в exceptions)
        ('status_code_blacklist_exceptions', None, list, False),
        # ограничит время жизни keepalive соединени в состоянии отсутствия активности
        ('keepalive_timeout', None, str, False),
        # websocket options
        ('backend_read_timeout', None, str, False),
        ('backend_write_timeout', None, str, False),
        ('client_read_timeout', None, str, False),
        ('client_write_timeout', None, str, False),
        ('allow_connection_upgrade', None, bool, False),
    ])

    def process(self, options):
        self.check_options_sanity(options)

        balancer_prefix, balancer_result = self.construct_result()
        self.set_specific_options(options, balancer_result)
        self.set_backend_comment(options, balancer_result)
        self.set_attempts(options, balancer_result)
        self.set_timeout(options, balancer_result)

        is_sd = False
        if options['backends'] is not None:
            is_sd = isinstance(options['backends'][0], OrderedDict) or isinstance(options['backends'][0], dict)

        self.set_balancer_type(options, balancer_result, is_sd)
        self.fill_backends(options, balancer_result, balancer_prefix, is_sd)

        return balancer_prefix, {self.MODULE_NAME: balancer_result}, None

    def check_options_sanity(self, options):
        backend_count = (options['backends'] is not None) + (options['custom_backends'] is not None)
        if backend_count > 1:
            raise Exception("Module balancer: you must specify 'backends' or 'custom_backends' option")
        if backend_count == 0:
            raise Exception("Module balancer: you must specify exaclty one of 'backends' or 'custom_backends' option")
        if options['balancer_options'].get('weights_file') and not (options['balancer_type'] in ['rr', 'weighted2', 'by_location']):
            raise Exception("Module balancer: you can use weight file only with by_location, rr and weighted2 balancing type")

    def construct_result(self):
        balancer_prefix = LuaPrefixes()
        balancer_prefix += self.LUA_PREFIX
        balancer_result = OrderedDict()
        return balancer_prefix, balancer_result

    def set_specific_options(self, options, balancer_result):
        pass

    def set_backend_comment(self, options, balancer_result):
        backends = options['backends']
        if backends and isinstance(backends[0], str) and backends[0][0].istitle():
            balancer_result['backends_group_name'] = LuaComment('group %s' % ','.join(backends))

    def set_attempts(self, options, balancer_result):
        attempts = options['attempts']
        if attempts <= 0:
            raise Exception('Module %s: attempts must be a positive value, not %d' % (self.MODULE_NAME, attempts))
        balancer_result['attempts'] = attempts

        if options['attempts_file'] is not None:
            balancer_result['attempts_file'] = options['attempts_file']

        if options['active_skip_attempts'] is not None:
            if options['balancer_type'] == 'hashing':
                balancer_result['active_skip_attempts'] = options['active_skip_attempts']
            else:
                raise Exception('active_skip_attempts must use with hashing balancer_type only')

    def set_timeout(self, options, balancer_result):
        global_timeout = options['global_timeout']
        if global_timeout:
            balancer_result['timeout'] = global_timeout

    def set_balancer_type(self, options, balancer_result, is_sd):
        balancer_type = options['balancer_type']
        if balancer_type == 'weighted2':
            if len(options['balancer_options']) == 0:
                options['balancer_options'] = OrderedDict([('correction_params', {'feedback_time': 300})])

        if (
            self.MODULE_NAME == 'balancer2' and balancer_type == 'rr'
            and options['balancer_options'].get('randomize_initial_state') is None
        ):
            options['balancer_options']['randomize_initial_state'] = True

        if is_sd:
            balancer_result['sd'] = OrderedDict([
                (balancer_type, OrderedDict(options['balancer_options'])),
            ])
        else:
            balancer_result[balancer_type] = OrderedDict(options['balancer_options'])

    def fill_backends(self, options, balancer_result, balancer_prefix, is_sd):
        if options['custom_backends'] is not None:
            self.fill_custom_backends(options, balancer_result, balancer_prefix)
        elif options['2tier_balancer']:
            self.fill_2tier_backends(options, balancer_result, balancer_prefix)
        elif is_sd:
            self.fill_service_discovery(options, balancer_result, balancer_prefix)
        else:
            self.fill_ordinary_backends(options, balancer_result, balancer_prefix)
        self.add_on_error(options, balancer_result, balancer_prefix)

    def gen_proxy_options(self, options):
        proxy_options = apply_options_checker(options['proxy_options'], self.PROXY_OPTIONS_CHECKER)

        if proxy_options['status_code_blacklist'] is not None:
            ValidateStatusCodes({'codes_list': proxy_options['status_code_blacklist']})
            proxy_options['status_code_blacklist'] = OrderedDict([
                (LuaAnonymousKey(), x) for x in proxy_options['status_code_blacklist']
            ])

        if proxy_options['status_code_blacklist_exceptions'] is not None:
            ValidateStatusCodes({'codes_list': proxy_options['status_code_blacklist_exceptions']})
            proxy_options['status_code_blacklist_exceptions'] = OrderedDict([
                (LuaAnonymousKey(), x) for x in proxy_options['status_code_blacklist_exceptions']
            ])

        if not proxy_options['buffering']:
            proxy_options.pop('buffering')
        if 'connect_timeout' in proxy_options:
            proxy_options['connect_timeout'] = proxy_options.pop('connect_timeout')

        return proxy_options


    def fill_service_discovery(self, options, balancer_result, balancer_prefix):
        for endpoint_set in options['backends']:
            assert isinstance(endpoint_set, OrderedDict)
            assert sorted(endpoint_set.keys()) == ['cluster_name', 'endpoint_set_id']
            assert isinstance(endpoint_set['cluster_name'], str)
            assert isinstance(endpoint_set['endpoint_set_id'], str)

        balancer_result['sd']['endpoint_sets'] = OrderedDict([
            (LuaAnonymousKey(), x) for x in options['backends']
        ])

        balancer_result['sd']['proxy_options'] = self.gen_proxy_options(options)

    def fill_custom_backends(self, options, balancer_result, balancer_prefix):
        for weight, name, backend in options['custom_backends']:
            sub_prefix, balancer_result[options['balancer_type']][name] = SeqProcessor(backend)
            balancer_prefix += sub_prefix
            balancer_result[options['balancer_type']][name]['weight'] = weight

    def fill_2tier_backends(self, options, balancer_result, balancer_prefix):
        backends_by_dc = defaultdict(list)

        backend_instances = get_instances(options['backends'], options['override_domain'])
        if len(backend_instances) == 0:
            raise SystemExit("Group <%s> produce empty instances list" % (options['backends']))

        for backend in backend_instances:
            backends_by_dc[(backend.dc, backend.power)].append(backend)

        for (dc, backend_power), backends in backends_by_dc.items():
            dc_id = '%s_%s' % (dc, hashlib.sha224(str(backend_power)).hexdigest()[:6])

            sub_options = self.copy_options_for_subbalancer(options)
            sub_options['2tier_balancer'] = False
            if 'on_error' in sub_options:
                sub_options.pop('on_error')
            sub_options['backends'] = backends
            if sub_options['2tier_attempts'] is not None:
                sub_options['attempts'] = sub_options['2tier_attempts']
            else:
                sub_options['attempts'] = min(3, options['attempts'])

            sub_prefix, sub_result, _ = self.create_subbalancer(sub_options, options)
            balancer_result[options['balancer_type']][dc_id] = sub_result
            balancer_result[options['balancer_type']][dc_id]['weight'] = sum(x.power for x in backends)
            balancer_prefix += sub_prefix

    def copy_options_for_subbalancer(self, options):
        return copy.copy(options)

    def create_subbalancer(self, sub_options, options):
        return Balancer(sub_options)

    def fill_ordinary_backends(self, options, balancer_result, balancer_prefix):
        luafunc_options = self.gen_proxy_options(options)
        luafunc_options.update({
            'timelines': options['timelines'],
            'balancer_options': options['balancer_options'],
            'stats_attr': options.pop('stats_attr', ''),
        })

        shift_ports = options.pop('shift_ports', False)

        # do not generate backends multiple times (too slow and required to much memory)
        backends_hash_key = (tuple(options['backends']), options['override_domain'], shift_ports)
        if backends_hash_key not in BALANCER_BACKENDS_BY_HASH:
            backend_instances = get_instances(options['backends'], options['override_domain'])
            if options.get('backends_shuffle_seed', None) is not None:
                random.seed(options['backends_shuffle_seed'])
                random.shuffle(backend_instances)

            if len(backend_instances) == 0:
                raise SystemExit("Group <%s> produce empty instances list" % (options['backends']))

            backend_names = set()
            backends = []
            for backend in backend_instances:
                host = backend.hostname
                port = backend.port + (backendPortShiftLuaGlobal if shift_ports else 0)

                name = "{}:{}".format(
                    host,
                    port
                )
                assert name not in backend_names
                backend_names.add(name)

                backends.append(
                    (LuaAnonymousKey(), OrderedDict([
                       ('host', host),
                       ('cached_ip', resolve_host(backend, options['resolve_protocols'])),
                       ('port', port),
                       ('weight', backend.power),
                       ('name', name)
                    ]))
                )
            BALANCER_BACKENDS_BY_HASH[backends_hash_key] = OrderedDict(backends)

        luafunc_options['backends'] = LuaBackendList(BALANCER_BACKENDS_BY_HASH[backends_hash_key], backends_hash_key)

        balancer_result[options['balancer_type']] = LuaFuncCall('genBalancer', luafunc_options)

    def add_on_error(self, options, balancer_result, balancer_prefix):
        on_error = options['on_error']
        if on_error:
            on_error_prefix, balancer_result['on_error'] = SeqProcessor(on_error)
            balancer_prefix += on_error_prefix


@Helper(
    'Modules.Balancer',
    '''
        Балансирует нагрузку на backend-ы.
        Поддерживается несколько различных видов балансировки:
          * **hashing** - по хешу (см. описание модуля hasher);
          * **weighted2** - динамическая (вес источника изменяется в зависимоcти
                от времени ответа и количества неответов);
          * **active** - со статическими весами и активными чеками (для проверки
                работоспособности используется специальный http-запрос);
          * **rr** - weighted round-robin с возможностью динамически применять веса
                из файлика и отключать backend-ы
        Балансер задает до **attempts** попыток различным backend-ам (одному и тому же backend-у
            запрос задается не более одного раза).

        У балансера есть специальный подмодуль arbiter,
        который может сбрасывать количество количество попыток до 1 в случае плохой работы backend-ов
    ''',
    BalancerGenerator.OPTIONS,
    examples=[
        (
            'Мордальные backend-ы в поисковом балансере',
            '''
                (Modules.Balancer, {
                    'backends': Constants.MORDA_INSTANCES_COM,
                    'fail_on_5xx': True,
                    'balancer_type': 'active',
                    'balancer_options': OrderedDict({
                        'request': 'GET /ok.html HTTP/1.1\\nHost: www.yandex.ru\\n\\n',
                        'delay': '10s'
                    })
                })
            '''
        ),
        (
            'Поисковые backend-ы в поисковом балансере',
            '''
                (Modules.Balancer, {
                    'backends': 'VLA_WEB_NMETA_RKUB,VLA_IMGS_NMETA_RKUB',
                    '2tier_balancer': True,
                    'stats_attr': 'balancer-backend',
                }),
            '''
        ),
    ]
)
def Balancer(options):
    generator = BalancerGenerator()

    return generator.process(options)


class Balancer2PolicyChecker(object):

    def __init__(self, options):
        self.options = {}
        for name, value, valuetype, obligatory, description in options:
            self.options[name] = OptionValue(value, valuetype, obligatory)

    def check(self, options):
        obligatory_opts = set(name for name, op in self.options.iteritems() if op.obligatory)
        for name, op in self.options.iteritems():
            if options.get(name) is not None:
                obligatory_opts.discard(name)
                if not isinstance(options[name], op.valuetype):
                    raise Exception('Balancer2PolicyChecker: wrong type of option %s: it is %s, but it should be %s' % (
                        name, options[name].__class__.__name__, op.valuetype.__name__))
            else:
                options[name] = op.value

        if len(obligatory_opts):
            raise Exception('Balancer2PolicyChecker: missing obligatory opts: %s' % obligatory_opts)


class Balancer2Policy(object):
    CHECKER = Balancer2PolicyChecker([])

    def check(self, options):
        self.CHECKER.check(options)
        self.do_check(options)

    def do_check(self, options):
        pass

    def build(self, options, factory):
        self.check(options)
        return self.do_build(options, factory)

    def do_build(self, options, factory):
        raise NotImplemented('do_build() is not implemented for ancestor of Balancer2Policy')


class Balancer2SimplePolicy(Balancer2Policy):

    def do_build(self, options, factory):
        if len(options) != 0:
            raise Exception('Balancer2SimplePolicy must have no options')

        return {}


class Balancer2UniquePolicy(Balancer2Policy):

    def do_build(self, options, factory):
        if len(options) != 0:
            raise Exception('Balancer2UniquePolicy must have no options')

        return {}


class Balancer2RetryPolicy(Balancer2Policy):

    def do_build(self, options, factory):
        if len(options) != 1:
            raise Exception('Balancer2RetryPolicy must have subpolicy')
        for name, opts in options.iteritems():
            return factory.build(name, opts)


class Balancer2TimeoutPolicy(Balancer2Policy):
    CHECKER = Balancer2PolicyChecker([
        ('timeout', None, str, True, 'Таймаут, по достижении которого перезапросы запрещаются'),
    ])

    def do_build(self, options, factory):
        subpolicy_names = []
        subpolicy_name = None
        subpolicy_params = None

        for name, value in options.iteritems():
            if name != 'timeout':
                subpolicy_names.append(name)
                subpolicy_name = name
                subpolicy_params = value

        if len(subpolicy_names) != 1:
            raise Exception(
                'Balancer2TimeoutPolicy: there must be one and only one subpolicy, not %s' % subpolicy_names)

        return {'timeout': options['timeout'],
                subpolicy_name: factory.build(subpolicy_name, subpolicy_params)[subpolicy_name]}


class Balancer2ActivePolicy(Balancer2Policy):
    CHECKER = Balancer2PolicyChecker([
        ('skip_attempts', None, int, False, 'Количество попыток перезапроса'),
    ])

    def do_build(self, options, factory):
        subpolicy_names = []
        subpolicy_name = None
        subpolicy_params = None
        for name, value in options.iteritems():
            if name not in Balancer2ActivePolicy.CHECKER.options.keys():
                subpolicy_names.append(name)
                subpolicy_name = name
                subpolicy_params = value

        if len(subpolicy_names) != 1:
            raise Exception(
                'Balancer2ActivePolicy: there must be one and only one subpolicy, not %s' % subpolicy_names)

        retval = {}

        for opt in Balancer2ActivePolicy.CHECKER.options.keys():
            retval[opt] = options[opt]

        retval.update(factory.build(subpolicy_name, subpolicy_params))
        return retval


class Balancer2WatermarkPolicy(Balancer2Policy):
    CHECKER = Balancer2PolicyChecker([
        ('lo', None, float, True, 'Выключает дозапросы, если success rate < lo'),
        ('hi', None, float, True, 'Включает дозапросы, если success rate > hi'),
        ('coeff', None, float, True, 'Коэффициент забвения'),
        ('switch_default', None, bool, True, 'Включает политику по-умолчанию'),
        ('switch_file', None, str, False, 'Имя файла для переключения политики'),
        ('switch_key', None, str, True, 'Ключ для включения политики в switch-файле'),
        ('is_shared', None, bool, True, 'Использовать общую статистику для всех воркеров'),
        ('params_file', None, str, False, 'Имя файла для динамического управления политикой'),
    ])

    def do_check(self, options):
        def check_val(val, name):
            if val < 0.0:
                raise Exception('Balancer2WatermarkPolicy: "%s" < 0.0 (%f)' % (name, val))
            if val > 1.0:
                raise Exception('Balancer2WatermarkPolicy: "%s" > 1.0 (%f)' % (name, val))

        check_val(options['lo'], 'lo')
        check_val(options['hi'], 'hi')

        if options['lo'] > options['hi']:
            raise Exception('Balancer2WatermarkPolicy: \"lo\" (%f) > (%f) \"hi\"' % (options['lo'], options['hi']))

    def do_build(self, options, factory):
        subpolicy_names = []
        subpolicy_name = None
        subpolicy_params = None

        for name, value in options.iteritems():
            if name not in Balancer2WatermarkPolicy.CHECKER.options.keys():
                subpolicy_names.append(name)
                subpolicy_name = name
                subpolicy_params = value

        if len(subpolicy_names) != 1:
            raise Exception(
                'Balancer2WatermarkPolicy: there must be one and only one subpolicy, not %s' % subpolicy_names)

        retval = {}

        for opt in Balancer2WatermarkPolicy.CHECKER.options.keys():
            retval[opt] = options[opt]

        retval.update(factory.build(subpolicy_name, subpolicy_params))
        return retval


class Balancer2ByNamePolicy(Balancer2Policy):
    # Temporary commented out for use in name str or LuaGlobal
    # CHECKER = Balancer2PolicyChecker([
    #     ('name', None, str, True, 'Идентификатор бекенда'),
    # ])

    def do_build(self, options, factory):
        subpolicy_names = []
        subpolicy_name = None
        subpolicy_params = None

        for name, value in options.iteritems():
            if name != 'name':
                subpolicy_names.append(name)
                subpolicy_name = name
                subpolicy_params = value

        if len(subpolicy_names) != 1:
            raise Exception(
                'Balancer2ByNamePolicy: there must be one and only one subpolicy, not %s' % subpolicy_names)

        return {
            'name': options['name'],
            subpolicy_name: factory.build(subpolicy_name, subpolicy_params)[subpolicy_name]
        }


class Balancer2ByHashPolicy(Balancer2Policy):

    def do_build(self, options, factory):
        if len(options) != 1:
            raise Exception('Balancer2ByHashPolicy must have subpolicy')
        for name, opts in options.iteritems():
            return factory.build(name, opts)


class Balancer2ByNameFromHeaderPolicy(Balancer2Policy):
    CHECKER = Balancer2PolicyChecker([
        ('allow_zero_weights', False, bool, False, 'Разрешает ходить в бэкенды с нулевым весом'),
        ('strict', False, bool, False, 'Запрещает ходить в бэкенды с другими именами'),
        ('header_name', 'X-Yandex-Balancing-Hint', str, False, 'Из какого заголовка брать имя бекенда'),
        ('hints', None, list, True, 'Сопоставление значения заголовка и имени бекенда'),
    ])

    def do_build(self, options, factory):
        subpolicy_names = []
        subpolicy_name = None
        subpolicy_params = None

        for name, value in options.iteritems():
            if name not in Balancer2ByNameFromHeaderPolicy.CHECKER.options.keys():
                subpolicy_names.append(name)
                subpolicy_name = name
                subpolicy_params = value

        if len(subpolicy_names) != 1:
            raise Exception(
                'Balancer2ByNameFromHeaderPolicy: there must be one and only one subpolicy, not %s' % subpolicy_names)

        for hint in options['hints']:
            assert isinstance(hint, OrderedDict)
            assert sorted(hint.keys()) == ['backend', 'hint']
            assert isinstance(hint['hint'], str)
            assert isinstance(hint['backend'], str)

        options['hints'] = OrderedDict([
            (LuaAnonymousKey(), x) for x in options['hints']
        ])

        retval = {}

        for opt in Balancer2ByNameFromHeaderPolicy.CHECKER.options.keys():
            retval[opt] = options[opt]

        retval.update(factory.build(subpolicy_name, subpolicy_params))
        return retval

class Balancer2PolicyFactory(object):

    def __init__(self):
        super(Balancer2PolicyFactory, self).__init__()
        self.policies = {
            'simple_policy': Balancer2SimplePolicy,
            'unique_policy': Balancer2UniquePolicy,
            'retry_policy': Balancer2RetryPolicy,
            'timeout_policy': Balancer2TimeoutPolicy,
            'active_policy': Balancer2ActivePolicy,
            'watermark_policy': Balancer2WatermarkPolicy,
            'by_name_policy': Balancer2ByNamePolicy,
            'by_hash_policy': Balancer2ByHashPolicy,
            'by_name_from_header_policy': Balancer2ByNameFromHeaderPolicy,
        }

    def build(self, name, options):
        policy = self.policies[name]()
        return {name: policy.build(options, self)}

    @classmethod
    def make_policies_options(cls):
        policy_factory = Balancer2PolicyFactory()
        return [(policy_name, None, dict, False, 'balancer2 policy') for policy_name in policy_factory.policies]


@OptionsChecker([
    ('name', None, str, False),
    ('skip', None, bool, False),
    ('quorum', None, float, False),
    ('hysteresis', None, float, False),
    ('amount_quorum', None, int, False),
    ('amount_hysteresis', None, int, False),
])
def check_check_backends_options(options):
    if not options['skip']:
        if options['name'] is None:
            raise Exception('check_backends name is required, unless skip is set')
        if options['quorum'] is None and options['amount_quorum'] is None:
            raise Exception('neither quorum, nor amount_quorum is set in check_backends, but it is required to set at least one of them')
    return options


class Balancer2Generator(BalancerGenerator):
    MODULE_NAME = 'balancer2'
    SPECIFIC_OPTIONS = Balancer2PolicyFactory.make_policies_options() + [
        ('policies', None, OrderedDict, True,
         'Дерево политик перезапросов'),
        ('hedged_delay', None, str, False, 'Время ожидания перед отправкой hedged-запроса в другой бэкенд'),
        ('connection_attempts', 0, int, False, 'Количество попыток на соединение c бэкендом'),
        ('attempts_limit', None, float, False, 'Лимит ограничителя презапросов'),
        ('rate_limiter_coeff', None, float, False, 'Коэффициент ограничителя презапросов'),
        ('rate_limiter_max_budget', None, int, False, 'Максимальный бюджет ограничителя презапросов'),
        ('retry_non_idempotent', True, bool, False, 'Ретраить неидемпотентные запросы (POST, PATCH), работает только в модуле balancer2, в balancer всегда True INFRAINCIDENTS-375'),
        ('check_backends', None, OrderedDict, False, 'Проверка бэкендов'),
    ]

    def set_specific_options(self, options, balancer_result):
        balancer_result['retry_non_idempotent'] = options['retry_non_idempotent']
        balancer_result['hedged_delay'] = options['hedged_delay']
        self.fill_policies(options, balancer_result)
        self.set_connection_attempts(options, balancer_result)
        self.set_attempts_rate_limiter(options, balancer_result)
        self.set_check_backends(options, balancer_result)

    def set_attempts_rate_limiter(self, options, balancer_result):
        limit = options['attempts_limit']

        if limit is None:
            return

        if limit < 0 or limit > options['attempts']:
            raise Exception('invalid limit for attempts_rate_limiter: {}'.format(limit))

        coeff = options['rate_limiter_coeff']
        max_budget = options['rate_limiter_max_budget']

        if coeff and max_budget:
            raise Exception(
                'inconsistent use of old and new attempts rate limiter options: coeff={} together with max_budget={}'.format(
                    coeff, max_budget
                )
            )

        if not coeff and not max_budget:
            coeff = 0.99

        balancer_result['attempts_rate_limiter'] = OrderedDict((
            ('limit', limit),
            ('coeff', coeff),
            ('max_budget', max_budget),
        ))

    def fill_policies(self, options, balancer_result):
        policies_option = options['policies']
        if not policies_option or len(policies_option) != 1:
            raise Exception('there must be only one top-evel policy in balancer2')
        policy_factory = Balancer2PolicyFactory()
        for name, policy_option in policies_option.iteritems():
            policies = policy_factory.build(name, policy_option)
            break
        balancer_result.update(policies)

    def set_connection_attempts(self, options, balancer_result):
        if options['connection_attempts']:
            connection_attempts = options['connection_attempts']
            if connection_attempts > 0:
                balancer_result['connection_attempts'] = connection_attempts
            elif connection_attempts == 0:
                pass
            else:
                raise Exception('Module {}: connection_attempts must be a positive value, not {}'.format(
                    self.MODULE_NAME, connection_attempts
                ))

    def set_timeout(self, options, balancer_result):
        pass

    def set_check_backends(self, options, balancer_result):
        if options['check_backends'] and (options['custom_backends'] is None or options['balancer_type'] == 'by_location'):
            balancer_result['check_backends'] = check_check_backends_options(options['check_backends'])

    def create_subbalancer(self, sub_options, options):
        return Balancer2(sub_options)


@Helper(
    'Modules.Balancer2',
    '''
        Заполните описание
    ''',
    Balancer2Generator.OPTIONS,
    examples=[
        (
            'Первый пример',
            '''
            '''
        ),
    ]
)
def Balancer2(options):
    generator = Balancer2Generator()

    return generator.process(options)


@Helper(
    'Modules.Pinger',
    '''
    Проверяет доступность backend-ов и отвечает на чеки НОКов.
    Работает примерно следующим образом: смотрит все запросы,
    которые проходят через него и собирает статистику успешних (за некоторый промежуток времени).
    Если запросов через него прходит слишком мало, добавляет туда "пингующие" запросы,
    которые сам задает на бэкэнды.
    При превышении некоторого порога плохих ответов,
    начинает закрывать соединение на запросы чекилки от НОК-ов (вместо того, чтобы отдать 200)
    ''',
    [
        (
            'lo', 0.5, float, True,
            '''Если доля успешных запросов упадет ниже <<lo>>,
            модуль будет закрывать соединение (сигнализируя, что backend-ы не работают)'''
        ),
        (
            'hi', 0.7, float, True,
            '''Если доля успешных запросов вырастет выше <<hi>>,
            модуль будет отвечать, что backend-ы работают нормально'''
        ),
        (
            'delay', '5s', str, True,
            '''Промежуток между пингующими запросами.
            Если пингующий запрос длится 1 секунду, то промежуток будет в реальности 6 секунд, а не 5.
            '''
        ),
        (
            'histtime', '10s', str, True,
            '''Временной интервал, по которому считается статистика.
            То есть для определения того, живы ли backend-ы,
            мы берем статистику за последние <<histtime>> секунд'''
        ),
        (
            'ping_request_data',
            r'GET /robots.txt HTTP/1.1\r\nHost: yandex.ru\r\n\r\n',
            str, False,
            '''Запрос, которым пингуются backend-ы в ситуации,
            когда пользовательских запросов слишком мало.'''
        ),
        (
            'admin_request_uri', '/check.txt', str, False,
            '''URI админского запроса.
            Все остальные запросы этот модуль пропускает вниз без изменений,
            а по этому запросу отдает статус backend-ов
            '''
        ),
        ('admin_ips', None, str, False, 'Маски подсетей для админских запросов.'),
        (
            'enable_tcp_check_file', './controls/tcp_check_on', str, False,
            '''Имя файла, при наличии которого проверка отключается
            и модуль всегда рапортует о том, что backend-ы живы.
            Модуль проверяет наличие этого файла 1 раз в секунду.'''
        ),
        (
            'switch_off_file', './controls/slb_check.weights', str, False,
            '''Имя файла, при наличии в котором строки из switch_off_key,
            модуль будет закрывать соединение (сигнализируя, что backend-ы не работают) st/BALANCER-596'''
        ),
        (
            'switch_off_key', None, str, False,
            'Задает строку, значение которой, проверяется в switch_off_file st/BALANCER-596'
        ),
        (
            'admin_error_replier', False, bool, False,
            'В случае закрытия от слб отдавать 503 код вместо ресета st/BALANCER-662'
        ),
    ],
)
def Pinger(options):
    result = {'pinger': options}
    if options['admin_error_replier']:
        admin_error_replier_prefix, result['pinger']['admin_error_replier'] = SeqProcessor(
            [(ErrorDocument, {'status': 503})])
    else:
        result['pinger'].pop('admin_error_replier')
    result['pinger']['module'] = OrderedDict()
    return LuaPrefixes(), result, result['pinger']['module']


@Helper(
    'Modules.Antirobot',
    '''
    Проверяет запросы на то, что они были заданы роботом.
    Состоит из двух частей: checker и submodule.
      * **checker** - посылает запрос в backend-ы антиробота (для проверки, либо для отображения капчи),
      * **submodule** - подмодуль, куда посылается запрос, прошедший валидацию checker-ом
    ''',
    [
        ('attempts', 2, int, False, 'Количество попыток хождения в backend-ы антиробота'),
        ('connection_attempts', 3, int, False, 'Количество попыток приконнектиться к бекенду антиробота'),
        ('proxy_options', OrderedDict(), OrderedDict, False, 'Опции для backend-ов'),
        ('backends', None, list, True, 'Список backend-ов. Представляет собой группу генерилки или имя intlookup-а'),
        ('sink_backends', None, list, False, 'Список дополнительных backend-ов для request replier'),
        ('stats_attr', 'balancer-antirobot_query', str, False, 'Имя атрибута для статистики golovan-а'),
        ('report_robotness', False, bool, False, 'Управляет наличием признаков роботности в статистике модуля report.'),
        (
            'report_sslness', False, bool, False,
            'Управляет наличием признаков эсэсэльности (бггг) в статистике модуля report.'
        ),
        ('timelines', '10ms,20ms,50ms,100ms,1s', str, False, 'Тайминги для графика времен ответа антиробота'),
        (
            'cut_request', True, bool, False,
            'Опция, позволяющая отправлять неполное тело пользовательского запроса антироботному бэкэнду.'
        ),
        (
            'no_cut_request_file', './controls/no_cut_request_file', str, False,
            '''
            При наличии файла запрещается отправлять неполное тело пользовательского запроса бэкэнду,
            при отсутствии - разрешается.
            Эта опция главнее cut_request
            '''
        ),
        (
            'cutter_bytes', 512, int, False,
            '''
            Опция для модуля Cutter.
            Достаточно получить столько байт тела,
            чтобы пропустить запрос вниз до наступления timeout
            '''
        ),
        (
            'cutter_timeout', '0.1s', str, False,
            '''Опция для модуля Cutter.
            Максимальное время ожидание получения bytes байт или полного тела'''
        ),
        (
            'cut_request_bytes', 512, int, False,
            '''Ограничение сверху на размер тела пользовательского запроса.

            Значения по умолчанию 512, согласно:
            anelyubin
            13:50:30 на части запросов точно смотрим. например, xmlsearch бывает GET-ом,
            а бывает POST-ом, когда в теле запрос: <request><query>пластиковые окна</query></request>
            13:50:50 этот запрос мы извлекаем и считаем по нему текстовые факторы
            13:50:58 точнее, Визард для нас

            # Значение 0 согласно BALANCER-791
            '''
        ),
        (
            'file_switch', './controls/disable_antirobot_module', str, False,
            '''Добавляет возможность использования файлового флага для отключения модуля.
            Подробности в BALANCER-868.'''
        ),
        ('take_ip_from_header', 'X-Real-IP', str, False, 'Заголовок, откуда надо брать ip для балансировки по subnet'),
    ],
    examples=[(
        'Antirobot в блогах',
        '''
        (Modules.Antirobot, {
            'backends': [
                'MSK_ANTIROBOT_ANTIROBOT',
                'MSK_ANTIROBOT_ANTIROBOT_PRESTABLE',
                'SAS_ANTIROBOT_ANTIROBOT',
                'SAS_ANTIROBOT_ANTIROBOT_PRESTABLE',
            ]
            'stats_attr': 'balancer-antirobot_query'
        }),
        '''
    )],
)
def Antirobot(options):
    result_prefix = LuaPrefixes()

    proxy_options = options['proxy_options']
    if proxy_options.get('backend_timeout') is None:
        proxy_options['backend_timeout'] = '200ms'
    if proxy_options.get('connect_timeout') is None:
        proxy_options['connect_timeout'] = '0.03s'

    checker_modules = [
        (Report, {
            'uuid': 'antirobot',
            'disable_robotness': options['report_robotness'],
            'disable_sslness': options['report_sslness'],
        })
    ]

    if options['sink_backends']:
        sink_modules = [
            (Report, {
                'uuid': 'antirobot_replier',
                'disable_robotness': options['report_robotness'],
                'disable_sslness': options['report_sslness'],
            }),
            (Balancer2, {
                'balancer_type': 'dynamic_hashing',
                'backends': options['sink_backends'],
                'check_backends': OrderedDict([
                    ('name', gen_check_backends_name(options['sink_backends'])),
                    ('quorum', 0.35),
                ]),
                'attempts': 1,
                'attempts_limit': 0.2,
                'rate_limiter_max_budget': 50,
                'connection_attempts': 2,
                'policies': OrderedDict([
                    ('unique_policy', {})
                ]),
                'proxy_options': proxy_options,
                'balancer_options': OrderedDict([
                    ('active', OrderedDict([
                        ('request', r'GET /ping HTTP/1.1\r\n\r\n'),
                        ('delay', '20s'),
                        ('use_backend_weight', True),
                        ('weight_normalization_coeff', 10),
                    ])),
                ])
            })
        ]

        checker_modules.append((RequestReplier, {
            'rate_file': Constants.L7_DEFAULT_WEIGHTS_DIR + '/antirobot.replier.rate',
            'sink': sink_modules,
            'rate': 0.0,
            'enable_failed_requests_replication': True
        }))

    checker_modules += [
        (Balancer2, {
            'balancer_type': 'dynamic_hashing',
            'backends': options['backends'],
            'check_backends': OrderedDict([
                ('name', gen_check_backends_name(options['backends'])),
                ('quorum', 0.35),
            ]),
            'attempts': options['attempts'],
            'attempts_limit': 0.2,
            'rate_limiter_max_budget': 50,
            'connection_attempts': options['connection_attempts'],
            'stats_attr': '%s' % options.get('stats_attr', ''),
            'timelines': options['timelines'],
            'policies': OrderedDict([
                ('unique_policy', {})
            ]),
            'proxy_options': proxy_options,
            'balancer_options': OrderedDict([
                ('active', OrderedDict([
                    ('request', r'GET /ping HTTP/1.1\r\n\r\n'),
                    ('delay', '20s'),
                    ('use_backend_weight', True),
                    ('weight_normalization_coeff', 10),
                ])),
            ])
        })
    ]

    checker_prefix, checker_dict = SeqProcessor(checker_modules)
    result_prefix += checker_prefix

    result_dict = {'antirobot': OrderedDict()}
    result_dict['antirobot']['cut_request'] = options['cut_request']
    result_dict['antirobot']['no_cut_request_file'] = options['no_cut_request_file']
    result_dict['antirobot']['file_switch'] = options['file_switch']
    result_dict['antirobot']['cut_request_bytes'] = options['cut_request_bytes']

    result_dict['antirobot']['checker'] = checker_dict
    result_dict['antirobot']['module'] = OrderedDict()

    # Adding Cutter module
    _, cutter_dict, _ = Cutter({
        'bytes': options['cutter_bytes'],
        'timeout': options['cutter_timeout'],
    })
    cutter_dict['cutter'].update(result_dict)
    # End of Added Cutter modules

    # fill hasher
    _, hasher_dict, _ = Hasher({'mode': 'subnet', 'take_ip_from': options['take_ip_from_header'], 'subnet_v4_mask': 32})
    hasher_dict['hasher'].update(cutter_dict)

    return result_prefix, hasher_dict, result_dict['antirobot']['module']


@Helper(
    'Modules.Rpcrewrite',
    '''Заменяет пользовательский запроc. Запрос исправляется удалённым сервисом''',
    [
        ('host', None, str, True, 'Значение заголовка Host, которое пойдёт в rpc backend'),
        ('url', None, str, True, 'url запроса на rpc backend'),
        ('rpc', None, list, True, 'Подмодуль, в котором будет rpc backend'),
        (
            'dry_run', False, bool, False,
            'При включении этого флага продолжаем ходить в rpc backend, но не переписываем запрос'
        ),
        (
            'rpc_success_header', None, str, False,
            '''Название заголовка, в который будет записано 1,
            если rpc бэкэнд ответил и тело внутреннего запроса распарсили, и 0 в обратном случае'''
        ),
        (
            'file_switch', './controls/disable_rpcrewrite_module', str, False,
            'Файловый флаг отключения модуля'
        )
    ],
    examples=[(
        'Rpcrewrite в bolver.yandex-team.ru',
        '''
        (Modules.Rpcrewrite, {
            'host': 'bolver.yandex-team.ru',
            'url': '/proxy',
            'dry_run': False
        }),
        '''
    )],
)
def Rpcrewrite(options):
    result = {'rpcrewrite': options}
    rpc_prefix, result['rpcrewrite']['rpc'] = SeqProcessor(options['rpc'])

    return rpc_prefix, result, result['rpcrewrite']


_SSL_SECONDARY_CERT_PARAMS = [
    ('cert', None, str, True, 'Путь к файлу с публичным ключем'),
    ('priv', None, str, True, 'Путь к файлу с приватным ключем'),
    ('ca', None, str, False, 'Путь к CA-файлу'),
    ('ocsp', None, str, False, 'Путь к файлу с DER закодированным OCSP респонзом'),
    (
        'events', None, OrderedDict, False,
        'Хэндлеры событий ("reload_ocsp_response" для перечитывания файла с ocsp ответом)'
    ),
    ('ocsp_file_switch', None, str, False, 'Файловый флаг отключения выдачи ocsp пользователю')
]

_SSL_PARAMS = [
    (
        'ciphers', Constants.SSL_CIPHERS_SUITES, str, False,
        'Список шифров, которые следует использовать, в формате OpenSSL'
    ),
    ('cert', None, str, True, 'Путь к файлу с публичным ключем'),
    ('priv', None, str, True, 'Путь к файлу с приватным ключем'),
    ('ca', None, str, False, 'Путь к CA-файлу'),
    ('ocsp', None, str, False, 'Путь к файлу с DER закодированным OCSP респонзом'),
    ('ticket_keys', None, str, False, 'Путь до файла, содержащего ssl ticket key'),
    ('ticket_keys_list', None, OrderedDict, False, 'Приоритеты и пути до файлов, содержащих tls tickets key'),
    ('timeout', '100800s', str, False, 'Таймаут сессий'),
    (
        'log', None, str, False,
        '''Название лога для записи событий.
        Лог должен быть зарегестрирован ранее, например, в модуле errrolog'''
    ),
    (
        'events', None, OrderedDict, False,
        '''Хэндлеры событий:
            * "reload_ocsp_response" для перечитывания файла с ocsp ответом
            * "reload_ticket_keys" для перечитывания фалов с tls session ticket-ами)'''
    ),
    ('disable_sslv3', None, bool, False, 'Disable sslv3 on balancer'),
    ('ssl_protocols', OrderedDict(map(lambda x: (LuaAnonymousKey(), x), ['tlsv1', 'tlsv1.1', 'tlsv1.2', 'tlsv1.3'])), OrderedDict, False, 'Список включенных протоколов'),
    ('secrets_log', None, str, False, 'Путь до дампа сессионных ключей'),
    ('secrets_log_freq', None, str, False, 'Дефолтная частота дампа сессионных ключей'),
    ('secrets_log_freq_file', None, str, False, 'Ручка управления частотой дампа сессионных ключей'),
    ('sni_contexts', None, OrderedDict, False, 'Описание контекстов не по умолчанию для SNI'),
    ('servername', None, dict, False, 'Словарь с регулярным выражением и настройками для матчинга хоста в SNI'),
    ('priority', None, int, False, 'Приоритет в дереве контекстов.'),
    (
        'secondary', None, OrderedDict, False,
        'Второй сертификат с ключом (+ опциональные ocsp и event на его перечитывание)'
    ),
    ('ocsp_file_switch', None, str, False, 'Файловый флаг отключения выдачи ocsp пользователю'),
    ('force_ssl', None, bool, False, 'Запрет обработки запросов пришедших по http в ssl секциях.'),
    ('ja3_enabled', None, bool, False, 'Включает ja3 fingerprint'),
    ('http2_alpn_file', None, str, False, 'Имя файла с числом вероятности использования http2, от 0 до 1.'),
    ('http2_alpn_freq', None, float, False, 'Вероятность использования http2, от 0 до 1.'),
    ('http2_alpn_rand_mode', None, str, False, 'Режим выбора http2.'),
    ('http2_alpn_rand_mode_file', None, str, False, 'Имя файла с режимом выбора http2.'),
    ('http2_alpn_exp_id', None, int, False, 'Идентификатор эксперимента http2'),
    ('exp_contexts', None, dict, False, 'Список экспериментальных контекстов'),
]


@Helper(
    'Modules.SslSni',
    '''
        Обрабатывает https при помощи библиотеки OpenSSL с поддержкой SNI.
        Занимается запаковкой/распаковкой данных, пришедших по протоколам SSL/TLS
    ''',
    _SSL_PARAMS,
)
def SslSni(options):
    secretlog_prefix = LuaPrefixes()
    secretlog_prefix += '''function GetPathToSecretLog(data)
    if SslLogPath ~= nil then
        secret_log = SslLogPath
    else
        secret_log = nil
    end
    return secret_log
end'''
    retval = {
        'ssl_sni': OrderedDict([
            ('contexts', OrderedDict([])),
            ('events', OrderedDict([
                ('reload_ticket_keys', 'reload_ticket'),
                ('force_reload_ticket_keys', 'force_reload_ticket'),
                ('reload_ocsp_response', 'reload_ocsp'),
                ('stats', 'report'),
            ])),
        ])
    }

    for opt in (
        'force_ssl',
        'ja3_enabled',
        'http2_alpn_file',
        'http2_alpn_freq',
        'http2_alpn_rand_mode',
        'http2_alpn_rand_mode_file',
        'http2_alpn_exp_id',
    ):
        if options[opt] is not None:
            retval['ssl_sni'][opt] = options[opt]
            options.pop(opt)

    exp_contexts = options.pop('exp_contexts')

    if options['sni_contexts'] is not None:
        _sni_contexts = options.pop('sni_contexts')
        _SNI_CTX_PARAMS = [o[0:-1] for o in _SSL_PARAMS]
        for key in _sni_contexts.keys():
            retval['ssl_sni']['contexts'][key] = apply_options_checker(
                _sni_contexts[key],
                OptionsChecker(_SNI_CTX_PARAMS)
            )
            if 'secondary' in _sni_contexts[key]:
                _SNI_CTX_PARAMS_SECONDARY = [o[0:-1] for o in _SSL_SECONDARY_CERT_PARAMS]
                apply_options_checker(_sni_contexts[key]['secondary'], OptionsChecker(_SNI_CTX_PARAMS_SECONDARY))

    retval['ssl_sni']['contexts']['default'] = options
    if len(retval['ssl_sni']['contexts']) > 1:
        retval['ssl_sni']['contexts']['default']['priority'] = 2
    if exp_contexts is not None:
        exps = []
        for exp in exp_contexts.keys():
            ctx = copy.deepcopy(retval['ssl_sni']['contexts'])
            if 'patch' in exp_contexts[exp]:
                for path,value in exp_contexts[exp]['patch']:
                    for k in ctx.keys():
                        reduce(operator.getitem, path[:-1], ctx[k])[path[-1]] = value
            exps.append(
                (exp, OrderedDict([
                    ('exp_id', exp_contexts[exp]['exp_id']),
                    ('max_send_fragment', exp_contexts[exp]['max_send_fragment'] if 'max_send_fragment' in exp_contexts[exp] else None),
                    ('contexts', ctx)
                ]))
            )
        retval['ssl_sni']['exps'] = OrderedDict(exps)
    return secretlog_prefix, retval, retval['ssl_sni']


@Helper(
    'Modules.Rewrite',
    '''
    Переписывает строку запроса или произвольный хедер (**rewrite**) на основании регулярного выражения и паттерна
    ''',
    [(
        'actions', [], list, False,
        '''
        Список правил для замены:
            * **split**,
            * **regexp**,
            * **header_name**,
            * **rewrite**,
            * **literal**,
            * **case_insensitive**,
            * **global**
        Подробнее читайте в руководстве по балансеру.
        '''
    )],
)
def Rewrite(options):
    result = {'rewrite': OrderedDict()}
    result['rewrite']['actions'] = OrderedDict()
    for action in options['actions']:
        result['rewrite']['actions'][LuaAnonymousKey()] = action

    return LuaPrefixes(), result, result['rewrite']


@OptionsChecker([
    ('cpu_usage_coeff', 0.05, float, False),
    ('enable_conn_reject', False, bool, False),
    ('enable_keepalive_close', False, bool, False),
    ('enable_http2_drop', False, bool, False),
    ('conn_reject_lo', 0.95, float, False),
    ('conn_reject_hi', 1., float, False),
    ('keepalive_close_lo', 0.95, float, False),
    ('keepalive_close_hi', 1., float, False),
    ('http2_drop_lo', 0.9, float, False),
    ('http2_drop_hi', 0.95, float, False),
    ('disable_file', None, str, False),
    ('disable_http2_file', None, str, False),
    ('conn_hold_count', 10000, int, False),
    ('conn_hold_duration', "10s", str, False),
])
def check_cpu_limiter_options(options):
    if not 0 <= options['cpu_usage_coeff'] <= 1:
        raise Exception('cpu usage coeff should be in [0,1]')

    def check_cpu_ranges(name):
        enable_option = 'enable_{}'.format(name)
        lo_option = '{}_lo'.format(name)
        hi_option = '{}_hi'.format(name)

        if options[enable_option]:
            if not 0 <= options[lo_option] <= 1:
                raise Exception(name + ' lower bound should be in [0,1]')
            if not 0 <= options[hi_option] <= 1:
                raise Exception(name + ' upper bound should be in [0,1]')
            if options[lo_option] > options[hi_option]:
                raise Exception(name + ' lower bound shouldn\'t be above than upper bound')
        else:
            del options[lo_option]
            del options[hi_option]

    check_cpu_ranges('conn_reject')
    check_cpu_ranges('keepalive_close')
    check_cpu_ranges('http2_drop')

    if not options['enable_conn_reject']:
        del options['conn_hold_count']
        del options['conn_hold_duration']

    if not options['enable_conn_reject'] and not options['enable_keepalive_close'] and not options['enable_http2_drop']:
        del options['disable_file']

    return options


@OptionsChecker([
    ('capacity', None, int, False),
    ('ignore_per_proxy_limit', False, bool, False),
])
def check_connection_manager_options(options):
    return options


@Helper(
    'Modules.Main',
    '''
        Главный модуль, который должен быть наверху
    ''',
    [
        ('use_port_admin_as_log_suffix', False, bool, False, 'Использовать значение port_admin как суффикс лока'),
        (
            'maxconn', 50000, int, True,
            'Максимальное количество запросов, которые может держать worker одновременно'
        ),
        ('config_check', None, dict, False, 'Дополнительные проверки конфигурации в prepare'),
        ('addrs', None, OrderedDict, True, 'Список адресов, на которых балансер слушает клиентов'),
        ('admin_port', None, int, True, 'Порт на котором слушает админка.'),
        ('config_tag', 'trunk', str, False, 'Название тага, из которого сгенерен конфиг.'),
        (
            'workers', 5, int, False,
            'Количество worker-процессов, которые непосредственно занимаются обработкой запросов'
        ),
        ('log', None, str, False, 'Имя лога для событий, связанных с управлением worker-ами.'),
        ('write_log', True, bool, False, 'Флаг, указывающий на то, стоит ли писать лог'),
        ('port', None, int, False, 'Порт админки.'),
        ('buffer', 64 * 1024, int, False, 'Buffer size.'),
        ('dns_timeout', None, str, False, 'Таймаут на DNS-резолвинг в TDuration формате.'),
        (
            'tcp_nodelay', None, bool, False,
            'Включает/выключает TCP_NODELAY (Nagle) на пользовательском сокете. По умолчанию включено'
        ),
        ('tcp_keep_cnt', None, int, False, 'Устанавливает TCP_KEEPCNT на пользовательском сокете'),
        ('tcp_keep_idle', None, int, False, 'Устанавливает TCP_KEEPIDLE на пользовательском сокете'),
        ('tcp_keep_intvl', None, int, False, 'Устанавливает TCP_KEEPINTVL на пользовательском сокете'),
        ('tcp_fastopen', None, int, False, 'Устанавливает TCP_FASTOPEN на пользовательском сокете'),
        ('sosndbuf', None, int, False, 'Устанавливает SO_SNDBUF на пользовательском сокете'),
        ('deferaccept', None, bool, False, 'Устанавливает TCP_DEFER_ACCEPT на слушающем сокете'),
        ('enable_reuse_port', False, bool, False, 'Устанавливает SO_REUSEPORT на слушающем сокете'),
        (
            'stats_connect_timeout', None, str, False,
            'Таймаут на соединение при запросе стат ручки мастера к воркерам'
        ),
        (
            'stats_timeout', None, str, False,
            'Таймаут на получение ответа при запросе стат ручки мастера к воркерам'
        ),
        (
            'stats_event_timeout', None, str, False,
            'Таймаут на получение ответа при запросе /admin/event/.* мастера к воркерам'
        ),
        (
            'events', OrderedDict([('stats', 'report')]), OrderedDict, False,
            'Ивенты для сбора статистики через модуль Report'
        ),
        (
            '_coro_fail_on_error', None, bool, False,
            '''
            Отладочная опция - включает дополнительные проверки в корутинах, при ошибках вызывает abort().
            По умолчанию выключено
            '''
        ),
        (
            'logs_path', '/usr/local/www/logs/', str, False,
            'Директория для логов'
        ),
        (
            'private_address', '127.0.0.10', str, False,
            '[BALANCER-1070] Адрес для биндинга воркеров для межворкерного общения. По умолчанию 127.0.0.1'
        ),
        (
            'default_tcp_rst_on_error', None, bool, False,
            '''[BALANCER-1060] Определяет наличие опции SO_LINGER на сокете и поведение при закрытии соединения.
            По умолчанию True, т.е. при ошибке пользователю будет отдан RST+ACK, False - FIN+ACK'''
        ),
        (
            'reset_dns_cache_file', None, str, False,
            '''[BALANCER-1122] Файловый флаг для сброса dns-кеша. Использовать строго с версиями 77+'''
        ),
        (
            'tcp_listen_queue', 0, int, False,
            'Размер бэклога слушающих сокетов'
        ),
        ('cpu_limiter', None, OrderedDict, False, 'Опции для цпу лимитера BALANCER-1910'),
        ('sd_client_name', None, str, False, 'Имя клиента service discovery'),
        ('sd_update_frequency', None, str, False, 'Частота опроса резолвера service discovery'),
        ('unistat', None, OrderedDict, False, 'Опции для unistat-ручки'),
        ('pinger_required', None, bool, False, 'Опция для включения пингера для всех секций active'),
        ('state_directory', None, str, None, 'Путь до папки, в которой балансер будет хранить своё состояние'),
        ('connection_manager', None, OrderedDict, False, 'Опции для connection manager'),
        ('enable_dynamic_balancing_log', False, bool, False, 'Включить лог динамической балансировки'),
        ('enable_pinger_log', False, bool, False, 'Включить лог пингера'),
        ('backends_blacklist', None, str, False, 'Файл блэклиста бекендов под dynamic балансировкой'),
        ('tcp_congestion_control', None, str, False, 'TCP congestion control для сокетов'),
        ('dns_async_resolve', None, bool, False, 'Включить c-ares DNS'),
        ('shutdown_accept_connections', None, bool, False, 'Принимать новые соединения при shutdown'),
        ('shutdown_close_using_bpf', None, bool, False, 'Использовать bpf фильтр чтобы дропать syn пакеты при shutdown BALANCER-2487'),
        ('temp_buf_prealloc', None, int, False, 'Предварительное выделение памяти для TTempBuf, для улучшения производительности при росте нагрузки'),
        ('p0f_enabled', None, bool, False, 'Включить p0f fingerprint'),
        ('p0f_map_max_size', None, int, False, 'Размер буфера для p0f'),
        ('coro_pool_allocator', None, bool, False, 'Pool allocator для корутин, уменьшает расход RSS памяти'),
        ('coro_pool_stacks_per_chunk', None, int, False, 'Количество стеков в одном чанке pool allocator-а для корутин'),
        ('coro_pool_rss_keep', None, int, False, 'Сколько страниц RSS памяти сохранять для больших стеков корутин'),
        ('coro_pool_small_rss_keep', None, int, False, 'Сколько страниц RSS памяти сохранять для маленьких (service) стеков корутин'),
        ('coro_pool_release_rate', None, int, False, 'RSS память будет удаляться с каждой coro_pool_release_rate корутины'),
        ('config_uid', None, str, False, 'Уникальный ИД конфига данного сервиса. Используется для нахождения циклов на пути запроса через балансеры'),
    ],
    examples=[(
        'Админка для production web-а',
        '''
            (Modules.Main, {'workers': 5, 'maxconn': 10000, 'port': 8080})
        '''
    )],
)
def Main(options):
    if options['port'] is None and options['log'] is None:
        raise Exception("Both <log> and <port> options in module <Main> undefined")

    if options['sd_client_name']:
        options['sd'] = OrderedDict([
            ('client_name', options['sd_client_name']),
            ('host', LuaGlobal('sd_host', 'sd.yandex.net')),
            ('port', LuaGlobal('sd_port', '8080')),
            ('cache_dir', LuaGlobal('sd_cache', './sd_cache')),
        ])
        if options['sd_update_frequency']:
            options['sd']['update_frequency'] = LuaGlobal('sd_update_frequency', options['sd_update_frequency'])

    if options['port'] is not None:
        def gen_log_path(prefix):
            if options['use_port_admin_as_log_suffix']:
                return LuaFuncCall('GetLog', {
                    'dir': LuaGlobal('LogDir', options['logs_path']),
                    'prefix': prefix,
                    'port': LuaGlobal('port_localips', options['port'])
                })
            else:
                return LuaGlobal('LogDir', options['logs_path']) + (prefix + '%s' % options['port'])

        options['log'] = gen_log_path('/current-childs_log-balancer-')

        if options['enable_dynamic_balancing_log']:
            options['dynamic_balancing_log'] = gen_log_path('/current-dynamic_balancing_log-balancer-')

        if options['enable_pinger_log']:
            options['pinger_log'] = gen_log_path('/current-pinger_log-balancer-')

        if options['sd_client_name']:
            options['sd']['log'] = gen_log_path('/current-sd_log-balancer-')

        options.pop('port')


    options['admin_addrs'] = LuaGlobal('instance_admin_addrs', OrderedDict((
        (LuaAnonymousKey(), OrderedDict([('ip', '127.0.0.1'), ('port', options['admin_port'])])),
        (LuaAnonymousKey(), OrderedDict([('ip', '::1'), ('port', options['admin_port'])])),
    )))

    options.pop('admin_port')
    options.pop('sd_client_name')
    options.pop('sd_update_frequency')
    options.pop('enable_dynamic_balancing_log')
    options.pop('enable_pinger_log')
    options.pop('use_port_admin_as_log_suffix')

    if options['logs_path']:
        options.pop('logs_path')

    if options['write_log'] is not None:
        if not options['write_log']:
            options.pop('log')
        options.pop('write_log')

    if not options.get('enable_reuse_port', False):
        options.pop('enable_reuse_port')

    if options['tcp_listen_queue'] is not None:
        if options['tcp_listen_queue'] <= 0:
            options.pop('tcp_listen_queue')

    if options['cpu_limiter'] is not None:
        options['cpu_limiter'] = check_cpu_limiter_options(options['cpu_limiter'])

    if options['connection_manager'] is not None:
        options['connection_manager'] = check_connection_manager_options(options['connection_manager'])

    result = {'instance': options}
    return LuaPrefixes(), result, result['instance']


@Helper(
    'Modules.Meta',
    '''Добавляет метаинформацию в аксес-логи''',
    [
        ('id', None, str, True, 'Идентификатор мета-модуля'),
        ('fields', None, OrderedDict, True, 'Список полей (пар ключ-значение), которые будут добавлены в лог'),
    ],
)
def Meta(options):
    result = {'meta': options}
    return LuaPrefixes(), result, result['meta']


@OptionsChecker([
    ('host', None, str, True),
    ('port', None, int, True),
    ('connect_timeout', '0.3s', str, False),
    ('backend_timeout', '10s', str, False),
    ('connect_retry_timeout', None, str, False),
    ('connect_retry_delay', None, str, False),
    ('keepalive_count', 0, int, False),
    ('fail_on_5xx', True, bool, False),
    ('http_backend', True, bool, False),
    ('buffering', False, bool, False),
    ('resolve_protocols', [4, 6], list, False),
    ('https_settings', None, OrderedDict, False),
    ('need_resolve', None, bool, False),
    # статус-коды или их семейства, которые считаются плохими
    ('status_code_blacklist', None, list, False),
    # исключения из status_code_blacklist (чтобы можно было указать "4xx" в blacklist и 404 в exceptions)
    ('status_code_blacklist_exceptions', None, list, False),
    # ограничит время жизни keepalive соединени в состоянии отсутствия активности
    ('keepalive_timeout', None, str, False),
])
def Proxy(options):
    options['cached_ip'] = resolve_host(DummyInstance(options['host'], 0, 1.0), options['resolve_protocols'])
    options['connect_timeout'] = options.pop('connect_timeout')  # FIXME
    options.pop('resolve_protocols')

    if options['status_code_blacklist'] is not None:
        ValidateStatusCodes({'codes_list': options['status_code_blacklist']})
        options['status_code_blacklist'] = OrderedDict([
            (LuaAnonymousKey(), x) for x in options['status_code_blacklist']
        ])

    if options['status_code_blacklist_exceptions'] is not None:
        ValidateStatusCodes({'codes_list': options['status_code_blacklist_exceptions']})
        options['status_code_blacklist_exceptions'] = OrderedDict([
            (LuaAnonymousKey(), x) for x in options['status_code_blacklist_exceptions']
        ])

    result = {'proxy': options}
    return LuaPrefixes(), result, None


@Helper(
    'Modules.CacheServer',
    '''Серверный модуль кешера''',
    [
        ('check_modified', False, bool, True,
         'Использовать ли If-Modified-Since запросы для обновления устаревших закешированных ответов'),
        ('memory_limit', None, int, True, 'Лимит памяти для кеша'),
        ('stats_attr', '', str, False, 'Имя атрибута для статистики golovan-а'),
        ('valid_for', None, str, False, 'Время устаревания закешированных ответов'),
        ('async_init', None, bool, False, 'Асинхронная инициализация кеша'),
    ],
)
def CacheServer(options):
    return LuaPrefixes(), {'cache_server': options}, None


@Helper(
    'Modules.CacheClient',
    '''Клиентсий модуль кешера''',
    [
        (
            'id_regexp', None, str, True,
            'Регулярное выражение для строки запроса, из групп захвата которого формируется cache id'
        ),
        ('server', None, list, True, 'Модуль, куда посылаются запросы к кеширующему серверу'),
        ('module', None, list, True, 'Модуль, куда посылаются запросы к бэкенду'),
        ('stats_attr', '', str, False, 'Имя атрибута для статистики golovan-а'),
    ],
)
def CacheClient(options):
    prefix = LuaPrefixes()
    result = OrderedDict()
    result['id_regexp'] = options['id_regexp']
    result['stats_attr'] = options['stats_attr']
    server_prefix, result['server'] = SeqProcessor(options['server'])
    prefix += server_prefix
    module_prefix, result['module'] = SeqProcessor(options['module'])
    prefix += module_prefix
    return prefix, {'cache_client': result}, None


@Helper(
    'Modules.ThumbsBan',
    '''Банит тумбнейлы''',
    [
        ('id_regexp', None, str, True, 'To be filled'),
        ('checker', None, list, True, 'To be filled'),
        ('ban_handler', None, list, True, 'To be filled'),
        ('module', None, list, True, 'To be filled'),
    ]
)
def ThumbsBan(options):
    thumbsban_prefix = LuaPrefixes()
    thumbsban_result = OrderedDict()
    thumbsban_result['id_regexp'] = options['id_regexp']
    checker_prefix, thumbsban_result['checker'] = SeqProcessor(options['checker'])
    thumbsban_prefix += checker_prefix
    ban_handler_prefix, thumbsban_result['ban_handler'] = SeqProcessor(options['ban_handler'])
    thumbsban_prefix += ban_handler_prefix
    module_prefix, thumbsban_result['module'] = SeqProcessor(options['module'])
    thumbsban_prefix += module_prefix
    return thumbsban_prefix, {'thumbsban': thumbsban_result}, None

@Helper(
    'Modules.ThumbsSpreader',
    '''Посылает кучу ''',
    [
        ('id_regexp', None, str, True, 'To be filled'),
        ('module', None, list, True, 'To be filled'),
    ]
)
def ThumbsSpreader(options):
    thumbsspreader_prefix = LuaPrefixes()
    thumbsspreader_result = OrderedDict()
    thumbsspreader_result['id_regexp'] = options['id_regexp']
    module_prefix, thumbsspreader_result['module'] = SeqProcessor(options['module'])
    thumbsspreader_prefix += module_prefix
    return thumbsspreader_prefix, {'thumbs_spreader': thumbsspreader_result}, None

@Helper(
    'Modules.Hdrcgi',
    '''Добавляет cgi параметры со значением, взятым из хедеров, и наоборот''',
    [
        ('cgi_from_hdr', None, OrderedDict, False,
         'Список пар из имени cgi параметра, который нужно добавить, и имени хедера, откуда взять значение'),
        ('hdr_from_cgi', None, OrderedDict, False,
         'Список пар из имени хедера, который нужно добавить, и имени cgi параметра, откуда взять значение.'),
    ],
)
def Hdrcgi(options):
    result = {'hdrcgi': {}}

    if options['cgi_from_hdr'] is not None:
        result['hdrcgi']['cgi_from_hdr'] = options['cgi_from_hdr']
    if options['hdr_from_cgi'] is not None:
        result['hdrcgi']['hdr_from_cgi'] = options['hdr_from_cgi']

    return LuaPrefixes(), result, result['hdrcgi']


@Helper(
    'Modules.Report',
    '''Добавляет расширенную статистику для ответов, содержащих определенный заголовой''',
    [
        ('uuid', None, str, True, 'Уникальный идентификатор ключа для статистики'),
        (
            'refers', None, str, False,
            '''Список uuid других модулей report из этого конфигурационного файла,
            разделенный запятыми.
            В эти модули будет передаваться статистика
            '''
        ),
        (
            'all_default_ranges', True, bool, False,
            '''
            Включает сбор статистики для всех гистограмм с дефолтными ренжами
            '''
        ),
        (
            'ranges', None, str, False,
            '''
            На какие временные интервалы разбивать статистику
            '''
        ),
        (
            'client_fail_time_ranges', None, str, False,
            'На какие временные интервалы разбивать статистику client fail.'
        ),
        (
            'input_size_ranges', None, str, False,
            'На какие интервалы размера запроса в байт разбивать статистику.'
        ),
        (
            'output_size_ranges', None, str, False,
            'На какие интервалы размера ответа в байт разбивать статистику.'
        ),
        (
            'matcher_map', None, OrderedDict, False,
            'Маппинг суффиксов uuid в их матчеры.'
        ),
        (
            'stats', "report", str, False,
            '''
            Регулярное выражение,
            по которому будет матчиться отдельный эвент для отдачи статистики только этого модуля репорт.
            Например, stats = "report.*" отдаст свою статистику по /admin/events/call/report-my
            '''
        ),
        ('just_storage', False, bool, False, 'Включает режим хранения результатов'),
        ('disable_robotness', True, bool, False, 'Управляет наличием признаков роботности в статистике.'),
        ('disable_sslness', True, bool, False, 'Управляет наличием признаков эсэсэльности в статистике.'),
        ('outgoing_codes', None, str, False, 'Доп.статистика по кодам ответов')
    ],
)
def Report(options):
    x = []
    for key, value in options.iteritems():
        if key and key != 'stats':  # stats defined manualy, see below
            x.append((key, value))

    x.append(('events', {'stats': options['stats']}))
    opts = OrderedDict(x)
    result = {'report': opts}
    return LuaPrefixes(), result, result['report']


@Helper(
    'Modules.Click',
    '''
    Отвечает редиректами на кликовые запросы.
    Запрос дублируется в ClickDaemon для сохранения статистики,
    но ответ от бэкэндов не используется,
    кроме тех случаев, когда не удается сформировать редирект при помощи библиотеки кликдемона.
    Про файл ключей, задаваемый переменной keys, читайте в описании ClickDaemon.
    ''',
    [
        ('keys', None, str, True, 'Путь к файлу с ключами'),
        ('json_keys', None, str, True, 'Путь к json файлу с ключами'),
    ],
)
def Click(options):
    result = {'click': options}
    return LuaPrefixes(), result, result['click']


@Helper(
    'Modules.ThumbConsistentHash',
    '''Распределяет запросы по шардам в зависимости от значения хеша в id тумбнейла''',
    [
        ('id_regexp', None, str, True, 'Регулярка для выделения хеша из урла.'),
        ('locations', None, list, False, 'Список пар (<шард>, <обработчики для шарда>).'),
        ('default', None, list, False, 'Обработчик запросов, для которых не удалось найти шард.'),
    ],
)
def ThumbConsistentHash(options):
    global_section_key = (id(options['locations']), id(options['id_regexp']), id(options['default']))

    if global_section_key not in ParseContext().global_sections:
        locations = [(str(num), {}, handler) for num, handler in options['locations']]
        locations.append(('default', {}, options['default']))

        result_prefix, global_section_dict, _ = _Dispatcher(locations, lambda x: (LuaPrefixes(), x, x))

        global_section_dict['id_regexp'] = options['id_regexp']
        global_section_name = ParseContext().generate_global_section_name('thumb_consistent_hash_section_')

        ParseContext().global_sections[global_section_key] = LuaGlobal(global_section_name, global_section_dict)
    else:
        result_prefix = LuaPrefixes()

    return result_prefix, {'thumb_consistent_hash': ParseContext().global_sections[global_section_key]}, None


@Helper(
    'Modules.Debug',
    '''Отладочный модуль''',
    [
        ('delay', None, float, False, 'Задержка в ответе в секундах'),
    ],
    examples=[
        ('Задержка в ответе', '''(Modules.Debug, { 'delay' : 0.2}),'''),
    ],
)
def Debug(options):
    result = {'debug': options}
    return LuaPrefixes(), result, result['debug']


@Helper(
    'Modules.ExpGetter',
    '''Модуль для хождения в Usersplit-as-a-Service''',
    [
        ('uaas', None, list, True, 'Подмодуль, в котором определяется, как ходить в UaaS'),
        ('file_switch', None, str, False, 'Путь до файла; если файл существует, то модуль выключает хождение и в UaaS'),
        ('service_name_header', None, str, False, 'Заголовок, в котором для UaaS будет передано название сервиса'),
        ('service_name_to_backend_header', None, str, False, 'Имя заголовка, в котором на бекенд будет отправлено название сервиса'),
        ('service_name', None, str, False, 'Значение заголовка из параметра service_name_header'),
        (
            'exp_headers', None, str, False,
            '''
            Регулярка, которая определяет, какие заголовки из ответа UaaS надо скопировать в основной запрос.
            В бинарнике зашит некоторый дефолт, его значение лучше смотреть в коде самого балансера.
            '''
        ),
        (
            'trusted', None, bool, False,
            '''
            Флаг о доверии внутренней сети - если включён и в запросе есть хотя бы один заголовок,
            удовлетворяющий exp_headers,
            то хождения в UaaS не будет и заголовки улетят вниз как есть.
            Если флаг выключен, то заголовки удаляем, идём в UaaS и берём данные отттуда
            '''
        ),
        ('processing_time_header', False, bool, False, ''),
        ('headers_size_limit', None, int, False, 'Ограничение на суммарный размер заголовков, берущихся из ответа UaaS'),
    ],
    examples=[],
)
def ExpGetter(options):
    result = copy.copy(options)

    if options['service_name'] is not None and options['service_name_header'] is None:
        raise Exception('exp_getter: "service_name_header" must be specified if you specify "service_name"')

    uaas_prefix, uaas_dict = SeqProcessor(options['uaas'])
    result['uaas'] = uaas_dict

    result = {'exp_getter': result}
    return uaas_prefix, result, result['exp_getter']


@Helper(
    'Modules.StaffLoginChecker',
    '''Модуль предоставляет доступ в эксперименты на внутренюю сеть, чьи логины привязаны к
    стаффу''',
    [
        ('trusted', None, bool, False, 'Флаг о доверии внутренней сети'),
        (
            'file_switch', None, str, False,
            'Путь до файла; если файл существует, то модуль выключает проверку на стафф логины'
        ),
        ('enable_check_sessionid', True, bool, False, 'Флаг о включении проверки куки sessionid'),
        ('enable_check_oauth_token', True, bool, False, 'Флаг о включении проверки OAuth токена'),
        ('use_blackbox', True, bool, False, 'Флаг об использовании blackbox для валидации токенов и кук'),
        ('no_blackbox_file', None, str, False,
         'Путь до файла; если файл существует, то не используем blackbox для валидации кук и токена'),
        ('reload_duration', None, str, False,
         "(Формат: TDuration) Период загрузки новых uid'ов в случае успешной предыдущей попытки (Default: 1 day)"),
        ('error_reload_duration', None, str, False,
         "(Формат: TDuration) Период загрузки новых uid'ов в случае неудачной предыдущей попытки (Default: 1 hour)"),
        (
            'take_ip_from', 'X-Forwarded-For-Y', str, False,
            '''
            Если параметр задан, то IP пользователя берётся из соответствующего HTTP-заголовка.
            Если параметр не задан, берется IP-адрес запрашивающего
            '''
        ),
        ('blackbox_proxy', None, list, True, 'Подмодуль, в котором определяется, как ходить в blackbox'),
        ('uids_hunter', None, list, True, 'Подмодуль, в котором определяется, как ходить в uid storage'),
    ],
    examples=[],
)
def StaffLoginChecker(options):
    result = copy.copy(options)

    blackbox_prefix, blackbox_dict = SeqProcessor(options['blackbox_proxy'])
    uids_hunter_prefix, uids_hunter_dict = SeqProcessor(options['uids_hunter'])
    result['blackbox_proxy'] = blackbox_dict
    result['uids_hunter'] = uids_hunter_dict

    blackbox_prefix += uids_hunter_prefix

    result = {'staff_login_checker': result}
    return blackbox_prefix, result, result['staff_login_checker']


@Helper(
    'Modules.RemoteLog',
    '''Модуль для сохранения логов путем посылки информации
    о запросе и ответе в удаленные машины для последующего логирования''',
    [
        ('no_remote_log_file', None, str, False, 'Файл, при наличии которого модуль отключается'),
        ('delay', None, str, False, 'Частота отправки логов'),
        (
            'remote_log_storage', None, list, True,
            'Стек модулей, через которые осуществеляется удаленное логировение'
        ),
        (
            'uaas_mode', None, bool, False,
            'Если данный флаг установлен в "true", то в выводе json-a появляется "uaas_mode":"true"'
        )
    ],
    examples=[],
)
def RemoteLog(options):
    result = {'remote_log': options}
    logstorage_prefix, result['remote_log']['remote_log_storage'] = SeqProcessor(options['remote_log_storage'])
    return logstorage_prefix, result, result['remote_log']


def _Dispatcher(options, me):
    result_prefix = LuaPrefixes()
    result_dict = OrderedDict()

    for section, params, modules in options:
        ParseContext().stack.append('DispatchSection %s' % section)

        if section in result_dict:
            print(ParseContext())
            raise Exception("Section %s found more then once" % section)
        sub_prefix, sub_dict = SeqProcessor([(me, params)] + modules)
        result_prefix += sub_prefix
        result_dict[section] = sub_dict

        ParseContext().stack.pop()

    return result_prefix, result_dict, None


def SeqProcessor(modules):
    result_prefix = LuaPrefixes()
    result_dict = OrderedDict()

    next_dict = result_dict

    for i in range(len(modules)):
        ParseContext().stack.append("Module %s" % modules[i][0].__name__)

        module, module_options = modules[i]
        module_prefix, module_dict, module_next = module(module_options)

        if module_next is None and i < len(modules) - 1:
            print(ParseContext())
            raise Exception("Finishing %s is not the last in modules sequence" % (ParseContext().stack[-1]))
        if module_next is not None and i == len(modules) - 1:
            print(ParseContext())
            raise Exception("Non-finishing %s is the last in modules sequence" % (ParseContext().stack[-1]))

        result_prefix += module_prefix
        next_dict.update(module_dict)

        if module_dict != module_next:
            next_dict = module_next

        ParseContext().stack.pop()

    return result_prefix, result_dict


@Helper(
    'Modules.Threshold',
    '''Модуль дял буферизации медленных POST запросов.''',
    [
        (
            'lo_bytes', None, int, False,
            'Минимальное количество байт, которые должны быть переданы за pass_timeout'
        ),
        (
            'hi_bytes', None, int, False,
            '''Количество переданных байт,
            после которого запрос будет отправлен в подмодуль до достижения pass_timeout'''
        ),
        (
            'pass_timeout', None, str, False,
            '''Таймаут для получения lo_bytes от клиента,
            чтобы пропустить запрос в нижележащий модуль'''
        ),
        (
            'recv_timeout', None, str, False,
            'Таймаут на операцию чтения от клиента, перевзводится после каждой из операций'
        ),
        (
            'on_pass_timeout_failure', [], list, False,
            'Набор модулей для ответа клиенту при истечении pass_timeout.'
        ),
    ],
    examples=[],
)
def Threshold(options):
    if options['hi_bytes'] is not None:
        hi_bytes = int(options['hi_bytes'])

        if hi_bytes < 0:
            raise Exception('threshold: hi_bytes cannot be less than 0')

        if hi_bytes != 0 and options['pass_timeout'] is None:
            raise Exception('threshold: setting hi_bytes without pass_timeout is meaningless')

    if options['lo_bytes'] is not None:
        lo_bytes = int(options['lo_bytes'])

        if lo_bytes < 0:
            raise Exception('threshold: lo_bytes cannot be less than 0')

        if options['hi_bytes'] is None:
            raise Exception('threshold: set hi_bytes if you have set lo_bytes')

        if lo_bytes > int(options['hi_bytes']):
            raise Exception('threshold: lo_bytes > hi_bytes (%d > %s), which is meaningless' % (lo_bytes, hi_bytes))

    result = {'threshold': options}
    if options['on_pass_timeout_failure']:
        on_pass_timeout_failure_prefix, result['threshold']['on_pass_timeout_failure'] = SeqProcessor(
            options['on_pass_timeout_failure'])
    else:
        result['threshold'].pop('on_pass_timeout_failure')

    return LuaPrefixes(), result, result['threshold']


@Helper(
    'Modules.Geobase',
    '''Модуль для хождения в Libgeobase-as-a-Service''',
    [
        ('geo', None, list, True, 'Подмодуль, в котором определяется, как ходить в LaaS'),
        ('geodata', None, str, False, 'Путь до файла с локальной копией геоданных'),
        ('geo_host', 'laas.yandex.ru', str, False, 'Значение поля Host при запросе в LaaS'),
        ('geo_path', '/region?response_format=header&version=1&service=balancer', str, False, 'Path запроса в LaaS'),
        ('file_switch', None, str, False,
         'Путь до файла; если файл существует, то модуль выключает хождение и в LaaS, и в локальную libgeobase'),
        (
            'take_ip_from', 'X-Forwarded-For-Y', str, False,
            '''
            Название хедера, из которого нужно брать Ip-адрес.
            Хедер по умолчанию - X-Forwarded-For-Y.
            Значение этого заголовка из оригинального запроса
            станет значением X-Forwarded-For-Y в подмодуле geo.
            '''
        ),
        (
            'laas_answer_header', 'X-LaaS-Answered', str, False,
            '''
            Название хедера, в котором будет передан признак успеха работы модуля.
            Хедр по умолчанию - X-LaaS-Answered.
            '''
        ),
        ('processing_time_header', False, bool, False, ''),
    ],
    examples=[],
)
def Geobase(options):
    result = copy.copy(options)

    geo_prefix, geo_dict = SeqProcessor(options['geo'])
    result['geo'] = geo_dict

    result = {'geobase': result}
    return geo_prefix, result, result['geobase']


@Helper(
    'Modules.ThdbVersion',
    '''Модуль добавляет специальный HTTP-заголовок при отправке запроса к бэкенду''',
    [
        ('file_name', None, str, True, 'Имя файла, из которого читается значение для заголовка'),
        ('file_read_timeout', None, str, True, 'Таймаут для чтения значения из файла'),
    ],
    examples=[],
)
def ThdbVersion(options):
    result = {"thdb_version": copy.copy(options)}
    return LuaPrefixes(), result, result["thdb_version"]


@Helper(
    'Modules.H100',
    '''
        Модуль проверят запрос на наличие заголовков Expect: 100-continue в запросе.
        Если такой заголовок есть, то модуль отвечает 100-continue, удаляет заголовок и
        прокидывает запрос в подмодуль, который тоже должен отдать ответ.
        Также модуль удаляет другие Expect: заголовки
    ''',
    [],
    examples=[],
)
def H100(options):
    result = {"h100": copy.copy(options)}
    return LuaPrefixes(), result, result["h100"]


@Helper(
    'Modules.Cutter',
    '''
        Модуль, похожий на threshold.
        Запрос, попавший в него, пройдёт дальше при наступлении любого из трёх условий:
            а) получен запрос целиком
            б) получено bytes байт тела
            в) Прошло timeout времени
        ''',
    [
        ('bytes', None, int, False,
         'Достаточно получить столько байт тела, чтобы пропустить запрос вниз до наступления timeout'),
        ('timeout', None, str, False,
         'Максимальное время ожидание получения bytes байт или полного тела'),
    ],
    examples=[],
)
def Cutter(options):
    if options['bytes'] is not None:
        if int(options['bytes']) < 0:
            raise Exception('cutter: bytes(%s) cannot be less than 0' % options['bytes'])

    result = {'cutter': options}
    return LuaPrefixes(), result, result['cutter']


@Helper(
    'Modules.RequestReplier',
    '''
        Модуль для отфоркивания входящего трафика.
        Подробности в https://st.yandex-team.ru/BALANCER-759
    ''',
    [

        (
            'sink', None, list, True,
            'Секция для модуля, куда пойдёт повторный запрос.'
        ),
        (
            'enable_failed_requests_replication', None, bool, False,
            'Опциональное дублирование сфейлившихся запросов. BALANCER-891'
        ),
        (
            'rate', 0.0, float, False,
            '''
                Вероятность того, что запрос будет повторён в случае успеха основного модуля.
                Можно указывать значения от 0.0 (нет повтора) до 1.0 (всегда повторять).
            '''
        ),
        (
            'rate_file', './controls/request_repl.ratefile', str, True,
            '''
                Файл, в котором можно переопределить значение rate.
                Если файл будет удалён, вернётся значение rate из конфига.
            '''
        ),

    ],
    examples=[],
)
def RequestReplier(options):
    result = {'request_replier': OrderedDict()}
    result_prefix = LuaPrefixes()
    result['request_replier']['rate'] = options['rate']
    result['request_replier']['rate_file'] = options['rate_file']

    if options['enable_failed_requests_replication']:
        result['request_replier']['enable_failed_requests_replication'] = options['enable_failed_requests_replication']

    sink_prefix, sink_dict = SeqProcessor(options['sink'])
    result_prefix += sink_prefix
    result['request_replier']['sink'] = sink_dict

    return result_prefix, result, result['request_replier']


@Helper(
    'Module.Icookie',
    """
    i-cookie - идентифицирующая браузер пользователя. Является развитием fuid и yandexuid.
    https://wiki.yandex-team.ru/serp/experiments/icookie2/opisanie/#i-cookie
    https://wiki.yandex-team.ru/users/valgushev/moduleicookie/
    st/TRAFFIC-2282
    """,
    [
        ('keys_file', None, str, False, 'Файл с ключом'),
        ('use_default_keys', False, bool, False, 'Использовать дефолтные  ключи, нелья использовать с keys_file'),
        ('domains', None, str, True, 'Домены на которых ставится кука.'),
        ('trust_parent', False, bool, False, 'Флаг о доверии внутренней сети. Нужен для trusted nets'),
        ('trust_children', False, bool, False, 'Флаг о доверии к бекендам.'),
        ('enable_set_cookie', True, bool, False, 'Разрешить модулю делать выставление icookie.'),
        (
            'enable_decrypting', True, bool, False,
            'Разрешить модулю расшифровывать значение icookie и записывать результат в заголовки',
        ),
        ('decrypted_uid_header', 'X-Yandex-ICookie', str, False, 'Icookie header'),
        ('error_header', 'X-Yandex-ICookie-Error', str, False, 'Icookie error header'),
        (
            'take_randomuid_from', 'X-Yandex-RandomUID', str, False,
            'Имя заголовка, из которого модуль будет брать значение RandomUID'
        ),
        ('encrypted_header', None, str, False, 'Icookie encrypted header'),
        (
            'file_switch', 'controls/balancer_icookie_switch', str, False,
            'Если файл существует, то модуль выключает обработку icookie'
        ),
        ('force_equal_to_yandexuid', False, bool, False, 'выдавать всем пользователям icookie == yandexuid'),
        ('force_generate_from_searchapp_uuid', False, bool, False, 'инициализировать icookie из uuid в searchapp'),
        ('enable_parse_searchapp_uuid', True, bool, False, 'включить парсинг icookie из uuid'),
        ('enable_guess_searchapp', False, bool, False, 'не перевыставлять icookie, выставленную по uuid'),
        # To be deleted MINOTAUR-2654
        ('exp_salt', None, str, False, ''),
        ('exp_A_testid', None, int, False, ''),
        ('exp_B_testid', None, int, False, ''),
        ('exp_A_slots', None, int, False, ''),
        ('exp_B_slots', None, int, False, ''),
    ],
    examples=[]
)
def Icookie(options):
    """
    """
    if options['keys_file'] is not None and options['use_default_keys']:
        raise Exception("Imposible use both keys_file and use_default_keys in Icookie")
    elif options['keys_file'] is None and not options['use_default_keys']:
        raise Exception("keys_file or use_default_keys must set in Icookie")

    result = {'icookie': options}
    return LuaPrefixes(), result, result['icookie']


@Helper(
    'Modules.TcpRstOnError',
    '''
    Упростить завершение tcp соединения с "плохими" клиентами.
    st/BALANCER-1060
    ''',
    [
        (
            'send_rst', None, bool, False,
            '''Булевая переменная определяющая, что отправляется при закрытии соединения при ошибке.
            Модуль переопределяет значение опции default_tcp_rst_on_error из модуля Main.
            По умолчанию True, т.е. при ошибке отправляеся RST+ACK'''
        ),
    ],
    examples=[]
)
def TcpRstOnError(options):
    result = {'tcp_rst_on_error': options}
    return LuaPrefixes(), result, result['tcp_rst_on_error']


@Helper(
    'Modules.LogHeaders',
    '''
        Модуль для логирования заголовков
    ''',
    [
        ('name_re', None, str, True, 'Регулярка с именем заголовка'),
        ('response_name_re', None, str, False, 'Регулярка для заголовков ответа'),
        ('cookie_fields', None, str, False, 'Список кук через запятую'),
        ('log_cookie_meta', None, bool, False, 'Логирование имён кук в Cookie'),
        ('log_set_cookie_meta', None, bool, False, 'Логирование метаданных кук в Set-Cookie')
    ],
    examples=[],
)
def LogHeaders(options):
    result = {'log_headers': options}
    return LuaPrefixes(), result, result['log_headers']


@Helper(
    'Modules.AdBlockBlock',
    '''
        Модуль для расшифроки урлов
    ''',
    [
        ('url_header', None, str, True, 'Регулярка с именем заголовка'),
        ('url_cut_prefix', None, str, True, 'Cтрока, префикс, который надо отрезать от значения  url_header'),
        (
            'decryptor', None, list, True,
            'секция с деревом модулей, которые должны привести к "расшифровывальщику урлов"'
        ),
        (
            'on_error', None, list, False,
            'секция с деревом модулей, которые будут вызываны в случае неответа  decryptor секции'
        ),
    ],
    examples=[],
)
# BALANCER-1130
def AdBlockBlock(options):
    result = {'adblockblock': options}
    result_prefix = LuaPrefixes()

    decryptor_prefix, decryptor_dict = SeqProcessor(options['decryptor'])
    result_prefix += decryptor_prefix
    result['adblockblock']['decryptor'] = decryptor_dict

    if options['on_error']:
        on_error_prefix, on_error_dict = SeqProcessor(options['on_error'])
        result_prefix += on_error_prefix
        result['adblockblock']['on_error'] = on_error_dict
    return result_prefix, result, result['adblockblock']


@Helper(
    'Modules.ExpStatic',
    '''
        Модуль для внутренних экспериментов балансера
    ''',
    [
        ('exp_id', None, int, True, 'Идентификатор эксперимента'),
        ('cont_id', None, int, False, 'Идентификатор контрольной группы'),
        ('salt', None, str, False, 'Соль для изменения рандомизации эксперимента'),
        ('rate_file', None, str, False, 'Путь до файла регулирующего эксперимент'),
        ('slots_count', 100, int, False, 'Количество слотов для бакетов'),
        ('http3_mode', None, bool, False, 'MINOTAUR-2915'),
        ('http3_upstream', None, bool, False, 'MINOTAUR-2915'),
    ],
    examples=[],
)
def ExpStatic(options):
    options.update({"events": {"stats": "report"}})
    result = {'exp_static': options}
    return LuaPrefixes(), result, result['exp_static']


@Helper(
    'Modules.RpsLimiter',
    '''Ходит в бэкенд rpslimiter'а. При превышении квоты на rps отдает пользователю 429''',
    [
        ('checker', None, list, True, 'Подмодуль, в котором описано хождение в бэкенд rpslimiter'),
        ('module', None, list, True, 'Подмодуль, в который передастся запрос в случае успеха'),
        ('on_error', None, list, False, 'Подмодуль, в который передастся запрос в случае ошибки rpslimiter-а'),
        ('on_limited_request', None, list, False, 'Подмодуль, в который передастся запрос в случае если rpslimiter ограничил запрос'),
        ('skip_on_error', None, bool, False, 'Отправлять ли запрос в подмодуль в случае ошибки rpslimiter-а'),
        ('namespace', None, str, False, 'Название секции в rpslimiter. Передается в заголовке X-Rpslimiter-Balancer'),
        ('register_only', None, bool, False, 'Асинхронный поход в rpslimiter с целью регистрации, но не лимитирования'),
        ('register_backend_attempts', None, bool, False,
            'Регистрировать столько запросов, сколько было попыток в бэкенды, в случае асинхронного хождения'),
        ('disable_file', None, str, False, 'Файловая ручка для отключения походов в rpslimiter'),
    ],
    examples=[],
)
def RpsLimiter(options):
    result = {'rps_limiter': options}
    prefix = LuaPrefixes()

    checker_prefix, result['rps_limiter']['checker'] = SeqProcessor(options['checker'])
    prefix += checker_prefix

    module_prefix, result['rps_limiter']['module'] = SeqProcessor(options['module'])
    prefix += module_prefix

    if options['on_error']:
        on_error_prefix, result['rps_limiter']['on_error'] = SeqProcessor(options['on_error'])
        prefix += on_error_prefix
    
    if options['on_limited_request']:
        on_limited_request_prefix, result['rps_limiter']['on_limited_request'] = SeqProcessor(options['on_limited_request'])
        prefix += on_limited_request_prefix

    return prefix, result, None


@Helper(
    'Modules.CookiePolicy',
    '''Валидация и исправление кук''',
    [
        ('uuid', None, str, True, 'uuid модуля политик'),
        ('file_switch', './controls/cookie_policy_modes', str, False, 'Выключение отдельных политик'),
        ('gdpr_file_switch', './controls/cookie_policy_gdpr', str, False, 'Управление опциями gdpr'),
        ('protect_gdpr', None, bool, False, 'Защищать управляющие gdpr куки от перетирания бэкендами'),
        ('mode_overrides', None, dict, False, 'Переопределение политик'),
        ('default_yandex_policies', None, str, False, 'Политики по умолчанию (off/stable/unstable)'),
    ],
    examples=[],
)
def CookiePolicy(options):
    opts = OrderedDict(
        uuid=options["uuid"],
        file_switch=options["file_switch"],
        gdpr_file_switch=options["gdpr_file_switch"],
        default_yandex_policies=options["default_yandex_policies"]
    )

    if options['mode_overrides']:
        opts["mode_controls"] = {}
        opts["mode_controls"]["policy_modes"] = {}
        d = opts["mode_controls"]["policy_modes"]
        for k, v in sorted(options['mode_overrides'].items()):
            d[k] = {'mode': v}

    if options['protect_gdpr']:
        opts['protected_cookie'] = {'gdpr_protected': {
            'mode': 'fix',
            'name_re': 'is_gdpr|is_gdpr_b|gdpr_popup|gdpr'
        }}

    return LuaPrefixes(), {'cookie_policy': opts}, opts

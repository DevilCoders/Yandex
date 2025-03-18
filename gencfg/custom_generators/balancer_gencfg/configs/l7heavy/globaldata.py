#!/skynet/python/bin/python
# -*- coding: utf-8 -*-
import itertools
import os
import sys
from collections import OrderedDict
from functools import partial

import src.constants as Constants
import src.utils as Utils
from src.lua_globals import LuaFuncCall
from src.lua_globals import LuaProdOrTesting

from l7macro import BackendsRegexp, BackendsLocation

import fukraine as Fukraine
import default as Default
import images as Images
import maps as Maps
import messenger as Messenger
import morda as Morda
import search as Search
import static as Static
import suggest as Suggest
import misc as Misc
import ugc as Ugc
import video as Video
import web_search as WebSearch


sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', 'custom_generators', 'src', 'src'))
)


def _yp_endpoint_set(cluster_name, endpoint_set_id):
    return OrderedDict((
        ('cluster_name', cluster_name),
        ('endpoint_set_id', endpoint_set_id)
    ))

# ## CONSTANTS (only for immutable variables)


YALITE_BACKENDS_KUBR_IPV6 = [
    'VLA_WEB_RUS_YALITE',
    'SAS_WEB_RUS_YALITE',
]

YALITE_BACKENDS_TUR_IPV6 = [
    'VLA_WEB_TUR_YALITE',
    'SAS_WEB_TUR_YALITE',
]

YALITE_MORDA_BACKENDS = [
    _yp_endpoint_set('sas', 'production-pumpi-yp'),
    _yp_endpoint_set('vla', 'production-pumpi-yp'),
]

DOMAINS = [
    'yandex.ru',
    'yandex.ua',
    'yandex.uz',
    'yandex.by',
    'yandex.kz',
    'yandex.com',
    'yandex.com.tr',
    'yandex.com.ge',
    'yandex.fr',
    'yandex.az',
    'yandex.com.am',
    'yandex.co.il',
    'yandex.kg',
    'yandex.lt',
    'yandex.lv',
    'yandex.md',
    'yandex.tj',
    'yandex.tm',
    'yandex.ee',
    'yandex.eu',
    'yandex.fi',
    'yandex.pl',
    'ya.ru',
    'm.ya.ru',
    'www.ya.ru',
]

ICOOKIE_DOMAINS = ','.join(['.' + d for d in DOMAINS])

SERVICE_L7_SEARCH = [
    Search.tmpl_search_service,
]

SERVICE_L7_MORDA = [
    Morda.template_morda_service,
]

L7_HEAVY = [
    Static.template_status_check_noc_regexp,
    partial(Default.template_gen_regexp, 'yandex', Default.gen_yandex_matcher, [
        Suggest.template_suggest,  # выше WebSearchSection потому что там wildcard suggest*
        WebSearch.template_web_search,
        Images.tmpl_images,
        Search.tmpl_search,
        Video.tmpl_video,
        Search.tmpl_turbo,
        Messenger.tmpl_messenger_api_alpha,
        Messenger.tmpl_messenger_api,
        Maps.tmpl_maps,
        Misc.tmpl_geohelper,
        Misc.tmpl_ick,
        Misc.tmpl_ebell,
        Misc.tmpl_conflagexp,
        Morda.template_android_widget_api,
        Morda.template_partner_strmchannels,
        Morda.template_portal,
        Morda.template_wy,
        Ugc.tmpl_ugc_my,
        Ugc.tmpl_user,
        Ugc.tmpl_ugcpub,
        Images.tmpl_collections,
        Misc.tmpl_uslugi,
        Misc.tmpl_gnc,
        Misc.tmpl_sport_live,  # should be before tmpl_news
        Misc.tmpl_news,
        Misc.tmpl_service_workers,
        Misc.tmpl_comments,
        Misc.tmpl_sprav,
        Misc.tmpl_znatoki,
        Misc.tmpl_appcry,
        Misc.tmpl_games,
        Misc.tmpl_weather,
        Misc.tmpl_an,
        Misc.tmpl_ads,
        Default.template_default,  # ALWAYS KEEP THE LAST!!!
    ]),
    partial(Default.template_gen_regexp, 'yaru', Default.gen_yaru_matcher, [
        Morda.template_yaru,
    ]),
    Default.template_error_default,
]

L7_FUKRAINE = [
    Static.template_status_check_noc_regexp,
    Fukraine.template_proxies,
]

L7_YARU = [
    Static.template_status_check_noc,
    Morda.template_yaru,
]

# =========================================== RX-294 START ==================================================
TRANSPORT = None
# =========================================== RX-294 FINISH =================================================


class Location(object):
    SAS = 'sas'
    MAN = 'man'
    VLA = 'vla'
    ALL = [
        SAS,
        MAN,
        VLA,
    ]


class Domain(object):
    RKUB = 'rkub'
    FUKRAINE = 'fukraine'
    L7_YARU = 'yaru'
    SERVICE_L7_SEARCH = 'service_search'
    SERVICE_L7_MORDA = 'service_morda'

    ALL = [
        RKUB,
        FUKRAINE,
        L7_YARU,
        SERVICE_L7_SEARCH,
        SERVICE_L7_MORDA,
    ]


class ConfigType(object):

    TUNNEL_COMMON = 'tunnel_common'
    TUNNEL_MMS = 'tunnel_mms'
    TUNNEL_TESTING = 'tunnel_testing'
    EXPERIMENTS = 'experiments'
    TYPE_FUKRAINE = 'tunnel_fukraine'
    L7_YARU = 'yaru'
    L7_YARU_TESTING = 'yaru_testing'
    SERVICE_L7_SEARCH = 'service_search'
    SERVICE_L7_MORDA = 'service_morda'
    ALL = [
        TUNNEL_COMMON,
        TUNNEL_MMS,
        TUNNEL_TESTING,
        TYPE_FUKRAINE,
        L7_YARU,
        L7_YARU_TESTING,
        EXPERIMENTS,
        SERVICE_L7_SEARCH,
        SERVICE_L7_MORDA,
    ]

    @staticmethod
    def is_mms(conftype):
        return conftype == ConfigType.TUNNEL_MMS


def attr_factory_method(key_names):
    @staticmethod
    def create_attr(val_map):
        return Attr(key_names, val_map)
    return create_attr


class AttrException(Exception):
    pass


class Attr(object):
    NAMESPACES = {
        'location': Location,
        'domain': Domain,
        'conftype': ConfigType,
    }

    def __init__(self, key_names, val_map, is_prop=False):
        super(Attr, self).__init__()
        self.__key_names = key_names
        if is_prop or not self.__key_names:
            self.__val_map = val_map
        else:
            self.__val_map = dict()
            for metakey, value in reversed(val_map.items()):
                if len(key_names) == 1:
                    metakey = (metakey, )
                unmasked = list()
                for key_name, key_value in zip(key_names, metakey):
                    all_values = self.NAMESPACES[key_name].ALL
                    if key_value == '*':
                        assert(type(val_map) is OrderedDict)
                        res_keys = all_values
                    else:
                        if key_value not in all_values:
                            raise AttrException(
                                'Unknown {key_name} "{key_value}" in attribute {attr}. Known {key_name}s: {all_values}'
                                .format(key_name=key_name, key_value=key_value, attr=self.name,
                                        all_values=', '.join(all_values)))
                        res_keys = [key_value]
                    unmasked.append(res_keys)
                self.__val_map.update({tuple(key): value for key in itertools.product(*unmasked)})
        self.__is_prop = is_prop
        self.name = None

    def __get__(self, obj, type_):
        if self.name in obj.custom:
            return obj.custom[self.name]
        if self.__is_prop:
            return self.__val_map(obj)
        if not self.__key_names:  # constant attribute
            return self.__val_map
        key = tuple(getattr(obj, name) for name in self.__key_names)
        if key not in self.__val_map:
            raise AttrException('{key} not found for attribute {attr}'.format(key=str(key), attr=self.name))
        return self.__val_map[key]

    def __set__(self, obj, value):
        raise AttributeError()

    constant = attr_factory_method([])

    location = attr_factory_method(['location'])

    domain = attr_factory_method(['domain'])

    conftype = attr_factory_method(['conftype'])

    location_domain = attr_factory_method(['location', 'domain'])

    location_conftype = attr_factory_method(['location', 'conftype'])

    domain_conftype = attr_factory_method(['domain', 'conftype'])

    location_domain_conftype = attr_factory_method(['location', 'domain', 'conftype'])

    full = location_domain_conftype

    @staticmethod
    def prop(func):
        return Attr([], func, is_prop=True)


class AttrNameResolverMetaClass(type):
    def __new__(cls, classname, bases, class_dict):
        for name, attr in class_dict.iteritems():
            if isinstance(attr, Attr):
                attr.name = name
        return type.__new__(cls, classname, bases, class_dict)


def _get_ip_by_route(ip_ver_list):
    return [LuaFuncCall('GetIpByIproute', {'key': ip_ver}) for ip_ver in ip_ver_list]


class ConfigData(object):
    __metaclass__ = AttrNameResolverMetaClass

    def __init__(self, location, domain, conftype, custom=None):
        super(ConfigData, self).__init__()
        self.__location = location
        self.__domain = domain
        self.__conftype = conftype
        self.__iplist = None
        if custom is None:
            self.__custom = dict()
        else:
            self.__custom = custom

    # FIX_ME: move to __init__ after proper paralleling
    def verify(self, transport):
        global TRANSPORT
        try:
            TRANSPORT = transport
            main_ips = set(self.main_iplist)
            familysearch_ips = set(self.familysearch_iplist)
            assert main_ips.isdisjoint(familysearch_ips)
        finally:
            TRANSPORT = None

    @property
    def location(self):
        return self.__location

    @property
    def domain(self):
        return self.__domain

    @property
    def conftype(self):
        return self.__conftype

    @property
    def custom(self):
        return self.__custom

    @property
    def mms(self):
        return ConfigType.is_mms(self.conftype)

    sect_list = Attr.domain_conftype(OrderedDict([
        (('rkub', ConfigType.TUNNEL_COMMON), L7_HEAVY),
        (('rkub', ConfigType.TUNNEL_TESTING), L7_HEAVY),
        (('rkub', ConfigType.TUNNEL_MMS), L7_HEAVY),
        (('fukraine', ConfigType.TYPE_FUKRAINE), L7_FUKRAINE),
        (('yaru', ConfigType.L7_YARU), L7_YARU),
        (('yaru', ConfigType.L7_YARU_TESTING), L7_YARU),
        (('service_search', ConfigType.SERVICE_L7_SEARCH), SERVICE_L7_SEARCH),
        (('service_morda', ConfigType.SERVICE_L7_MORDA), SERVICE_L7_MORDA),
        (('rkub', ConfigType.EXPERIMENTS), L7_HEAVY),
    ]))

    section_name = Attr.full(OrderedDict([
        (('sas', 'rkub', ConfigType.TUNNEL_COMMON), 'slb_ru_sas'),
        (('man', 'rkub', ConfigType.TUNNEL_COMMON), 'slb_ru_man'),
        (('vla', 'rkub', ConfigType.TUNNEL_COMMON), 'slb_ru_vla'),
        (('sas', 'rkub', ConfigType.TUNNEL_MMS), 'slb_ru_sas'),
        (('man', 'rkub', ConfigType.TUNNEL_MMS), 'slb_ru_man'),
        (('vla', 'rkub', ConfigType.TUNNEL_MMS), 'slb_ru_vla'),
        (('man', 'rkub', ConfigType.TUNNEL_TESTING), 'slb_ru_man_test'),
        (('sas', 'rkub', ConfigType.TUNNEL_TESTING), 'slb_ru_sas_test'),
        (('vla', 'rkub', ConfigType.TUNNEL_TESTING), 'slb_ru_vla_test'),

        (('man', 'fukraine', ConfigType.TYPE_FUKRAINE), 'slb_fukraine_man'),
        (('sas', 'fukraine', ConfigType.TYPE_FUKRAINE), 'slb_fukraine_man'),
        (('vla', 'fukraine', ConfigType.TYPE_FUKRAINE), 'slb_fukraine_man'),

        (('man', 'yaru', ConfigType.L7_YARU), 'slb_yaru_man'),
        (('sas', 'yaru', ConfigType.L7_YARU), 'slb_yaru_sas'),
        (('vla', 'yaru', ConfigType.L7_YARU), 'slb_yaru_vla'),

        (('sas', 'yaru', ConfigType.L7_YARU_TESTING), 'slb_yaru_sas_test'),

        (('man', 'service_search', ConfigType.SERVICE_L7_SEARCH), 'slb_service_search_man'),
        (('sas', 'service_search', ConfigType.SERVICE_L7_SEARCH), 'slb_service_search_sas'),
        (('vla', 'service_search', ConfigType.SERVICE_L7_SEARCH), 'slb_service_search_vla'),

        (('man', 'service_morda', ConfigType.SERVICE_L7_MORDA), 'slb_service_morda_man'),
        (('sas', 'service_morda', ConfigType.SERVICE_L7_MORDA), 'slb_service_morda_sas'),
        (('vla', 'service_morda', ConfigType.SERVICE_L7_MORDA), 'slb_service_morda_vla'),

        (('vla', 'rkub', ConfigType.EXPERIMENTS), 'slb_ru_vla_experiments'),
    ]))

    http_instance_port = Attr.full(OrderedDict([
        (('*', 'rkub', ConfigType.TUNNEL_MMS), 8084),
        (('*', 'rkub', ConfigType.TUNNEL_TESTING), 80),
        (('*', 'fukraine', ConfigType.TYPE_FUKRAINE), 8720),
        (('*', 'yaru', ConfigType.L7_YARU), 16200),
        (('*', 'yaru', ConfigType.L7_YARU_TESTING), 8080),
        (('*', 'service_search', ConfigType.SERVICE_L7_SEARCH), 80),
        (('*', 'service_morda', ConfigType.SERVICE_L7_MORDA), 80),
        (('*', 'rkub', ConfigType.TUNNEL_COMMON), 8080),
        (('vla', 'rkub', ConfigType.EXPERIMENTS), 8080),
    ]))

    https_instance_port = Attr.domain_conftype({
        ('rkub', ConfigType.TUNNEL_MMS): 8085,
        ('rkub', ConfigType.TUNNEL_TESTING): 8701,
        ('fukraine', ConfigType.TYPE_FUKRAINE): 8721,
        ('yaru', ConfigType.L7_YARU): 16201,
        ('yaru', ConfigType.L7_YARU_TESTING): 8081,
        ('service_search', ConfigType.SERVICE_L7_SEARCH): None,
        ('service_morda', ConfigType.SERVICE_L7_MORDA): None,
        ('rkub', ConfigType.TUNNEL_COMMON): 8081,
        ('rkub', ConfigType.EXPERIMENTS): 8081,
    })

    workers = Attr.full(OrderedDict([
        (('vla', 'rkub', ConfigType.EXPERIMENTS), 45),

        (('vla', 'rkub', ConfigType.TUNNEL_COMMON), 45),
        (('sas', 'rkub', ConfigType.TUNNEL_COMMON), 45),
        (('man', 'rkub', ConfigType.TUNNEL_COMMON), 30),

        (('*', 'rkub', ConfigType.TUNNEL_MMS), 1),

        (('*', 'rkub', ConfigType.TUNNEL_TESTING), 1),

        (('*', 'fukraine', ConfigType.TYPE_FUKRAINE), 2),
        (('vla', 'yaru', ConfigType.L7_YARU), 2),
        (('sas', 'yaru', ConfigType.L7_YARU), 2),
        (('man', 'yaru', ConfigType.L7_YARU), 4),
        (('*', 'yaru', ConfigType.L7_YARU_TESTING), 2),
        (('*', 'service_search', ConfigType.SERVICE_L7_SEARCH), 7),
        (('*', 'service_morda', ConfigType.SERVICE_L7_MORDA), 7),
    ]))

    temp_buf_prealloc = Attr.full(OrderedDict([
        (('vla', 'rkub', ConfigType.EXPERIMENTS), 6144),

        (('vla', 'rkub', ConfigType.TUNNEL_COMMON), 6144),
        (('sas', 'rkub', ConfigType.TUNNEL_COMMON), 6144),
        (('man', 'rkub', ConfigType.TUNNEL_COMMON), 4096),

        (('*', '*', '*'), 0),
    ]))

    maxconn = Attr.full(OrderedDict([
        (('sas', 'rkub', ConfigType.TUNNEL_COMMON), 15000),
        (('vla', 'rkub', ConfigType.TUNNEL_COMMON), 15000),
        (('vla', 'rkub', ConfigType.EXPERIMENTS), 15000),

        (('*', '*', '*'), 20000),
    ]))

    # enable trusted ip sections for project
    trusted_networks = Attr.domain_conftype(OrderedDict([
        (('rkub', '*'), True),
        (('fukraine', '*'), False),
        (('yaru', '*'), False),
        (('service_search', '*'), False),
        (('service_morda', '*'), False),
    ]))

    hsts = Attr.domain_conftype(OrderedDict([
        (('rkub', '*'), True),
        (('fukraine', '*'), False),
        (('yaru', '*'), True),
        (('service_search', '*'), False),
        (('service_morda', '*'), False),
    ]))

    main_iplist = Attr.full(OrderedDict([
        (('*', '*', ConfigType.TUNNEL_TESTING), Utils.get_ip_from_rt('l7b-test.yandex.ru')),

        # EXPERIMENTS
        (('*', 'rkub', ConfigType.EXPERIMENTS), Utils.get_ip_from_rt(['yandex.ru', 'yandex.com', 'yandex.com.tr', 'yandex.ua', 'ya.ru'])),

        # TERM
        (('*', 'rkub', ConfigType.TUNNEL_COMMON), Utils.get_ip_from_rt(['yandex.ru', 'yandex.com', 'yandex.com.tr', 'yandex.ua', 'ya.ru'])),

        # TERM MMS
        (('man', 'rkub', ConfigType.TUNNEL_MMS), Utils.get_ip_from_rt(['man.yandex.ru', 'man.yandex.com.tr', 'man.yandex.com'])),
        (('sas', 'rkub', ConfigType.TUNNEL_MMS), Utils.get_ip_from_rt(['sas.yandex.ru', 'sas.yandex.com.tr', 'sas.yandex.com'])),
        (('vla', 'rkub', ConfigType.TUNNEL_MMS), Utils.get_ip_from_rt(['vla.yandex.ru', 'vla.yandex.com.tr', 'vla.yandex.com'])),

        # FUKRAINE
        (('*', 'fukraine', ConfigType.TYPE_FUKRAINE), Utils.get_ip_from_rt('proxy.yandex.ua')),

        # YARU
        (('*', 'yaru', ConfigType.L7_YARU), Utils.get_ip_from_rt('ya.ru')),

        # YARU_TESTING
        (('*', '*', ConfigType.L7_YARU_TESTING), Utils.get_ip_from_rt('l7test.ya.ru')),

        # knoss
        (('*', '*', ConfigType.SERVICE_L7_SEARCH), []),
        (('*', '*', ConfigType.SERVICE_L7_MORDA), []),
    ]))

    familysearch_iplist = Attr.full(OrderedDict([
        # EXPERIMENTS
        (('*', 'rkub', ConfigType.EXPERIMENTS), Utils.get_ip_from_rt(['familysearch.yandex.ru', 'family.yandex.com.tr'])),

        # TERM
        (('*', 'rkub', ConfigType.TUNNEL_COMMON), Utils.get_ip_from_rt(['familysearch.yandex.ru', 'family.yandex.com.tr'])),

        (('*', '*', '*'), []),
    ]))

    instance_iplist = Attr.full(OrderedDict([
        (('*', '*', ConfigType.TUNNEL_TESTING), _get_ip_by_route(['v6'])),
        (('sas', 'rkub', ConfigType.TUNNEL_MMS), _get_ip_by_route(['v6'])),
        (('sas', 'rkub', ConfigType.TUNNEL_COMMON), _get_ip_by_route(['v6'])),
        (('man', 'rkub', ConfigType.TUNNEL_MMS), _get_ip_by_route(['v6'])),
        (('man', 'rkub', ConfigType.TUNNEL_COMMON), _get_ip_by_route(['v6'])),
        (('vla', 'rkub', ConfigType.TUNNEL_MMS), _get_ip_by_route(['v6'])),
        (('vla', 'rkub', ConfigType.TUNNEL_COMMON), _get_ip_by_route(['v6'])),
        (('*', 'fukraine', ConfigType.TYPE_FUKRAINE), _get_ip_by_route(['v6'])),
        (('man', 'yaru', ConfigType.L7_YARU), _get_ip_by_route(['v4', 'v6'])),
        (('sas', 'yaru', ConfigType.L7_YARU), _get_ip_by_route(['v4', 'v6'])),
        (('vla', 'yaru', ConfigType.L7_YARU), _get_ip_by_route(['v4', 'v6'])),
        (('*', 'yaru', ConfigType.L7_YARU_TESTING), _get_ip_by_route(['v6'])),

        (('man', 'service_search', ConfigType.SERVICE_L7_SEARCH), _get_ip_by_route(['v6'])),
        (('sas', 'service_search', ConfigType.SERVICE_L7_SEARCH), _get_ip_by_route(['v6'])),
        (('vla', 'service_search', ConfigType.SERVICE_L7_SEARCH), _get_ip_by_route(['v6'])),

        (('man', 'service_morda', ConfigType.SERVICE_L7_MORDA), _get_ip_by_route(['v6'])),
        (('sas', 'service_morda', ConfigType.SERVICE_L7_MORDA), _get_ip_by_route(['v6'])),
        (('vla', 'service_morda', ConfigType.SERVICE_L7_MORDA), _get_ip_by_route(['v6'])),

        (('vla', 'rkub', ConfigType.EXPERIMENTS), _get_ip_by_route(['v6'])),
    ]))

    weight_matrix = Attr.location({
        'sas': OrderedDict([
            ('sas', 1),
            ('man', -1),
            ('vla', -1),
            ('apphost_sas', -1),
            ('apphost_vla', -1),
            ('apphost_man', -1),
            ('turbo_slb_vla', -1),
            ('turbo_slb_sas', -1),
            ('turbo_slb_man', -1),
            ('pumpkin', -1),
        ]),
        'man': OrderedDict([
            ('sas', -1),
            ('man', 1),
            ('vla', -1),
            ('apphost_sas', -1),
            ('apphost_vla', -1),
            ('apphost_man', -1),
            ('turbo_slb_vla', -1),
            ('turbo_slb_sas', -1),
            ('turbo_slb_man', -1),
            ('pumpkin', -1),
        ]),
        'vla': OrderedDict([
            ('sas', -1),
            ('man', -1),
            ('vla', 1),
            ('pumpkin', -1),
            ('apphost_vla', -1),
            ('apphost_sas', -1),
            ('apphost_man', -1),
            ('turbo_slb_vla', -1),
            ('turbo_slb_sas', -1),
            ('turbo_slb_man', -1),
            ('pumpkin', -1),
        ]),
    })

    morda_weight_matrix = Attr.constant(OrderedDict([
        ('man', -1),
        ('sas', 1),
        ('vla', 1),
        ('man_ah', -1),
        ('sas_ah', -1),
        ('vla_ah', -1),
        ('man_fallback', -1),
        ('sas_fallback', -1),
        ('vla_fallback', -1),
        ('pumpkin', -1),
    ]))

    search_weight_matrix = Attr.constant(OrderedDict([
        ('man', -1),
        ('sas', 1),
        ('vla', 1),
        ('pumpkin', -1),

        ('apphost_sas', -1),
        ('apphost_vla', -1),
        ('apphost_man', -1),

        ('prestable_sas', -1),
        ('prod_sas', -1),
        ('apphost_prestable_sas', -1),
        ('apphost_prod_sas', -1),
    ]))

    morda_app_weight_matrix = Attr.constant(OrderedDict([
        ('man', -1),
        ('sas', 1),
        ('vla', 1),
        ('man_ah', -1),
        ('sas_ah', -1),
        ('vla_ah', -1),
        ('man_fallback', -1),
        ('sas_fallback', -1),
        ('vla_fallback', -1),
        ('pumpkin', -1),
    ]))

    # MINOTAUR-2109
    maps_knoss_backends = Attr.constant(OrderedDict([
        ('man', LuaProdOrTesting(
            [_yp_endpoint_set('man', 'rtc_balancer_front-maps_slb_maps_yandex_net_man')],
            [_yp_endpoint_set('man', 'rtc_balancer_front-maps_tst_slb_maps_yandex_net_man')],
        )),
        ('sas', LuaProdOrTesting(
            [_yp_endpoint_set('sas', 'rtc_balancer_front-maps_slb_maps_yandex_net_sas')],
            [_yp_endpoint_set('sas', 'rtc_balancer_front-maps_tst_slb_maps_yandex_net_sas')],
        )),
        ('vla', LuaProdOrTesting(
            [_yp_endpoint_set('vla', 'rtc_balancer_front-maps_slb_maps_yandex_net_vla')],
            [_yp_endpoint_set('vla', 'rtc_balancer_front-maps_tst_slb_maps_yandex_net_vla')],
        )),
    ]))

    search_pumpkin = BackendsRegexp([
        (
            'comtr',
            {
                'match_fsm': OrderedDict([
                    ('host', '(.*\\\\.)?yandex\\\\.com\\\\.tr'),
                    ('case_insensitive', True),
                ])
            },
            YALITE_BACKENDS_TUR_IPV6,
        ),
        (
            'default',
            {},
            YALITE_BACKENDS_KUBR_IPV6,
        )
    ])

    search_backends_split_sas = Attr.constant(OrderedDict([
        ('man', [_yp_endpoint_set('man', 'production-report-man-web-yp')]),
        ('sas', [
            _yp_endpoint_set('sas', 'production-report-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-report-sas-web-yp')
        ]),
        ('prestable_sas', [
            _yp_endpoint_set('sas', 'prestable-report-sas-web-yp')
        ]),
        ('prod_sas', [
            _yp_endpoint_set('sas', 'production-report-sas-web-yp'),
        ]),
        ('vla', [_yp_endpoint_set('vla', 'production-report-vla-web-yp')]),
        ('pumpkin', search_pumpkin),
    ]))

    search_backends = Attr.constant(OrderedDict([
        ('man', [_yp_endpoint_set('man', 'production-report-man-web-yp')]),
        ('sas', [
            _yp_endpoint_set('sas', 'production-report-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-report-sas-web-yp')
        ]),
        ('vla', [_yp_endpoint_set('vla', 'production-report-vla-web-yp')]),
        ('pumpkin', search_pumpkin),
    ]))

    search_knoss_backends = Attr.constant(OrderedDict([
        ('man', [_yp_endpoint_set('man', 'balancer_knoss_search_yp_man')]),
        ('sas', [_yp_endpoint_set('sas', 'balancer_knoss_search_yp_sas')]),
        ('vla', [_yp_endpoint_set('vla', 'balancer_knoss_search_yp_vla')]),
    ]))

    morda_knoss_backends = Attr.constant(OrderedDict([
        ('man', LuaProdOrTesting(
            [_yp_endpoint_set('man', 'balancer_knoss_morda_yp_man')],
            ['SAS_L7_BALANCER_KNOSS_MORDA_TESTING(hbf_mtn=1)']
        )),
        ('sas', LuaProdOrTesting(
            [_yp_endpoint_set('sas', 'balancer_knoss_morda_yp_sas')],
            ['SAS_L7_BALANCER_KNOSS_MORDA_TESTING(hbf_mtn=1)']
        )),
        ('vla', LuaProdOrTesting(
            [_yp_endpoint_set('vla', 'balancer_knoss_morda_yp_vla')],
            ['SAS_L7_BALANCER_KNOSS_MORDA_TESTING(hbf_mtn=1)']
        )),
    ]))

    ugc_knoss_backends = Attr.constant(OrderedDict([
        ('man', LuaProdOrTesting(
            [_yp_endpoint_set('man', 'awacs-rtc_balancer_ugc_search_yandex_net_yp_man')],
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_knoss_ugc_testing_yp_sas')],
        )),
        ('sas', LuaProdOrTesting(
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_ugc_search_yandex_net_yp_sas')],
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_knoss_ugc_testing_yp_sas')],
        )),
        ('vla', LuaProdOrTesting(
            [_yp_endpoint_set('vla', 'awacs-rtc_balancer_ugc_search_yandex_net_yp_vla')],
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_knoss_ugc_testing_yp_sas')],
        )),
    ]))


    turbo_knoss_backends = Attr.constant(OrderedDict([
        ('man', [_yp_endpoint_set('man', 'rtc-balancer-turbopages-service-balancer-yp-man')]),
        ('sas', [_yp_endpoint_set('sas', 'rtc-balancer-turbopages-service-balancer-yp-sas')]),
        ('vla', [_yp_endpoint_set('vla', 'rtc-balancer-turbopages-service-balancer-yp-vla')]),
    ]))

    _web_apphost_options = {
        'balancer_options': OrderedDict([
            ('request', Constants.APPHOST_HEALTH_CHECK_REQUEST),
            ('delay', '10s'),
            ('use_backend_weight', True),
            ('weight_normalization_coeff', 100)
        ]),
        'balancer_type': 'active',
    }

    search_apphost_migration_backends = Attr.constant(OrderedDict([
        ('vla', [_yp_endpoint_set('vla', 'production-report-vla-web-yp')]),
        ('sas', [
            _yp_endpoint_set('sas', 'production-report-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-report-sas-web-yp')
        ]),
        ('prestable_sas', [
            _yp_endpoint_set('sas', 'prestable-report-sas-web-yp')
        ]),
        ('prod_sas', [
            _yp_endpoint_set('sas', 'production-report-sas-web-yp'),
        ]),
        ('man', [_yp_endpoint_set('man', 'production-report-man-web-yp')]),
        ('apphost_vla', BackendsLocation([
            _yp_endpoint_set('vla', 'production-app-host-http_adapter-vla-web-yp')
        ], _web_apphost_options)),
        ('apphost_sas', BackendsLocation([
            _yp_endpoint_set('sas', 'production-app-host-http_adapter-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-app-host-http_adapter-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-exp-app-host-http_adapter-sas-web-yp'),
        ], _web_apphost_options)),
        ('apphost_prestable_sas', BackendsLocation([
            _yp_endpoint_set('sas', 'prestable-app-host-http_adapter-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-exp-app-host-http_adapter-sas-web-yp'),
        ], _web_apphost_options)),
        ('apphost_prod_sas', BackendsLocation([
            _yp_endpoint_set('sas', 'production-app-host-http_adapter-sas-web-yp'),
        ], _web_apphost_options)),
        ('apphost_man', BackendsLocation([
            _yp_endpoint_set('man', 'production-app-host-http_adapter-man-web-yp')
        ], _web_apphost_options)),
    ]))

    search_apphost_migration_backends_step_2 = Attr.constant(OrderedDict([
        ('vla', BackendsLocation([
            _yp_endpoint_set('vla', 'production-app-host-http_adapter-vla-web-yp')
        ], _web_apphost_options)),
        ('sas', BackendsLocation([
            _yp_endpoint_set('sas', 'production-app-host-http_adapter-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-app-host-http_adapter-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-exp-app-host-http_adapter-sas-web-yp'),
        ], _web_apphost_options)),
        ('prestable_sas', BackendsLocation([
            _yp_endpoint_set('sas', 'prestable-app-host-http_adapter-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-exp-app-host-http_adapter-sas-web-yp'),
        ], _web_apphost_options)),
        ('prod_sas', BackendsLocation([
            _yp_endpoint_set('sas', 'production-app-host-http_adapter-sas-web-yp'),
        ], _web_apphost_options)),
        ('man', BackendsLocation([
            _yp_endpoint_set('man', 'production-app-host-http_adapter-man-web-yp')
        ], _web_apphost_options)),
        ('apphost_vla', BackendsLocation([
            _yp_endpoint_set('vla', 'production-app-host-http_adapter-vla-web-yp')
        ], _web_apphost_options)),
        ('apphost_sas', BackendsLocation([
            _yp_endpoint_set('sas', 'production-app-host-http_adapter-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-app-host-http_adapter-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-exp-app-host-http_adapter-sas-web-yp'),
        ], _web_apphost_options)),
        ('apphost_prestable_sas', BackendsLocation([
            _yp_endpoint_set('sas', 'prestable-app-host-http_adapter-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-exp-app-host-http_adapter-sas-web-yp'),
        ], _web_apphost_options)),
        ('apphost_prod_sas', BackendsLocation([
            _yp_endpoint_set('sas', 'production-app-host-http_adapter-sas-web-yp'),
        ], _web_apphost_options)),
        ('apphost_man', BackendsLocation([
            _yp_endpoint_set('man', 'production-app-host-http_adapter-man-web-yp')
        ], _web_apphost_options)),
    ]))

    http_adapter_search_backends = Attr.constant(OrderedDict([
        ('vla', BackendsLocation([
            _yp_endpoint_set('vla', 'production-app-host-http_adapter-vla-web-yp')
        ], _web_apphost_options)),
        ('sas', BackendsLocation([
            _yp_endpoint_set('sas', 'production-app-host-http_adapter-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-app-host-http_adapter-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-exp-app-host-http_adapter-sas-web-yp'),
        ], _web_apphost_options)),
        ('man', BackendsLocation([
            _yp_endpoint_set('man', 'production-app-host-http_adapter-man-web-yp')
        ], _web_apphost_options)),
        ('pumpkin', search_pumpkin),
    ]))

    # MINOTAUR-3111
    http_adapter_search_split_sas_backends = Attr.constant(OrderedDict([
        ('vla', BackendsLocation([
            _yp_endpoint_set('vla', 'production-app-host-http_adapter-vla-web-yp')
        ], _web_apphost_options)),
        ('sas', BackendsLocation([
            _yp_endpoint_set('sas', 'production-app-host-http_adapter-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-app-host-http_adapter-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-exp-app-host-http_adapter-sas-web-yp'),
        ], _web_apphost_options)),
        ('prestable_sas', BackendsLocation([
            _yp_endpoint_set('sas', 'prestable-app-host-http_adapter-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-exp-app-host-http_adapter-sas-web-yp'),
        ], _web_apphost_options)),
        ('prod_sas', BackendsLocation([
            _yp_endpoint_set('sas', 'production-app-host-http_adapter-sas-web-yp'),
        ], _web_apphost_options)),
        ('man', BackendsLocation([
            _yp_endpoint_set('man', 'production-app-host-http_adapter-man-web-yp')
        ], _web_apphost_options)),
        ('pumpkin', search_pumpkin),
    ]))

    http_adapter_search_split_sas_backends_sas_vla = Attr.constant(OrderedDict([
        ('vla', BackendsLocation([
            _yp_endpoint_set('vla', 'production-app-host-http_adapter-vla-web-yp')
        ], _web_apphost_options)),
        ('sas', BackendsLocation([
            _yp_endpoint_set('sas', 'production-app-host-http_adapter-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-app-host-http_adapter-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-exp-app-host-http_adapter-sas-web-yp'),
        ], _web_apphost_options)),
        ('prestable_sas', BackendsLocation([
            _yp_endpoint_set('sas', 'prestable-app-host-http_adapter-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-exp-app-host-http_adapter-sas-web-yp'),
        ], _web_apphost_options)),
        ('prod_sas', BackendsLocation([
            _yp_endpoint_set('sas', 'production-app-host-http_adapter-sas-web-yp'),
        ], _web_apphost_options)),
    ]))

    http_adapter_search_backends_sas_vla = Attr.constant(OrderedDict([
        ('vla', BackendsLocation([
            _yp_endpoint_set('vla', 'production-app-host-http_adapter-vla-web-yp')
        ], _web_apphost_options)),
        ('sas', BackendsLocation([
            _yp_endpoint_set('sas', 'production-app-host-http_adapter-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-app-host-http_adapter-sas-web-yp'),
            _yp_endpoint_set('sas', 'prestable-exp-app-host-http_adapter-sas-web-yp'),
        ], _web_apphost_options)),
    ]))

    # TRAFFIC-1793
    morda_backends = Attr.constant(OrderedDict([
        ('sas', [
            _yp_endpoint_set('sas', 'stable-portal-apphost-sas')
        ]),
        ('man', [
            _yp_endpoint_set('man', 'stable-portal-apphost-man')
        ]),
        ('vla', [
            _yp_endpoint_set('vla', 'stable-portal-apphost-vla')
        ]),
        ('sas_ah', [
            _yp_endpoint_set('sas', 'stable-portal-apphost-sas')
        ]),
        ('man_ah', [
            _yp_endpoint_set('man', 'stable-portal-apphost-man')
        ]),
        ('vla_ah', [
            _yp_endpoint_set('vla', 'stable-portal-apphost-vla')
        ]),
        ('sas_fallback', [
            _yp_endpoint_set('sas', 'stable-morda-sas-yp')
        ]),
        ('man_fallback', [
            _yp_endpoint_set('man', 'stable-morda-man-yp')
        ]),
        ('vla_fallback', [
            _yp_endpoint_set('vla', 'stable-morda-vla-yp')
        ]),
        ('pumpkin', YALITE_MORDA_BACKENDS),
    ]))

    morda_apphost_backends = Attr.constant(OrderedDict([
        ('sas', [
            _yp_endpoint_set('sas', 'stable-portal-apphost-sas')
        ]),
        ('man', [
            _yp_endpoint_set('man', 'stable-portal-apphost-man')
        ]),
        ('vla', [
            _yp_endpoint_set('vla', 'stable-portal-apphost-vla')
        ]),
        ('sas_ah', [
            _yp_endpoint_set('sas', 'stable-portal-apphost-sas')
        ]),
        ('man_ah', [
            _yp_endpoint_set('man', 'stable-portal-apphost-man')
        ]),
        ('vla_ah', [
            _yp_endpoint_set('vla', 'stable-portal-apphost-vla')
        ]),
        ('sas_fallback', [
            _yp_endpoint_set('sas', 'stable-morda-sas-yp')
        ]),
        ('man_fallback', [
            _yp_endpoint_set('man', 'stable-morda-man-yp')
        ]),
        ('vla_fallback', [
            _yp_endpoint_set('vla', 'stable-morda-vla-yp')
        ]),
        ('pumpkin', YALITE_MORDA_BACKENDS),
    ]))

    morda_app_backends = Attr.constant(OrderedDict([
        ('sas', [
            _yp_endpoint_set('sas', 'stable-portal-app-apphost-sas')
        ]),
        ('man', [
            _yp_endpoint_set('man', 'stable-portal-app-apphost-man')
        ]),
        ('vla', [
            _yp_endpoint_set('vla', 'stable-portal-app-apphost-vla')
        ]),
        ('sas_ah', [
            _yp_endpoint_set('sas', 'stable-portal-apphost-sas')
        ]),
        ('man_ah', [
            _yp_endpoint_set('man', 'stable-portal-apphost-man')
        ]),
        ('vla_ah', [
            _yp_endpoint_set('vla', 'stable-portal-apphost-vla')
        ]),
        ('sas_fallback', [
            _yp_endpoint_set('sas', 'stable-portal-morda-app-sas')
        ]),
        ('man_fallback', [
            _yp_endpoint_set('man', 'stable-portal-morda-app-man')
        ]),
        ('vla_fallback', [
            _yp_endpoint_set('vla', 'stable-portal-morda-app-vla')
        ]),
        ('pumpkin', YALITE_MORDA_BACKENDS),
    ]))

    morda_androidwidget_backends = Attr.constant(OrderedDict([
        ('sas', [
            _yp_endpoint_set('sas', 'stable-portal-androidwidget-sas')
        ]),
        ('man', [
            _yp_endpoint_set('man', 'stable-portal-androidwidget-man')
        ]),
        ('vla', [
            _yp_endpoint_set('vla', 'stable-portal-androidwidget-vla')
        ]),
        ('pumpkin', YALITE_MORDA_BACKENDS),
    ]))

    # MINOTAUR-1984
    portal_station_backends = Attr.constant(OrderedDict([
        ('sas', [
            _yp_endpoint_set('sas', 'stable-morda-station-sas-yp')
        ]),
        ('man', [
            _yp_endpoint_set('man', 'stable-morda-station-man-yp')
        ]),
        ('vla', [
            _yp_endpoint_set('vla', 'stable-morda-station-vla-yp')
        ]),
        ('pumpkin', YALITE_MORDA_BACKENDS),
    ]))

    # MINOTAUR-3013
    portal_front_backends = Attr.constant(OrderedDict([
        ('sas', [
            _yp_endpoint_set('sas', 'stable-portal-apphost-sas')
        ]),
        ('man', [
            _yp_endpoint_set('man', 'stable-portal-apphost-man')
        ]),
        ('vla', [
            _yp_endpoint_set('vla', 'stable-portal-apphost-vla')
        ]),
    ]))

    # MINOTAUR-396
    yaru_backends = Attr.constant(OrderedDict([
        ('sas', [
            _yp_endpoint_set('sas', 'stable-morda-yaru-sas-yp')
        ]),
        ('man', [
            _yp_endpoint_set('man', 'stable-morda-yaru-man-yp')
        ]),
        ('vla', [
            _yp_endpoint_set('vla', 'stable-morda-yaru-vla-yp')
        ]),
        ('pumpkin', YALITE_BACKENDS_KUBR_IPV6),
    ]))

    any_backends = Attr.constant(OrderedDict([
        ('sas', [
            _yp_endpoint_set('sas', 'stable-morda-any-sas-yp')
        ]),
        ('man', [
            _yp_endpoint_set('man', 'stable-morda-any-man-yp')
        ]),
        ('vla', [
            _yp_endpoint_set('vla', 'stable-morda-any-vla-yp')
        ]),
        ('pumpkin', YALITE_BACKENDS_KUBR_IPV6),
    ]))

    video_yp_knoss_backends = Attr.constant(OrderedDict([
        ('man', LuaProdOrTesting(
            [_yp_endpoint_set('man', 'awacs-rtc_balancer_knoss_video_yp_man')],
            [_yp_endpoint_set('man', 'awacs-rtc_balancer_knoss_video_testing_yp_man')],
        )),
        ('sas', LuaProdOrTesting(
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_knoss_video_yp_sas')],
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_knoss_video_testing_yp_sas')],
        )),
        ('vla', LuaProdOrTesting(
            [_yp_endpoint_set('vla', 'awacs-rtc_balancer_knoss_video_yp_vla')],
            [_yp_endpoint_set('vla', 'awacs-rtc_balancer_knoss_video_testing_yp_vla')],
        )),
    ]))

    # MINOTAUR-1627
    # MINOTAUR-2471
    collections_knoss_backends = Attr.constant(OrderedDict([
        ('man', LuaProdOrTesting(
            [_yp_endpoint_set('man', 'collections-service-balancer-production-man-16020')],
            [_yp_endpoint_set('man', 'rtc-balancer-collections-test-yandex-ru-yp-man')]
        )),
        ('sas', LuaProdOrTesting(
            [_yp_endpoint_set('sas', 'collections-service-balancer-production-sas-16020')],
            [_yp_endpoint_set('sas', 'rtc-balancer-collections-test-yandex-ru-yp-sas')],
        )),
        ('vla', LuaProdOrTesting(
            [_yp_endpoint_set('vla', 'collections-service-balancer-production-vla-16020')],
            [_yp_endpoint_set('vla', 'rtc-balancer-collections-test-yandex-ru-yp-vla')],
        )),
    ]))

    images_knoss_backends = Attr.constant(OrderedDict([
        ('man', LuaProdOrTesting(
            [_yp_endpoint_set('man', 'awacs-rtc_balancer_knoss-images-yp_man')],
            [_yp_endpoint_set('man', 'awacs-rtc_balancer_knoss-images-testing-yp_man')],
        )),
        ('sas', LuaProdOrTesting(
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_knoss-images-yp_sas')],
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_knoss-images-testing-yp_sas')],
        )),
        ('vla', LuaProdOrTesting(
            [_yp_endpoint_set('vla', 'awacs-rtc_balancer_knoss-images-yp_vla')],
            [_yp_endpoint_set('vla', 'awacs-rtc_balancer_knoss-images-testing-yp_vla')],
        )),
    ]))

    sport_live_knoss_backends = Attr.constant(OrderedDict([
        ('man', LuaProdOrTesting(
            [_yp_endpoint_set('man', 'awacs-rtc_balancer_ott-front-sport-prod-internal_ott_yandex_net_man')],
            [_yp_endpoint_set('vla', 'awacs-rtc_balancer_ott-front-sport-nonprod_ott_yandex_net_vla')]
        )),
        ('sas', LuaProdOrTesting(
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_ott-front-sport-prod-internal_ott_yandex_net_sas')],
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_ott-front-sport-nonprod_ott_yandex_net_sas')]
        )),
        ('vla', LuaProdOrTesting(
            [_yp_endpoint_set('vla', 'awacs-rtc_balancer_ott-front-sport-prod-internal_ott_yandex_net_vla')],
            [_yp_endpoint_set('vla', 'awacs-rtc_balancer_ott-front-sport-nonprod_ott_yandex_net_vla')]
        )),
    ]))

    news_knoss_backends_yp = Attr.constant(OrderedDict([
        ('man', [_yp_endpoint_set('man', 'awacs-rtc_balancer_news_yandex_ru_yp_man')]),
        ('sas', [_yp_endpoint_set('sas', 'awacs-rtc_balancer_news_yandex_ru_yp_sas')]),
        ('vla', [_yp_endpoint_set('vla', 'awacs-rtc_balancer_news_yandex_ru_yp_vla')]),
    ]))

    comments_knoss_backends = Attr.constant(OrderedDict([
        ('man', LuaProdOrTesting(
            [_yp_endpoint_set('man', 'awacs-rtc_balancer_cmnt-prod-balancer_yandex_net_yp_man')],
            [_yp_endpoint_set('man', 'awacs-rtc_balancer_cmnt-alpha-balancer_yandex_net_yp_man')]
        )),
        ('sas', LuaProdOrTesting(
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_cmnt-prod-balancer_yandex_net_yp_sas')],
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_cmnt-alpha-balancer_yandex_net_yp_sas')]
        )),
        ('vla', LuaProdOrTesting(
            [_yp_endpoint_set('vla', 'awacs-rtc_balancer_cmnt-prod-balancer_yandex_net_yp_vla')],
            [_yp_endpoint_set('vla', 'awacs-rtc_balancer_cmnt-alpha-balancer_yandex_net_yp_vla')]
        )),
    ]))

    antirobot_backends = Attr.location({
        'sas': [
            _yp_endpoint_set('sas', 'prod-antirobot-yp-sas'),
            _yp_endpoint_set('sas', 'prod-antirobot-yp-prestable-sas'),
        ],
        'man': [
            _yp_endpoint_set('man', 'prod-antirobot-yp-man'),
            _yp_endpoint_set('man', 'prod-antirobot-yp-prestable-man'),
        ],
        'vla': [
            _yp_endpoint_set('vla', 'prod-antirobot-yp-vla'),
            _yp_endpoint_set('vla', 'prod-antirobot-yp-prestable-vla'),
        ]
    })

    antirobot_sink_backends = Attr.location({
        'sas': [
            _yp_endpoint_set('sas', 'testing-antirobot-iss')
        ],
        'man': [],
        'vla': []
    })

    # MINOTAUR-3108
    suggest_knoss_backends = Attr.constant(OrderedDict([
        ('man', LuaProdOrTesting(
            [_yp_endpoint_set('man', 'awacs-rtc_balancer_knoss-suggest-yp_man')],
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_knoss-suggest-testing-yp_sas')]
        )),
        ('sas', LuaProdOrTesting(
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_knoss-suggest-yp_sas')],
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_knoss-suggest-testing-yp_sas')]
        )),
        ('vla', LuaProdOrTesting(
            [_yp_endpoint_set('vla', 'awacs-rtc_balancer_knoss-suggest-yp_vla')],
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_knoss-suggest-testing-yp_sas')]
        )),
    ]))

    laas_backends = Attr.location({
        'vla': [_yp_endpoint_set('vla', 'vla-prod-laas-yp')],
        'sas': [_yp_endpoint_set('sas', 'sas-prod-laas-yp')],
        'man': [_yp_endpoint_set('man', 'man-prod-laas-yp')],
    })

    laas_backends_onerror = Attr.location({
        'vla': [_yp_endpoint_set('vla', 'vla-prod-laas-fallback-yp')],
        'sas': [_yp_endpoint_set('sas', 'sas-prod-laas-fallback-yp')],
        'man': [_yp_endpoint_set('man', 'man-prod-laas-fallback-yp')],
    })

    # MINOTAUR-2461
    fast_knoss_backends = Attr.constant(OrderedDict([
        ('man', LuaProdOrTesting([_yp_endpoint_set('man', 'awacs-rtc_balancer_knoss_fast_yp_man')], [_yp_endpoint_set('man', 'awacs-rtc_balancer_knoss_fast_testing_yp_man')])),
        ('sas', LuaProdOrTesting([_yp_endpoint_set('sas', 'awacs-rtc_balancer_knoss_fast_yp_sas')], [_yp_endpoint_set('sas', 'awacs-rtc_balancer_knoss_fast_testing_yp_sas')])),
        ('vla', LuaProdOrTesting([_yp_endpoint_set('vla', 'awacs-rtc_balancer_knoss_fast_yp_vla')], [_yp_endpoint_set('vla', 'awacs-rtc_balancer_knoss_fast_testing_vla')])),
    ]))

    # MINOTAUR-2202
    geohelper_knoss_backends = Attr.constant(OrderedDict([
        ('man', LuaProdOrTesting(
            [_yp_endpoint_set('man', 'rtc_balancer_knoss_geohelper_man')],
            [_yp_endpoint_set('man', 'rtc_balancer_knoss_geohelper_testing_man'), _yp_endpoint_set('sas', 'rtc_balancer_knoss_geohelper_testing_sas')],
        )),
        ('sas', LuaProdOrTesting(
            [_yp_endpoint_set('sas', 'rtc_balancer_knoss_geohelper_sas')],
            [_yp_endpoint_set('man', 'rtc_balancer_knoss_geohelper_testing_man'), _yp_endpoint_set('sas', 'rtc_balancer_knoss_geohelper_testing_sas')],
        )),
        ('vla', LuaProdOrTesting(
            [_yp_endpoint_set('vla', 'rtc_balancer_knoss_geohelper_vla')],
            [_yp_endpoint_set('man', 'rtc_balancer_knoss_geohelper_testing_man'), _yp_endpoint_set('sas', 'rtc_balancer_knoss_geohelper_testing_sas')],
        )),
    ]))

    # MINOTAUR-2322
    ick_knoss_backends = Attr.constant(OrderedDict([
        ('man', [_yp_endpoint_set('man', 'rtc_balancer_knoss_ick_man')]),
        ('sas', [_yp_endpoint_set('sas', 'rtc_balancer_knoss_ick_sas')]),
        ('vla', [_yp_endpoint_set('vla', 'rtc_balancer_knoss_ick_vla')]),
    ]))

    ebell_knoss_backends = Attr.constant(OrderedDict([
        ('man', [_yp_endpoint_set('man', 'rtc-balancer-bell-yandex-net-man')]),
        ('sas', [_yp_endpoint_set('sas', 'rtc-balancer-bell-yandex-net-sas')]),
        ('vla', [_yp_endpoint_set('vla', 'rtc-balancer-bell-yandex-net-vla')]),
    ]))

    conflagexp_knoss_backends = Attr.constant(OrderedDict([
        ('man', [_yp_endpoint_set('man', 'awacs-rtc_balancer_conflagexp_yandex-team_ru_man')]),
        ('sas', [_yp_endpoint_set('sas', 'awacs-rtc_balancer_conflagexp_yandex-team_ru_sas')]),
        ('vla', [_yp_endpoint_set('vla', 'awacs-rtc_balancer_conflagexp_yandex-team_ru_vla')]),
    ]))

    spravapi_knoss_backends = Attr.constant(OrderedDict([
        ('man', LuaProdOrTesting(
            [_yp_endpoint_set('man', 'rtc-balancer-sprav-api-yandex-net-man')],
            [_yp_endpoint_set('man', 'rtc-balancer-sprav-api-test-man')]
        )),
        ('sas', LuaProdOrTesting(
            [_yp_endpoint_set('myt', 'rtc-balancer-sprav-api-yandex-net-myt')],
            [_yp_endpoint_set('myt', 'rtc-balancer-sprav-api-test-myt')]
        )),
        ('vla', LuaProdOrTesting(
            [_yp_endpoint_set('vla', 'rtc-balancer-sprav-api-yandex-net-vla')],
            [_yp_endpoint_set('vla', 'rtc-balancer-sprav-api-test-vla')]
        )),
    ]))

    # MINOTAUR-2188
    uslugi_knoss_backends = Attr.constant(OrderedDict([
        ('man', LuaProdOrTesting(
            [_yp_endpoint_set('man', 'rtc_balancer_uslugi_yandex_ru_man')],
            [_yp_endpoint_set('sas', 'rtc_balancer_l7test_uslugi_yandex_ru_sas')],
        )),
        ('sas', LuaProdOrTesting(
            [_yp_endpoint_set('sas', 'rtc_balancer_uslugi_yandex_ru_sas')],
            [_yp_endpoint_set('sas', 'rtc_balancer_l7test_uslugi_yandex_ru_sas')],
        )),
        ('vla', LuaProdOrTesting(
            [_yp_endpoint_set('vla', 'rtc_balancer_uslugi_yandex_ru_vla')],
            [_yp_endpoint_set('sas', 'rtc_balancer_l7test_uslugi_yandex_ru_sas')],
        )),
    ]))

    # MINOTAUR-2464
    gnc_knoss_backends = Attr.constant(OrderedDict([
        ('man', [_yp_endpoint_set('man', 'rtc_balancer_gnc_yandex_ru_man')]),
        ('sas', [_yp_endpoint_set('sas', 'rtc_balancer_gnc_yandex_ru_sas')]),
        ('vla', [_yp_endpoint_set('vla', 'rtc_balancer_gnc_yandex_ru_vla')]),
    ]))

    # MINOTAUR-2517
    znatoki_knoss_backends = Attr.constant(OrderedDict([
        ('man', LuaProdOrTesting(
            [_yp_endpoint_set('man', 'production-balancer-answers-man-yp')],
            [_yp_endpoint_set('man', 'awacs-rtc_balancer_answers-test_yandex_net_yp_man')],
        )),
        ('sas', LuaProdOrTesting(
            [_yp_endpoint_set('sas', 'production-balancer-answers-sas-yp')],
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_answers-test_yandex_net_yp_sas')],
        )),
        ('vla', LuaProdOrTesting(
            [_yp_endpoint_set('vla', 'production-balancer-answers-vla-yp')],
            [_yp_endpoint_set('vla', 'awacs-rtc_balancer_answers-test_yandex_net_yp_vla')],
        )),
    ]))

    # HOME-54281
    wy_backends = Attr.constant(OrderedDict([
        ('vla', [_yp_endpoint_set('vla', 'awacs-rtc_balancer_morda-yp-hw_vla')]),
        ('man', [_yp_endpoint_set('man', 'awacs-rtc_balancer_morda-yp-hw_man')]),
        ('sas', [_yp_endpoint_set('sas', 'awacs-rtc_balancer_morda-yp-hw_sas')]),
    ]))

    wy_weight_matrix = OrderedDict([
        ('vla', 1),
        ('man', -1),
        ('sas', 1),
    ])

    # MINOTAUR-1666
    uaas_backends = Attr.location({
        'man': ['MAN_UAAS(hbf_mtn=1)'],
        'sas': ['SAS_UAAS(hbf_mtn=1)'],
        'vla': ['VLA_UAAS(hbf_mtn=1)'],
    })

    uaas_new_backends = Attr.location({
        'man':  [
            _yp_endpoint_set('man', 'production_uaas_man')
        ],
        'sas': [
            _yp_endpoint_set('sas', 'production_uaas_sas')
        ],
        'vla': [
            _yp_endpoint_set('vla', 'production_uaas_vla')
        ],
    })

    # TRAFFIC-422
    l7_weights_file = Attr.constant(Constants.L7_WEIGHT_SWITCH_FILE)

    yalite_backends = Attr.constant(YALITE_BACKENDS_KUBR_IPV6)
    morda_yalite_backends = Attr.constant(YALITE_MORDA_BACKENDS)

    # MINOTAUR-17
    # MINOTAUR-1908
    remote_log_backends = Attr.location({
        'man':  [
            _yp_endpoint_set('man', 'production-explogdaemon-man')
        ],
        'sas': [
            _yp_endpoint_set('sas', 'production-explogdaemon-sas')
        ],
        'vla': [
            _yp_endpoint_set('vla', 'production-explogdaemon-vla')
        ],
    })

    clck_knoss_backends = Attr.constant(OrderedDict([
        ('man', [_yp_endpoint_set('man', 'awacs-rtc_balancer_knoss_clicks_yp_man')]),
        ('sas', [_yp_endpoint_set('sas', 'awacs-rtc_balancer_knoss_clicks_yp_sas')]),
        ('vla', [_yp_endpoint_set('vla', 'awacs-rtc_balancer_knoss_clicks_yp_vla')]),
    ]))

    service_workers_knoss_backends = Attr.constant(OrderedDict([
        ('man', ['MAN_SERVICE_WORKERS_BALANCER(hbf_mtn=1)']),
        ('sas', ['SAS_SERVICE_WORKERS_BALANCER(hbf_mtn=1)']),
        ('vla', ['VLA_SERVICE_WORKERS_BALANCER(hbf_mtn=1)']),
    ]))

    messenger_api_knoss_backends = Attr.constant(OrderedDict([
        ('man', LuaProdOrTesting(
            [_yp_endpoint_set('man', 'awacs-rtc_balancer_l7_mssngr_search_yandex_net_yp_man')],
            [_yp_endpoint_set('man', 'awacs-rtc_balancer_l7_mssngr_search_yandex_net_yp_man')]
        )),
        ('sas', LuaProdOrTesting(
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_l7_mssngr_search_yandex_net_yp_sas')],
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_l7_mssngr_search_yandex_net_yp_sas')]
        )),
        ('vla', LuaProdOrTesting(
            [_yp_endpoint_set('vla', 'awacs-rtc_balancer_l7_mssngr_search_yandex_net_yp_vla')],
            [_yp_endpoint_set('vla', 'awacs-rtc_balancer_l7_mssngr_search_yandex_net_yp_vla')]
        )),
    ]))

    messenger_api_alpha_knoss_backends = Attr.constant(OrderedDict([
        ('man', LuaProdOrTesting(
            [_yp_endpoint_set('man', 'awacs-rtc_balancer_alpha_l7_mssngr_search_yandex_net_man')],
            [_yp_endpoint_set('man', 'awacs-rtc_balancer_alpha_l7_mssngr_search_yandex_net_man')]
        )),
        ('sas', LuaProdOrTesting(
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_alpha_l7_mssngr_search_yandex_net_sas')],
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_alpha_l7_mssngr_search_yandex_net_sas')]
        )),
        ('vla', LuaProdOrTesting(
            [_yp_endpoint_set('vla', 'awacs-rtc_balancer_alpha_l7_mssngr_search_yandex_net_vla')],
            [_yp_endpoint_set('vla', 'awacs-rtc_balancer_alpha_l7_mssngr_search_yandex_net_vla')]
        )),
    ]))

    cryprox_backends = Attr.constant(OrderedDict([
        ('man', [_yp_endpoint_set('man', 'rtc_balancer_cryprox_yandex_net_man')]),
        ('sas', [_yp_endpoint_set('sas', 'rtc_balancer_cryprox_yandex_net_sas')]),
        ('vla', [_yp_endpoint_set('vla', 'rtc_balancer_cryprox_yandex_net_vla')]),
    ]))

    games_knoss_backends = Attr.constant(OrderedDict([
        ('man', [_yp_endpoint_set('man', 'awacs-rtc_balancer_games-prod-balancer_yandex_net_man')]),
        ('sas', LuaProdOrTesting(
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_games-prod-balancer_yandex_net_sas')],
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_games-testing-balancer_yandex_net_sas')]
        )),
        ('vla', LuaProdOrTesting(
            [_yp_endpoint_set('vla', 'awacs-rtc_balancer_games-prod-balancer_yandex_net_vla')],
            [_yp_endpoint_set('vla', 'awacs-rtc_balancer_games-testing-balancer_yandex_net_vla')]
        )),
    ]))

    # MINOTAUR-3021
    weather_knoss_backends = Attr.constant(OrderedDict([
        ('man', LuaProdOrTesting(
            [_yp_endpoint_set('man', 'awacs-rtc_balancer_knoss_fast_yp_man')],
            [_yp_endpoint_set('man', 'awacs-rtc_balancer_frontend_weather-tst_yandex_net_man')]
        )),
        ('sas', LuaProdOrTesting(
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_knoss_fast_yp_sas')],
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_frontend_weather-tst_yandex_net_sas')]
        )),
        ('vla', [_yp_endpoint_set('vla', 'awacs-rtc_balancer_knoss_fast_yp_vla')]),
    ]))

    # MINOTAUR-2753
    an_knoss_backends = Attr.constant(OrderedDict([
        ('iva', [_yp_endpoint_set('iva', 'awacs-rtc_balancer_bs_yandex_ru_iva')]),
        ('man', [_yp_endpoint_set('man', 'awacs-rtc_balancer_bs_yandex_ru_man')]),
        ('sas', LuaProdOrTesting(
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_bs_yandex_ru_sas')],
            [_yp_endpoint_set('sas', 'awacs-rtc_balancer_pre_bs_yandex_ru_sas')]
        )),
        ('vla', [_yp_endpoint_set('vla', 'awacs-rtc_balancer_bs_yandex_ru_vla')]),
    ]))

    # MINOTAUR-2753
    ads_knoss_backends = Attr.constant(OrderedDict([
        ('man', [_yp_endpoint_set('man', 'awacs-rtc_balancer_yabs-pcode_yandex_net_man')]),
        ('sas', [_yp_endpoint_set('sas', 'awacs-rtc_balancer_yabs-pcode_yandex_net_sas')]),
        ('vla', [_yp_endpoint_set('vla', 'awacs-rtc_balancer_yabs-pcode_yandex_net_vla')]),
    ]))


    icookie_domains = ICOOKIE_DOMAINS

    dynamic_balancing_options = Attr.constant(LuaProdOrTesting(OrderedDict(), OrderedDict([
        ('max_pessimized_share', 0.3),
    ])))

    rps_limiter_backends = Attr.location({
        'man':  [
            _yp_endpoint_set('man', 'rpslimiter-man')
        ],
        'sas': [
            _yp_endpoint_set('sas', 'rpslimiter-sas')
        ],
        'vla': [
            _yp_endpoint_set('vla', 'rpslimiter-vla')
        ],
    })

    rps_limiter_web_backends = Attr.location({
        'man':  [
            _yp_endpoint_set('man', 'rpslimiter-web-man')
        ],
        'sas': [
            _yp_endpoint_set('sas', 'rpslimiter-web-sas')
        ],
        'vla': [
            _yp_endpoint_set('vla', 'rpslimiter-web-vla')
        ],
    })

    ecoo_backends = Attr.constant(OrderedDict([
        ('man', ['MAN_EXPCOOKIER(hbf_mtn=1)']),
        ('sas', ['SAS_EXPCOOKIER(hbf_mtn=1)']),
        ('vla', ['VLA_EXPCOOKIER(hbf_mtn=1)']),
    ]))

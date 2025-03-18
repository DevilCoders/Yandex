#!/skynet/python/bin/python
# -*- coding: utf-8 -*-

import src.constants as Constants

from collections import OrderedDict
from src.lua_globals import LuaFuncCall, LuaGlobal, LuaAnonymousKey


def _gen_regexp(sni):
    return sni.replace(".", "[.]").replace("*", "[^.]+")


def _join_regexp(regexps):
    return '|'.join(regexps)


_ANY_YANDEX_TLD_REGEXP = _join_regexp([
    _gen_regexp('*.xn--d1acpjx3f.xn--p1ai'),
    _gen_regexp('*.ya.ru'),
    _gen_regexp('*.yandex.aero'),
    _gen_regexp('*.yandex.az'),
    _gen_regexp('*.yandex.by'),
    _gen_regexp('*.yandex.co.il'),
    _gen_regexp('*.yandex.com'),
    _gen_regexp('*.yandex.com.am'),
    _gen_regexp('*.yandex.com.ge'),
    _gen_regexp('*.yandex.com.tr'),
    _gen_regexp('*.yandex.com.ua'),
    _gen_regexp('*.yandex.de'),
    _gen_regexp('*.yandex.ee'),
    _gen_regexp('*.yandex.fr'),
    _gen_regexp('*.yandex.jobs'),
    _gen_regexp('*.yandex.kg'),
    _gen_regexp('*.yandex.kz'),
    _gen_regexp('*.yandex.lt'),
    _gen_regexp('*.yandex.lv'),
    _gen_regexp('*.yandex.md'),
    _gen_regexp('*.yandex.net'),
    _gen_regexp('*.yandex.org'),
    _gen_regexp('*.yandex.ru'),
    _gen_regexp('*.yandex.tj'),
    _gen_regexp('*.yandex.tm'),
    _gen_regexp('*.yandex.ua'),
    _gen_regexp('*.yandex.uz'),
    _gen_regexp('xn--d1acpjx3f.xn--p1ai'),
    _gen_regexp('ya.ru'),
    _gen_regexp('yandex.aero'),
    _gen_regexp('yandex.az'),
    _gen_regexp('yandex.by'),
    _gen_regexp('yandex.co.il'),
    _gen_regexp('yandex.com'),
    _gen_regexp('yandex.com.am'),
    _gen_regexp('yandex.com.ge'),
    _gen_regexp('yandex.com.tr'),
    _gen_regexp('yandex.com.ua'),
    _gen_regexp('yandex.de'),
    _gen_regexp('yandex.ee'),
    _gen_regexp('yandex.fr'),
    _gen_regexp('yandex.jobs'),
    _gen_regexp('yandex.kg'),
    _gen_regexp('yandex.kz'),
    _gen_regexp('yandex.lt'),
    _gen_regexp('yandex.lv'),
    _gen_regexp('yandex.md'),
    _gen_regexp('yandex.net'),
    _gen_regexp('yandex.org'),
    _gen_regexp('yandex.ru'),
    _gen_regexp('yandex.tj'),
    _gen_regexp('yandex.tm'),
    _gen_regexp('yandex.ua'),
    _gen_regexp('yandex.uz'),
])

_ANY_CUSTOM_AFISHA_REGEXP = _join_regexp([
    _gen_regexp('*.legal.yandex.*'),
    _gen_regexp('*.disk.yandex.*'),
    _gen_regexp('*.feedback.yandex.*'),
    _gen_regexp('*.afisha.yandex.*'),
    _gen_regexp('*.legal.yandex.com.tr'),
    _gen_regexp('*.legal.yandex.co.il'),
    _gen_regexp('*.disk.yandex.com.tr'),
    _gen_regexp('m.disk.yandex.*'),
    _gen_regexp('www.m.feedback.yandex.*'),
    _gen_regexp('www.m.afisha.yandex.*'),
    _gen_regexp('www.adv.yandex.*'),
    _gen_regexp('www.reklama.yandex.*'),
    _gen_regexp('www.advertising.yandex.*'),
    _gen_regexp('www.help.yandex.*'),
    _gen_regexp('www.taxi.yandex.*'),
    _gen_regexp('www.go.yandex.*'),
    _gen_regexp('www.business.taxi.yandex.*'),
    _gen_regexp('www.mobile-feedback.yandex.ru'),
    _gen_regexp('www.advertising.yandex.com.tr'),
    _gen_regexp('www.help.yandex.co.il'),
    _gen_regexp('www.help.yandex.com.tr'),
    _gen_regexp('www.taxi.yandex.com.ge'),
    _gen_regexp('www.go.yandex.com.ge'),
    _gen_regexp('www.agg.taxi.yandex.net'),
    _gen_regexp('www.agg.taximeter.yandex.ru'),
    _gen_regexp('www.aggregator.taxi.yandex.net'),
    _gen_regexp('www.business.yango.yandex.com'),
    _gen_regexp('www.yango.yandex.com'),
    _gen_regexp('www.go.ya.ru'),
    _gen_regexp('www.go.yandex'),
    _gen_regexp('www.taxi.ya.ru'),
    _gen_regexp('www.yandex.taxi'),
])

_ANY_CUSTOM_YANDEX_REGEXP = _join_regexp([
    _gen_regexp('*.yandex'),
])

def get_any_yandex_tld_sni(priority):
    return [
        ('any_yandex_tld', OrderedDict([
            ('cert', LuaGlobal(
                'yandex_public_cert',
                '/dev/shm/certs/allCAs-any.yandex.tld.ecc.pem'
            )),
            ('priv', LuaGlobal(
                'yandex_private_cert',
                '/dev/shm/certs/priv/any.yandex.tld.ecc.pem'
            )),
            ('secondary', OrderedDict([
                ('cert', LuaGlobal(
                    'yandex_secondary_cert_certum',
                    '/dev/shm/certs/allCAs-any.yandex.tld.rsa.pem'
                )),
                ('priv', LuaGlobal(
                    'yandex_secondary_priv_certum',
                    '/dev/shm/certs/priv/any.yandex.tld.rsa.pem'
                )),
            ])),
            ('events', OrderedDict([
                ('reload_ticket_keys', 'reload_ticket'),
                ('force_reload_ticket_keys', 'force_reload_ticket'),
            ])),
            ('priority', priority),
            ('servername', {
                'servername_regexp': _ANY_YANDEX_TLD_REGEXP,
                'case_insensitive': True,
            }),
            ('ticket_keys_list', OrderedDict([
                ('tls_1stkey', OrderedDict([
                    ('keyfile', '/dev/shm/certs/priv/1st.yandex_sha2ecc.key'),
                    ('priority', 1000)
                ])),
                ('tls_2ndkey', OrderedDict([
                    ('keyfile', '/dev/shm/certs/priv/2nd.yandex_sha2ecc.key'),
                    ('priority', 999)
                ])),
                ('tls_3rdkey', OrderedDict([
                    ('keyfile', '/dev/shm/certs/priv/3rd.yandex_sha2ecc.key'),
                    ('priority', 998)
                ])),
            ]))

        ])),
    ]

def _generate_optional_tld_sni(tld, priority=999):
    return (
        'yandex_{0}'.format(tld),
        OrderedDict(
            [
                ('ciphers', Constants.SSL_CIPHERS_SUITES_CHACHA_SHA2),
                ('cert', LuaGlobal(
                    'yandex_{0}_public'.format(tld),
                    '/dev/shm/certs/allCAs-yandex.{0}_sha2_rsa_certum.pem'.format(tld),
                )),
                ('priv', LuaGlobal(
                    'yandex_{0}_private'.format(tld),
                    '/dev/shm/certs/priv/yandex.{0}_sha2_rsa_certum.pem'.format(tld),
                )),
                ('servername', {
                    'servername_regexp': _join_regexp([
                        _gen_regexp('*.yandex.{}'.format(tld)),
                        _gen_regexp('yandex.{}'.format(tld))
                    ]),
                    'case_insensitive': True,
                }),
                ('events', OrderedDict([
                    ('reload_ticket_keys', 'reload_ticket'),
                    ('force_reload_ticket_keys', 'force_reload_ticket'),
                ])),
                ('priority', priority),
                ('ticket_keys_list', OrderedDict([
                    ('tls_1stkey', OrderedDict([
                        ('keyfile', '/dev/shm/certs/priv/1st.yandex_sha2ecc.key'),
                        ('priority', 1000)
                    ])),
                    ('tls_2ndkey', OrderedDict([
                        ('keyfile', '/dev/shm/certs/priv/2nd.yandex_sha2ecc.key'),
                        ('priority', 999)
                    ])),
                    ('tls_3rdkey', OrderedDict([
                        ('keyfile', '/dev/shm/certs/priv/3rd.yandex_sha2ecc.key'),
                        ('priority', 998)
                    ])),
                ]))
            ])
    )


def _generate_optional_sni(name, servername_regexp, priority):
    return (
        name,
        OrderedDict(
            [
                ('ciphers', Constants.SSL_CIPHERS_SUITES_CHACHA_SHA2),
                ('cert', LuaGlobal(
                    'yandex_{0}_public'.format(name),
                    '/dev/shm/certs/allCAs-{0}.pem'.format(name),
                )),
                ('priv', LuaGlobal(
                    'yandex_{0}_private'.format(name),
                    '/dev/shm/certs/priv/{0}.pem'.format(name),
                )),
                ('servername', {
                    'servername_regexp': servername_regexp,
                    'case_insensitive': True,
                }),
                ('events', OrderedDict([
                    ('reload_ticket_keys', 'reload_ticket'),
                    ('force_reload_ticket_keys', 'force_reload_ticket'),
                ])),
                ('priority', priority),
                ('ticket_keys_list', OrderedDict([
                    ('tls_1stkey', OrderedDict([
                        ('keyfile', '/dev/shm/certs/priv/1st.yandex_sha2ecc.key'),
                        ('priority', 1000)
                    ])),
                    ('tls_2ndkey', OrderedDict([
                        ('keyfile', '/dev/shm/certs/priv/2nd.yandex_sha2ecc.key'),
                        ('priority', 999)
                    ])),
                    ('tls_3rdkey', OrderedDict([
                        ('keyfile', '/dev/shm/certs/priv/3rd.yandex_sha2ecc.key'),
                        ('priority', 998)
                    ])),
                ]))
            ]
        )
    )


def get_optional_sni():
    sni_priority = 999
    result = get_any_yandex_tld_sni(sni_priority)
    sni_priority -= 1

    for idx, tld in enumerate(['eu', 'fi', 'pl']):
        result.append(_generate_optional_tld_sni(tld, priority=sni_priority))
        sni_priority -= 1

    result.append(_generate_optional_sni('any_custom_yandex_rsa', _ANY_CUSTOM_YANDEX_REGEXP, priority=sni_priority))
    sni_priority -= 1
    result.append(_generate_optional_sni('any_custom_afisha_rsa', _ANY_CUSTOM_AFISHA_REGEXP, priority=sni_priority))
    sni_priority -= 1

    return result

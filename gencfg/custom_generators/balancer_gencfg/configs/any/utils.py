#!/usr/bin/env python


from src.lua_globals import LuaAnonymousKey
import src.modules as Modules


from collections import OrderedDict
from urlparse import urlsplit


ALL_YANDEX_ZONES = [
    str('asia'), str('az'), str('biz.tr'), str('by'), str('co.il'),
    str('co.no'), str('com'), str('com.am'), str('com.de'), str('com.ge'), str('com.kz'),
    str('com.tr'), str('com.ua'), str('de'), str('dk'), str('do'), str('ee'),
    str('es'), str('eu'), str('fi'), str('fr'), str('ie'), str('in'), str('info.tr'),
    str('it'), str('jobs'), str('jp.net'), str('kg'), str('kz'), str('lt'),
    str('lu'), str('lv'), str('md'), str('mobi'), str('mx'), str('name'),
    str('net'), str('net.ru'), str('no'), str('nu'), str('org'), str('pl'), str('pt'),
    str('qa'), str('ro'), str('rs'), str('ru'), str('sk'), str('st'), str('sx'),
    str('tj'), str('tm'), str('ua'), str('uz'), str('web.tr'), str('xxx')
]


def redirect(src, dst, code=302, dst_rewrites=None):
    redir = OrderedDict(
        dst=_fix_dst(dst),
        code=code
    )
    if dst_rewrites:
        redir['dst_rewrites'] = OrderedDict((LuaAnonymousKey(), rwr) for rwr in (dst_rewrites or []))
    return OrderedDict(src=_fix_src(src), redirect=redir)


def permanent(src, dst, dst_rewrites=None):
    return redirect(src, dst, code=301, dst_rewrites=dst_rewrites)


def forward(src, dst, dst_rewrites=None):
    dst = _fix_dst(dst, scheme='http://')

    if dst.startswith('https://'):
        raise Exception("https:// not supported in forwards: '{}' -> '{}'".format(src, dst))

    fwd = OrderedDict(
        dst=dst
    )

    if dst_rewrites:
        fwd['dst_rewrites'] = OrderedDict((LuaAnonymousKey(), rwr) for rwr in (dst_rewrites or []))

    _, host, _, _, _ = urlsplit(dst)

    if ':' not in host:
        host += ':80'

    fwd['modules'] = [
        (Modules.Balancer2, {
            'attempts': 1,
            'connection_attempts': 5,
            'backends': [host],
            'policies': OrderedDict([
                ('retry_policy', {
                    'unique_policy': {}
                }),
            ]),
            'rate_limiter_coeff': 0.99,
            'attempts_limit': 0.1,
            'balancer_type': 'rr',
            'proxy_options': OrderedDict([
                ('keepalive_count', 1),
                ('keepalive_timeout', '60s'),
                ('backend_timeout', '5s'),
                ('connect_timeout', '50ms'),
            ]),
        }),
    ]

    return OrderedDict(src=_fix_src(src), forward=fwd)


def rewrite_xml():
    return OrderedDict(regexp='[.]xml$', rewrite='')


def _fix_src(src):
    if not _has_scheme(src):
        src = '//' + src
    return src


def _fix_dst(dst, scheme='https://'):
    if not _has_scheme(dst):
        dst = scheme + dst
    return dst


def _has_scheme(url):
    return url.startswith('//') or url.startswith('http://') or url.startswith('https://')

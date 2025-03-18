#!/usr/bin/python
# -*- coding: utf-8 -*-
import os
import json
import mimetypes
from collections import Counter, defaultdict

from urlparse import urlparse

from antiadblock.libs.decrypt_url.lib import get_key
from antiadblock.cryprox.cryprox.common.cryptobody import CryptUrlPrefix, crypt_url

mimetypes.init()


def make_ammo(method, url, headers, case, body=None):
    """ makes phantom ammo """
    # http request w/o entity body template
    req_template = ("%s %s HTTP/1.1\r\n"
                    "%s\r\n"
                    "\r\n")

    # http request with entity body template
    req_template_w_entity_body = ("%s %s HTTP/1.1\r\n"
                                  "%s\r\n"
                                  "Content-Length: %d\r\n"
                                  "\r\n"
                                  "%s\r\n")

    if not body:
        req = req_template % (method, url, headers)
    else:
        req = req_template_w_entity_body % (method, url, headers, len(body), body)

    # phantom ammo template
    ammo_template = ("%d %s\n"
                     "%s")

    return ammo_template % (len(req), case, req)


class Ammo:
    def __init__(self):
        self.ammo = list()
        self.stats = defaultdict(Counter)

    def make(self, path, headers, case, mime, domain):
        self.ammo.append(make_ammo('GET', path, headers, case))
        self.stats['mimes'][mime] += 1
        self.stats['domains'][domain] += 1
        self.stats['total'][mime + ' - ' + domain] += 1

    def get_result(self):
        return ''.join(self.ammo), json.dumps(self.stats, indent=4, sort_keys=True)


def generate_ammo(path, crypt_key, token, seed='8pzm0y'):  # seed is specially constructed, see test_generic_rule_evade in test_crypt_methods
    # Стреляем в тестовый сервис, с его токеном
    headers = '\r\n'.join(["Host: test.local", "User-Agent: tank", "Accept: */*", "X-AAB-PartnerToken: {}".format(token)]) + '\r\n'
    headers_yauid = headers + 'Cookie: yandexuid=8805864381527174306;\r\n'
    headers_icookie = headers + 'Cookie: crookie=xHXanNwoeKGr6/m+LC+e0ZF+rIHvxTcOnbiIHLTUiqF/JBDIILyb3CE5H55wQk80BsSkkgOyGDFpskx1au5X4EdyD3k=;\r\n'
    headers_extuid = headers + 'Cookie: addruid=myaddruidcutecookie;\r\n'
    key = get_key(crypt_key, seed)
    binurlprefix = CryptUrlPrefix('http', host='test.local', seed=seed, prefix='/')

    def get_crypted_path(host, path):
        url = 'http://' + host + path
        return urlparse(crypt_url(binurlprefix, url, key, enable_trailing_slash=False))._replace(scheme='', netloc='').geturl()

    ammo = Ammo()
    for root, dirs, files in os.walk(path):
        for f in files:
            if 'load_ammo.txt' in f or 'description.txt' in f:
                continue
            uri_path = '/' + os.path.join(root, f).split(os.path.sep, 1)[1]
            mime = mimetypes.guess_type(uri_path)[0]
            domain = 'test.local'
            if 'bk_rtb' in uri_path or 'adfox_rtb' in uri_path:
                domain = 'an.yandex.ru' if 'bk_rtb' in uri_path else 'ads.adfox.ru'
                params = "_" + f.split('?')[-1] if f.find('?') >= 0 else ""
                ammo.make(get_crypted_path(domain, uri_path), headers_extuid, f.split('.')[0] + '_extuid' + params, mime, domain)
                ammo.make(get_crypted_path(domain, uri_path), headers_icookie, f.split('.')[0] + '_icookie' + params, mime, domain)
                ammo.make(get_crypted_path(domain, uri_path), headers_yauid, f.split('.')[0] + '_yauid' + params, mime, domain)
            elif 'context.js' in uri_path:
                domain = 'an.yandex.ru'
                ammo.make(get_crypted_path(domain, uri_path), headers, 'PCODE_' + f, mime, domain)
            elif 'generated' in uri_path:
                if mime == 'text/html':
                    ammo.make(uri_path, headers, "encrypt_" + mime, mime, domain)
                domain = 'yastatic.net'
                ammo.make(get_crypted_path(domain, uri_path), headers, "decrypt_" + mime, mime, domain)
            else:
                ammo.make(get_crypted_path(domain, uri_path), headers_yauid, f.split('.')[0], mime, domain)
    return ammo.get_result()

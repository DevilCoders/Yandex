#!/usr/bin/env python

import logging
import unittest

from tornado import gen
from tornado.testing import AsyncTestCase
from tornado.testing import gen_test

from cocaine.services import Service

import langdetect

detector = langdetect.lookup('/usr/share/yandex/lang_detect_data.txt')

log = logging.getLogger("cocaine")
log.setLevel(logging.ERROR)

geo_regions = '24896,20529,20524,187,166,10001,10000'
pass_lang = 'ru'

class TestLangdetect(AsyncTestCase):

    def setUp(self):
        super(TestLangdetect, self).setUp()
        self.langdetect = Service("langdetect")

    @gen.coroutine
    def unpack(self, future):
        ch = yield future
        res = yield ch.rx.get()
        raise gen.Return(res)

    @gen_test(timeout=2)
    def test_cookie2language(self):
        res = yield self.unpack(self.langdetect.cookie2language(100))
        lang = detector.cookie2language(100)
        self.assertEqual(res, lang)

    @gen_test(timeout=2)
    def test_language2cookie(self):
        res = yield self.unpack(self.langdetect.language2cookie("ru"))
        lang = detector.language2cookie("ru")
        self.assertEqual(res, lang)

    @gen_test(timeout=2)
    def test_find_domain(self):
        req = {'domain': 'http://mail.yandex.ru/neo2', 'filter': 'ua,by,kz', 'geo': geo_regions}
        lang = detector.findDomainEx(req)
        changed, domain, region = yield self.unpack(self.langdetect.find_domain(req))
        self.assertEqual(changed, lang["changed"])
        self.assertEqual(domain, lang["domain"])
        self.assertEqual(region, lang["content-region"])

    @gen_test(timeout=2)
    def test_find_language(self):
        req = {'geo': geo_regions,
               'language': 'en',
               'domain': 'yandex.ru',
               'cookie': '213',
               'filter': 'tt,ru,uk',
               'pass-language': pass_lang,
               'default': 'ru'}
        lang = detector.find(req)
        res = yield self.unpack(self.langdetect.find_language(req))
        self.assertEqual(res, lang[0][0])

    @gen_test(timeout=2)
    def test_list_languages(self):
        req = {'geo': geo_regions,
               'language': 'en',
               'domain': 'yandex.ru',
               'cookie': '213',
               'filter': 'tt,ru,uk',
               'pass-language': pass_lang,
               'default': 'ru'}
        lang = detector.list(req)
        res = yield self.unpack(self.langdetect.list_languages(req))
        self.assertEqual(list(res), [i[0] for i in lang])


if __name__ == "__main__":
    unittest.main()

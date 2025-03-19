#!/usr/bin/env python

import logging
import unittest

from tornado import gen
from tornado.testing import AsyncTestCase
from tornado.testing import gen_test

from cocaine.services import Service

import uatraits

detector = uatraits.detector('/usr/share/uatraits/browser.xml', '/usr/share/uatraits/profiles.xml')


log = logging.getLogger("cocaine")
log.setLevel(logging.ERROR)
REQ_W_HEADERS = {"User-Agent" : "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_3) AppleWebKit/537.22 (KHTML, like Gecko) Chrome/25.0.1364.36 YaBrowser/2.0.1364.6141 Safari/537.22",
                 "X-Wap-Profile" : "http://nds1.nds.nokia.com/uaprof/N6230ir200.xml"}

user_agent = 'Mozilla/5.0 (Linux; U;Android 4.1.2; en-us; GT-I9300 Build/JZO54K) AppleWebkit/534.30 (KHTML, like Gecko) Version/4.0 Mobile Safari/534.30'

class TestUatraits(AsyncTestCase):

    def setUp(self):
        super(TestUatraits, self).setUp()
        self.uatraits = Service("uatraits")

    @gen.coroutine
    def unpack(self, future):
        ch = yield future
        res = yield ch.rx.get()
        raise gen.Return(res)

    @gen_test(timeout=2)
    def test_detect_by_headers(self):
        res = yield self.unpack(self.uatraits.detect_by_headers(REQ_W_HEADERS))
        profile_info = detector.detect_by_headers(REQ_W_HEADERS)
        self.assertEqual(set(res.keys()), set(profile_info.keys()))
        for k, v in res.items():
            if v == 'true':
                v = True
            elif v == 'false':
                v = False
            self.assertEqual(v, profile_info[k])

    @gen_test(timeout=2)
    def test_detect(self):
        res = yield self.unpack(self.uatraits.detect(user_agent))
        profile_info = detector.detect(user_agent)
        self.assertEqual(set(res.keys()), set(profile_info.keys()))
        for k, v in res.items():
            if v == 'true':
                v = True
            elif v == 'false':
                v = False
            self.assertEqual(v, profile_info[k])


if __name__ == "__main__":
    unittest.main()

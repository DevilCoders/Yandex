#!/usr/bin/env python

import logging
import unittest

from tornado import gen
from tornado.testing import AsyncTestCase
from tornado.testing import gen_test

from cocaine.services import Service

import regional_units

lookup = regional_units.Lookup()

log = logging.getLogger("cocaine")
log.setLevel(logging.ERROR)


class TestRegionalUnits(AsyncTestCase):

    def setUp(self):
        super(TestRegionalUnits, self).setUp()
        self.regional_units = Service("regional-units")

    @gen.coroutine
    def unpack(self, future):
        ch = yield future
        res = yield ch.rx.get()
        raise gen.Return(res)

    @gen_test(timeout=2)
    def test_transformed_by_iso(self):
        res = yield self.unpack(self.regional_units.transformed_by_iso("distance", "RU", 10.))
        value = lookup.transform_by_iso("distance", "RU", 10.)
        self.assertEqual(res, [ value.value, value.runit.id, value.runit.name ])
        
    @gen_test(timeout=2)
    def test_transformed_by_system(self):
        res = yield self.unpack(self.regional_units.transformed_by_system("distance", 1, 10.))
        value = lookup.transform_by_system("distance", 1, 10.)
        self.assertEqual(res, [ value.value, value.runit.id, value.runit.name ])

    @gen_test(timeout=2)
    def test_transformed_datetime(self):
        res = yield self.unpack(self.regional_units.transformed_datetime(1315147857, "en-US", "%a"))
        value = lookup.transform_datetime(1315147857, "en-US", "%a")
        self.assertEqual(res, value)


if __name__ == "__main__":
    unittest.main()


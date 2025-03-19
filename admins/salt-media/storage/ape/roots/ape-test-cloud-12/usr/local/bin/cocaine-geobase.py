#!/usr/bin/env python
# coding=utf8
import logging
import unittest

from geobase5 import Lookup
import geobase5

from tornado import gen
from tornado.testing import AsyncTestCase
from tornado.testing import gen_test

from cocaine.services import Service

geo = Lookup("/var/cache/geobase/geodata4.bin")


log = logging.getLogger("cocaine")
log.setLevel(logging.ERROR)


class TestRegionInfo(AsyncTestCase):

    @gen.coroutine
    def unpack(self, future):
        ch = yield future
        res = yield ch.rx.get()
        raise gen.Return(res)

    def setUp(self):
        super(TestRegionInfo, self).setUp()
        self.geobase = Service("geobase")

    @gen_test(timeout=2)
    def test_region_id(self):
        res = yield self.unpack(self.geobase.region_id('195.98.64.75'))
        self.assertEqual(res, 193)

        res = yield self.unpack(self.geobase.region_id('92.36.94.80'))
        self.assertEqual(res, 213)

        try:
            yield self.unpack(self.geobase.region_id('127.0.0.1'))
        except Exception:
            pass

        try:
            yield self.unpack(self.geobase.region_id('asdsad'))
        except Exception:
            pass

    @gen_test(timeout=2)
    def test_chief_region_id(self):
        res = yield self.unpack(self.geobase.chief_region_id(1))
        self.assertEqual(res, 213)

        res = yield self.unpack(self.geobase.chief_region_id(225))
        self.assertEqual(res, 213)

        try:
            res = yield self.unpack(self.geobase.chief_region_id(213))
            self.assertEqual(res, -1)
            self.assertTrue(False)
        except Exception:
            pass

    @gen_test(timeout=2)
    def test_children(self):
        res = yield self.unpack(self.geobase.children(213))
        self.assertEqual(tuple(res),
                         (216, 9000, 9999, 20279, 20356, 20357, 20358, 20359, 20360, 20361, 20362, 20363, 114619, 114620))

    @gen_test(timeout=2)
    def test_in(self):
        res = yield self.unpack(self.geobase.id_in(20279, 213))
        self.assertTrue(res)

        res = yield self.unpack(self.geobase.id_in(214, 213))
        self.assertFalse(res)

        res = yield self.unpack(self.geobase.ip_in('92.36.94.80', 213))
        self.assertTrue(res)

        res = yield self.unpack(self.geobase.ip_in('173.194.39.162', 213))
        self.assertFalse(res)

    @gen_test(timeout=2)
    def test_parent_id(self):
        res = yield self.unpack(self.geobase.parent_id(213))
        self.assertEqual(res, 1)

        res = yield self.unpack(self.geobase.parent_id(213))
        self.assertNotEqual(res, 2)

        res = yield self.unpack(self.geobase.parent_id(114619))
        self.assertEqual(res, 213)

    @gen_test(timeout=2)
    def test_parents(self):
        res = yield self.unpack(self.geobase.parents(213))
        self.assertEqual(tuple(res),
                         (213, 1, 3, 225, 10001, 10000))

        res = yield self.unpack(self.geobase.parents(213))
        self.assertNotEqual(tuple(res), (214, ))

    @gen_test(timeout=2)
    def test_find_country(self):
        res = yield self.unpack(self.geobase.find_country(213))
        self.assertEqual(res, 225)

    @gen_test(timeout=2)
    def test_parents_translocality(self):
        res = yield self.unpack(self.geobase.parents(146, 'ru'))
        self.assertEqual(tuple(res),
                         (146, 121220, 977, 115092, 225, 10001, 10000))

        res = yield self.unpack(self.geobase.parents(146, 'ua'))
        self.assertEqual(tuple(res),
                         (146, 121220, 977, 187, 166, 10001, 10000))

    @gen_test(timeout=2)
    def test_region_id_by_location(self):
        res = yield self.unpack(self.geobase.region_id_by_location(55.733684, 37.588496))
        self.assertEqual(res, 213)

        res = yield self.unpack(self.geobase.region_id_by_location(51.658562, 39.206203))
        self.assertEqual(res, 193)

    @gen_test(timeout=2)
    def test_subtree(self):
        res = yield self.unpack(self.geobase.subtree(193))
        self.assertEqual(tuple(res),
                         (193, 108206, 108207, 108208, 121720, 108209, 108210, 108211))

    @gen_test(timeout=2)
    def test_supported_linguistics(self):
        res = yield self.unpack(self.geobase.supported_linguistics())
        self.assertEqual(tuple(res),
                         ('be', 'en', 'kk', 'ru', 'tr', 'tt', 'uk'))

    @gen_test(timeout=2)
    def test_timezone_name(self):
        res = yield self.unpack(self.geobase.timezone_name(213))
        self.assertEqual(res, 'Europe/Moscow')

    @gen_test(timeout=2)
    def test_timezone(self):
        tz_name = 'Europe/Moscow'
        name, abbr, dst, offset = yield self.unpack(self.geobase.timezone(tz_name))
        eq = geobase5.get_timezone(tz_name)
        self.assertEqual(name, eq.name)
        self.assertEqual(abbr, eq.abbr)
        self.assertEqual(dst, eq.dst)
        self.assertEqual(offset, eq.offset)

    @gen_test(timeout=2)
    def test_timezone_for_time(self):
        name, abbr, dst, offset = yield self.unpack(self.geobase.timezone_for_time('Europe/Moscow', 1366360257))
        self.assertEqual(name, 'Europe/Moscow')
        self.assertEqual(abbr, 'MSK')
        self.assertEqual(dst, 'std')
        self.assertEqual(offset, 14400)

    @gen_test(timeout=2)
    def test_linguistics_for_region(self):
        expected = geo.linguistics(9999, 'en')
        a = yield self.unpack(self.geobase.linguistics_for_region(9999, 'en'))
        self.assertEqual(tuple(a[1:3]), (expected.nominative, expected.genitive))

    @gen_test(timeout=2)
    def test_get_services(self):
        res = yield self.unpack(self.geobase.get_services(213))
        self.assertEqual(tuple(res),
                         ('bs', 'yaca', 'weather', 'afisha', 'maps', 'tv', 'ad', 'etrain', 'subway', 'delivery', 'route'))

    @gen_test(timeout=2)
    def test_have_services(self):
        res = yield self.unpack(self.geobase.have_service(213, 'weather'))
        self.assertTrue(res)

        res = yield self.unpack(self.geobase.have_service(193, 'subway'))
        self.assertFalse(res)
        try:
            yield self.unpack(self.geobase.have_service(213, "not_existent_service"))
            self.assertFalse(True)
        except Exception:
            pass

    @gen_test(timeout=2)
    def test_set_services(self):
        # nothing to do here
        self.assertEqual(True, True)

    @gen_test(timeout=2)
    def test_parent(self):
        res = yield self.unpack(self.geobase.parent(20358))
        self.assertEqual(tuple(res), (213, 0))

    @gen_test(timeout=2)
    def test_coordinates(self):
        coordinates = yield self.unpack(self.geobase.coordinates(20358))
        coordinates = map(lambda x: int(x * 1000), coordinates)
        self.assertEqual(coordinates, [55849, 37789, 37, 87, 14000])

    @gen_test(timeout=2)
    def test_names(self):
        names = yield self.unpack(self.geobase.names(213))
        self.assertEqual(tuple(names), ('MSK', '', 'Moskau, Moskva'))

    @gen_test(timeout=2)
    def test_codes(self):
        codes = yield self.unpack(self.geobase.codes(213))
        self.assertEqual(tuple(codes), ('495 499', ''))

    @gen_test(timeout=2)
    def test_data(self):
        codes = yield self.unpack(self.geobase.region_data(213))
        self.assertEqual(tuple(codes), (6, 1000000000, 2047, True))

    @gen_test(timeout=2)
    def test_pinpoint_geolocation(self):
        location = yield self.unpack(self.geobase.pinpoint_geolocation({
            'yandex_gid': '0',
            'ip': '81.19.70.3',
            'yp_cookie': "",
            'ys_cookie': "",
            'allow_yandex': '1',
            'x_forwarded_for': '195.98.64.75',
        }))

        eq = geo.pinpoint_geolocation(0, "81.19.70.3", "", "", 1, "195.98.64.75")
        self.assertEqual(location[0], eq.region_id)
        self.assertEqual(location[1], eq.region_id_by_ip)
        self.assertEqual(location[2], 2)
        self.assertEqual(location[3], eq.suspected_region_id)
        self.assertEqual(location[4], eq.precision)
        self.assertEqual(location[5], eq.point_id)
        self.assertEqual(location[6], eq.should_update_cookie)
        self.assertEqual(location[7], False)

    @gen_test(timeout=2)
    def test_GetIdByNameSingleMatch(self):
        res = yield self.unpack(self.geobase.region_ids_by_name('Москва', 1))
        self.assertEqual((213, ), tuple(res))

    @gen_test(timeout=2)
    def test_GetIdByNameMultipleMatch(self):
        res = yield self.unpack(self.geobase.region_ids_by_name('Москва', 100))
        self.assertEqual((1, 213), tuple(res))

    @gen_test(timeout=2)
    def test_GetIdByNameMultipleMatchLessThanMax(self):
        expected = (2, 10174, 10517, 20782, 104412, 105196, 105197, 105219, 111168, 111847, 115189, 115322, 115395, 115668, 120759)
        res = yield self.unpack(self.geobase.region_ids_by_name('Санкт', 100))
        self.assertEqual(expected, tuple(res))

    @gen_test(timeout=2)
    def test_GetIdByNameMultipleMatchTruncated(self):
        expected = (20782, 111168, 115189, 115322, 115395)
        res = yield self.unpack(self.geobase.region_ids_by_name('Санкт', 5))
        self.assertEqual(expected, tuple(res))

    @gen_test(timeout=2)
    def test_GetIdByNameEmptyResult(self):
        res = yield self.unpack(self.geobase.region_ids_by_name('blah-blah-blah', 100))
        self.assertEqual(tuple(), tuple(res))

    @gen_test(timeout=2)
    def test_asset(self):
        res = yield self.unpack(self.geobase.asset('37.9.101.102'))
        self.assertEqual("AS13238", res)

    #@unittest.skip("Not implemneted in v0.12 yet")
    @gen_test(timeout=2)
    def test_path(self):
        ip = '37.9.101.102'
        res = yield self.unpack(self.geobase.path(ip))
        expected = geo.regions(ip)
        self.assertEqual(len(res), len(expected))
        for i, j in zip(res, expected):
            (capital_id,
             en_name,
             _id,
             is_main,
             iso_name,
             latitude,
             latitude_size,
             linguistics_list,
             longitude,
             longitude_size,
             name,
             native_name,
             official_languages,
             parent_id,
             phone_code,
             phone_code_old,
             population,
             position,
             services,
             short_en_name,
             synonyms,
             _type,
             tzname,
             widespread_languages,
             zip_code,
             zoom) = i
            self.assertEqual(parent_id, j.parent)
            self.assertEqual(_id, j.id)
            self.assertEqual(is_main, j.main)
            self.assertEqual(name, j.name)
            self.assertEqual(zoom, j.zoom)
            self.assertEqual(en_name, j.ename)
            self.assertEqual(short_en_name, j.short_ename)
            self.assertEqual(iso_name, j.iso_name)
            self.assertEqual(_type, j.type)
            self.assertEqual(tzname, j.timezone)
            self.assertEqual(position, j.pos)
            self.assertEqual(phone_code, j.phone_code)
            self.assertEqual(zip_code, j.zip_code)
            self.assertEqual(latitude, j.lat)
            self.assertEqual(longitude, j.lon)
            self.assertEqual(latitude_size, j.spn_lat)
            self.assertEqual(longitude_size, j.spn_lon)


if __name__ == '__main__':
    unittest.main()


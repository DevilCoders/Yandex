import datetime

import adminka.control_pool
import yaqutils.time_helpers as utime


def _test_date(day):
    assert 1 <= day <= 31
    return datetime.date(2017, 1, day)


def _test_range(day1, day2):
    return utime.DateRange(_test_date(day1), _test_date(day2))


# noinspection PyClassHasNoInit
class TestControlPool(object):
    def test_split_dates_1(self):
        dates = _test_range(1, 31)
        expected = [utime.DateRange(d, d) for d in dates]
        actual = list(adminka.control_pool.split_dates_by_max(dates, max_range=1))
        assert expected == actual

    def test_split_dates_14(self):
        dates = _test_range(1, 31)
        expected = [
            _test_range(1, 14),
            _test_range(15, 28),
            _test_range(29, 31),
        ]
        actual = list(adminka.control_pool.split_dates_by_max(dates, max_range=14))
        assert expected == actual

    def test_split_dates_40(self):
        dates = _test_range(1, 31)
        expected = [dates]
        actual = list(adminka.control_pool.split_dates_by_max(dates, max_range=40))
        assert expected == actual

    def test_generate_ranges(self):
        dates = _test_range(1, 31)
        salt = {
            _test_date(10),
            _test_date(11),
            _test_date(20),
            _test_date(31),
        }
        expected = [
            _test_range(1, 9),
            _test_range(12, 19),
            _test_range(21, 30),
        ]
        actual = list(adminka.control_pool.generate_salt_change_ranges(dates,
                                                                       salt,
                                                                       salt_change_dates_manual=set(),
                                                                       use_split_change_day=False))
        assert expected == actual

    def test_generate_ranges_use_split_change_day(self):
        dates = _test_range(1, 31)
        salt = {
            _test_date(10),
            _test_date(11),
            _test_date(20),
            _test_date(31),
        }
        salt_manual = {
            _test_date(20),
        }
        expected = [
            _test_range(1, 9),
            _test_range(10, 10),
            _test_range(11, 19),
            _test_range(21, 30),
            _test_range(31, 31),
        ]
        actual = list(adminka.control_pool.generate_salt_change_ranges(dates,
                                                                       salt,
                                                                       salt_change_dates_manual=salt_manual,
                                                                       use_split_change_day=True))
        assert expected == actual

    def test_generate_ranges_for_control_pool_force_single(self):
        dates = _test_range(1, 31)
        salt = {
            _test_date(10),
            _test_date(11),
            _test_date(12),
            _test_date(13),
            _test_date(19),
            _test_date(20),
            _test_date(31),
        }
        salt_manual = {
            _test_date(19),
            _test_date(20),
        }
        expected = [
            _test_range(1, 1),
            _test_range(1, 3),
            _test_range(2, 2),
            _test_range(3, 3),
            _test_range(4, 4),
            _test_range(4, 6),
            _test_range(5, 5),
            _test_range(6, 6),
            _test_range(7, 7),
            _test_range(7, 9),
            _test_range(8, 8),
            _test_range(9, 9),
            _test_range(10, 10),
            _test_range(11, 11),
            _test_range(12, 12),
            _test_range(13, 13),
            _test_range(13, 15),
            _test_range(14, 14),
            _test_range(15, 15),
            _test_range(16, 16),
            _test_range(16, 18),
            _test_range(17, 17),
            _test_range(18, 18),
            _test_range(21, 21),
            _test_range(21, 23),
            _test_range(22, 22),
            _test_range(23, 23),
            _test_range(24, 24),
            _test_range(24, 26),
            _test_range(25, 25),
            _test_range(26, 26),
            _test_range(27, 27),
            _test_range(27, 29),
            _test_range(28, 28),
            _test_range(29, 29),
            _test_range(30, 30),
            _test_range(31, 31),
        ]
        actual = adminka.control_pool.generate_ranges_for_control_pool(
            dates=dates,
            max_range=3,
            salt_change_dates=salt,
            salt_change_dates_manual=salt_manual,
            use_split_change_day=True,
            force_single_days=True,
        )
        assert expected == actual

    def test_generate_ranges_for_control_pool(self):
        dates = _test_range(1, 31)
        salt = {
            _test_date(10),
            _test_date(11),
            _test_date(12),
            _test_date(13),
            _test_date(19),
            _test_date(20),
            _test_date(31),
        }
        salt_manual = {
            _test_date(19),
            _test_date(20),
        }
        expected = [
            _test_range(1, 3),
            _test_range(4, 6),
            _test_range(7, 9),
            _test_range(10, 10),
            _test_range(11, 11),
            _test_range(12, 12),
            _test_range(13, 15),
            _test_range(16, 18),
            _test_range(21, 23),
            _test_range(24, 26),
            _test_range(27, 29),
            _test_range(30, 30),
            _test_range(31, 31),
        ]
        actual = adminka.control_pool.generate_ranges_for_control_pool(
            dates=dates,
            max_range=3,
            salt_change_dates=salt,
            salt_change_dates_manual=salt_manual,
            use_split_change_day=True,
            force_single_days=False,
        )
        assert expected == actual

# coding=utf-8
import datetime

import pytest

import adminka.activity as adm_act
import yaqutils.time_helpers as utime

OFF_EVENT = {
    "testid": 12345,
    "tag": "online",
    "status": "off",
    "time": "2015-09-16T10:19:28Z"
}
ON_EVENT = {
    "testid": 12345,
    "tag": "online",
    "status": "on",
    "time": "2015-08-31T15:19:07Z"
}
WEIRD_EVENT = {
    "testid": 12345,
    "tag": "not production",
    "status": "off",
    "time": "2015-09-16T10:19:28Z"
}

EVENT_DATES = utime.DateRange(
    datetime.date(2015, 8, 31),
    datetime.date(2015, 9, 16)
)


# noinspection PyClassHasNoInit
class TestEnabledPairs:
    def test_basic(self):
        #                                         v-- activity пишется в обратном хронологическом порядке
        actual = list(adm_act.enabled_event_pairs([OFF_EVENT, ON_EVENT]))
        expected = [adm_act.ActivityPair(ON_EVENT, OFF_EVENT)]
        assert actual == expected

    def test_weird(self):
        with pytest.raises(AssertionError):
            list(adm_act.enabled_event_pairs([
                WEIRD_EVENT,
                OFF_EVENT,
                WEIRD_EVENT,
                ON_EVENT,
                WEIRD_EVENT,
            ]))

    def test_enabled_after_end(self):
        actual = list(adm_act.enabled_event_pairs([ON_EVENT]))
        expected = [adm_act.ActivityPair(ON_EVENT)]
        assert actual == expected

    def test_closest(self):
        actual = adm_act.closest_enabled_event_pair([OFF_EVENT, ON_EVENT], EVENT_DATES)
        expected = adm_act.ActivityPair(ON_EVENT, OFF_EVENT)
        assert actual == expected

    def test_closest_none(self):
        dates = utime.DateRange(datetime.date(1999, 1, 1), datetime.date(2000, 1, 1))
        pair = adm_act.closest_enabled_event_pair([OFF_EVENT, ON_EVENT], dates)
        assert pair is None


# noinspection PyClassHasNoInit
class TestDateRangeFromEventPair:
    def test_basic(self):
        dates = adm_act.date_range_from_event_pair(adm_act.ActivityPair(ON_EVENT, OFF_EVENT))
        assert dates == EVENT_DATES

    def test_endless(self):
        actual = adm_act.date_range_from_event_pair(adm_act.ActivityPair(ON_EVENT))
        expected = utime.DateRange(EVENT_DATES.start, None)
        assert actual == expected

    def test_startless(self):
        with pytest.raises(Exception):
            # noinspection PyTypeChecker
            adm_act.date_range_from_event_pair(adm_act.ActivityPair(None, OFF_EVENT))

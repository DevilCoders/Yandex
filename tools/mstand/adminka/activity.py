# coding=utf-8
import datetime
import logging

import mstand_utils.testid_helpers as utestid
import yaqutils.time_helpers as utime
from yaqab.ab_client import AbClient  # noqa


class ActivityPair(object):
    def __init__(self, on, off=None, middle=None):
        """
        :type on: dict
        :type off: dict | None
        :type middle: list[dict] | None
        """
        assert on is not None
        self.on = on
        self.off = off
        if middle is None:
            middle = []
        self.middle = middle
        """:type: list[dict]"""
        for event in self.middle:
            assert event is not None

    def add_middle(self, on):
        self.middle.append(on)

    def set_off(self, off):
        assert self.off is None
        self.off = off

    def __repr__(self):
        if self.middle:
            return "ActivityPair({!r}, {!r}, {!r})".format(self.on, self.off, self.middle)
        else:
            return "ActivityPair({!r}, {!r})".format(self.on, self.off)

    def __str__(self):
        return self.__repr__()

    def __eq__(self, other):
        """
        :type other: ActivityPair
        """
        if not isinstance(other, ActivityPair):
            return False
        return self.on == other.on and self.off == other.off and self.middle == other.middle

    def __iter__(self):
        current = self.on
        for middle in self.middle:
            yield current, middle
            current = middle
        yield current, self.off


def get_testid_activity(client, testid):
    """
    :type client: AbClient
    :type testid: str
    :rtype: list[dict[str]]
    """
    if utestid.testid_from_adminka(testid):
        return client.get_testid_activity(testid)

    return [
        {
            "testid": testid,
            "tag": "online",
            "status": "on",
            "time": "2000-01-01T00:00:00Z",
            "footprints": [],
        },
    ]


def enabled_event_pairs(activity):
    """
    :type activity: list[dict]
    :rtype: __generator[ActivityPair]
    """
    logging.debug("enabled_event_pairs activity: %s", activity)
    current_pair = None
    for event in reversed(activity):
        assert event["tag"] == "online"

        if event["status"] == "on":
            if current_pair is None:
                current_pair = ActivityPair(event)
            else:
                current_pair.add_middle(event)
        elif event["status"] == "off":
            if current_pair is not None:  # ignore off without on
                current_pair.set_off(event)
                yield current_pair
                current_pair = None

    if current_pair is not None:
        yield current_pair


def date_from_event(event):
    """
    :type event: dict
    :rtype: datetime.date
    """
    if event is None:
        return None
    return datetime.datetime.strptime(event['time'], "%Y-%m-%dT%H:%M:%SZ").date()


def date_range_from_event_pair(pair):
    """
    :type pair: ActivityPair
    :rtype: utime.DateRange
    """
    date_on = date_from_event(pair.on)
    date_off = date_from_event(pair.off)
    if date_on is None:
        raise Exception("Enable event can't be None")
    return utime.DateRange(date_on, date_off)


def closest_enabled_event_pair(activity, dates):
    """
    :type activity: list[dict]
    :type dates: utime.DateRange
    :rtype: ActivityPair | None
    """
    candidates = {}
    for pair in enabled_event_pairs(activity):
        range_enabled = date_range_from_event_pair(pair)
        if range_enabled.intersect(dates):
            candidates[range_enabled] = pair

    if candidates:
        latest_candidate = max(candidates.keys(), key=lambda dr: dr.number_of_days())
        return candidates[latest_candidate]
    return None

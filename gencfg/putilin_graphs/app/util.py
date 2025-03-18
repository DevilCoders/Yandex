import simplejson as json
import calendar
from datetime import date
import urllib
import logging
import time

from flask import request


def get_in_statement_list(strs):
    return "({})".format(", ".join(["'{}'".format(s) for s in strs]))


def get_historical_graph_params():
    if request.args.get('params'):
        try:
            params = json.loads(request.args.get('params'))
        except:
            params = json.loads(urllib.unquote(request.args.get('params')))

    else:
        params = {}

    start_ts = int(request.args.get('start_ts') or params['start_ts'] or 0)
    end_ts = int(request.args.get('end_ts') or params.get('end_ts') or int(time.time()))

    zoom_level = request.args.get('zoom_level', params.get('zoom_level', 'auto'))
    if zoom_level == "auto":
        zoom_level = calculate_zoom_level(start_ts, end_ts)

    return [start_ts, end_ts, zoom_level, params.get('graphs')]


class DateEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, date):
            return calendar.timegm(obj.timetuple()) * 1000

        # Let the base class default method raise the TypeError
        return json.JSONEncoder.default(self, obj)


_supported_zoom_levels = [
    ("2m", 2 * 60),
    ("15m", 15 * 60),
    ("1h", 60 * 60),
    ("1d", 60 * 60 * 24)
]


def calculate_zoom_level(start_ts, end_ts, max_points=1000):
    for level, period in _supported_zoom_levels:
        if (end_ts - start_ts) / period < max_points:
            return level

    return level


def zoom_level_to_period(zoom_level):
    return dict(_supported_zoom_levels)[zoom_level]


def zoom_level_to_period_start_adjustment(zoom_level):
    """
    Returns "offset hack" for zoom_level.

    It is needed to make JS graphs work properly. Using highcharts we deal with timestamps(instead of dates).
    We need to show data as points. Each period is identified as two timestamps


    start_ts      --- end
      |         /
      |        |
      V        v
    ----------------->
      ^        ^
      |        |
    e.g. 14:00 ----- e.g. 15:00

    Let's say we have a precomputed data point for 14:00-15:00 time period(i.e. avg usage for this hour).
    We have to show it as a single point on a chart.
    The simplest way is to simply pick start_ts and use it as a point ts(i.e. NOT labeling the point as 14:00-15:00).
    Properly labeling points seems possible, but tricky/ugly, and this approach works reasonable well UI-wise.

    start_ts has the following properly: start_ts % period_length == 0
    (I.e. timestamp corresponding to 14:00:00 is divisible by 3600,
    timestamp corresponding to 15:42:00 is divisible by 120)

    So it is fairly easy to work with(especially given the fact that unix time doesn't really have the concept of leap seconds:
    http://stackoverflow.com/questions/16539436/unix-time-and-leap-seconds

    This works well for 2 minute, 15 minute, 1 hour long timestamps. (Also probably would work for other, but at this moment
                                                                      we have these three)

    HOWEVER, for large time periods(days+) it doesn't relly work, because we are in Moscow Timezone(UTC+3).
    The start timestamp of the day in UTC is divisible by 24*60*60. But the start of of the day in Moscow time isn't.
    So, to find the start timestamp of the day, we need to add 3 hours.
    """
    return 3 * 60 * 60 if zoom_level == '1d' else 0


class Timer:
    def __init__(self, name=None):
        self.__start = time.time()
        self.__last = None
        self.__name = name

    def time(self):
        return time.time() - self.__start

    def time_diff(self):
        now = time.time()
        res = now - (self.__last if self.__last else self.__start)
        self.__last = now
        return res

    def __str__(self, message=None):
        res = []

        if self.__name:
            res.append(self.__name)
        if message:
            time = self.time_diff()
            res.append(message)
        else:
            time = self.time()

        res.append(str(time))

        return " ".join(res)

    def write(self, message=None):
        logging.info("%s\n" % self.__str__(message=message))

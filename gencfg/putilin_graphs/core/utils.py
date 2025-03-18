import datetime
import pytz


MINUTE = 60
HOUR = MINUTE * 60
DAY = HOUR * 24
WEEK = DAY * 7


def get_ch_formatted_date_from_timestamp(ts):
    return datetime.datetime.fromtimestamp(ts, tz=pytz.timezone("Europe/Moscow")).strftime('%Y-%m-%d')


def floor_timestamp_to_period(ts, period):
    return ts - (ts % period)

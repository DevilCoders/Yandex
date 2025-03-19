from typing import Union
from datetime import datetime, timezone, timedelta
from time import mktime
from pandas._libs.tslibs.timestamps import Timestamp

RUS_DATE_FORMAT = "%d.%m.%Y"
DATETIME_FORMAT = "%Y-%m-%d %H:%M:%S"
DATE_FORMAT = "%Y-%m-%d"


def parse_date(x: str) -> datetime:
    return datetime.strptime(x, DATE_FORMAT)


def parse_date_time(x: str) -> datetime:
    return datetime.strptime(x, DATETIME_FORMAT)


def curr_utc_date_iso(days_lag: int = 0) -> str:
    curr_utc_date = datetime.utcnow().date()
    if days_lag > 0:
        curr_utc_date -= timedelta(days=days_lag)
    return curr_utc_date.isoformat()


def rus_format_date(date: datetime) -> str:
    return date.strftime(RUS_DATE_FORMAT)


def datetime2utcms(t: datetime) -> int:
    """
    Convert time to utc milliseconds
    :param t: time object
    :return: utc milliseconds
    """
    return int(mktime(t.timetuple()) * 1e3 + t.microsecond / 1e3)


def utcms2datetime(ms: float) -> datetime:
    return datetime.utcfromtimestamp(ms / 1000.0)


def utcms2tzdatetime(ms: float) -> datetime:
    return datetime.fromtimestamp(ms / 1000.0)


def time_to_str(dt: datetime) -> str:
    return datetime.strftime(dt, DATETIME_FORMAT)


def to_utc(dt: Union[datetime, Timestamp]) -> str:
    if isinstance(dt, Timestamp):
        dt = dt.to_pydatetime()
    return dt.astimezone(timezone.utc).replace(tzinfo=timezone.utc).isoformat()


def time_to_unix(time_str: str) -> int:
    dt = parse_date_time(time_str)
    timestamp = dt.replace(tzinfo=timezone.utc).timestamp()
    return int(timestamp)

__all__ = ['parse_date', 'parse_date_time', 'curr_utc_date_iso', 'rus_format_date', 'datetime2utcms',
           'utcms2datetime', 'utcms2tzdatetime', 'time_to_str', 'to_utc', 'time_to_unix']

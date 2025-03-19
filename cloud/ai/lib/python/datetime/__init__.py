from datetime import datetime, timezone, timedelta
from isodate import parse_duration, duration_isoformat

import typing

"""
Purpose of this module is to unify datetime format.
"""


def parse_datetime(s: typing.Union[typing.Optional[str], typing.Any]) -> typing.Optional[datetime]:
    if s is None:
        return None
    if not isinstance(s, str):
        # more likely it is YSON "null object"
        return None
    if s.endswith('Z'):
        # Proxy logs are formatted like '2019-12-12T14:28:56.759Z' using UTC tz
        return datetime.fromisoformat(s[:-1]).replace(tzinfo=timezone.utc)
    dt = datetime.fromisoformat(s)
    if dt.tzinfo is None:
        raise ValueError('Use tz aware datetime')
    return dt.astimezone(timezone.utc)


def parse_nirvana_datetime(s: typing.Optional[str]) -> typing.Optional[datetime]:
    """
    Parse string content of Nirvana block parameter with type Date.
    There is two ways to fill this parameter:

    1) With UI using calendar widget.
    2) With Hitman using Groovy functions.
    3) With Reactor using time format "yyyy-MM-dd'T'HH:mm:ssZ"

    Date string format in this cases are:

    1) 2020-06-18T14:25:17+0300 (missing a colon in the time zone to parse as ISO format)
    2) 2020-06-18T11:25:17.776774Z
    """
    if s is None or s == '':
        return None
    if s.endswith('Z'):
        return parse_datetime(s)
    return datetime.strptime(s, '%Y-%m-%dT%H:%M:%S%z').astimezone(timezone.utc)


def parse_timedelta(s: typing.Optional[str]) -> typing.Optional[timedelta]:
    if not s:
        return None
    result = parse_duration(s)

    assert isinstance(result, timedelta)
    return result


def format_datetime(dt: typing.Optional[datetime]) -> typing.Optional[str]:
    if dt is None:
        return None
    if dt.tzinfo is None:
        raise ValueError('Use tz aware datetime')
    return dt.astimezone(timezone.utc).isoformat()


def format_nirvana_datetime(dt: datetime) -> str:
    return dt.strftime('%Y-%m-%dT%H:%M:%S')


def format_datetime_for_s3_subpath(dt: datetime) -> str:
    return f'{dt.year:04d}/{dt.month:02d}/{dt.day:02d}'


def format_timedelta(td: typing.Optional[timedelta]) -> typing.Optional[str]:
    if td is None:
        return None
    assert td.microseconds == 0
    return duration_isoformat(td)


def now() -> datetime:
    return datetime.utcnow().replace(tzinfo=timezone.utc)

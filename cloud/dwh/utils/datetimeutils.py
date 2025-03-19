import datetime
from typing import Generator
from typing import Tuple

import pytz

SECOND = 1
MINUTE = 60 * SECOND
HOUR = 60 * MINUTE
DAY = HOUR * 24

ONE_SECOND_DELTA = datetime.timedelta(seconds=1)

MSK_TIMEZONE = 'Europe/Moscow'
UTC_TIMEZONE = 'UTC'

MSK_TIMEZONE_OBJ = pytz.timezone(MSK_TIMEZONE)
UTC_TIMEZONE_OBJ = pytz.UTC


def get_now(tz: datetime.tzinfo = pytz.UTC) -> datetime.datetime:
    return datetime.datetime.now(tz=tz)


def get_now_utc() -> datetime.datetime:
    return get_now(tz=UTC_TIMEZONE_OBJ)


def get_now_msk() -> datetime.datetime:
    return get_now(tz=MSK_TIMEZONE_OBJ)


def get_now_utc_timestamp() -> int:
    return int(get_now_utc().timestamp())


def dttm_paginator(from_dttm: datetime.datetime,
                   to_dttm: datetime.datetime,
                   page_size: int) -> Generator[Tuple[datetime.datetime, datetime.datetime], None, None]:
    while from_dttm <= to_dttm:
        current_to_dttm = from_dttm + datetime.timedelta(seconds=page_size)
        yield from_dttm, min(current_to_dttm, to_dttm)
        from_dttm = current_to_dttm + ONE_SECOND_DELTA


def parse_isoformat_to_tz_dttm(iso_string: str, tz: pytz.tzinfo.DstTzInfo) -> datetime.datetime:
    return tz.localize(datetime.datetime.fromisoformat(iso_string))


def parse_isoformat_to_msk_dttm(iso_string: str) -> datetime.datetime:
    return parse_isoformat_to_tz_dttm(iso_string, tz=MSK_TIMEZONE_OBJ)

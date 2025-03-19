import datetime
from types import MappingProxyType

from dateutil import relativedelta

from cloud.dwh.nirvana.operations.solomon_to_yt.lib.exceptions import InvalidYtSplitInterval


class YtSplitIntervals:
    class Types:
        MONTHLY = 'MONTHLY'
        DAILY = 'DAILY'
        HOURLY = 'HOURLY'

        ALL = {MONTHLY, DAILY, HOURLY}

    _SHORT_NAME_BY_TYPE = MappingProxyType({
        Types.MONTHLY: '1mo',
        Types.DAILY: '1d',
        Types.HOURLY: '1h',
    })

    _CEILING_BY_TYPE = MappingProxyType({
        Types.MONTHLY: relativedelta.relativedelta(day=31, hour=23, minute=59, second=59),
        Types.DAILY: relativedelta.relativedelta(hour=23, minute=59, second=59),
        Types.HOURLY: relativedelta.relativedelta(minute=59, second=59),
    })

    _FORMAT_BY_TYPE = MappingProxyType({
        Types.MONTHLY: '%Y-%m-01',
        Types.DAILY: '%Y-%m-%d',
        Types.HOURLY: '%Y-%m-%dT%H:00:00',
    })

    @classmethod
    def check_interval(cls, interval: str):
        if interval not in cls.Types.ALL:
            raise InvalidYtSplitInterval(f'Interval {interval} is not exits. Expected one of {cls.Types.ALL}')

    @classmethod
    def get_interval_short_name(cls, interval: str):
        return cls._SHORT_NAME_BY_TYPE[interval]

    @classmethod
    def ceil_to_interval(cls, dttm: datetime.datetime, interval: str) -> datetime.datetime:
        ceiling = cls._CEILING_BY_TYPE[interval]

        return dttm + ceiling

    @classmethod
    def format_by_interval(cls, dttm: datetime.datetime, interval: str) -> str:
        fmt = cls._FORMAT_BY_TYPE[interval]

        return dttm.strftime(fmt)

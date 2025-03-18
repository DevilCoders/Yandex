# coding=utf-8

from copy import deepcopy
from datetime import datetime, timedelta

from antiadblock.cryprox.cryprox.config.system import EXPERIMENT_START_TIME_FMT, Experiments


def hash_murmur3_32(key, seed=0):
    """
    Implements 32bit murmur3 hash.
    >>> hash_murmur3_32('1000088891471457014'[:9])
    3434522062
    >>> hash_murmur3_32('9996493261521650359'[:9], seed=123456)
    2855925469
    >>> hash_murmur3_32('9996493261521650359'[:9], seed=datetime.strptime('1970-01-01T00:00:00', '%Y-%m-%dT%H:%M:%S').toordinal())
    2951998901
    """
    key = bytearray(key)
    bit32 = 2 ** 32 - 1
    c1 = 0xcc9e2d51
    c2 = 0x1b873593
    length = len(key)
    nblocks = int(length / 4)
    h = seed

    for block_start in range(0, nblocks * 4, 4):
        k = key[block_start + 3] << 24 | key[block_start + 2] << 16 | key[block_start + 1] << 8 | key[block_start]
        k = (c1 * k) & bit32
        k = (k << 15 | k >> 17) & bit32
        k = (c2 * k) & bit32

        h ^= k
        h = (h << 13 | h >> 19) & bit32
        h = (h * 5 + 0xe6546b64) & bit32

    tail_index = nblocks * 4
    tail_size = length & 3
    k = 0
    if tail_size >= 3:
        k ^= key[tail_index + 2] << 16
    if tail_size >= 2:
        k ^= key[tail_index + 1] << 8
    if tail_size >= 1:
        k ^= key[tail_index]
    if tail_size > 0:
        k = (k * c1) & bit32
        k = (k << 15 | k >> 17) & bit32
        k = (k * c2) & bit32
        h ^= k

    h = h ^ length
    h ^= h >> 16
    h = (h * 0x85ebca6b) & bit32
    h ^= h >> 13
    h = (h * 0xc2b2ae35) & bit32
    h ^= h >> 16
    return int(h)


def uid_is_experimental(uid, seed, percent, metrics=None):
    """
    >>> uid_is_experimental('not actually a uid', seed=12345, percent=50)
    False
    >>> uid_is_experimental('91299312', seed=0, percent=5)
    True
    >>> uid_is_experimental('7009000171558371332', seed=datetime.strptime('1970-01-01T00:00:00', '%Y-%m-%dT%H:%M:%S').toordinal(), percent=10)
    False
    """
    is_experimental = hash_murmur3_32(uid[:9], seed) % 100 < percent  # using first 9 digits of yandexuid, cause last 10 is utc timestamp
    if metrics is not None:
        metrics.increase_counter('experimental_bypass', experimental=int(is_experimental))
    return is_experimental


def uid_is_control(uid, seed, percent):
    """
    For yql request to calculate experiment stats
    """
    return percent <= hash_murmur3_32(uid[:9], seed) % 100 < 2 * percent


def periods_since_date(experiment_start, period, date=None):
    """
    >>> periods_since_date(datetime(2019, 7, 10, 10, 0, 0), period=7, date=datetime(2019, 7, 11, 10, 0, 0))
    0
    >>> periods_since_date(datetime(2019, 7, 10, 10, 0, 0), period=7, date=datetime(2019, 7, 17, 10, 0, 0))
    1
    >>> periods_since_date(datetime(2019, 7, 10), period=7, date=datetime(2019, 7, 24))
    2
    >>> periods_since_date(datetime(2019, 7, 10), period=14, date=datetime(2019, 7, 25, 10, 0, 0))
    1
    """
    date = date or datetime.utcnow()
    return (date - experiment_start).days / period


def is_experiment_date(date_range, period=0, date=None):
    """
    >>> date_range=(datetime(2019, 7, 10, 10, 0, 0), datetime(2019, 7, 10, 15, 0, 0))
    >>> is_experiment_date(date_range, date=datetime(2019, 7, 10, 15, 0, 0))
    True
    >>> is_experiment_date(date_range, date=datetime(2019, 7, 10, 15, 1, 0))
    False
    >>> is_experiment_date(date_range, date=datetime(2019, 7, 12, 14, 0, 0))
    False
    >>> is_experiment_date(date_range, date=datetime(2019, 7, 12, 14, 0, 0), period=2)
    True
    >>> is_experiment_date(date_range, date=datetime(2019, 7, 12, 14, 0, 0), period=7)
    False
    >>> is_experiment_date(date_range, date=datetime(2019, 7, 17, 14, 0, 0), period=7)
    True
    >>> is_experiment_date(date_range, date=datetime(2019, 7, 17, 15, 1, 0), period=7)
    False
    >>> is_experiment_date(date_range, date=datetime(2019, 7, 17, 9, 59, 0), period=7)
    False
    >>> is_experiment_date(date_range, date=datetime(2019, 7, 24, 10, 00, 0), period=7)
    True
    """
    date = date or datetime.utcnow()
    if period != 0:
        return date_range[0] <= date - timedelta(days=periods_since_date(date_range[0], period, date) * period) <= date_range[1]
    else:
        return date_range[0] <= date <= date_range[1]


def get_active_experiment(experiments):
    now = datetime.utcnow()
    weekday = now.weekday()
    curr_year = now.year
    curr_month = now.month
    curr_day = now.day
    for exp in experiments:
        if exp["EXPERIMENT_TYPE"] == Experiments.NONE or exp["EXPERIMENT_START"] is None:
            continue
        start = datetime.strptime(exp["EXPERIMENT_START"], EXPERIMENT_START_TIME_FMT)
        experiment_days = exp.get("EXPERIMENT_DAYS", [])
        if not experiment_days:
            # разовый эксперимент без повторов
            end = start + timedelta(hours=exp["EXPERIMENT_DURATION"])
            if start <= now <= end:
                _exp = deepcopy(exp)
                _exp["EXPERIMENT_START"] = start
                return _exp
        else:
            # текущий день недели есть в эксперименте
            if weekday in experiment_days:
                _start = start.replace(year=curr_year, month=curr_month, day=curr_day)
                end = _start + timedelta(hours=exp["EXPERIMENT_DURATION"])
                if _start <= now <= end:
                    _exp = deepcopy(exp)
                    _exp["EXPERIMENT_START"] = _start
                    return _exp

    return {}

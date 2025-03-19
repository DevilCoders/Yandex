"""Test throttler"""

import time

from yc_common.throttler import Throttler


def test_throttler():
    limit = 7
    period = 0.1
    rejects = 3

    throttler = Throttler(limit=limit, period=period)

    for i in range(limit):
        assert throttler.acquire("one") == (True, 0)
        assert throttler.acquire("two") == (True, 0)

    for i in range(1, rejects + 1):
        assert throttler.acquire("one") == (False, i)
        assert throttler.acquire("two") == (False, i)

    time.sleep(period)

    for i in range(limit):
        assert throttler.acquire("one") == (True, 0 if i else rejects)
        assert throttler.acquire("two") == (True, 0 if i else rejects)

    assert throttler.acquire("one") == (False, 1)
    assert throttler.acquire("two") == (False, 1)
    assert throttler.acquire("two") == (False, 2)

    assert sorted(throttler.iter_rejects()) == [("one", 1), ("two", 2)]
    assert throttler.acquire("one") == (False, 1)
    assert throttler.acquire("two") == (False, 1)

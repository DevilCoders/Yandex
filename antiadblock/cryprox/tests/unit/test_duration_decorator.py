import pytest
import time
from antiadblock.cryprox.cryprox.common.tools.misc import duration_usec


@pytest.mark.parametrize('time_to_sleep', (0.2, 0.5, 1, 3))
def test_duration_usec_decorator(time_to_sleep):
    (_, duration) = duration_usec(function_sleep, time_to_sleep)
    assert duration - time_to_sleep * 10**6 < 0.15 * time_to_sleep * 10**6  # delta - 15%


def function_sleep(sleep_time):
    time.sleep(sleep_time)
    return 1

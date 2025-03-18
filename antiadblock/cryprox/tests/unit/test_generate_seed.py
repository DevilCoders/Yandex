from time import struct_time, localtime, mktime
from functools import partial

import pytest

from antiadblock.cryprox.cryprox.common.cry import generate_seed


def make_fixed_shifted_timefunc(hours, minutes, seconds):
    seconds_since_epoch_local = mktime(struct_time((2019, 1, 9, hours, minutes, seconds, 2, 9, 0)))

    def shifted_timefunc(seconds=0):
        return localtime(seconds_since_epoch_local + seconds)

    return shifted_timefunc


@pytest.mark.parametrize('change_period', (1, 6, 12, 24))
def test_generate_seed_throw_the_day(change_period):
    seeds = set()
    for h in range(24):
        seeds.add(generate_seed(change_period=change_period,
                                shifted_time_func=lambda: struct_time((2019, 1, 9, h, 34, 48, 2, 9, 0))))
    assert len(seeds) == 24 / change_period


@pytest.mark.parametrize('gen,shifts_same,shifts_diff', [
    (
        partial(generate_seed, change_period=12, shifted_time_func=make_fixed_shifted_timefunc(22, 12, 1)),
        [0, 60, 60+40, 300],
        [-3*60, -10*60, -12*60, -60-48, 12*60]
    ),
    (
        partial(generate_seed, change_period=12, shifted_time_func=make_fixed_shifted_timefunc(0, 1, 5)),
        [0, 1, -1, -60, -11*60-58],
        [2, 30, 60, 6*60, -11*60-59, 12*60]
    ),
    (
        partial(generate_seed, change_period=3, shifted_time_func=make_fixed_shifted_timefunc(0, 1, 5)),
        [0, 1, -1, -30, -60, -2*60-58],
        [2, -2*60-59, 3*60, 12*60, 30, 60, 103]
    )
])
def test_generate_seed_respects_time_shift(gen, shifts_same, shifts_diff):
    for shift in shifts_same:
        assert gen(time_shift_minutes=shift) == gen(), shift
    for shift in shifts_diff:
        assert gen(time_shift_minutes=shift) != gen(), shift

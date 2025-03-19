"""Test for backoff library"""


from yc_common import backoff


def _test_rand():
    return 1


def test_exponential_gen_unlimited():
    gen = backoff.gen_exponential_backoff(100, rand=_test_rand)
    assert next(gen) == 100 * (2 ** 0)
    assert next(gen) == 100 * (2 ** 1)
    assert next(gen) == 100 * (2 ** 2)


def test_exponential_gen_truncated():
    gen = backoff.gen_exponential_backoff(100, 300, rand=_test_rand)
    assert next(gen) == 100 * (2 ** 0)
    assert next(gen) == 100 * (2 ** 1)
    assert next(gen) == 300


def test_exponential_calc():
    value = backoff.calc_exponential_backoff(5, 100, rand=_test_rand)
    assert value == 100 * (2 ** 5)

"""Various backoff strategies"""

import itertools
import random


_SYSTEM_RANDOM = random.SystemRandom()


def _default_random():
    return _SYSTEM_RANDOM.random()


def gen_exponential_backoff(min_value, max_value=None, multiplier=2, rand=None):
    """
    Generate sleep times with exponential backoff
    """

    if rand is None:
        rand = _default_random

    value = min_value
    while max_value is None or value < max_value:
        yield rand() * value
        value *= multiplier

    while True:
        yield max_value


def calc_exponential_backoff(n, min_value, multiplier=2, rand=None):
    """
    Calculate exponential backoff for nth iteration
    """

    if rand is None:
        rand = _default_random

    return rand() * (min_value * multiplier ** n)

#!/usr/bin/env python

from default_getter import get_default_tags
from redis_getter import get_role


def health_tags():
    return ' '.join((get_default_tags('mdbhealth'), 'a_tier_' + get_role()))


if __name__ == '__main__':
    print(health_tags())

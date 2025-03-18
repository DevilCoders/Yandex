# coding: utf-8
from __future__ import with_statement

import os


__all__ = []


STABLE = 'stable'
PRESTABLE = 'prestable'
UNSTABLE = 'unstable'
TESTING = 'testing'
DEVELOPMENT = 'development'
PREPROD = 'preprod'
PRODUCTION = 'production'
STRESS = 'stress'
DATATESTING = 'datatesting'
OTHER = 'other'


_fallbacks = {
    DEVELOPMENT: [TESTING, PRESTABLE, PREPROD, PRODUCTION, STABLE],
    UNSTABLE: [TESTING, PRESTABLE, PREPROD, PRODUCTION, STABLE],
    TESTING: [PRESTABLE, PREPROD, PRODUCTION, STABLE],
    PRESTABLE: [PREPROD, PRODUCTION, STABLE],
    PREPROD: [PRESTABLE, PRODUCTION, STABLE],
    STABLE: [PRODUCTION],
    PRODUCTION: [STABLE],
    STRESS: [TESTING, PREPROD, PRESTABLE, PRODUCTION, STABLE],
    DATATESTING: [OTHER],
}


def _choose(available, value, fallback=False, separator=None):
    separator = separator or '.'
    available = frozenset(available)

    def pick(value):
        if value in available:
            return value

        if fallback:
            for env in _fallbacks.get(value, []):
                if env in available:
                    return env

    for env in _chain(value, separator):
        picked = pick(env)

        if picked:
            return picked

    raise ValueError('No environment is available')


def _chain(value, separator=None):
    separator = separator or '.'
    bits = value.split(separator)

    while bits:
        yield separator.join(bits)

        bits = bits[:-1]


def chain_type(separator=None):
    return _chain(type, separator)


def chain_name(separator=None):
    return _chain(name, separator)


def choose_type(available, fallback=False, separator=None):
    return _choose(available, type, fallback, separator)


def choose_name(available, fallback=False, separator=None):
    return _choose(available, name, fallback, separator)


def choose_key_by_type(dict_, fallback=False, separator=None):
    return dict_[choose_type(dict_.keys(), fallback, separator)]


def choose(dict_, *args, **kwargs):
    kwargs['fallback'] = True
    return choose_key_by_type(dict_, *args, **kwargs)


def choose_kw(**kwargs):
    separator = kwargs.pop('separator', None)
    return choose(dict_=kwargs, separator=separator)


def choose_key_by_name(dict_, fallback=False, separator=None):
    return dict_[choose_name(dict_.keys(), fallback, separator)]


def _load(suffix, default):
    var_name = 'YENV_%s' % suffix.upper()

    value = os.environ.get(var_name)

    if not value:
        value = default

        try:
            with open('/etc/yandex/environment.%s' % suffix) as f:
                value = f.read().strip()
        except IOError:
            pass

        os.environ[var_name] = value

    return value


type = _load('type', 'development')
name = _load('name', 'localhost')

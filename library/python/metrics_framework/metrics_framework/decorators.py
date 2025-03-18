# coding: utf-8

import functools

from django.conf import settings

from metrics_framework.models import Value


def metric(slug, ValueModel=Value):
    def _decorator(func):
        @functools.wraps(func)
        def _wrapper(*args, **kwargs):
            return func(*args, **kwargs)

        if slug in settings.METRIC_SLUG_MAPPING:
            func = settings.METRIC_SLUG_MAPPING[slug][0]
            raise ValueError('Metric slug %s was already used by function %s' % (slug, func.__name__))
        settings.METRIC_SLUG_MAPPING[slug] = (func, ValueModel)
        return _wrapper

    return _decorator

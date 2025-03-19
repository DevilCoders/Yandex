# -*- coding: utf-8 -*-
import copy
from ..utils import register

_METRICS = {
    'count': 0,
    'count_2xx': 0,
    'count_3xx': 0,
    'count_4xx': 0,
    'count_5xx': 0,
    'timings': {
        100: 0,
        500: 0,
        1000: 0,
        10000: 0,
    },
}
errors = {'count_5xx': 0, 'count_total': 0}

STAT = {'common': copy.deepcopy(_METRICS)}


def init_stat(app):
    clusters = register.get_supported_clusters()
    for endpoint in app.view_functions:
        STAT[endpoint] = copy.deepcopy(errors)
        for cluster in clusters:
            STAT[f"cluster_{cluster.removesuffix('_cluster')}_{endpoint}"] = copy.deepcopy(errors)

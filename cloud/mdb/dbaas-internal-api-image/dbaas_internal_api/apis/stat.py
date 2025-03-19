# -*- coding: utf-8 -*-
"""
DBaaS Internal API Stat handler namespace
"""

from flask.views import MethodView

from . import API
from ..core import STAT
from ..dbs.postgresql import DB

PREFIX = 'dbaas_internal_api'
SUM_SUFFIX = 'dmmm'
CUR_SUFFIX = 'ammv'


@API.resource('/stat')
class StatResource(MethodView):
    """
    Stat Response handler
    """

    def get(self):
        """
        Return JSON list with stat
        """
        res = []
        for host, pool in DB.governor.pools.items():
            if not pool['pool']:
                continue
            stats = pool['pool'].stats
            res.append(
                [
                    '%s_%s_%s' % (PREFIX, 'db_%s_conn_open' % host.split('.')[0], CUR_SUFFIX),
                    stats.open,
                ]
            )
            res.append(
                [
                    '%s_%s_%s' % (PREFIX, 'db_%s_conn_used' % host.split('.')[0], CUR_SUFFIX),
                    stats.used,
                ]
            )
        for metric, stat in STAT.items():
            for key in stat:
                if key.startswith('count'):
                    if metric == 'common':
                        path = f"{PREFIX}_{key}_{SUM_SUFFIX}"
                    elif metric.startswith('cluster_'):
                        path = f"{metric}_{key}_{SUM_SUFFIX}"
                    else:
                        path = f"{PREFIX}_{metric}_{key}_{SUM_SUFFIX}"
                    res.append([path, float(stat[key])])
                elif key == 'timings':
                    for bound in stat['timings']:
                        if metric == 'common':
                            path = f"{PREFIX}_timings_less_{bound}_{SUM_SUFFIX}"
                        elif metric.startswith('cluster_'):
                            path = f"{metric}_timings_less_{bound}_{SUM_SUFFIX}"
                        else:
                            path = f"{PREFIX}_{metric}_timings_less_{bound}_{SUM_SUFFIX}"
                        res.append(
                            [
                                path,
                                float(stat['timings'][bound]),
                            ]
                        )
        return res

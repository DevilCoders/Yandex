import json
import urllib
import time
import logging
from collections import defaultdict

import numpy as np
import numpy.linalg

from flask import render_template, url_for

from core.utils import get_ch_formatted_date_from_timestamp
from core.query import run_query

from .background_utils import get_last_week_groups, BACKGROUND_PRIORITY
from .util import get_in_statement_list
from . import app, cache_manager


def distance_euclidean(a, b):
    return numpy.linalg.norm(a - b)


@app.route('/host_anomalies')
def host_anomalies():
    def instance_graph_param(instance, is_outlier, memusage):
        host, port = instance
        res = {
            'host': host,
            'port': port,
            'signal': 'instance_memusage' if memusage else 'instance_cpuusage',
            'graph_name': "{}:{} [{}]".format(host, port, "anomaly" if is_outlier else "normal")
        }

        if not is_outlier:
            res['color'] = "#999999"

        return res

    def get_params(memusage, group_name, outliers):
        graphs = []
        for instance in outliers:
            graphs.append(instance_graph_param(instance, True, memusage))

        graphs.append({
            'group': group_name,
            'signal': 'group_memusage_distribution' if memusage else 'group_cpuusage_distribution',
        })

        ts1 = int(time.time() - 7 * 24 * 60 * 60)
        ts2 = int(time.time())
        zoom_level = "auto"

        return urllib.quote(json.dumps({
            'graphs': graphs,
            'start_ts': ts1,
            'end_ts': ts2,
            'zoom_level': zoom_level
        }))

    data = get_all_anomalies()
    for group, group_data in data.items():
        # workaround. Somehow there is data for only memory or cpu
        if True not in group_data:
            # Can happen for older/newer groups
            group_data[True] = {'outliers': [], 'n_instances': group_data[False]['n_instances']}

        if False not in group_data:
            group_data[False] = {'outliers': [], 'n_instances': group_data[True]['n_instances']}

        if len(group_data[True]['outliers']) == 0 and len(group_data[False]['outliers']) == 0:
            del data[group]
            continue

        for memusage in [False, True]:
            outliers = group_data[memusage]['outliers']
            data[group][memusage]['graph_params_serialized'] = get_params(memusage, group, outliers)
            data[group][memusage]['outlier_graph_links'] = [url_for('instance_graph', params=get_params(memusage, group, [o])) for o in outliers]

    return render_template("anomalies.html", data=data)


@cache_manager.memoize(key_prefix="get_all_anomalies", timeout=7 * 24 * 60 * 60)
def get_all_anomalies():
    groups = get_last_week_groups()
    groups = groups

    cpu_anomalies = get_anomalies(False, groups)
    mem_anomalies = get_anomalies(True, groups)

    result = cpu_anomalies
    for group in groups:
        if group not in mem_anomalies:
            continue
        result[group][True] = mem_anomalies[group][True]

    return result


def get_anomalies(memusage, groups):
    now = time.time()
    seconds_in_week = 7 * 24 * 60 * 60

    query = """ SELECT host, port, group, avg({field}), median({field}),
                       quantile(0.75)({field}), quantile(0.95)({field}),
                       stddevPop({field})
                FROM instanceusage_aggregated_15m
                WHERE group in {group_list}
                      AND eventDate >= '{ed1}' AND eventDate <= '{ed2}'
                      {extra_filter_conditions}
                GROUP BY host, port, group
              """.format(
        field="instance_memusage" if memusage else "instance_cpuusage_power_units",
        group_list=get_in_statement_list(groups),
        ed1=get_ch_formatted_date_from_timestamp(now - seconds_in_week),
        ed2=get_ch_formatted_date_from_timestamp(now),
        extra_filter_conditions="AND instance_cpuusage <= 1.0" if not memusage else ""
    )

    data = run_query(query, priority=BACKGROUND_PRIORITY)

    group_instances = defaultdict(list)
    group_instances_stats = defaultdict(list)

    for row in data:
        host, port, group = row[:3]
        stats = map(float, row[3:])
        group_instances[group].append((host, port))
        group_instances_stats[group].append(stats)

    result = defaultdict(dict)
    for group in group_instances.keys():
        stats = np.array(group_instances_stats[group])
        instances = group_instances[group]
        assert len(stats) == len(instances)

        if len(instances) < 11:
            logging.debug("Group %s is too small, skipping it", group)
            continue
        logging.info("Trying to find anomalies for group %s", group)

        instances_outliers, instances_dist = find_outliers(instances, stats)

        result[group][memusage] = {
            'normal': sorted(instances_dist.keys(), key=instances_dist.get)[:len(instances_dist) / 2],
            'outliers': instances_outliers,
            'n_instances': len(instances)
        }

    return result


def find_outliers(instances, stats):
    # TODO: normalize stats vectors???
    common_median_distance = np.median([
        distance_euclidean(stats[i], stats[j])
        for i in xrange(len(stats))
        for j in xrange(i, len(stats))
    ])

    instances_dist = {}
    instances_outliers = []
    for i, instance in enumerate(instances):
        self_median_distance = np.median([
            distance_euclidean(stats[i], stats[j])
            for j in xrange(len(instances))
        ])

        instances_dist[instance] = self_median_distance

        # There are too much false positive-ish outliers, so let's consider
        # both relative distance and absolute distance
        if (self_median_distance > 7 * common_median_distance) and (self_median_distance > 3):
            instances_outliers.append(instance)

    return instances_outliers, instances_dist

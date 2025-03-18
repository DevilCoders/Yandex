"""Openstack-specific pages"""

import time

from flask import render_template, jsonify

from . import app
from core.utils import get_ch_formatted_date_from_timestamp
from core.query import run_query


@app.route('/metaprj')
def metaprj():
    return render_template(
        "metaprj.html",
        data_endpoint='metaprj_data',
    )


def get_metaprj_peak_usage():
    """Calculate peak usage for metaprj"""

    end_ts = int(time.time())
    end_ed = get_ch_formatted_date_from_timestamp(end_ts)
    start_ts = end_ts - 24 * 60 * 60
    start_ed = get_ch_formatted_date_from_timestamp(start_ts)

    inner_query = (
        "SELECT metaprj, ts, sum(memory_usage) AS memory_usage, sum(memory_allocated) as memory_allocated, "
        "       sum(cpu_usage) AS cpu_usage,  sum(cpu_allocated) as cpu_allocated"
        "    FROM metaprjusage_aggregated_2m "
        "    WHERE ts >= {start_ts} AND ts <= {end_ts} AND eventDate >= '{start_ed}' AND eventDate <= '{end_ed}' "
        "    GROUP BY metaprj, ts"
    ).format(start_ts=start_ts, end_ts=end_ts, start_ed=start_ed, end_ed=end_ed)

    query = ("SELECT metaprj, quantile(0.99)(memory_usage), quantile(0.99)(memory_allocated), "
             "       quantile(0.99)(cpu_usage), quantile(0.99)(cpu_allocated) FROM ({inner_query}) GROUP BY metaprj").format(inner_query=inner_query)

    result = []
    for row in run_query(query, shuffle=True):
        project_id, memory_usage, memory_allocated, cpu_usage, cpu_allocated = row
        memory_usage = float(memory_usage)
        memory_allocated = float(memory_allocated)
        cpu_usage = float(cpu_usage)
        cpu_allocated = float(cpu_allocated)

        result.append((project_id, memory_usage, memory_allocated, cpu_usage, cpu_allocated))

    return result


@app.route('/metaprj_data')
def metaprj_data():
    metaprj_peak_usage = get_metaprj_peak_usage()

    result = []

    # add total data
    total_memory_usage = sum(x[1] for x in metaprj_peak_usage)
    total_memory_allocated = sum(x[2] for x in metaprj_peak_usage)
    total_cpu_usage = sum(x[3] for x in metaprj_peak_usage)
    total_cpu_allocated = sum(x[4] for x in metaprj_peak_usage)
    result.append(dict(project='ALL', url='/metaprj/usage/cpu?', memory_usage=total_memory_usage, memory_allocated=total_memory_allocated,
                       cpu_usage=total_cpu_usage, cpu_allocated=total_cpu_allocated))

    # add per-project data
    for project_id, memory_usage, memory_allocated, cpu_usage, cpu_allocated in metaprj_peak_usage:
        result.append(dict(project=project_id, url='/metaprj/usage/cpu?metaprj={}'.format(project_id), memory_usage=memory_usage,
                           memory_allocated=memory_allocated, cpu_usage=cpu_usage, cpu_allocated=cpu_allocated))

    return jsonify(result)


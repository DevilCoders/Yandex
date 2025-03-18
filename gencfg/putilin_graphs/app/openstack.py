"""Openstack-specific pages"""

import time

from flask import render_template, jsonify

from . import app
from .consts import OPENSTACK_PRJS
from core.utils import get_ch_formatted_date_from_timestamp
from core.query import run_query


@app.route('/openstack')
def openstack():
    return render_template(
        "openstack.html",
        data_endpoint='openstack_data',
    )


def get_openstack_peak_usage():
    """Calculate peak usage for openstack"""

    end_ts = int(time.time())
    end_ed = get_ch_formatted_date_from_timestamp(end_ts)
    start_ts = end_ts - 24 * 60 * 60
    start_ed = get_ch_formatted_date_from_timestamp(start_ts)

    inner_query = (
        "SELECT projectId, ts, sum(mem_usage) AS mem_usage, sum(cpu_usage_power_units) AS cpu_usage "
        "    FROM openstackusage_aggregated_2m "
        "    WHERE ts >= {start_ts} AND ts <= {end_ts} AND eventDate >= '{start_ed}' AND eventDate <= '{end_ed}' "
        "    GROUP BY projectId, ts"
    ).format(start_ts=start_ts, end_ts=end_ts, start_ed=start_ed, end_ed=end_ed)

    query = "SELECT projectId, quantile(0.99)(mem_usage), quantile(0.99)(cpu_usage) FROM ({inner_query}) GROUP BY projectId".format(inner_query=inner_query)

    result = []
    for row in run_query(query, shuffle=True):
        project_id, mem_usage, cpu_usage = row
        mem_usage = float(mem_usage)
        cpu_usage = float(cpu_usage)

        result.append((project_id, mem_usage, cpu_usage))

    return result


@app.route('/openstack_data')
def openstack_data():
    openstack_peak_usage = get_openstack_peak_usage()

    result = []

    # add total data
    total_memory_usage = sum(x[1] for x in openstack_peak_usage)
    total_cpu_usage = sum(x[2] for x in openstack_peak_usage)
    result.append(dict(project='ALL', url='/openstack/usage/cpu?format=cores', memory_usage=total_memory_usage, cpu_usage=total_cpu_usage))

    # add per-project data
    for project_id, memory_usage, cpu_usage in openstack_peak_usage:
        human_project_id = OPENSTACK_PRJS.get(project_id, project_id)
        result.append(dict(project=human_project_id, url='/openstack/usage/cpu?projectId={}&format=cores'.format(project_id), memory_usage=memory_usage, cpu_usage=cpu_usage))

    return jsonify(result)


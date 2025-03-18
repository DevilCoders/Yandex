import json
import urllib
import re
import time
import requests
from operator import itemgetter
from collections import defaultdict, namedtuple
from datetime import datetime
from functools import partial

from flask import render_template, url_for, request, jsonify

from . import app
from .consts import OPENSTACK_PRJS
from .util import calculate_zoom_level, zoom_level_to_period, zoom_level_to_period_start_adjustment
from .util import get_historical_graph_params
from core.utils import get_ch_formatted_date_from_timestamp
from core.query import run_query, run_query_format_json, run_single_int_query, get_roughly_last_event_date_from_table


QLOUD_NODE_TYPES = ('installType', 'project', 'app', 'environ', 'service')
OPENSTACK_NODE_TYPES = ('projectId', )
ABC_NODE_TYPES = ('l1project', 'l2project', 'l3project', 'l4project', 'l5project', 'l6project', 'l7project',
                  'l8project', 'l9project', 'l10project')
METAPRJ_NODE_TYPES = ('metaprj', )
YP_NODE_TYPES = ('group', )


class EGencfgSignals(object):
    MEM = 'memory_usage'
    CPU = 'cpu_usage'
    NET_RX = 'net_rx'
    NET_TX = 'net_tx'
    ALL = [MEM, CPU, NET_RX, NET_TX]


TGencfgSignal = namedtuple('TGencfgSignal', [
        'name',
        'instance_signal_name',
        'group_allocated_signal_name',
        'graph_title',
        'units_postfix',
        'units_convert_func',
        'extra_filter_conditions',  # extra filter conditions to filter out instances with corrupted signal value
        'group_data_endpoint',  # endpoing for signal data graph
])

GENCFG_SIGNALS = {
    EGencfgSignals.MEM: TGencfgSignal(
        name=EGencfgSignals.MEM,
        instance_signal_name='instance_memusage',
        group_allocated_signal_name='memory_guarantee',
        graph_title='Memory',
        units_postfix='Gb',
        units_convert_func=lambda x: x,
        extra_filter_conditions='',
        group_data_endpoint='group_memory_graph_data',
    ),
    EGencfgSignals.CPU: TGencfgSignal(
        name=EGencfgSignals.CPU,
        instance_signal_name='instance_cpuusage_power_units',
        group_allocated_signal_name='cpu_guarantee',
        graph_title='Cpu',
        units_postfix='Power Units',
        units_convert_func=lambda x: x,
        extra_filter_conditions='AND instance_cpuusage <= 1.0',
        group_data_endpoint='group_cpu_graph_data',
    ),
    EGencfgSignals.NET_RX: TGencfgSignal(
        name=EGencfgSignals.NET_RX,
        instance_signal_name='instance_net_rx',
        group_allocated_signal_name='net_guarantee',
        graph_title='Net Rx',
        units_postfix='Mbit',
        units_convert_func=lambda x: x / 1024. / 1024 * 8,
        extra_filter_conditions='',
        group_data_endpoint='group_net_rx_graph_data',
    ),
    EGencfgSignals.NET_TX: TGencfgSignal(
        name=EGencfgSignals.NET_TX,
        instance_signal_name='instance_net_tx',
        group_allocated_signal_name='net_guarantee',
        graph_title='Net Tx',
        units_postfix='Mbit',
        units_convert_func=lambda x: x / 1024. / 1024 * 8,
        extra_filter_conditions='',
        group_data_endpoint='group_net_tx_graph_data',
    ),
}


def convert_abc_signal_to_gb(v):
    """Convert abc signal (like memory/hdd/ssd) to Gb from bytes)"""
    return v / 1024. / 1024 / 1024


class ESections(object):
    QLOUD = 'qloud'
    GENCFG = 'gencfg'
    OPENSTACK = 'openstack'
    ABC = 'abc'
    METAPRJ = 'metaprj'
    YP = 'yp'


def app_usage_data_routes(route_template, endpoint_template, resources=("cpu", "mem")):
    def decorator(f):
        for resource in resources:
            app.add_url_rule(
                route_template.format(resource),
                endpoint_template.format(resource),
                partial(f, resource=resource)
            )
        return f

    return decorator


def get_in_statement_list_for_groups(group_list):
    flattened_group_list = set(sum((list(x) for x in group_list), []))
    return "({})".format(", ".join("'{}'".format(group_name) for group_name in flattened_group_list))


def get_in_statement_list_for_instances(instances_list):
    return "({})".format(", ".join("('{}', {})".format(host, port) for host, port in instances_list))


def get_in_statements_for_strings(strings_list):
    return "({})".format(", ".join("'{}'".format(s) for s in strings_list))


def get_histogram_params():
    try:
        params = json.loads(request.args.get('params'))
    except:
        params = json.loads(urllib.unquote(request.args.get('params')))

    if params.get('ts'):
        ts = int(params['ts'])
    else:
        ts = None

    n_bins = int(params['n_bins'])
    graphs = params['graphs']

    return graphs, ts, n_bins


def _get_groups_sum_id(groups, usage_list, name_prefix):
    groups = groups if isinstance(groups, (list, tuple)) else [groups]
    return name_prefix + str(usage_list.index(groups))


def _get_groups_sum_name(groups):
    return "+".join(groups if isinstance(groups, list) else [groups])


def _is_cpu_graph():
    params_str = request.args.get('params')
    params = json.loads(urllib.unquote(params_str))
    for graph_param in params['graphs']:
        if 'cpu' in graph_param['signal']:
            return True
        else:
            return False

    assert False

def detect_signal_type(params):
    """Here we must detect what type of graph (cpu/mem/net_rx/net_tx/...) we should render (pisdec kakoi-to)"""
    for graph_param in params['graphs']:
        if 'cpu' in graph_param['signal']:
            return GENCFG_SIGNALS[EGencfgSignals.CPU]
        elif 'mem' in graph_param['signal']:
            return GENCFG_SIGNALS[EGencfgSignals.MEM]
        elif 'net_rx' in graph_param['signal']:
            return GENCFG_SIGNALS[EGencfgSignals.NET_RX]
        elif 'net_tx' in graph_param['signal']:
            return GENCFG_SIGNALS[EGencfgSignals.NET_TX]
        else:
            raise Exception('Can not detect signal type from <{}>'.format(graph_param['signal']))

    assert False



def render_graph_template(cpu_graph_endpoint, memory_graph_endpoint, template_name):
    # TODO better error handling: 400 and such (instead of asserts leading to possible 500's)
    params_str = request.args.get('params')

    if _is_cpu_graph():
        cpu_graph_url = url_for(cpu_graph_endpoint, params=params_str)
        return render_template(template_name, cpu_graph_url=cpu_graph_url)
    else:
        memory_graph_url = url_for(memory_graph_endpoint, params=params_str)
        return render_template(template_name, memory_graph_url=memory_graph_url)


def _generate_arrayjoin_helper_subquery(usage_list, name_prefix):
    array = "["
    first_group = True
    for id, groups in enumerate(usage_list):
        for group in groups:
            if not first_group:
                array += ", "
            array += "('{}', '{}')".format(group, _get_groups_sum_id(groups, usage_list, name_prefix))
            first_group = False

    array += "]"
    return """
        SELECT arrayJoin(
          {}
        ) AS pair,
        tupleElement(pair, 1) AS group,
        tupleElement(pair, 2) AS group_names
    """.format(array)


def group_histogram_data(graphs, ts, n_bins, gencfg_signal):
    group_instanceusage_list, group_hostusage_list, group_allocated_list = get_usage_lists(graphs, gencfg_signal)
    group_instanceusage_list = [x[2] for x in group_instanceusage_list]
    group_hostusage_list = [x[2] for x in group_hostusage_list]
    group_allocated_list = [x[2] for x in group_allocated_list]

    table_name = "instanceusage_aggregated_2m"

    assert not group_allocated_list
    assert group_instanceusage_list or group_hostusage_list

    if group_hostusage_list and group_instanceusage_list:
        assert group_instanceusage_list == group_hostusage_list

    group_list = group_instanceusage_list or group_hostusage_list
    flattened_group_list = get_in_statement_list_for_groups(group_list)

    if not ts:
        last_event_date = get_roughly_last_event_date_from_table(table_name, days_to_watch=2)
        if last_event_date:
            last_ts_query = """
               SELECT max(ts)
               FROM {table_name}
               WHERE (group IN {flattened_group_list}) AND eventDate >= '{ed}'
            """.format(
                table_name=table_name,
                flattened_group_list=flattened_group_list,
                ed=last_event_date,
            )
            ts = run_single_int_query(last_ts_query)

    if group_instanceusage_list:
        iu_query = """
            SELECT
                host,
                group_names,
                {usage_field} AS usage
            FROM {table_name}
            ALL INNER JOIN (
                {helper_subquery}
            ) USING group
            WHERE (group IN {flattened_group_list}) AND (ts = {ts}) AND (eventDate = '{ed}') {extra_filter_conditions}
        """.format(
            flattened_group_list=flattened_group_list,
            helper_subquery=_generate_arrayjoin_helper_subquery(group_instanceusage_list, "i"),
            table_name=table_name,
            ts=ts,
            ed=get_ch_formatted_date_from_timestamp(ts),
            usage_field=gencfg_signal.instance_signal_name,
            extra_filter_conditions=gencfg_signal.extra_filter_conditions,
        )

    if group_hostusage_list:
        hu_query = """
            SELECT DISTINCT
                host,
                group_names,
                {usage_field} AS usage
            FROM {table_name} ALL INNER JOIN
            (
                SELECT DISTINCT
                    host,
                    group_names
                FROM {table_name}
                ALL INNER JOIN ({helper_subquery}) USING group
                WHERE (group IN {flattened_group_list}) AND (ts = {ts}) AND (eventDate = '{ed}')
            ) USING host
            WHERE (ts = {ts}) AND (eventDate = '{ed}') AND (port = 65535) {extra_filter_conditions}
        """.format(
            flattened_group_list=flattened_group_list,
            helper_subquery=_generate_arrayjoin_helper_subquery(group_hostusage_list, "h"),
            table_name=table_name, ts=ts,
            ed=get_ch_formatted_date_from_timestamp(ts),
            usage_field=gencfg_signal.instance_signal_name,
            extra_filter_conditions=gencfg_signal.extra_filter_conditions,
        )

    if group_instanceusage_list and group_hostusage_list:
        query = "{} UNION ALL {}".format(iu_query, hu_query)
    elif group_instanceusage_list:
        query = iu_query
    elif group_hostusage_list:
        query = hu_query
    else:
        assert False

    data = run_query(query)

    all_values = map(float, map(itemgetter(2), data))
    if not all_values:
        raise Exception("No data for groups {} at timestamp {}".format(map(itemgetter('group'), graphs), ts))
    min_value = min(all_values)
    max_value = max(all_values)

    histograms = defaultdict(lambda: [[] for _ in xrange(n_bins)])
    bin_size = (max_value - min_value) / n_bins
    for row in data:
        host, group_id, value = row
        value = float(value)
        bin_num = int((value - min_value) / bin_size)

        if bin_num == n_bins and value + 1e-5 > max_value:
            bin_num -= 1

        assert 0 <= bin_num <= n_bins - 1
        histograms[group_id][bin_num].append(host.replace('.search.yandex.net', ''))

    labels = []
    bin_template = "{:.0f} {}" if bin_size > 2.0 else "{:.3f} {}"
    for i in xrange(n_bins):
        bin_start = min_value + i * bin_size
        bin_end = min_value + (i + 1) * bin_size
        bin_label = bin_template.format(bin_start + (bin_end - bin_start) / 2, gencfg_signal.units_postfix)

        labels.append(bin_label)

    result = {'graphs': [],
              'labels': labels,
              'title': '{title} distribution for {ts} ({dt})'.format(
                  title=gencfg_signal.graph_title,
                  ts=ts,
                  dt=datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S')
                  )}
    for graph_param in graphs:
        signal = graph_param['signal']

        is_instance_signal = signal.startswith('instance')
        if is_instance_signal:
            name_prefix = "i"
            usage_list = group_instanceusage_list
        else:
            name_prefix = "h"
            usage_list = group_hostusage_list

        group_names = _get_groups_sum_name(graph_param['group'])
        group_names_id = _get_groups_sum_id(graph_param['group'], usage_list, name_prefix)

        print group_names_id, group_names
        result['graphs'].append({
            'name': graph_param.get('graph_name', group_names + (" instances" if is_instance_signal else " hosts")),
            'data': [{
                'y': len(hosts),
                'hosts': hosts
            } for hosts in histograms[group_names_id]]
        })

    return result


@app.route("/cpu_histogram_data")
def cpu_histogram_data():
    return jsonify(group_histogram_data(*get_histogram_params(), gencfg_signal=GENCFG_SIGNALS[EGencfgSignals.CPU]))


@app.route("/memory_histogram_data")
def memory_histogram_data():
    return jsonify(group_histogram_data(*get_histogram_params(), gencfg_signal=GENCFG_SIGNALS[EGencfgSignals.MEM]))


@app.route("/histogram")
def histogram():
    return render_graph_template('cpu_histogram_data', 'memory_histogram_data', 'histogram.html')


def host_graph_data(host, start_ts, end_ts, zoom_level, gencfg_signal):
    assert zoom_level not in ["None", "none", "0"]

    if zoom_level == "auto":
        zoom_level = calculate_zoom_level(start_ts, end_ts, max_points=2500)

    table_name = "instanceusage_aggregated_{}".format(zoom_level)
    query = """
        SELECT ts, group, sum({usage_field})
        FROM {table_name}
        WHERE host='{host}' AND ts >= {ts1} AND ts <= {ts2} AND (eventDate >= '{ed1}') AND (eventDate <= '{ed2}')
        GROUP BY ts, group
        ORDER BY ts, group
        """.format(
        table_name=table_name, ts1=start_ts, ts2=end_ts,
        host=host,
        ed1=get_ch_formatted_date_from_timestamp(start_ts),
        ed2=get_ch_formatted_date_from_timestamp(end_ts),
        usage_field=(gencfg_signal.instance_signal_name)
    )

    print "QUERY", query
    data = run_query(query)

    stats = defaultdict(list)
    total_usage = defaultdict(lambda: 0)

    for row in data:
        ts, group, usage = row
        ts = int(ts) * 1000
        usage = float(usage)
        stats[group].append((ts, usage))
        total_usage[group] += usage

    result = []

    for group in sorted(stats.keys(), key=lambda g: total_usage[g], reverse=True):
        graph = {
            'name': group if group != "" else "{} total".format(host),
            'type': 'area' if group != "" else 'line',
            'data': stats[group]
        }

        if group == "":
            graph['color'] = "#000000"

        result.append(graph)

    if gencfg_signal.name == EGencfgSignals.CPU:
        result.append({
            'name': 'Total host power',
            'type': 'line',
            'data': get_host_power_data_for_list([host], start_ts, end_ts, zoom_level)
        })

    return result


@app.route("/host_cpu_graph_data/<host>")
def host_cpu_graph_data(host):
    start_ts, end_ts, zoom_level, _ = get_historical_graph_params()
    return jsonify(host_graph_data(host, start_ts, end_ts, zoom_level, GENCFG_SIGNALS[EGencfgSignals.CPU]))


@app.route("/host_memory_graph_data/<host>")
def host_memory_graph_data(host):
    start_ts, end_ts, zoom_level, _ = get_historical_graph_params()
    return jsonify(host_graph_data(host, start_ts, end_ts, zoom_level, GENCFG_SIGNALS[EGencfgSignals.MEM]))


@app.route("/host_memory_graph/<host>")
def host_memory_graph(host):
    params = request.args.get('params')
    memory_graph_url = url_for('host_memory_graph_data', host=host, params=params)
    return render_template("graph.html", memory_graph_url=memory_graph_url)


@app.route("/host_cpu_graph/<host>")
def host_cpu_graph(host):
    params = request.args.get('params')
    graph_url = url_for('host_cpu_graph_data', host=host, params=params)
    return render_template("graph.html", signal=GENCFG_SIGNALS[EGencfgSignals.CPU], graph_url=graph_url)


def align_stats_to_period(stats, start_ts, end_ts, period, tz_offset_hack):
    """ Make stats aligned to grid
        For example, allocated stats have irregular timestamps


        ^        ^  ^              ^
        |        |  |              |

        We have to align these stats to a regular grid(2 minute, 15 minute, 1 hour or 1 day intervals):

        ^   ^   ^   ^   ^   ^
        |   |   |   |   |   |   ... etc


        tz_offset_hack: read extensive explanation for zoom_level_to_period_start_adjustment function
    """

    if len(stats) == 0:
        return stats

    new_stats = []
    current_point_index = 0
    start_ts = max(start_ts, stats[0][0] / 1000)
    for ts in xrange((start_ts - start_ts % period) * 1000, end_ts * 1000 + period * 1000, period * 1000):
        original_ts, original_value = stats[current_point_index]

        while original_ts < ts and current_point_index < len(stats) - 1:
            current_point_index += 1
            original_ts, original_value = stats[current_point_index]

        new_stats.append((ts - tz_offset_hack * 1000, original_value))

    return new_stats


def get_usage_lists(graphs, gencfg_signal):
    class ETableType(object):
        INSTANCE = 0
        HOST = 1
        ALLOCATED = 2

    group_instanceusage_list = []
    group_hostusage_list = []
    group_allocated_list = []

    if gencfg_signal.name == 'memory_usage':
        supported_signals = {
            'instance_memusage': ('instance_memusage', ETableType.INSTANCE),
            'instance_anon_memusage': ('instance_anon_memusage', ETableType.INSTANCE),
            'host_memusage': ('instance_memusage', ETableType.HOST),
            'instance_mem_allocated': ('memory_guarantee', ETableType.ALLOCATED),
            'host_memory_total': ('host_memory_total', ETableType.ALLOCATED),
        }
    elif gencfg_signal.name == 'cpu_usage':
        supported_signals = {
            'instance_cpuusage': ('instance_cpuusage_power_units', ETableType.INSTANCE),
            'host_cpuusage': ('instance_cpuusage_power_units', ETableType.HOST),
            'instance_cpu_allocated': ('cpu_guarantee', ETableType.ALLOCATED),
            'host_cpu_total': ('host_cpu_total', ETableType.ALLOCATED),
        }
    elif gencfg_signal.name == 'net_rx':
        supported_signals = {
            'instance_net_rx': ('instance_net_rx', ETableType.INSTANCE),
            'instance_net_tx': ('instance_net_tx', ETableType.INSTANCE),
            'host_net_rx': ('instance_net_rx', ETableType.HOST),
            'host_net_tx': ('instance_net_tx', ETableType.HOST),
            'instance_net_rx_allocated': ('net_guarantee', ETableType.ALLOCATED),
        }
    elif gencfg_signal.name == 'net_tx':
        supported_signals = {
            'instance_net_rx': ('instance_net_rx', ETableType.INSTANCE),
            'instance_net_tx': ('instance_net_tx', ETableType.INSTANCE),
            'host_net_rx': ('instance_net_rx', ETableType.HOST),
            'host_net_tx': ('instance_net_tx', ETableType.HOST),
            'instance_net_tx_allocated': ('net_guarantee', ETableType.ALLOCATED),
        }
    else:
        raise Exception('Unknown signal name <{}>'.format(gencfg_signal.name))

    usage_lists = defaultdict(list)
    for graph_param in graphs:
        signal = graph_param['signal']

        assert signal in supported_signals, 'Signal <{}> is not supported (not found in <{}>'.format(signal, ','.join(supported_signals))

        table_signal_name, table_type = supported_signals[signal]

        group_param = graph_param['group']
        groups = [group_param] if not isinstance(group_param, list) else group_param
        assert type(groups) == list

        usage_lists[table_type].append((table_signal_name, signal, groups))

    return usage_lists[ETableType.INSTANCE], usage_lists[ETableType.HOST], usage_lists[ETableType.ALLOCATED]


def get_group_allocated_data(start_ts, end_ts, gencfg_signal, group_allocated_list, alloc_column_name=None):
    def _get_int_from_query(query, default):
        data = run_query(query)
        assert len(data) == 0 or (len(data) == 1 and len(data[0]) <= 1)
        return int(data[0][0]) if len(data) == 1 and len(data[0]) == 1 else default

    if alloc_column_name is None:
        alloc_column_name = gencfg_signal.group_allocated_signal_name

    ts1_query = """SELECT max(ts) from group_allocated_resources
                   WHERE ts <= {start_ts} AND group IN {flattened_group_list}
                """.format(
        start_ts=start_ts,
        flattened_group_list=get_in_statement_list_for_groups(tuple(group_allocated_list))
    )
    ts1 = _get_int_from_query(ts1_query, start_ts)

    ts2_query = "SELECT min(ts) from group_allocated_resources WHERE ts >= {end_ts}".format(end_ts=end_ts)
    ts2 = _get_int_from_query(ts2_query, end_ts)

    resources_subquery = """SELECT DISTINCT ts, group, {field_allocated} FROM group_allocated_resources
                            WHERE ts >= {ts1} AND ts <= {ts2} AND group IN {flattened_group_list}
                         """.format(
        field_allocated=alloc_column_name,
        ts1=ts1,
        ts2=ts2,
        flattened_group_list=get_in_statement_list_for_groups(group_allocated_list))

    allocated_query = """SELECT ts, group_names, sum({field_allocated})
                         FROM ({resources_subquery})
                         ALL INNER JOIN (
                           {helper_subquery}
                         ) USING group
                         GROUP BY ts, group_names
                         ORDER BY ts, group_names
                      """.format(
        field_allocated=alloc_column_name,
        ts1=ts1,
        ts2=ts2,
        helper_subquery=_generate_arrayjoin_helper_subquery(group_allocated_list, "a"),
        flattened_group_list=get_in_statement_list_for_groups(group_allocated_list),
        resources_subquery=resources_subquery)

    allocated_data = run_query(allocated_query)

    allocated_stats = defaultdict(list)
    for row_data in allocated_data:
        ts, group_names, allocated = row_data
        ts = int(ts) * 1000

        ts = max(ts, start_ts * 1000)
        ts = min(ts, end_ts * 1000)
        assert 1000 * start_ts <= ts <= end_ts * 1000

        allocated = gencfg_signal.units_convert_func(float(allocated))

        allocated_stats[group_names].append((ts, allocated))

    for group_names in allocated_stats.iterkeys():
        last_ts, last_allocated = allocated_stats[group_names][-1]
        if last_ts != end_ts * 1000:
            allocated_stats[group_names].append((end_ts * 1000, last_allocated))

    return allocated_stats


def get_host_usage_data_for_list(hosts, field, table_name, start_ts, end_ts):
    if not hosts:
        raise Exception("No hosts in list! Cannot make query with empty host list to clickhouse")
    query = """
        SELECT ts, sum({field}) AS sum_host_usage
        FROM {table_name}
        WHERE (ts >= {ts1}) AND (ts <= {ts2}) AND (eventDate >= '{ed1}') AND (eventDate <= '{ed2}') AND (port = 65535)
              AND host in {hosts_list}
        GROUP by ts
    """.format(
        field=field,
        table_name=table_name,
        ts1=start_ts,
        ts2=end_ts,
        ed1=get_ch_formatted_date_from_timestamp(start_ts),
        ed2=get_ch_formatted_date_from_timestamp(end_ts),
        hosts_list=get_in_statements_for_strings(hosts)
    )

    data = sorted((int(ts) * 1000, float(usg)) for ts, usg in run_query(query))

    return data


def get_host_power_data_for_list(hosts, start_ts, end_ts, zoom_level):
    if not hosts:
        raise Exception("No hosts in list! Cannot make query with empty host list to clickhouse")

    # use date which is a week earlier, so we always have enough data to cover the entire start_ts..end_ts period
    start_ts_adjusted = start_ts - 7 * 24 * 60 * 60
    query = """
        SELECT ts, sum(cpu_power) AS sum_power
        FROM (
            SELECT DISTINCT eventDate, ts, commit, host, cpu_power FROM host_resources
            WHERE (ts >= {ts1}) AND (ts <= {ts2}) AND (eventDate >= '{ed1}') AND (eventDate <= '{ed2}') AND host in {hosts_list}
        )
        GROUP by eventDate, ts, commit
    """.format(
        ts1=start_ts_adjusted,
        ts2=end_ts,
        ed1=get_ch_formatted_date_from_timestamp(start_ts_adjusted),
        ed2=get_ch_formatted_date_from_timestamp(end_ts),
        hosts_list=get_in_statements_for_strings(hosts)
    )

    data = sorted((int(ts) * 1000, float(power)) for ts, power in run_query(query))
    data = align_stats_to_period(data,
                                 start_ts, end_ts,
                                 zoom_level_to_period(zoom_level),
                                 zoom_level_to_period_start_adjustment(zoom_level))

    return data


def get_host_power_median_data(hosts, start_ts, end_ts):
    if not hosts:
        raise Exception("No hosts in list! Cannot make query with empty host list to clickhouse")

    query = """
        SELECT ts, median(cpu_power) AS median_power
        FROM (
            SELECT DISTINCT eventDate, ts, commit, host, cpu_power FROM host_resources
            WHERE (ts >= {ts1}) AND (ts <= {ts2}) AND (eventDate >= '{ed1}') AND (eventDate <= '{ed2}') AND host in {hosts_list}
        )
        GROUP by eventDate, ts, commit
    """.format(
        ts1=start_ts,
        ts2=end_ts,
        ed1=get_ch_formatted_date_from_timestamp(start_ts),
        ed2=get_ch_formatted_date_from_timestamp(end_ts),
        hosts_list=get_in_statements_for_strings(hosts)
    )

    data = sorted((int(ts) * 1000, float(median_power)) for ts, median_power in run_query(query))

    return data


def download_fake_group_hosts_list(group_name):
    r = requests.get('https://api.gencfg.yandex-team.ru/trunk/searcherlookup/groups/{}/instances'.format(group_name), verify=False)
    r.raise_for_status()
    hosts = list(set([i['hostname'] for i in r.json()['instances']]))

    return hosts


def fake_group_host_usage_from_trunk_data(start_ts, end_ts, zoom_level, group_name, resource):
    hosts = download_fake_group_hosts_list(group_name)
    table_name = "instanceusage_aggregated_{}".format(zoom_level)
    field = 'instance_{}usage'.format(resource)

    result = [{
        'name': 'Total host usage ({} hosts from trunk)'.format(group_name),
        'type': 'line',
        'data': get_host_usage_data_for_list(hosts, field, table_name, start_ts, end_ts)
    }]

    # TODO: uncomment when use aggregated hostusage tables
    # if resource == "cpu":
    #     result.append({
    #         'name': 'Total host power ({} hosts from trunk)'.format(group_name),
    #         'type': 'line',
    #         'data': get_host_power_data_for_list(hosts, start_ts, end_ts, zoom_level)
    #     })

    return result


def prepare_and_run_host_distribution_query(table_name, start_ts, end_ts, resource, hosts):
    if not hosts:
        raise Exception("Empty hosts list")

    query = """
        SELECT ts, median({usage_field}),
               quantile(0.15)({usage_field}), quantile(0.85)({usage_field}),
               quantile(0.05)({usage_field}), quantile(0.95)({usage_field})
        FROM {table_name}
        WHERE ts >= {ts1} AND ts <= {ts2} AND (eventDate >= '{ed1}') AND (eventDate <= '{ed2}')
              AND host in {hosts_list} AND port=65535 {extra_filter_conditions}
        GROUP BY ts
        ORDER BY ts
        """.format(
            table_name=table_name, ts1=start_ts, ts2=end_ts,
            ed1=get_ch_formatted_date_from_timestamp(start_ts),
            ed2=get_ch_formatted_date_from_timestamp(end_ts),
            usage_field='instance_{}usage'.format(resource),
            extra_filter_conditions="AND instance_cpuusage <= 1.0" if resource == "cpu" else "",
            hosts_list=get_in_statements_for_strings(hosts)
        )

    data = run_query(query)
    return data


def fake_group_host_usage_distribution_from_trunk_data(start_ts, end_ts, zoom_level, group, resource, abstract_cores=False):
    hosts = download_fake_group_hosts_list(group)
    if zoom_level == "auto":
        zoom_level = calculate_zoom_level(start_ts, end_ts, max_points=2500)

    table_name = "instanceusage_aggregated_{}".format(zoom_level)
    distribution_data = prepare_and_run_host_distribution_query(table_name, start_ts, end_ts, resource, hosts)
    result = format_lines_for_distribution_graph(distribution_data, line_base_description=group)

    # TODO: uncomment when use aggregated hostusage tables
    # if resource == "cpu":
    #     result.append({
    #         'name': 'Median host power',
    #         'type': 'line',
    #         'data': get_host_power_median_data(hosts, start_ts, end_ts)
    #     })

    return result


def sum_host_usage_data(start_ts, end_ts, zoom_level, hosts, gencfg_signal):
    table_name = "instanceusage_aggregated_{}".format(zoom_level)
    field = gencfg_signal.instance_signal_name
    result = [{
        'name': 'Total host usage',
        'type': 'line',
        'data': get_host_usage_data_for_list(hosts, field, table_name, start_ts, end_ts)
    }]

    if gencfg_signal.name == EGencfgSignals.CPU:
        result.append({
            'name': 'Total host power',
            'type': 'line',
            'data': get_host_power_data_for_list(hosts, start_ts, end_ts, zoom_level)
        })

    return result


def host_usage_distribution_data(start_ts, end_ts, zoom_level, hosts, resource):
    table_name = "instanceusage_aggregated_{}".format(zoom_level)
    data = prepare_and_run_host_distribution_query(table_name, start_ts, end_ts, resource, hosts)
    return format_lines_for_distribution_graph(data, line_base_description="Hosts")


def group_graph_data(start_ts, end_ts, zoom_level, graphs, gencfg_signal):
    group_instanceusage_list, group_hostusage_list, group_allocated_list = get_usage_lists(graphs, gencfg_signal)

    actual_usage_stats = dict()

    table_name = "instanceusage_aggregated_{}".format(zoom_level)
    table_signal_names = {x[0] for x in group_instanceusage_list}
    for table_signal_name in table_signal_names:
        graph_name_by_group_name = {tuple(x[2]):x[1] for x in group_instanceusage_list if x[0] == table_signal_name}
        groups = graph_name_by_group_name.keys()
        signal_name = [x for x in group_instanceusage_list if x[0] == table_signal_name][0][1]

        iu_query = """
            SELECT
                ts,
                group_names,
                sum({signal_name}) as sum_instance_usage
            FROM {table_name}
            ALL INNER JOIN (
                {helper_subquery}
            ) USING group
            WHERE (group IN {flattened_group_list}) AND (ts >= {ts1}) AND (ts <= {ts2})
                  AND (eventDate >= '{ed1}') AND (eventDate <= '{ed2}') {extra_filter_conditions}
            GROUP BY ts, group_names
        """.format(
            flattened_group_list=get_in_statement_list_for_groups(groups),
            helper_subquery=_generate_arrayjoin_helper_subquery(groups, "i"),
            table_name=table_name, ts1=start_ts, ts2=end_ts,
            ed1=get_ch_formatted_date_from_timestamp(start_ts),
            ed2=get_ch_formatted_date_from_timestamp(end_ts),
            signal_name=table_signal_name,
            extra_filter_conditions=gencfg_signal.extra_filter_conditions,
        )

        actual_usage_query = """ SELECT ts, group_names, sum_instance_usage
                                 FROM ({})
                                 ORDER BY ts, group_names
                             """.format(iu_query)

        actual_usage_data = run_query(actual_usage_query)

        for row_data in actual_usage_data:
            ts, group_names_id, signal_value = row_data
            signal_value = gencfg_signal.units_convert_func(float(signal_value))
            ts = int(ts) * 1000
            # group_names = tuple(groups)  # FIXME

            group_names = groups[int(group_names_id[1:])]

            if group_names not in actual_usage_stats:
                actual_usage_stats[group_names] = dict()
            if ts not in actual_usage_stats[group_names]:
                actual_usage_stats[group_names][ts] = dict()

            actual_usage_stats[group_names][ts][signal_name] = signal_value

    table_name = "instanceusage_aggregated_{}".format(zoom_level)
    table_signal_names = {x[0] for x in group_hostusage_list}
    for table_signal_name in table_signal_names:
        graph_name_by_group_name = {tuple(x[2]):x[1] for x in group_hostusage_list if x[0] == table_signal_name}
        groups = graph_name_by_group_name.keys()
        signal_name = [x for x in group_hostusage_list if x[0] == table_signal_name][0][1]

        hu_query = """
            SELECT
                ts,
                group_names,
                sum(host_usage) as sum_host_usage
            FROM
            (
                SELECT DISTINCT
                    host,
                    ts,
                    group_names,
                    {signal_name} AS host_usage
                FROM {table_name} ALL INNER JOIN
                (
                    SELECT DISTINCT
                        host,
                        group_names,
                        ts
                    FROM {table_name}
                    ALL INNER JOIN ({helper_subquery}) USING group
                    WHERE (group IN {flattened_group_list}) AND (ts >= {ts1}) AND (ts <= {ts2}) AND (eventDate >= '{ed1}')
                          AND (eventDate <= '{ed2}') {extra_filter_conditions}
                ) USING host, ts
                WHERE (ts >= {ts1}) AND (ts <= {ts2}) AND (eventDate >= '{ed1}') AND (eventDate <= '{ed2}') AND (port = 65535)
            )
            GROUP BY ts, group_names
        """.format(
            flattened_group_list=get_in_statement_list_for_groups(groups),
            helper_subquery=_generate_arrayjoin_helper_subquery(groups, "h"),
            table_name=table_name, ts1=start_ts, ts2=end_ts,
            ed1=get_ch_formatted_date_from_timestamp(start_ts),
            ed2=get_ch_formatted_date_from_timestamp(end_ts),
            signal_name=table_signal_name,
            extra_filter_conditions=gencfg_signal.extra_filter_conditions,
        )

        actual_usage_query = """ SELECT ts, group_names, sum_host_usage
                                 FROM ({})
                                 ORDER BY ts, group_names
                             """.format(hu_query)

        actual_usage_data = run_query(actual_usage_query)

        for row_data in actual_usage_data:
            ts, group_names_id, signal_value = row_data
            signal_value = gencfg_signal.units_convert_func(float(signal_value))
            ts = int(ts) * 1000

            group_names = groups[int(group_names_id[1:])]

            if group_names not in actual_usage_stats:
                actual_usage_stats[group_names] = dict()
            if ts not in actual_usage_stats[group_names]:
                actual_usage_stats[group_names][ts] = dict()

            actual_usage_stats[group_names][ts][signal_name] = signal_value

    allocated_stats = {}
    allocated_group_name_indexes = {}
    table_signal_names = {x[0] for x in group_allocated_list}
    for table_signal_name in table_signal_names:
        filtered_list = [x for x in group_allocated_list if x[0] == table_signal_name]
        signal_name = filtered_list[0][1]
        filtered_list = [x[2] for x in filtered_list]

        for idx, lst in enumerate(filtered_list):
            allocated_group_name_indexes[tuple(lst)] = 'a{}'.format(idx)

        allocated_stats[signal_name] = get_group_allocated_data(start_ts, end_ts, gencfg_signal, filtered_list, alloc_column_name=table_signal_name)

    result = []

    for graph_param in graphs:
        signal = graph_param['signal']

        is_actual_usage = (signal not in allocated_stats)
        if is_actual_usage:
            is_instance_signal = signal.startswith('instance')
            if is_instance_signal:
                getter = itemgetter(0, 1)
                name_prefix = "i"
                usage_list = group_instanceusage_list
            else:
                getter = itemgetter(0, 2)
                name_prefix = "h"
                usage_list = group_hostusage_list

            group_names = _get_groups_sum_name(graph_param['group'])
            if isinstance(graph_param['group'], (tuple, list)):
                group_names_id = tuple(graph_param['group'])
            else:
                group_names_id = tuple([graph_param['group']])

            data = []
            for ts in sorted(actual_usage_stats.get(group_names_id, [])):
                data.append((ts, actual_usage_stats[group_names_id][ts][signal]))
            data.append((end_ts * 1000, None))

            result.append({
                'name': graph_param.get('graph_name', group_names + (" instances" if is_instance_signal else " hosts")),
                'type': 'line' if not graph_param['stacking'] else 'area',
                'data': data,
            })
        else:
            group_names = _get_groups_sum_name(graph_param['group'])
            if isinstance(graph_param['group'], list):
                group_names_index = allocated_group_name_indexes[tuple(graph_param['group'])]
            else:
                group_names_index = allocated_group_name_indexes[tuple([graph_param['group']])]

            if not group_hostusage_list and not group_instanceusage_list:
                data = allocated_stats[signal][group_names_index]
            else:
                data = align_stats_to_period(allocated_stats[signal][group_names_index],
                                             start_ts, end_ts,
                                             zoom_level_to_period(zoom_level),
                                             zoom_level_to_period_start_adjustment(zoom_level))

            result.append({
                'name': graph_param.get('graph_name', group_names + " allocated"),
                'type': 'line' if not graph_param['stacking'] else 'area',
                'data': data
            })

    graph_weights = dict()
    for item in result:
        graph_weights[item['name']] = sum(filter(None, map(itemgetter(1), item['data'])))

    result.sort(key=lambda item: graph_weights[item['name']], reverse=True)

    return result


@app.route("/group_memory_graph_data")
def group_memory_graph_data():
    args = get_historical_graph_params() + [GENCFG_SIGNALS[EGencfgSignals.MEM]]
    return jsonify(group_graph_data(*args))


@app.route("/group_cpu_graph_data")
def group_cpu_graph_data():
    args = get_historical_graph_params() + [GENCFG_SIGNALS[EGencfgSignals.CPU]]
    return jsonify(group_graph_data(*args))


@app.route("/group_net_rx_graph_data")
def group_net_rx_graph_data():
    args = get_historical_graph_params() + [GENCFG_SIGNALS[EGencfgSignals.NET_RX]]
    return jsonify(group_graph_data(*args))


@app.route("/group_net_tx_graph_data")
def group_net_tx_graph_data():
    args = get_historical_graph_params() + [GENCFG_SIGNALS[EGencfgSignals.NET_TX]]
    return jsonify(group_graph_data(*args))


@app.route("/group_graph")
def group_graph():
    graph_title = request.args.get('graph_title', '')

    params_str = request.args.get('params')
    params_json = json.loads(urllib.unquote(params_str))

    signal = detect_signal_type(params_json)
    graph_url = url_for(signal.group_data_endpoint, params=params_str)

    return render_template('graph.html', signal=signal, graph_url=graph_url, graph_title=graph_title)


def group_distribution_graph_data(start_ts, end_ts, zoom_level, group, gencfg_signal, abstract_cores=False):
    abstract_core_power_units = 40

    if zoom_level == "auto":
        zoom_level = calculate_zoom_level(start_ts, end_ts, max_points=2500)

    table_name = "instanceusage_aggregated_{}".format(zoom_level)

    query = """
        SELECT ts, median({usage_field}),
               quantile(0.15)({usage_field}), quantile(0.85)({usage_field}),
               quantile(0.05)({usage_field}), quantile(0.95)({usage_field})
        FROM {table_name}
        WHERE ts >= {ts1} AND ts <= {ts2} AND (eventDate >= '{ed1}') AND (eventDate <= '{ed2}')
              AND group='{group}' {extra_filter_conditions}
        GROUP BY ts
        ORDER BY ts
        """.format(
            table_name=table_name, ts1=start_ts, ts2=end_ts,
            ed1=get_ch_formatted_date_from_timestamp(start_ts),
            ed2=get_ch_formatted_date_from_timestamp(end_ts),
            group=group,
            usage_field=gencfg_signal.instance_signal_name,
            extra_filter_conditions=gencfg_signal.extra_filter_conditions,
        )

    data = run_query(query)

    if abstract_cores:
        convert_func = lambda x: x / 40.
    else:
        convert_func = gencfg_signal.units_convert_func

    return format_lines_for_distribution_graph(data, line_base_description=group, convert_func=convert_func)


def format_lines_for_distribution_graph(data, line_base_description, convert_func=None):
    if convert_func is None:
        convert_func = lambda x: x

    meds = []
    ranges = []
    ranges_wider = []
    for row in data:
        ts, med, qlow, qhigh, qlow2, qhigh2 = row
        ts = int(ts) * 1000

        med = convert_func(float(med))
        qlow = convert_func(float(qlow))
        qhigh = convert_func(float(qhigh))
        qlow2 = convert_func(float(qlow2))
        qhigh2 = convert_func(float(qhigh2))
        meds.append((ts, med))
        ranges.append((ts, qlow, qhigh))
        ranges_wider.append((ts, qlow2, qhigh2))

    return [{
        'name': '{}: median'.format(line_base_description),
        'data': meds,
        'zIndex': 1,
        'color': '#555555'
    }, {
        'name': '{}: 70% of instances'.format(line_base_description),
        'data': ranges,
        'type': 'arearange',
        'lineWidth': 0,
        'color': '#777777',
        'fillOpacity': 0.7,
        'zIndex': 0
    }, {
        'name': '{}: 90% of instances'.format(line_base_description),
        'data': ranges_wider,
        'type': 'arearange',
        'lineWidth': 0,
        'color': '#DDDDDD',
        'fillOpacity': 0.5,
        'zIndex': 0
    }]


def get_average_allocated_stats_per_instance(start_ts, end_ts, zoom_level, group, gencfg_signal, abstract_cores=False):
    if abstract_cores:
        expression = "cpu_guarantee / n_instances / 40"
    else:
        expression = '{} / n_instances'.format(gencfg_signal.group_allocated_signal_name)

    query = """
        SELECT ts, {expression} as res
        FROM group_allocated_resources
        WHERE group='{group}'
        ORDER BY ts
        FORMAT JSON
    """.format(expression=expression, group=group)

    data = [(row['ts'] * 1000, gencfg_signal.units_convert_func(1. if row['res'] is None else float(row['res']))) for row in run_query_format_json(query)['data']]

    return data


def instance_usage_distribution_vs_allocated_data(start_ts, end_ts, zoom_level, group, gencfg_signal, abstract_cores=False):
    if zoom_level == "auto":
        zoom_level = calculate_zoom_level(start_ts, end_ts, max_points=2500)

    result = group_distribution_graph_data(start_ts, end_ts, zoom_level, group, gencfg_signal, abstract_cores=abstract_cores)

    allocated_stats = get_average_allocated_stats_per_instance(start_ts, end_ts, zoom_level, group, gencfg_signal, abstract_cores=abstract_cores)

    aligned_stats = align_stats_to_period(allocated_stats,
                                          start_ts, end_ts,
                                          zoom_level_to_period(zoom_level),
                                          zoom_level_to_period_start_adjustment(zoom_level))

    result.append({
        'data': aligned_stats,
        'name': "{} allocated".format(group),
        'type': 'line'
    })

    return result


@app.route("/instance_memory_usage_distribution_vs_allocated_data/<group>")
def instance_memory_usage_distribution_vs_allocated_data(group):
    start_ts, end_ts, zoom_level, _ = get_historical_graph_params()
    return jsonify(instance_usage_distribution_vs_allocated_data(start_ts, end_ts, zoom_level, group, GENCFG_SIGNALS[EGencfgSignals.MEM]))


@app.route("/instance_cpu_usage_distribution_vs_allocated_data/<group>", defaults={'abstract_cores': False})
@app.route("/instance_cpu_usage_distribution_vs_allocated_data/<group>/abstract_cores", defaults={'abstract_cores': True})
def instance_cpu_usage_distribution_vs_allocated_data(group, abstract_cores):
    start_ts, end_ts, zoom_level, _ = get_historical_graph_params()
    return jsonify(
        instance_usage_distribution_vs_allocated_data(start_ts, end_ts, zoom_level, group, GENCFG_SIGNALS[EGencfgSignals.CPU], abstract_cores=abstract_cores)
    )


@app.route("/instance_net_rx_usage_distribution_vs_allocated_data/<group>")
def instance_net_rx_usage_distribution_vs_allocated_data(group):
    start_ts, end_ts, zoom_level, _ = get_historical_graph_params()
    return jsonify(instance_usage_distribution_vs_allocated_data(start_ts, end_ts, zoom_level, group, GENCFG_SIGNALS[EGencfgSignals.NET_RX]))


@app.route("/instance_net_tx_usage_distribution_vs_allocated_data/<group>")
def instance_net_tx_usage_distribution_vs_allocated_data(group):
    start_ts, end_ts, zoom_level, _ = get_historical_graph_params()
    return jsonify(instance_usage_distribution_vs_allocated_data(start_ts, end_ts, zoom_level, group, GENCFG_SIGNALS[EGencfgSignals.NET_TX]))


def __distribution_ts_params():
    ts2 = int(time.time())
    # ts1 = ts2 - 24 * 60 * 60 * 210
    ts1 = 1472688000

    params = urllib.quote(json.dumps({
        'start_ts': ts1,
        'end_ts': ts2,
        'zoom_level': "auto"
    }))

    return params


@app.route("/instance_cpu_usage_distribution_vs_allocated/<group>", defaults={'abstract_cores': False})
@app.route("/instance_cpu_usage_distribution_vs_allocated/<group>/abstract_cores", defaults={'abstract_cores': True})
def instance_cpu_usage_distribution_vs_allocated(group, abstract_cores):
    params = __distribution_ts_params()

    graph_url = url_for('instance_cpu_usage_distribution_vs_allocated_data',
                            abstract_cores=abstract_cores,
                            group=group,
                            params=params)

    return render_template("graph.html", signal=GENCFG_SIGNALS[EGencfgSignals.CPU],
                           graph_url=graph_url,
                           graph_title="CPU (In cores)" if abstract_cores else "CPU (In Power Units)")


@app.route("/instance_mem_usage_distribution_vs_allocated/<group>")
def instance_mem_usage_distribution_vs_allocated(group):
    params = __distribution_ts_params()

    graph_url = url_for('instance_memory_usage_distribution_vs_allocated_data', group=group, params=params)

    return render_template("graph.html", signal=GENCFG_SIGNALS[EGencfgSignals.MEM],
                           graph_url=graph_url,
                           graph_title="Memory (In Gb)")


@app.route("/instance_net_rx_usage_distribution_vs_allocated/<group>")
def instance_net_rx_usage_distribution_vs_allocated(group):
    params = __distribution_ts_params()

    graph_url = url_for('instance_net_rx_usage_distribution_vs_allocated_data', group=group, params=params)

    return render_template("graph.html", signal=GENCFG_SIGNALS[EGencfgSignals.NET_RX],
                           graph_url=graph_url,
                           graph_title="Net RX (In Mbit/sec)")


@app.route("/instance_net_tx_usage_distribution_vs_allocated/<group>")
def instance_net_tx_usage_distribution_vs_allocated(group):
    params = __distribution_ts_params()

    graph_url = url_for('instance_net_tx_usage_distribution_vs_allocated_data', group=group, params=params)

    return render_template("graph.html", signal=GENCFG_SIGNALS[EGencfgSignals.NET_TX],
                           graph_url=graph_url,
                           graph_title="Net TX (In Mbit/sec)")


def instance_graph_data(start_ts, end_ts, zoom_level, graphs, gencfg_signal):
    if zoom_level == "auto":
        zoom_level = calculate_zoom_level(start_ts, end_ts, max_points=2500)

    table_name = "instanceusage_aggregated_{}".format(zoom_level)

    if gencfg_signal.name == 'memory_usage':
        supported_signals = ['instance_memusage', 'group_memusage_distribution']
    elif gencfg_signal.name == 'cpu_usage':
        supported_signals = ['instance_cpuusage', 'group_cpuusage_distribution']
    else:
        raise Exception('Unknown signal <{}>'.format(gencfg_signal.name))

    instances_list = []

    for graph_param in graphs:
        signal = graph_param['signal']
        assert signal in supported_signals
        if signal.startswith('group'):
            continue

        instance = (graph_param['host'], graph_param['port'])
        instances_list.append(instance)

    if instances_list:
        query = """
            SELECT host, port, ts, {usage_field}
            FROM {table_name}
            WHERE ts >= {ts1} AND ts <= {ts2} AND (eventDate >= '{ed1}') AND (eventDate <= '{ed2}')
                  AND (host, port) in {instances_list} {extra_filter_conditions}
            ORDER BY host, port, ts
            """.format(
            table_name=table_name, ts1=start_ts, ts2=end_ts,
            instances_list=get_in_statement_list_for_instances(instances_list),
            ed1=get_ch_formatted_date_from_timestamp(start_ts),
            ed2=get_ch_formatted_date_from_timestamp(end_ts),
            usage_field=gencfg_signal.instance_signal_name,
            extra_filter_conditions=gencfg_signal.extra_filter_conditions,
        )

        print "QUERY", query
        data = run_query(query)

        stats = defaultdict(list)

        for row in data:
            host, port, ts, usage = row
            ts = int(ts) * 1000
            port = int(port)
            usage = float(usage)
            stats[(host, port)].append((ts, usage))

    result = []

    for graph_param in graphs:
        if graph_param['signal'].startswith('group'):
            result = group_distribution_graph_data(start_ts, end_ts, zoom_level, graph_param['group'], gencfg_signal) + result
        else:
            instance = graph_param['host'], int(graph_param['port'])
            graph = {
                'name': graph_param.get('graph_name') or "{}:{}".format(*instance),
                'type': 'line',
                'data': stats[instance]
            }
            if 'color' in graph_param:
                graph['color'] = graph_param['color']

            result.append(graph)

    return result


@app.route("/instance_cpu_graph_data")
def instance_cpu_graph_data():
    args = get_historical_graph_params() + [GENCFG_SIGNALS[EGencfgSignals.CPU]]
    return jsonify(instance_graph_data(*args))


@app.route("/instance_memory_graph_data")
def instance_memory_graph_data():
    args = get_historical_graph_params() + [GENCFG_SIGNALS[EGencfgSignals.MEM]]
    return jsonify(instance_graph_data(*args))


@app.route("/instance_graph")
def instance_graph():
    # TODO better error handling: 400 and such (instead of asserts leading to possible 500's)
    params_str = request.args.get('params')
    params_json = json.loads(urllib.unquote(params_str))

    signal = detect_signal_type(params_json)
    graph_url = url_for(signal.group_data_endpoint, params=params_str)

    return render_template('graph.html', signal=signal, graph_url=graph_url)


@app.route("/fake_group_host_usage_from_trunk_graph/cpu/<group>")
def fake_group_cpu_host_usage_from_trunk_graph(group):
    ts2 = int(time.time())
    ts1 = 1472688000

    params = urllib.quote(json.dumps({
        'start_ts': ts1,
        'end_ts': ts2,
        'zoom_level': "auto",
    }))

    graph_url = url_for('fake_group_host_cpuusage_from_trunk_data', group=group, params=params)

    return render_template("graph.html", graph_url=graph_url, signal=GENCFG_SIGNALS[EGencfgSignals.CPU])


@app.route("/fake_group_host_usage_from_trunk_graph/mem/<group>")
def fake_group_mem_host_usage_from_trunk_graph(group):
    ts2 = int(time.time())
    ts1 = 1472688000

    params = urllib.quote(json.dumps({
        'start_ts': ts1,
        'end_ts': ts2,
        'zoom_level': "auto",
    }))

    graph_url = url_for('fake_group_host_memusage_from_trunk_data', group=group, params=params)

    return render_template("graph.html", graph_url=graph_url, signal=GENCFG_SIGNALS[EGencfgSignals.MEM])


@app.route("/fake_group_host_usage_distribution_from_trunk_graph/<group>")
def fake_group_host_usage_distribution_from_trunk_graph(group):
    ts2 = int(time.time())
    ts1 = 1472688000

    params = urllib.quote(json.dumps({
        'start_ts': ts1,
        'end_ts': ts2,
        'zoom_level': "auto",
    }))

    cpu_graph_url = url_for('fake_group_host_cpuusage_distribution_from_trunk_data',
                            group=group,
                            params=params)

    memory_graph_url = url_for('fake_group_host_memusage_distribution_from_trunk_data',
                               group=group,
                               params=params)

    return render_template("graph.html",
                           cpu_graph_url=cpu_graph_url,
                           memory_graph_url=memory_graph_url)


@app_usage_data_routes("/fake_group_host_{}usage_from_trunk_data/<group>", "fake_group_host_{}usage_from_trunk_data")
def fake_group_host_mem_and_cpu_usage_from_trunk_data(group, resource):
    start_ts, end_ts, zoom_level, _ = get_historical_graph_params()
    return jsonify(
        fake_group_host_usage_from_trunk_data(start_ts, end_ts, zoom_level, group, resource=resource)
    )


@app_usage_data_routes("/fake_group_host_{}usage_distribution_from_trunk_data/<group>", "fake_group_host_{}usage_distribution_from_trunk_data")
def fake_group_host_mem_and_cpu_usage_distribution_from_trunk_data(group, resource):
    start_ts, end_ts, zoom_level, _ = get_historical_graph_params()
    return jsonify(fake_group_host_usage_distribution_from_trunk_data(start_ts, end_ts, zoom_level, group, resource=resource))


def parse_comma_delimited_hosts(comma_delimited_hosts):
    unquoted_hosts = urllib.unquote(comma_delimited_hosts)
    return [host.strip() for host in unquoted_hosts.split(',')]


@app.route("/sum_host_memusage_data/<comma_delimited_hosts>")
def sum_host_memusage_data(comma_delimited_hosts):
    start_ts, end_ts, zoom_level, _ = get_historical_graph_params()
    hosts = parse_comma_delimited_hosts(comma_delimited_hosts)
    return jsonify(sum_host_usage_data(start_ts, end_ts, zoom_level, hosts, GENCFG_SIGNALS[EGencfgSignals.MEM]))


@app.route("/sum_host_cpuusage_data/<comma_delimited_hosts>")
def sum_host_cpuusage_data(comma_delimited_hosts):
    start_ts, end_ts, zoom_level, _ = get_historical_graph_params()
    hosts = parse_comma_delimited_hosts(comma_delimited_hosts)
    return jsonify(
        sum_host_usage_data(start_ts, end_ts, zoom_level, hosts, GENCFG_SIGNALS[EGencfgSignals.CPU])
    )


@app_usage_data_routes("/host_{}usage_distribution_data/<comma_delimited_hosts>", "host_{}usage_distribution_data")
def host_cpu_and_mem_usage_distribution_data(comma_delimited_hosts, resource):
    start_ts, end_ts, zoom_level, _ = get_historical_graph_params()
    hosts = parse_comma_delimited_hosts(comma_delimited_hosts)
    return jsonify(
        host_usage_distribution_data(start_ts, end_ts, zoom_level, hosts, resource=resource)
    )


@app.route('/qloud/usage/memory')
def qloud_memory_usage():
    return qloud_usage_graph(section=ESections.QLOUD, signal_name='mem_usage', signal_description='Qloud services memory usage (in Gb)')


@app.route('/qloud/usage/cpu')
def qloud_cpu_usage():
    if 'format' in request.args and request.args['format'] == 'cores':
        signal_description = 'Qloud services cpu usage (in abstract cores)'
    else:
        signal_description = 'Qloud services cpu usage (in cpu power units)'
    return qloud_usage_graph(section=ESections.QLOUD, signal_name='cpu_usage', signal_description=signal_description)


@app.route('/qloud/usage/net_rx')
def qloud_net_rx_usage():
    return qloud_usage_graph(section=ESections.QLOUD, signal_name='net_rx', signal_description='Qloud services Net RX (in Mbit/sec)')


@app.route('/qloud/usage/net_tx')
def qloud_net_tx_usage():
    return qloud_usage_graph(section=ESections.QLOUD, signal_name='net_tx', signal_description='Qloud services Net TX (in Mbit/sec)')


@app.route('/openstack/usage/memory')
def openstack_memory_usage():
    return qloud_usage_graph(section=ESections.OPENSTACK, signal_name='mem_usage', signal_description='Openstack services memory usage (in Gb)')


@app.route('/openstack/usage/cpu')
def openstack_cpu_usage():
    if 'format' in request.args and request.args['format'] == 'cores':
        signal_description = 'Openstack services cpu usage (in abstract cores)'
    else:
        signal_description = 'Openstack services cpu usage (in cpu power units)'
    return qloud_usage_graph(section=ESections.OPENSTACK, signal_name='cpu_usage', signal_description=signal_description)


@app.route('/abc/usage/memory')
def abc_memory_usage():
    return qloud_usage_graph(section=ESections.ABC, signal_name='memory_usage', signal_description='Abc services memory usage (in Gb)')


@app.route('/abc/usage/cpu')
def abc_cpu_usage():
    return qloud_usage_graph(section=ESections.ABC, signal_name='cpu_usage', signal_description='Abc services cpu usage (in cpu power units)')


@app.route('/abc/usage/hdd')
def abc_hdd_usage():
    return qloud_usage_graph(section=ESections.ABC, signal_name='hdd_usage', signal_description='Abc services hdd usage (in Gb)')


@app.route('/abc/usage/ssd')
def abc_ssd_usage():
    return qloud_usage_graph(section=ESections.ABC, signal_name='ssd_usage', signal_description='Abc services ssd usage (in Gb)')


@app.route('/abc/usage/count')
def abc_count_usage():
    return qloud_usage_graph(section=ESections.ABC, signal_name='hosts_with_stats', signal_description='Abc services hosts count')


@app.route('/metaprj/usage/memory')
def metaprj_memory_usage():
    return qloud_usage_graph(section=ESections.METAPRJ, signal_name='memory_usage', signal_description='Metaprj memory usage (in Gb)')


@app.route('/metaprj/usage/cpu')
def metaprj_cpu_usage():
    return qloud_usage_graph(section=ESections.METAPRJ, signal_name='cpu_usage', signal_description='Metaprj cpu usage (in cpu power units)')


@app.route('/yp/usage/memory')
def yp_memory_usage():
    return qloud_usage_graph(section=ESections.YP, signal_name='memory_usage', signal_description='Yp memory usage (in Gb)')


@app.route('/yp/usage/cpu')
def yp_cpu_usage():
    return qloud_usage_graph(section=ESections.YP, signal_name='cpu_usage', signal_description='Yp cpu usage (in cpu power units)')


def qloud_usage_graph(section=None, signal_name=None, signal_description=None):
    assert (section is not None) and (signal_name is not None) and (signal_description is not None)

    if signal_name == 'mem_usage':
        endpoint = '{}_memory_usage_data'.format(section)
    elif signal_name == 'cpu_usage':
        endpoint = '{}_cpu_usage_data'.format(section)
    elif signal_name == 'memory_usage':
        endpoint = '{}_memory_usage_data'.format(section)
    elif signal_name == 'hdd_usage':
        endpoint = '{}_hdd_usage_data'.format(section)
    elif signal_name == 'ssd_usage':
        endpoint = '{}_ssd_usage_data'.format(section)
    elif signal_name == 'hosts_with_stats':
        endpoint = '{}_count_usage_data'.format(section)
    elif signal_name == 'net_rx':
        endpoint = '{}_net_rx_usage_data'.format(section)
    elif signal_name == 'net_tx':
        endpoint = '{}_net_tx_usage_data'.format(section)
    else:
        raise Exception('Unknown signal <{}>'.format(signal_name))

    params_str = request.args.get('params')
    graph_base_url = url_for(endpoint, params=params_str)

    where_data, group_by_node_type = qloud_extract_node_params(section, request.args)
    if where_data:
        extra_cgi = ['{}={}'.format(k, urllib.quote(v)) for k, v in where_data]
        extra_signal_description = 'having {}, '.format(' '.join(extra_cgi))
        extra_cgi = '&'.join(extra_cgi)
    else:
        extra_cgi = ''
        extra_signal_description = ''
    if group_by_node_type is not None:
        extra_signal_description = '{}group by {}'.format(extra_signal_description, group_by_node_type)

    if 'hosts' in request.args:
        extra_cgi = '{}&hosts={}'.format(extra_cgi, request.args['hosts'])
    if 'format' in request.args:
        extra_cgi = '{}&format={}'.format(extra_cgi, request.args['format'])

    signal_description = '{} ({})'.format(signal_description, extra_signal_description)

    return render_template('qloud_graph.html', graph_base_url=graph_base_url, signal_name=signal_name, extra_cgi=extra_cgi, signal_description=signal_description,
                           height='800px')


@app.route('/qloud/usage/memory/data')
def qloud_memory_usage_data():
    return jsonify(qloud_usage_data(ESections.QLOUD, 'mem_usage'))


@app.route('/qloud/usage/cpu/data')
def qloud_cpu_usage_data():
    if ('format' in request.args) and (request.args['format'] == 'cores'):
        signal_convert_func = lambda x: x / 40
    else:
        signal_convert_func = lambda x: x

    return jsonify(qloud_usage_data(ESections.QLOUD, 'cpu_usage_power_units', signal_convert_func=signal_convert_func))


@app.route('/qloud/usage/net_rx/data')
def qloud_net_rx_usage_data():
    return jsonify(qloud_usage_data(ESections.QLOUD, 'net_rx', signal_convert_func=lambda x: x / 1024 / 1024 * 8))


@app.route('/qloud/usage/net_tx/data')
def qloud_net_tx_usage_data():
    return jsonify(qloud_usage_data(ESections.QLOUD, 'net_tx', signal_convert_func=lambda x: x / 1024 / 1024 * 8))


@app.route('/openstack/usage/memory/data')
def openstack_memory_usage_data():
    return jsonify(qloud_usage_data(ESections.OPENSTACK, 'mem_usage'))



@app.route('/openstack/usage/cpu/data')
def openstack_cpu_usage_data():
    if ('format' in request.args) and (request.args['format'] == 'cores'):
        signal_convert_func = lambda x: x / 40
    else:
        signal_convert_func = lambda x: x

    return jsonify(qloud_usage_data(ESections.OPENSTACK, 'cpu_usage_power_units', signal_convert_func=signal_convert_func))


@app.route('/abc/usage/memory/data')
def abc_memory_usage_data():
    return jsonify(qloud_usage_data(ESections.ABC, 'memory_usage', signal_convert_func=convert_abc_signal_to_gb))


@app.route('/abc/usage/cpu/data')
def abc_cpu_usage_data():
    return jsonify(qloud_usage_data(ESections.ABC, 'cpu_usage'))


@app.route('/abc/usage/hdd/data')
def abc_hdd_usage_data():
    return jsonify(qloud_usage_data(ESections.ABC, 'hdd_usage', signal_convert_func=convert_abc_signal_to_gb))


@app.route('/abc/usage/ssd/data')
def abc_ssd_usage_data():
    return jsonify(qloud_usage_data(ESections.ABC, 'ssd_usage', signal_convert_func=convert_abc_signal_to_gb))


@app.route('/abc/usage/count/data')
def abc_count_usage_data():
    return jsonify(qloud_usage_data(ESections.ABC, 'hosts_with_stats'))


@app.route('/metaprj/usage/memory/data')
def metaprj_memory_usage_data():
    return jsonify(qloud_usage_data(ESections.METAPRJ, 'memory_usage'))


@app.route('/metaprj/usage/cpu/data')
def metaprj_cpu_usage_data():
    usage_graph = qloud_usage_data(ESections.METAPRJ, 'cpu_usage')
    allocated_graph = qloud_usage_data(ESections.METAPRJ, 'cpu_allocated')

    return jsonify(qloud_usage_data(ESections.METAPRJ, 'cpu_usage'))


@app.route('/yp/usage/memory/data')
def yp_memory_usage_data():
    return jsonify(qloud_usage_data(ESections.YP, 'instance_memusage'))


@app.route('/yp/usage/cpu/data')
def yp_cpu_usage_data():
    if ('format' in request.args) and (request.args['format'] == 'cores'):
        signal_convert_func = lambda x: x / 40
    else:
        signal_convert_func = lambda x: x

    return jsonify(qloud_usage_data(ESections.YP, 'instance_cpuusage_power_units', signal_convert_func=signal_convert_func))


def qloud_usage_data(section, signal_name, signal_convert_func=lambda x: x):
    """Main function to extract data on usage for qloud services

    :param signal_convert_func: python function, used to convert for example gencfg power units to cores"""

    # convert params from CGI
    start_ts = int(request.args.get('start_ts', 0))
    end_ts = int(request.args.get('end_ts', int(time.time())))
    start_ts_event_date = get_ch_formatted_date_from_timestamp(start_ts)
    end_ts_event_date = get_ch_formatted_date_from_timestamp(end_ts)
    zoom_level = request.args.get('zoom_level', request.args.get('zoom_level', 'auto'))
    if zoom_level == "auto":
        zoom_level = calculate_zoom_level(start_ts, end_ts)

    # get graph-specific params
    where_data, group_by_node_type = qloud_extract_node_params(section, request.args)
    if where_data:
        node_type_where_suffix = ["{} = '{}'".format(k, v.replace("'", "\\'")) for k, v in where_data]
        node_type_where_suffix = ' AND '.join(node_type_where_suffix)
        node_type_where_suffix = ' AND {}'.format(node_type_where_suffix)
    else:
        node_type_where_suffix = ''
    if group_by_node_type is None:
        group_by_node_type = where_data[-1][0]

    hosts = request.args.get('hosts', None)
    if hosts is not None:
        hosts = hosts.split(',')
        hosts_where_suffix = " AND host in ({})".format(",".join("'{}'".format(x) for x in hosts))
    else:
        hosts_where_suffix = ""

    # calculate table
    if section == ESections.QLOUD:
        table_prefix = 'qloudusage_aggregated'
    elif section == ESections.OPENSTACK:
        table_prefix = 'openstackusage_aggregated'
    elif section == ESections.ABC:
        table_prefix = 'abcusage_aggregated'
    elif section == ESections.METAPRJ:
        table_prefix = 'metaprjusage_aggregated'
    elif section == ESections.YP:
        table_prefix = 'ypusage_aggregated'
    else:
        raise Exception('Unsupported section <{}>'.format(section))


    # generate selector query
    query_tpl = ("SELECT {group_by_node_type}, ts, sum({signal_name}) FROM {table_prefix}_{zoom_level} WHERE\n"
                 "    (ts >= {start_ts}) AND\n"
                 "    (ts <= {end_ts}) AND\n"
                 "    (eventDate >= '{start_ts_event_date}') AND\n"
                 "    (eventDate <= '{end_ts_event_date}')\n"
                 "    {hosts_where_suffix}\n"
                 "    {node_type_where_suffix}\n"
                 "    GROUP BY {group_by_node_type}, ts")

    query = query_tpl.format(signal_name=signal_name, zoom_level=zoom_level, start_ts=start_ts, end_ts=end_ts,
                             start_ts_event_date=start_ts_event_date, end_ts_event_date=end_ts_event_date,
                             hosts_where_suffix=hosts_where_suffix, group_by_node_type=group_by_node_type,
                             node_type_where_suffix=node_type_where_suffix, table_prefix=table_prefix)


    print query.replace('\n', ' ')

    query_data = run_query(query)

    print query_data[:10]

    stats = defaultdict(list)
    total_usage = defaultdict(int)
    for row in query_data:
        ts = int(row[-2]) * 1000
        usage = signal_convert_func(float(row[-1]))
        usage = max(usage, 0.0)
        service_name = '/'.join(row[:-2])
        stats[service_name].append((ts, usage))
        total_usage[service_name] += usage

    # convert result to hicharts
    result = []

    for service_name in sorted(stats.keys(), key=lambda g: total_usage[g], reverse=True):
        stats[service_name].sort(key = lambda (x, y): x)

        # some adjustments for abc graphs
        if service_name == '-' and section == ESections.ABC:
            service_name_str = ' > '.join(x[1] for x in where_data) + ' <b>used</b>'
        elif section == ESections.OPENSTACK:
            service_name_str = OPENSTACK_PRJS.get(service_name, service_name)
        else:
            service_name_str = service_name

        graph = {
            'name': service_name_str,
            'type': 'area',
            'data': stats[service_name],
        }

        result.append(graph)

    # ============================================ QLOUD-2364 START ===================================================
    if section == ESections.QLOUD:
        # =========================================== https://st.yandex-team.ru/RX-361#1530554230000 START =================================
        if stats:
            total_usage_by_ts = defaultdict(float)
            for service_stats in stats.itervalues():
                for ts, usage in service_stats:
                    total_usage_by_ts[ts] += usage
            total_usage = [(x, y) for x, y in total_usage_by_ts.iteritems()]
            total_usage.sort(key=lambda (x, y): x)

            graph = {
                'name': 'total',
                'type': 'line',
                'color': '#FFFFFF',
                'data': total_usage,
            }

            result.insert(0, graph)
        # =========================================== https://st.yandex-team.ru/RX-361#1530554230000 FINISH =================================

        if len(where_data):
            qloud_group_name = ','.join('{}={}'.format(x, y) for x, y in where_data)
            qloud_group_name = 'qloud:{}'.format(qloud_group_name)
        else:
            qloud_group_name = 'qloud:'

        if signal_name == 'mem_usage':
            gencfg_signal = GENCFG_SIGNALS[EGencfgSignals.MEM]
        elif signal_name == 'net_rx':
            gencfg_signal = GENCFG_SIGNALS[EGencfgSignals.NET_RX]
        elif signal_name == 'net_tx':
            gencfg_signal = GENCFG_SIGNALS[EGencfgSignals.NET_TX]
        else:
            gencfg_signal = GENCFG_SIGNALS[EGencfgSignals.CPU]

        allocated_data = get_group_allocated_data(start_ts, end_ts, gencfg_signal, [[qloud_group_name]])
        allocated_data = allocated_data['a0']
        allocated_data = [(ts, signal_convert_func(signal_value)) for ts, signal_value in allocated_data]

        if signal_name == 'cpu_usage_power_units': # we use <guarantee> signal from qloud when must use <limit> signal (RX-361#1528131079000)
            border_ts = 1533731160 * 1000
            allocated_data = [(ts, signal_value*2) if ts < border_ts else (ts, signal_value) for ts, signal_value in allocated_data]

        if len(allocated_data):
            allocated_data = align_stats_to_period(allocated_data, start_ts, end_ts, zoom_level_to_period(zoom_level),
                                                   zoom_level_to_period_start_adjustment(zoom_level))
            graph = {
                'name': '<{}> allocated'.format(qloud_group_name),
                'type': 'line',
                'color' : 'blue',
                'data': allocated_data,
            }
            result.append(graph)
    elif section in (ESections.ABC, ESections.METAPRJ, ESections.YP,):
        if section == ESections.ABC:
            if signal_name == 'hosts_with_stats':
                alloc_signal_name = 'hosts_total'
            else:
                alloc_signal_name = signal_name.replace('usage', 'total')
        elif section in (ESections.METAPRJ, ESections.YP,):
            alloc_signal_name = signal_name.replace('usage', 'allocated')

        else:
            raise Exception('Unknown section {}'.format(section))

        query_tpl = ("SELECT ts, sum({alloc_signal_name}) FROM {table_prefix}_{zoom_level} WHERE\n"
                     "    (ts >= {start_ts}) AND\n"
                     "    (ts <= {end_ts}) AND\n"
                     "    (eventDate >= '{start_ts_event_date}') AND\n"
                     "    (eventDate <= '{end_ts_event_date}')\n"
                     "    {node_type_where_suffix}\n"
                     "    GROUP BY ts")
        query = query_tpl.format(alloc_signal_name=alloc_signal_name, zoom_level=zoom_level, start_ts=start_ts, end_ts=end_ts,
                                 start_ts_event_date=start_ts_event_date, end_ts_event_date=end_ts_event_date,
                                 node_type_where_suffix=node_type_where_suffix, table_prefix=table_prefix)
        allocated_data = run_query(query)
        allocated_data = [(int(ts) * 1000, signal_convert_func(float(value))) for ts, value in allocated_data]
        allocated_data.sort(key = lambda (ts, value): ts)
        if len(allocated_data):
            allocated_data = align_stats_to_period(allocated_data, start_ts, end_ts, zoom_level_to_period(zoom_level),
                                                   zoom_level_to_period_start_adjustment(zoom_level))

            abc_group_name = ' > '.join(x[1] for x in where_data)
            graph = {
                'name': '{} <b>allocated</b>'.format(abc_group_name),
                'type': 'line',
                'color' : 'blue',
                'data': allocated_data,
            }
            result.append(graph)
    # ============================================ QLOUD-2364 FINISH ==================================================

    return result


def qloud_extract_node_params(section, params):
    if section == ESections.QLOUD:
        node_types = QLOUD_NODE_TYPES
    elif section == ESections.OPENSTACK:
        node_types = OPENSTACK_NODE_TYPES
    elif section == ESections.ABC:
        node_types = ABC_NODE_TYPES
    elif section == ESections.METAPRJ:
        node_types = METAPRJ_NODE_TYPES
    elif section == ESections.YP:
        node_types = YP_NODE_TYPES
    else:
        raise Exception('Unsupported section <{}>'.format(section))

    where_data = []
    if node_types[0] in params:
        value = params[node_types[0]]
        if (section == ESections.OPENSTACK):
            openstack_prjs_rev = dict((y, x) for x, y in OPENSTACK_PRJS.iteritems())
            value = openstack_prjs_rev.get(value, value)

        where_data.append((node_types[0], value))
    for i in range(1, len(node_types)):
        node_type = node_types[i]
        parent_node_type = node_types[i-1]

        if node_type in params:
            if parent_node_type in params:
                where_data.append((node_type, params[node_type]))
            else:
                raise Exception('Missing <{}> parent node type when specifying <{}>'.format(parent_node_type, node_type))

    if len(where_data) < len(node_types):
        group_by_node_type = node_types[len(where_data)]
    else:
        group_by_node_type = None

    return where_data, group_by_node_type

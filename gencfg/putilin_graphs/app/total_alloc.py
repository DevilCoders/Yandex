import datetime
import time
import urllib
import json
from collections import defaultdict

import requests
import util

from flask import render_template, url_for, jsonify

from core.query import run_query

from . import app, cache_manager

lines_timeout = None  # store forever
deps_timeout = 60 * 60 * 24 * 3  # store for a few days


@cache_manager.memoize(key_prefix="get_total_alloc_and_usage_gencfg_stuff_new", timeout=7 * 24 * 60 * 60)
def get_gencfg_stuff():
    timer = util.Timer("get_gencfg_stuff")

    r_hosts_data = requests.get("http://api.gencfg.yandex-team.ru/unstable/hosts_data")
    r_hosts_data.raise_for_status()

    timer.write("r_hosts_data")

    r_cards = requests.get("http://api.gencfg.yandex-team.ru/unstable/groups_cards")
    r_cards.raise_for_status()

    timer.write("r_cards")

    host_to_available_resources = {}
    for row in r_hosts_data.json()['hosts_data']:
        host = row['name'] + row['domain']
        cpu = row['power']
        mem = row['botmem']

        host_to_available_resources[host] = {'cpu': cpu, 'mem': mem}

    cards = {}
    for card in r_cards.json()['groups_cards']:
        cards[card['name']] = card

    res = {
        "host_to_available_resources": host_to_available_resources,
        "cards": cards
    }

    timer.write("rest")
    timer.write()

    return res


@cache_manager.memoize(key_prefix="total_alloc.get_host_to_power", timeout=None, exclude_from_automatic_refresh=True)
def get_host_to_power(event_start, event_end):
    host_to_power_query = """
        SELECT host, max(cpu_power)
        FROM host_resources
        WHERE eventDate >= '{event_start}' AND eventDate <= '{event_end}'
        GROUP BY host
    """.format(event_start=event_start, event_end=event_end)
    host_to_power = {host: float(power) for host, power in run_query(host_to_power_query)}

    return host_to_power


@cache_manager.memoize(key_prefix="total_alloc.get_group_hosts", timeout=None, exclude_from_automatic_refresh=True)
def get_group_hosts(event_start, event_end):
    group_host_query = """
        SELECT DISTINCT group, host
        FROM instanceusage_aggregated_1d
        WHERE group != '' AND eventDate >= '{event_start}' AND eventDate <= '{event_end}'
    """.format(event_start=event_start, event_end=event_end)

    group_hosts = {}
    for group, host in run_query(group_host_query):
        group_hosts.setdefault(group, []).append(host)

    return group_hosts


def _make_dict_group_to_cpu_and_mem(rows):
    result = {}
    for row in rows:
        group = row[0]
        cpu = float(row[1])
        mem = float(row[2])
        result[group] = {'cpu': cpu, 'mem': mem}

    return result


@cache_manager.memoize(key_prefix="total_alloc.get_alloc_stats", timeout=None, exclude_from_automatic_refresh=True)
def get_alloc_stats(event_start, event_end):
    alloc_query = """
        SELECT group, max(cpu_guarantee), max(memory_guarantee)
        FROM group_allocated_resources
        WHERE eventDate=(
            SELECT max(eventDate) FROM group_allocated_resources WHERE eventDate <= '{event_end}'
        )
        GROUP BY group
    """.format(event_end=event_end)

    alloc_data = run_query(alloc_query, shuffle=True)
    return _make_dict_group_to_cpu_and_mem(alloc_data)


@cache_manager.memoize(key_prefix="total_alloc.get_usage_stats", timeout=None, exclude_from_automatic_refresh=True)
def get_usage_stats(event_start, event_end):
    usage_query = """
        SELECT group, quantile(0.99)(sum_cpu), quantile(0.99)(sum_mem) FROM (
            SELECT group, sum(instance_cpuusage_power_units) as sum_cpu, sum(instance_memusage) as sum_mem
            FROM instanceusage_aggregated_2m WHERE group != '' AND eventDate >= '{event_start}' AND eventDate <= '{event_end}' GROUP BY group, ts
        )
        WHERE group != ''
        GROUP by group
    """.format(event_start=event_start, event_end=event_end)

    usage_data = run_query(usage_query, shuffle=True)
    return _make_dict_group_to_cpu_and_mem(usage_data)


@cache_manager.memoize(key_prefix="total_alloc.get_graph_points", timeout=None)
def get_graph_points(event_start, event_end, resource="cpu", fast=False):
    timer = util.Timer("get_graph_points")

    if not fast:
        gencfg_stuff = get_gencfg_stuff()
    else:
        gencfg_stuff = {}

    timer.write("get_gencfg_stuff")

    host_to_available_resources = gencfg_stuff.get("host_to_available_resources", {})
    # TODO XXX: temporary patch, make this scheme work for memory too
    if not fast and resource == "cpu":
        host_to_power = get_host_to_power(event_start, event_end)
        for host, power in host_to_power.iteritems():
            if host in host_to_available_resources:
                host_to_available_resources[host]["cpu"] = power
            else:
                host_to_available_resources[host] = {"cpu": power}

    cards = gencfg_stuff.get("cards", {})

    usage_stats = get_usage_stats(event_start, event_end)
    timer.write("get_usage_stats")
    alloc_stats = get_alloc_stats(event_start, event_end)
    timer.write("get_alloc_stats")

    if not fast:
        group_host_data = get_group_hosts(event_start, event_end)

        timer.write("get_group_hosts")
    else:
        group_host_data = None

    usage_groups = set(usage_stats.iterkeys())
    all_groups = set(usage_stats) | set(alloc_stats)

    filtered_groups = []
    search_groups = []
    for group in all_groups:
        card = cards.get(group, {})
        props = card.get("properties", {})

        if not props.get('nonsearch'):
            search_groups.append(group)

        if not group.startswith("SAS_"):
            continue

        props = card.get("properties", {})

        if card.get("master") or props.get("fake_group") or props.get("created_from_portovm_group"):
            continue

        audit_class = card.get("audit", {}).get("cpu", {}).get("class_name")
        if audit_class in ["psi", "greedy"]:
            continue

        if props.get("full_host_group") and False:
            continue

        filtered_groups.append(group)

    sum_used = 0
    sum_clickhouse_alloc = 0
    sum_search_alloc = 0
    sum_all_alloc = 0
    sum_overused = 0

    timer.write("prepare")

    for group in filtered_groups:
        usage = usage_stats.get(group, {}).get(resource, 0)
        alloc = alloc_stats.get(group, {}).get(resource, 0)

        if group in usage_groups:
            if usage < alloc:
                sum_used += usage
            else:
                sum_used += alloc
                sum_overused += usage - alloc
            sum_clickhouse_alloc += alloc
            if group in search_groups:
                sum_search_alloc += alloc

        sum_all_alloc += alloc

    timer.write("sum")

    if not fast:
        clickhouse_hosts = set()
        search_hosts = set()

        for group, hosts in group_host_data.iteritems():
            if group not in filtered_groups:
                continue

            clickhouse_hosts |= set(hosts)
            if group in search_groups:
                search_hosts |= set(hosts)

        def sum_available_resources(hosts, resource):
            return sum(host_to_available_resources.get(host, {}).get(resource, 0) for host in hosts)

        all_power = sum_available_resources(clickhouse_hosts, resource)
        search_power = sum_available_resources(search_hosts, resource)

        timer.write("sum_power")

    else:
        all_power = search_power = 0

    timer.write()

    return {
        'sum_used': sum_used,
        'sum_clickhouse_alloc': sum_clickhouse_alloc,
        'sum_search_alloc': sum_search_alloc,
        'sum_all_alloc': sum_all_alloc,
        'sum_overused': sum_overused,
        'all_power': all_power,
        'search_power': search_power,
    }


def _round_date(date, step):
    if step == "week":
        return date - datetime.timedelta(days=date.weekday())
    if step == "month":
        return date - datetime.timedelta(days=date.day - 1)

    return date


def _next_date(date, step):
    if step == "week":
        return date + datetime.timedelta(days=7)
    if step == "month":
        date += datetime.timedelta(days=31)
        date -= datetime.timedelta(days=date.day - 1)
        return date

    return date + datetime.timedelta(days=1)


def time_intervals(start, end, step="week"):
    assert isinstance(start, datetime.date)
    assert isinstance(end, datetime.date)
    assert step in ["day", "week", "month"]

    first = _round_date(start, step)
    last = _next_date(first, step)

    while True:
        yield first, last - datetime.timedelta(days=1)

        if last >= end:
            return

        first = last
        last = _next_date(first, step)


@app.route("/total_graph_data")
def total_graph_data(resource="cpu", step="week"):
    timer = util.Timer("total_graph_data")

    # TODO discard last unfinished interval or add recalculation
    # to avoid uncomplete data at caches

    start_date = datetime.date(year=2017, month=1, day=1)
    end_date = _round_date(datetime.date.today(), step=step) - datetime.timedelta(days=1)

    line_to_data = defaultdict(list)

    for start_date, end_date in time_intervals(start_date, end_date, step=step):
        cached_value = get_graph_points.refresh_cache(start_date.isoformat(), end_date.isoformat())

        timer.write(start_date.isoformat())

        for key, val in cached_value.iteritems():
            line_to_data[key].append((end_date, val))

    timer.write()

    for points in line_to_data.itervalues():
        points.sort()

    if False:
        return [{
            'name': "{} guarantee (for groups in clickhouse)".format(resource),
            'type': 'line',
            'data': line_to_data['sum_clickhouse_alloc']
        }, {
            'name': "{} guarantee (for all groups)".format(resource),
            'type': 'line',
            'data': line_to_data['sum_all_alloc']
        }, {
            'name': "{} guarantee (for ALL_SEARCH groups in clickhouse)".format(resource),
            'type': 'line',
            'data': line_to_data['sum_search_alloc']
        }, {
            'name': "{} really available (for groups in clickhouse)".format(resource),
            'type': 'line',
            'data': line_to_data['all_power']
        }, {
            'name': "{} really available (for ALL_SEARCH groups in clickhouse)".format(resource),
            'type': 'line',
            'data': line_to_data['search_power']
        }, {
            'name': "{} used over guarantee".format(resource),
            'type': 'area',
            'data': line_to_data['sum_overused']
        }, {
            'name': "{} used within guarantee".format(resource),
            'type': 'area',
            'data': line_to_data['sum_used']
        }]

    # TODO
    # add used lines for ALL_SEARCH only
    # add modes

    return [{
        'name': "{} guarantee (for ALL_SEARCH groups in clickhouse)".format(resource),
        'type': 'line',
        'data': line_to_data['sum_search_alloc']
    }, {
        'name': "{} really available (for all groups in clickhouse)".format(resource),
        'type': 'line',
        'data': line_to_data['all_power']
    }, {
        'name': "{} really available (for ALL_SEARCH groups in clickhouse)".format(resource),
        'type': 'line',
        'data': line_to_data['search_power']
    }, {
        'name': "{} used over guarantee".format(resource),
        'type': 'area',
        'data': line_to_data['sum_overused']
    }, {
        'name': "{} used within guarantee".format(resource),
        'type': 'area',
        'data': line_to_data['sum_used']
    }]


@app.route("/total_mem_graph_data")
def total_mem_graph_data():
    return jsonify(total_graph_data(resource="mem"))


@app.route("/total_cpu_graph_data")
def total_cpu_graph_data():
    return jsonify(total_graph_data(resource="cpu"))


@app.route("/total_graph")
def total_graph(fast=True):
    if False:
        get_gencfg_stuff.refresh_cache()

    ts2 = int(time.time() - 24 * 60 * 60)
    ts1 = ts2 - 30 * 24 * 60 * 60
    params = {
        'start_ts': ts1,
        'end_ts': ts2
    }

    params_str = urllib.quote(json.dumps(params))

    cpu_graph_url = url_for('total_cpu_graph_data', params=params_str)
    memory_graph_url = url_for('total_mem_graph_data', params=params_str) if not fast else None

    return render_template("graph.html", cpu_graph_url=cpu_graph_url, memory_graph_url=memory_graph_url)


@app.route("/total_graph_debug")
def total_graph_debug():
    return total_graph(fast=True)

import time
import operator
from collections import defaultdict, OrderedDict
import logging
import urllib

import requests

from flask import render_template, jsonify, request

from core.utils import get_ch_formatted_date_from_timestamp
from core.query import run_query

from .background_utils import BACKGROUND_PRIORITY
from . import app, cache_manager


class NodeStats(object):
    def __init__(self, id):
        # cpu data
        self.cpu_usage = 0.0
        self.cpu_total = 0.0
        # memory data
        self.memory_usage = 0.0
        self.memory_total = 0.0
        # hdd data
        self.hdd_usage = 0.0
        self.hdd_total = 0.0
        # ssd data
        self.ssd_usage = 0.0
        self.ssd_total = 0.0
        # other
        self.num_clickhouse_hosts = 0
        self.sum_power = 0
        self.num_hosts = 0
        self.sum_clickhouse_power = 0
        self.id = id
        self.parent_id = None

    def __iadd__(self, o):
        for signal in ('cpu_usage', 'cpu_total', 'memory_usage', 'memory_total', 'hdd_usage', 'hdd_total',
                       'ssd_usage', 'ssd_total'):
            new_value = getattr(self, signal) + getattr(o, signal)
            setattr(self, signal, new_value)

        self.sum_clickhouse_power += o.sum_clickhouse_power
        self.num_clickhouse_hosts += o.num_clickhouse_hosts
        self.num_hosts += o.num_hosts

        return self

    def to_dict(self):
        return {
            k: getattr(self, k) for k in [
                'cpu_usage', 'cpu_total', 'memory_usage', 'memory_total', 'hdd_usage', 'hdd_total', 'ssd_usage', 'ssd_total',
                'sum_power', 'sum_clickhouse_power', 'num_clickhouse_hosts', 'num_hosts', 'id', 'parent_id',
            ]
        }


_hosts_list_key = "__hosts__"


class CalculateABCNodeUsage(object):
    def __init__(self):
        self._gg_id = 0

    def __call__(self, node, host_to_usage, group_hierarchy, res, host_to_info, parent_id=None):
        parent_id = parent_id or 0
        self._gg_id += 1
        stats = NodeStats(id=self._gg_id)

        for subgroup in sorted(node.iterkeys()):
            if subgroup != _hosts_list_key:
                sub_stats = self.__call__(
                    node[subgroup], host_to_usage, group_hierarchy + [subgroup],
                    res, host_to_info, stats.id,
                )
                stats += sub_stats

        hosts = node.get(_hosts_list_key, [])
        for host in hosts:
            stats.num_hosts += 1
            stats.sum_power += host_to_info[host]['power']

            if host in host_to_usage:
                # cpu
                stats.cpu_usage += host_to_usage[host]['cpu_usage_power_units']
                stats.cpu_total += host_to_info[host]['power']
                # memory
                stats.memory_usage += host_to_usage[host]['mem_usage']
                stats.memory_total += host_to_info[host]['memory']
                # hdd
                stats.hdd_usage += host_to_usage[host]['hdd_usage']
                stats.hdd_total += host_to_usage[host]['hdd_total']
                # hdd
                stats.ssd_usage += host_to_usage[host]['ssd_usage']
                stats.ssd_total += host_to_usage[host]['ssd_total']
                # other
                stats.num_clickhouse_hosts += 1
                stats.sum_clickhouse_power += host_to_info[host]['power']

        stats.parent_id = parent_id
        res[" -> ".join(group_hierarchy) or "ALL"] = stats.to_dict()

        return stats


def get_bot_hosts_hierarchy(host_to_info):
    rec_dd = lambda: defaultdict(rec_dd)
    tree = defaultdict(rec_dd)

    for host, host_info in host_to_info.iteritems():
        hierarchy = filter(None, [level.strip('-, ') for level in host_info['botprj'].split('>')])

        node = tree
        for level in hierarchy:
            assert level
            node = node[level]

        if _hosts_list_key not in node:
            node[_hosts_list_key] = []

        host_list = node[_hosts_list_key]
        assert isinstance(host_list, list)

        host_list.append(host)

    return tree


def get_hosts_peak_usage():
    ONEG = 1024 * 1024 * 1024.
    day = 24 * 60 * 60

    ed1 = get_ch_formatted_date_from_timestamp(time.time() - 1 * day)
    ed2 = get_ch_formatted_date_from_timestamp(time.time() - day)

    query = """
        SELECT host, quantile(0.99)(cpu_usage), quantile(0.99)(cpu_usage_power_units), quantile(0.99)(mem_usage),
               quantile(0.99)(hdd_usage), quantile(0.99)(hdd_total), quantile(0.99)(ssd_usage), quantile(0.99)(ssd_total)
        FROM hostusage_aggregated_2m
        WHERE eventDate >= '{ed1}' AND eventDate <= '{ed2}' AND cpu_usage <= 1.0
        GROUP BY host
    """.format(ed1=ed1, ed2=ed2)

    data = run_query(query, shuffle=True, priority=BACKGROUND_PRIORITY)

    host_usage = {}
    for row in data:
        host, cpu_usage, cpu_usage_power_units, mem_usage, hdd_usage, hdd_total, ssd_usage, ssd_total = row
        cpu_usage = float(cpu_usage)
        cpu_usage_power_units = float(cpu_usage_power_units)
        mem_usage = float(mem_usage)
        hdd_usage = float(hdd_usage) / ONEG
        hdd_total = float(hdd_total) / ONEG
        ssd_usage = float(ssd_usage) / ONEG
        ssd_total = float(ssd_total) / ONEG
        host_usage[host] = dict(cpu_usage=cpu_usage, cpu_usage_power_units=cpu_usage_power_units, mem_usage=mem_usage,
                                hdd_usage=hdd_usage, hdd_total=hdd_total, ssd_usage=ssd_usage, ssd_total=ssd_total)

    return host_usage


@cache_manager.memoize(key_prefix="get_cached_pgh_data_4", timeout=7 * 24 * 60 * 60)
def get_cached_pgh_data():
    host_to_info = get_host_to_info()
    host_to_usage = get_hosts_peak_usage()
    groups_cards = get_gencfg_groups_cards()

    pgh = get_project_group_host_hierarchy(host_to_info, groups_cards)

    pgh_usage = calc_pgh_usage(pgh, host_to_usage, host_to_info)

    return pgh_usage, pgh, host_to_usage, host_to_info


def get_host_to_info():
    r = requests.get("http://api.gencfg.yandex-team.ru/unstable/hosts_data")
    r.raise_for_status()

    data = r.json()

    result = defaultdict(dict)
    for row in data['hosts_data']:
        is_vm = (row['flags'] != 0)

        if is_vm:
            continue

        host = row['name'] + row['domain']

        for field in ['model', 'dc', 'botmem', 'memory', 'power', 'vmfor']:
            result[host][field] = row[field]

        result[host]['botprj'] = ' -> '.join(filter(None, [level.strip('-, ') for level in row['botprj'].split('>')]))

    return result


def trim_stats(stats, field):
    total_hosts = 0
    for key, bins in stats[field].iteritems():
        total_hosts += sum(bins)

    for key in stats[field].keys():
        bins = stats[field][key]
        if float(sum(bins)) / total_hosts < 0.01:
            stats[field]["OTHER"] = map(operator.add, stats[field]["OTHER"], stats[field][key])
            del stats[field][key]


def prepare_usage_vs_field(hosts, host_to_usage, host_to_info, bin_size, bins_count):
    usage_vs_field = defaultdict(lambda: defaultdict(lambda: [0] * bins_count))

    for host in hosts:
        if host not in host_to_usage:
            continue

        usage = host_to_usage[host]['cpu_usage']
        bin_num = int(usage / bin_size)

        if bin_num >= bins_count:
            bin_num = bins_count - 1
        if bin_num < 0:
            bin_num = 0

        mem = host_to_info[host]['botmem']
        model = host_to_info[host]['model']
        dc = host_to_info[host]['dc']

        usage_vs_field['memory']["{}GB".format(mem)][bin_num] += 1
        usage_vs_field['dc'][dc][bin_num] += 1
        usage_vs_field['model'][model][bin_num] += 1
        usage_vs_field['model_memory']["{}/{}GB".format(model, mem)][bin_num] += 1

    trim_stats(usage_vs_field, 'model_memory')
    trim_stats(usage_vs_field, 'model')
    trim_stats(usage_vs_field, 'memory')

    return usage_vs_field


@app.route('/abc_heatmap')
def abc_heatmap():
    project = []
    for arg_name in ('l{}project'.format(x) for x in xrange(1, 10)):
        arg_value = request.args.get(arg_name, '-')
        if arg_value != '-':
            project.append(arg_value)
        else:
            break
    project_cgi = '&'.join(['l{}project={}'.format(x+1, urllib.quote(y)) for (x, y) in enumerate(project)])
    if len(project):
        project = ' > '.join(project)
    else:
        project = 'ALL'

    return render_template("abc_histograms_new.html",
                           project=project,
                           project_cgi=project_cgi)


def get_all_gencfg_groups():
    r = requests.get('http://api.gencfg.yandex-team.ru/trunk/groups')
    r.raise_for_status()
    return r.json()['group_names']


def get_gencfg_groups_cards():
    r = requests.get('http://api.gencfg.yandex-team.ru/trunk/groups_cards')
    r.raise_for_status()

    return {
        c['name']: c
        for c in r.json()['groups_cards']
    }


def get_group_hosts(group_names):
    res = {}
    for group in group_names:
        r = requests.get('http://api.gencfg.yandex-team.ru/trunk/groups/{}'.format(group))
        r.raise_for_status()
        res[group] = r.json()['hosts']

    return res


def get_project_group_host_hierarchy(host_to_info, groups_cards):
    def is_vm_group(g):
        vm_count = 0
        hosts = master_group_hosts[g]
        for h in master_group_hosts[g]:
            if h not in host_to_info:
                vm_count += 1
        if vm_count == len(hosts):
            return True
        return False

    masters = [g['name'] for g in groups_cards.itervalues()
               if not g['name'].startswith('ALL_UNSORTED') and
               g['master'] is None and
               not g['properties']['background_group']]

    masters.sort()
    master_group_hosts = get_group_hosts(masters)
    masters = filter(lambda g: not is_vm_group(g), masters)
    logging.info("Found %i masters", len(masters))
    master_group_hosts = {g: h for g, h in master_group_hosts.iteritems() if g in masters}

    host_to_masters = defaultdict(set)
    host_to_metaprjs = defaultdict(set)
    for g, hosts in master_group_hosts.iteritems():
        for h in hosts:
            host_to_masters[h].add(g)
            host_to_metaprjs[h].add(groups_cards[g]['tags']['metaprj'])

    assert all(len(masters) <= 1 for masters in host_to_masters.itervalues())
    no_master_count = 0
    for h, m in host_to_masters.iteritems():
        if not m:
            no_master_count += 1
    logging.debug("host count without masters %i", no_master_count)
    assert all(len(metaprjs) == 1 for metaprjs in host_to_metaprjs.itervalues())

    all_pgh_hosts = set()

    pgh = {}
    for g, hosts in master_group_hosts.iteritems():
        for h in hosts:
            pgh.setdefault(groups_cards[g]['tags']['metaprj'], {}).setdefault(g, set()).add(h)
            all_pgh_hosts.add(h)

    return pgh


def calc_pgh_usage(pgh, host_to_usage, host_to_info):
    pgh_node_to_usage = dict()
    for project_id, (project, group_data) in enumerate(sorted(pgh.iteritems())):
        project_id_str = "p{:04d}".format(project_id)
        project_node = NodeStats(project_id_str)
        for group_id, (group, hosts) in enumerate(sorted(group_data.iteritems())):
            group_node = NodeStats("p{:04d}-g{:04d}".format(project_id, group_id))
            group_node.parent_id = project_id_str
            group_node.num_hosts = len(hosts)

            for host in hosts:
                if host not in host_to_info:  # TODO XXX: better vm check?
                    continue
                group_node.sum_power += host_to_info[host]['power']
                if host not in host_to_usage:
                    continue
                group_node.cpu_usage += host_to_usage[host]['cpu_usage_power_units']
                group_node.total_cpu += host_to_info[host]['power']
                group_node.sum_clickhouse_power += host_to_info[host]['power']
                group_node.num_clickhouse_hosts += 1

            pgh_node_to_usage[group] = group_node.to_dict()
            if group_node.num_clickhouse_hosts == 0:
                logging.debug("WHOLE MISSING GROUP %s", group)
            project_node += group_node

        pgh_node_to_usage[project] = project_node.to_dict()

    return pgh_node_to_usage


# Cached for a very long time, recalculated in background process
@cache_manager.memoize(key_prefix="get_cached_abc_data_new3", timeout=7 * 24 * 60 * 60)
def get_cached_abc_data():
    logging.info("get_cached_abc_data(): before get_host_to_info()")
    host_to_info = get_host_to_info()
    logging.info("get_cached_abc_data(): done calling get_host_to_info()")

    logging.info("get_cached_abc_data(): before get_hosts_hieararchy()")
    tree = get_bot_hosts_hierarchy(host_to_info)
    logging.info("get_cached_abc_data(): done calling get_bot_hosts_hierarchy()")

    logging.info("get_cached_abc_data(): before get_host_usage()")
    host_to_usage = get_hosts_peak_usage()
    logging.info("get_cached_abc_data(): done calling get_host_usage()")

    abc_usage = OrderedDict()
    calculate_node_usage = CalculateABCNodeUsage()
    calculate_node_usage(tree, host_to_usage, [], abc_usage, host_to_info)

    return abc_usage, host_to_usage, host_to_info


@app.route('/pgh_heatmap/<node>')
def pgh_heatmap(node):
    _, pgh, host_to_usage, host_to_info = get_cached_pgh_data()

    # TODO: this should be rewritten (probably)
    if node in pgh:  # metaprj, its hosts are hosts in all groups
        hosts = sum((list(hosts) for group, hosts in pgh[node].iteritems()), [])
    else:
        for metaprj in pgh:  # XXX: hackish, iterate over all metaprjs to find which metaprj has the group <node>
            if node in pgh[metaprj]:
                hosts = pgh[metaprj][node]
                break

    bin_size = 0.1
    bins_count = int(1.0 / bin_size)

    usage_vs_field = prepare_usage_vs_field(hosts, host_to_usage, host_to_info, bin_size, bins_count)

    return render_template("abc_histograms.html",
                           project=node,
                           usage_vs_field=usage_vs_field,
                           bin_size=bin_size,
                           bins_count=bins_count)


@app.route('/pgh')
def pgh():
    return render_template(
        "abc.html",
        data_endpoint='pgh_data',
        heatmap_endpoint='pgh_heatmap'
    )


@app.route('/abc')
def abc():
    return render_template(
        "abc.html",
        data_endpoint='abc_data',
        heatmap_endpoint='abc_heatmap'
    )


@app.route('/abc_data')
def abc2_data():
    abc_usage, _, _ = get_cached_abc_data()
    data = sorted(abc_usage.items(), key=lambda (k, v): v['id'])

    def format_row(path, stats):
        stats['project'] = path.split(' -> ')[-1]
        if path == 'ALL':
            stats['project_path'] = ''
        else:
            stats['project_path'] = '&'.join(['l{}project={}'.format(x+1, urllib.quote(y)) for x, y in enumerate(path.split(' -> '))])
        stats['path'] = path
        if stats['parent_id']:
            stats['pid'] = stats['parent_id']
        del stats['parent_id']

        return stats

    return jsonify([format_row(path, stats) for path, stats in data])


@app.route('/pgh_data')
def pgh2_data():
    pgh_node_to_usage = get_cached_pgh_data()[0]
    data = sorted(pgh_node_to_usage.items(), key=lambda (k, v): v['id'])

    def format_row(path, stats):
        stats['project'] = path
        stats['project_path'] = path
        stats['path'] = path
        if stats['parent_id']:
            stats['pid'] = stats['parent_id']
        del stats['parent_id']

        return stats

    return jsonify([format_row(path, stats) for path, stats in data])

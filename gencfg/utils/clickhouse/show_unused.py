#!/skynet/python/bin/python
"""
    Show unused hosts for specified groups
"""


import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict
import time

import gencfg
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
import gaux.aux_clickhouse
from core.exceptions import UtilNormalizeException
from gaux.aux_utils import correct_pfname
from core.db import CURDB


class TGroupResult(object):
    __slots__ = ('groupname', 'total_count', 'stats_count', 'hosts_with_usage')

    def __init__(self, groupname, host_filter):
        self.groupname = groupname
        if self.groupname != 'UNKNOWN':
            self.total_count = len(filter(host_filter, CURDB.groups.get_group(self.groupname).get_kinda_busy_hosts()))
        else:
            self.total_count = 0
        self.stats_count = 0
        self.hosts_with_usage = []

    def as_str(self):
        result = [
            "Group %s:" % self.groupname,
            "    Total hosts: %s" % self.total_count,
            "    Hosts with stats: %s" % self.stats_count,
            "    Unused hosts: (%d total)" % len(self.hosts_with_usage)
        ]
        result.extend(map(lambda x: "       %s (%.2f usage)" % (x[0], x[1]), self.hosts_with_usage))

        return "\n".join(result)


def get_parser():
    parser = ArgumentParserExt(description="Show unused machines")
    parser.add_argument("-g", "--groups", type=argparse_types.groups, default=None,
        help="Optional. List of groups to process (beware of specifying slave groups)")
    parser.add_argument("-s", "--hosts", type = argparse_types.hosts, default=None,
        help="Optional. List of hosts to process")
    parser.add_argument("--startt", type=argparse_types.xtimestamp, default="7d",
        help="Optional. Start timestamp (format '3m4d5h' mean 94 days 5 hours ago). By default '7d'")
    parser.add_argument("--endt", type=argparse_types.xtimestamp, default="0d",
        help="Optional. End timestamp (format '3m4d5h' mean 94 days 5 hours ago)")
    parser.add_argument("-r", "--usage-ratio", type=float, default=None,
        help="Optional. Boundary cpu usage of 'unused' hosts (0.05 mean hosts with less than 5%% usage counted as unused)")
    parser.add_argument("-t", "--topn", type=int, default=None,
        help="Optional. Show only top most unused hosts")
    parser.add_argument("-f", "--host-filter", type=argparse_types.pythonlambda, default=lambda x: True,
        help="Optional. Filter on hosts to process")

    return parser


def normalize(options):
    if options.groups is None and options.hosts is None:
        raise UtilNormalizeException(correct_pfname(__file__), ["groups", "hosts"], "You must specify either <--groups> or <--hosts> option")

    # first check if specified groups intersect
    groups_by_host = defaultdict(list)
    if options.groups is not None:
        for group in options.groups:
            for host in filter(options.host_filter, group.get_kinda_busy_hosts()):
                groups_by_host[host].append(group.card.name)
    if options.hosts is not None:
        for host in filter(options.host_filter, options.hosts):
            groups_by_host[host].append('UNKNOWN')

    for host, groups in groups_by_host.iteritems():
        if len(groups) > 1:
            raise UtilNormalizeException(correct_pfname(__file__), ["group"], "Host <%s> in more than one groups: <%s>" % (host.name, ",".join(map(lambda x: x.card.name, groups))))
    if len(groups_by_host) == 0:
        raise Exception, "No hosts left after filter applied"

def main(options):
    group_by_host = dict()
    if options.groups is not None:
        for group in options.groups:
            for host in filter(options.host_filter, group.get_kinda_busy_hosts()):
                group_by_host[host.name] = group.card.name
    if options.hosts is not None:
        for host in filter(options.host_filter, options.hosts):
            group_by_host[host.name] = 'UNKNOWN'

    # filter host with not enough statistics
    clickhouse_query = 'SELECT min(ts), host FROM instanceusage_aggregated_1d'
    if len(group_by_host) < 5000:
        clickhouse_query += " WHERE host in (%s)" % ", ".join(map(lambda x: "'%s'" % x, group_by_host))
    clickhouse_query += ' GROUP BY host'
    clickhouse_result = gaux.aux_clickhouse.run_query(clickhouse_query)

    filtered_count = 0
    for ts, hostname in clickhouse_result:
        ts = int(ts)
        if hostname not in group_by_host:
            continue
        if ts > options.startt:
            group_by_host.pop(hostname)
            filtered_count += 1
    print 'Filtered {} hosts with not enough statistics'.format(filtered_count)

    # execute main query
    clickhouse_startt = time.strftime("%Y-%m-%d", time.localtime(options.startt))
    clickhouse_endt = time.strftime("%Y-%m-%d", time.localtime(options.endt))
    # if testing group is not big, add IN for all hosts
    if len(group_by_host) < 5000:
        extra_filter = "and host in (%s)" % ", ".join(map(lambda x: "'%s'" % x, group_by_host))
    else:
        extra_filter = ""

    clickhouse_query = "SELECT host, avg(instance_cpuusage) AS usage FROM instanceusage_aggregated_1h WHERE port == 65535 AND eventDate >= '%(clickhouse_startt)s' AND eventDate <= '%(clickhouse_endt)s' AND ts >= %(startt)d AND ts <= %(endt)d %(extra_filter)s GROUP BY host" % {
        'clickhouse_startt': clickhouse_startt,
        'clickhouse_endt': clickhouse_endt,
        'startt': options.startt,
        'endt': options.endt,
        'extra_filter': extra_filter,
    }

    clickhouse_result = gaux.aux_clickhouse.run_query(clickhouse_query)

    result = dict(map(lambda x: (x, TGroupResult(x, options.host_filter)), sorted(set(group_by_host.values()))))

    for host, usage in clickhouse_result:
        if host not in group_by_host:
            continue

        usage = float(usage)
        result[group_by_host[host]].stats_count += 1
        result[group_by_host[host]].hosts_with_usage.append((host, usage))

    for v in result.itervalues():
        v.hosts_with_usage.sort(key=lambda x: x[1])
        if options.usage_ratio is not None:
            v.hosts_with_usage = [x for x in v.hosts_with_usage if x[1] <= options.usage_ratio]
        if options.topn is not None:
            v.hosts_with_usage = v.hosts_with_usage[:options.topn]

    return sorted(result.values(), key = lambda x: x.groupname)

def print_result(result, options):
    for elem in result:
        print elem.as_str()


def jsmain(d):
    options = get_parser().parse_json(d)

    normalize(options)

    return main(options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    result = main(options)

    print_result(result, options)

#!/skynet/python/bin/python
"""Manipulate graphs for abc in different ways

The following actions are supported:
   - add current usage statistics for all abc services based on current host usage
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import time
import datetime

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types
import pytz
import gaux.aux_clickhouse


MAX_ABC_DEPTH = 10  # maximal depth of abc projects
PERIOD = 120  # every <PERIOD> seconds new point added
ONEG = 1024. * 1024 * 1024

class EActions(object):
    ADDSTATS = 'addstats'
    FILLSTATS = 'fillstats'  # add stats starting with last update timestamp
    ALL = [ADDSTATS, FILLSTATS]


class THostUsage(object):
    """Host usage at specified timestamp"""
    def __init__(self, host, ts, cpu_usage=0, memory_usage=0, hdd_usage=0, ssd_usage=0, hdd_total=0, ssd_total=0):
        self.host = host
        self.ts = 0

        self.update(ts, cpu_usage=cpu_usage, memory_usage=memory_usage, hdd_usage=hdd_usage, ssd_usage=ssd_usage, hdd_total=hdd_total, ssd_total=ssd_total)

    def update(self, ts, cpu_usage=0, memory_usage=0, hdd_usage=0, ssd_usage=0, hdd_total=0, ssd_total=0):
        if ts > self.ts:
            self.cpu_usage = cpu_usage * self.host.power
            self.cpu_total = self.host.power

            self.memory_usage = memory_usage * ONEG
            self.memory_total = self.host.memory * ONEG

            self.hdd_usage = hdd_usage
            self.hdd_total = hdd_total

            self.ssd_usage = ssd_usage
            self.ssd_total = ssd_total

    def __str__(self):
        return 'Host {}: cpu <{:.2f} of {:.2f}> memory <{:.2f} of {:.2f}> ssd <{:.2f} of {:.2f}> hdd <{:.2f} of {:.2f}>'.format(
            self.host.name, self.cpu_usage, self.cpu_total, self.memory_usage / ONEG, self.memory_total / ONEG,
            self.ssd_usage / ONEG, self.ssd_total / ONEG, self.hdd_usage / ONEG, self.hdd_total / ONEG
        )


class TAbcUsage(object):
    """Abc usage at specified timestamp"""

    DATA_SIGNALS = ('cpu_usage', 'cpu_total', 'memory_usage', 'memory_total', 'hdd_usage', 'hdd_total', 'ssd_usage', 'ssd_total')

    def __init__(self, projects, hosts_total):
        assert len(projects) == MAX_ABC_DEPTH
        self.projects = tuple(projects)
        self.hosts_total = hosts_total
        self.hosts_with_stats = 0
        for signal in TAbcUsage.DATA_SIGNALS:
            setattr(self, signal, 0)

    def append(self, host_usage):
        for signal in TAbcUsage.DATA_SIGNALS:
            new_value = getattr(self, signal) + getattr(host_usage, signal)
            setattr(self, signal, new_value)
        self.hosts_with_stats += 1

    @staticmethod
    def fields_for_insert():
        """Return fields for insert"""
        return ['ts', 'eventDate'] + ['l{}project'.format(x) for x in range(1, MAX_ABC_DEPTH+1)] + list(TAbcUsage.DATA_SIGNALS) + ['hosts_with_stats', 'hosts_total']

    def values_for_insert(self, ts):
        event_date = ts_to_event_date(ts)

        values = [str(ts), "'{}'".format(event_date)]
        values.extend("'{}'".format(x.replace("'", "\\'")) for x in self.projects)
        values.extend("{:.2f}".format(getattr(self, x)) for x in TAbcUsage.DATA_SIGNALS)
        values.append("{}".format(self.hosts_with_stats))
        values.append("{}".format(self.hosts_total))

        return '({})'.format(', '.join(values))

    def __str__(self):
        abc_project_name = ' > '.join(x for x in self.projects if x != '-')
        return 'Abc project {} ({} hosts of {} with stats): cpu <{:.2f} of {:.2f}> memory <{:.2f} of <{:.2f}> ssd <{:.2f} of {:.2f}> hdd <{:.2f} of {:.2f}>'.format(
            abc_project_name, self.hosts_with_stats, self.hosts_total, self.cpu_usage, self.cpu_total, self.memory_usage / ONEG, self.memory_total / ONEG, self.ssd_usage / ONEG,
            self.ssd_total / ONEG, self.hdd_usage / ONEG, self.hdd_total / ONEG
        )


def get_parser():
    parser = ArgumentParserExt(description="Perform various actions with abc groups stats")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=EActions.ALL,
                        help="Obligatory. Action to execute")
    parser.add_argument('--startts', type=int, default=None,
                        help='Optional. Start timestamp for action <{}>'.format(EActions.FILLSTATS))
    parser.add_argument('--endts', type=int, default=None,
                        help='Optional. End timestamp for action <{}>'.format(EActions.FILLSTATS))

    return parser


def ts_to_event_date(ts):
    """Convert ts to datetime (for clickhouse queries)"""
    return datetime.datetime.fromtimestamp(int(ts), tz=pytz.timezone("Europe/Moscow")).strftime('%Y-%m-%d')


def host_usage_at(ts):
    """Get host usage data at specified timestamp"""
    start_ts = int(ts) - 300
    start_event_date = ts_to_event_date(start_ts)

    end_ts = int(ts)
    end_event_date = ts_to_event_date(end_ts)

    query = ('SELECT host, ts, mem_usage, cpu_usage, hdd_usage, ssd_usage, hdd_total, ssd_total '
             'FROM hostusage '
             'WHERE eventDate >= \'{start_event_date}\' AND eventDate <= \'{end_event_date}\' AND ts >= {start_ts} AND ts <= {end_ts}').format(
                     start_ts=start_ts, start_event_date=start_event_date, end_ts=end_ts, end_event_date=end_event_date)

    usage_by_host = dict()

    for hostname, ts, memory_usage, cpu_usage, hdd_usage, ssd_usage, hdd_total, ssd_total in gaux.aux_clickhouse.run_query(query):
        hostname = str(hostname)
        ts = int(ts)
        memory_usage = max(float(memory_usage), 0.)
        cpu_usage = max(float(cpu_usage), 0.)
        hdd_usage = max(float(hdd_usage), 0.)
        ssd_usage = max(float(ssd_usage), 0.)
        hdd_total = max(float(hdd_total), 0.)
        ssd_total = max(float(ssd_total), 0.)

        if not CURDB.hosts.has_host(hostname):
            continue
        host = CURDB.hosts.get_host_by_name(hostname)

        if host not in usage_by_host:
            usage_by_host[host] = THostUsage(host, ts, memory_usage=memory_usage, cpu_usage=cpu_usage, hdd_usage=hdd_usage, ssd_usage=ssd_usage, hdd_total=hdd_total, ssd_total=ssd_total)
        usage_by_host[host].update(ts, memory_usage=memory_usage, cpu_usage=cpu_usage, hdd_usage=hdd_usage, ssd_usage=ssd_usage, hdd_total=hdd_total, ssd_total=ssd_total)

    return usage_by_host


def get_abc_by_host(options):
    """Create mapping: host -> abc project as 3-tuple"""

    def split_botprj_to_abc(s):
        elems = [x.strip(' ') for x in s.split('>')]
        assert len(elems) < MAX_ABC_DEPTH, 'Can not split botprj <{}> to abc groups'.format(s)
        return tuple(elems + ['-'] * (MAX_ABC_DEPTH - len(elems)))

    return {x: split_botprj_to_abc(x.botprj) for x in CURDB.hosts.get_hosts()}


def main_addstats_at_ts(options, ts):
    """Add current usage stats"""

    abc_by_host = get_abc_by_host(options)

    usage_by_host = host_usage_at(ts)

    # calculate usage for all abc groups
    usage_by_abc_group = dict()
    for host, host_usage in usage_by_host.iteritems():
        abc_project_tuple = abc_by_host[host]
        if abc_project_tuple not in usage_by_abc_group:
            abc_project_hosts_count = len([x for x in abc_by_host.itervalues() if x == abc_project_tuple])
            usage_by_abc_group[abc_project_tuple] = TAbcUsage(abc_project_tuple, abc_project_hosts_count)
        usage_by_abc_group[abc_project_tuple].append(host_usage)

    # generate insert data elems
    insert_data = []
    for v in usage_by_abc_group.itervalues():
        insert_data.append(v.values_for_insert(ts))

    insert_data = ' '.join(insert_data)

    # generate query
    query = 'INSERT INTO abcusage ({}) VALUES {}'.format(', '.join(TAbcUsage.fields_for_insert()), insert_data)

    gaux.aux_clickhouse.run_query(query)


def main_addstats(options):
    """Add statistics at current timestamp"""
    return main_addstats_at_ts(options, int(time.time()))


def main_fillstats(options):
    # find last timestamp with stats
    if options.startts is None:
        startts = int(gaux.aux_clickhouse.run_query('SELECT max(ts) FROM abcusage')[0][0]) + 1
        if startts % PERIOD != 0:
            startts = startts + PERIOD - startts % PERIOD
    else:
        startts = options.startts
    if options.endts is None:
        endts = int(time.time())
    else:
        endts = options.endts

    add_stats_timestamps = range(startts, endts, PERIOD)

    print 'Filling interval ({}, {}): {} points: {}'.format(startts, endts, len(add_stats_timestamps), ' '.join(str(x) for x in add_stats_timestamps))

    for ts in add_stats_timestamps:
        print '    Adding stats at {}'.format(ts)
        main_addstats_at_ts(options, ts)


def main(options):
    if options.action == EActions.ADDSTATS:
        main_addstats(options)
    elif options.action == EActions.FILLSTATS:
        main_fillstats(options)
    else:
        raise Exception('Unknown action <{}>'.format(options.action))


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    main(options)

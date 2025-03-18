#!/skynet/python/bin/python
"""Manipulate graphs for metaprjs in different ways (RX-545)

The following actions are supported:
   - add current usage statistics for all abc services based on current host usage
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import re
import time
import datetime

import gencfg
from core.db import DB, CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types
import pytz
import gaux.aux_clickhouse


PERIOD = 120  # every <PERIOD> seconds new point added
ONEG = 1024. * 1024 * 1024

class EActions(object):
    ADDSTATS = 'addstats'
    FILLSTATS = 'fillstats'  # add stats starting with last update timestamp
    ALL = [ADDSTATS, FILLSTATS]


class TMetaprjStats(object):
    """Metaprj usage/allocate stats"""

    DATA_SIGNALS = ('cpu_usage', 'cpu_allocated', 'memory_usage', 'memory_allocated',)

    def __init__(self, name):
        self.name = name
        self.cpu_usage = 0.0
        self.cpu_allocated = 0.0
        self.memory_usage = 0.0
        self.memory_allocated = 0.0

    def update_usage(self, cpu_usage=None, memory_usage=None):
        if cpu_usage is not None:
            self.cpu_usage += cpu_usage
        if memory_usage is not None:
            self.memory_usage += memory_usage

    def update_allocated(self, cpu_allocated=None, memory_allocated=None):
        if cpu_allocated is not None:
            self.cpu_allocated += cpu_allocated
        if memory_allocated is not None:
            self.memory_allocated += memory_allocated

    @classmethod
    def fields_for_insert(cls):
        return ['ts', 'eventDate', 'metaprj'] + list(cls.DATA_SIGNALS)

    def values_for_insert(self, ts):
        event_date = gaux.aux_clickhouse.ts_to_event_date(ts)

        values = [str(ts), "'{}'".format(event_date), "'{}'".format(self.name)]
        values.extend("{:.2f}".format(getattr(self, x)) for x in self.DATA_SIGNALS)

        return '({})'.format(', '.join(values))

    def __str__(self):
        return 'Metaprj <{}>: cpu {:.2f} usage, cpu {:.2f} allocated, memory {:.2f} usage, memory {:.2f} allocated'.format(
            self.name, self.cpu_usage, self.cpu_allocated, self.memory_usage, self.memory_allocated,
        )


def get_parser():
    parser = ArgumentParserExt(description="Perform various actions with abc groups stats")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=EActions.ALL,
                        help="Obligatory. Action to execute")
    parser.add_argument("--db", type=core.argparse.types.gencfg_db, default=CURDB,
                        help="Optional. Path to db")
    parser.add_argument('--startts', type=int, default=None,
                        help='Optional. Start timestamp for action <{}>'.format(EActions.FILLSTATS))
    parser.add_argument('--endts', type=int, default=None,
                        help='Optional. End timestamp for action <{}>'.format(EActions.FILLSTATS))
    parser.add_argument('--use-timestamp-db', action='store_true', default=False,
                        help='Optional (for action {}). Use most actual db at specified timestamp'.format(EActions.ADDSTATS))
    parser.add_argument('--host-filter', type=core.argparse.types.pythonlambda, default=lambda x: True,
                        help='Optional. Filter on hosts')
    parser.add_argument('--skip-calculate-usage', action='store_true', default=False,
                        help='Optional. Skip calculating of current usage')
    parser.add_argument('--dry-run', action='store_true', default=False,
                        help='Optional. Do not update database, just print to stdout')

    return parser


def ts_to_event_date(ts):
    """Convert ts to datetime (for clickhouse queries)"""
    return datetime.datetime.fromtimestamp(int(ts), tz=pytz.timezone("Europe/Moscow")).strftime('%Y-%m-%d')


def update_usage_by_prj_at_ts(options, hosts_power, group_metaprjs, metaprj_stats, ts, common_groups, ignore_groups, fullhost_groups, halfythost_groups):
    """Update all instances usage at specified timestamp and group by metaprj"""
    start_ts = ts - 1200
    start_ed = ts_to_event_date(start_ts)
    end_ts = ts
    end_ed = ts_to_event_date(end_ts)

    host_metaprjs = dict()
    for group in fullhost_groups:
        for host in group.getHosts():
            host_metaprjs[host.name] = group.card.tags.metaprj

    common_groups = {x.card.name for x in common_groups}
    igrone_groups = {x.card.name for x in ignore_groups}
    fullhost_groups = {x.card.name for x in fullhost_groups}
    halfythost_groups = {x.card.name for x in halfythost_groups}

    result = dict()
    traversed_instances = set()

    # calculate instances usage
    query = ("SELECT host, port, group, instance_memusage, instance_cpuusage "
             "FROM instanceusage_aggregated_15m "
             "WHERE port != 65535 AND "
             "      eventDate >= '{start_ed}' AND "
             "      eventDate <= '{end_ed}' AND "
             "      ts >= {start_ts} AND "
             "      ts <= {end_ts} "
             "ORDER BY ts DESC").format(start_ts=start_ts, start_ed=start_ed, end_ts=end_ts, end_ed=end_ed)

    for host, port, groupname, memory_usage, cpu_usage in gaux.aux_clickhouse.run_query(query):
        if (host, port, groupname) not in traversed_instances:
            traversed_instances.add((host, port, groupname))
        else:
            continue

        if groupname not in common_groups:
            continue

        memory_usage = float(memory_usage)
        cpu_usage = float(cpu_usage) * hosts_power.get(host, 0.)
        metaprj_name = group_metaprjs.get(groupname, 'unknown')
        metaprj_stats[metaprj_name].update_usage(cpu_usage=cpu_usage, memory_usage=memory_usage)

    # calculate host usage
    query = ("SELECT host, port, instance_memusage, instance_cpuusage "
             "FROM instanceusage_aggregated_15m "
             "WHERE port == 65535 AND "
             "      eventDate >= '{start_ed}' AND "
             "      eventDate <= '{end_ed}' AND "
             "      ts >= {start_ts} AND "
             "      ts <= {end_ts} "
             "ORDER BY ts DESC").format(start_ts=start_ts, start_ed=start_ed, end_ts=end_ts, end_ed=end_ed)

    for host, port, memory_usage, cpu_usage in gaux.aux_clickhouse.run_query(query):
        if (host, port) not in traversed_instances:
            traversed_instances.add((host, port))
        else:
            continue

        if host not in host_metaprjs:
            continue

        memory_usage = float(memory_usage)
        cpu_usage = float(cpu_usage) * hosts_power.get(host, 0.)
        metaprj_name = host_metaprjs[host]
        metaprj_stats[metaprj_name].update_usage(cpu_usage=cpu_usage, memory_usage=memory_usage)


def update_guarantee_by_prj_at_ts(options, metaprj_stats):
    common_groups, ignore_groups, fullhost_groups, halfythost_groups = set(), set(), set(), set()

    for groupname in ('SAS_WEB_BASE', 'MAN_WEB_BASE',
                      'SAS_IMGS_BASE', 'MAN_IMGS_BASE',
                      'SAS_VIDEO_BASE', 'MAN_VIDEO_BASE',
                      'SAS_MARKET_TEST_GENERAL', 'SAS_MARKET_PROD_GENERAL',
                      'MAN_SAAS_CLOUD', 'SAS_SAAS_CLOUD', 'VLA_SAAS_CLOUD'):
        if options.db.groups.has_group(groupname):
            fullhost_groups.add(options.db.groups.get_group(groupname))

    for groupname in ('VLA_YT_RTC',):
        if options.db.groups.has_group(groupname):
            halfythost_groups.add(options.db.groups.get_group(groupname))

    # add extra to fullhost groups
    for group in options.db.groups.get_groups():
        if (group in fullhost_groups) or (group in halfythost_groups):
            continue
        if group.card.properties.full_host_group and len(group.card.slaves) == 0:
            fullhost_groups.add(group)

    # fill ignore groups as slaves of fullhost/halfhost groups
    for group in options.db.groups.get_groups():
        if (group in fullhost_groups) or (group in halfythost_groups):
            continue
        if (group.card.master in fullhost_groups) or (group.card.master in halfythost_groups):
            if group.card.tags.metaprj not in ('mail', 'disk'):
                continue

        # special processing of VLA_WEB_BASE
        if (group.card.master is not None) and (group.card.master.card.name == 'VLA_WEB_BASE'):
            if group.card.name not in ('VLA_WEB_TIER0_JUPITER_BASE', 'VLA_WEB_TIER1_JUPITER_BASE', 'VLA_WEB_PLATINUM_JUPITER_BASE',
                                       'VLA_IMGS_BASE', 'VLA_VIDEO_BASE'):
                continue

        common_groups.add(group)

    # now calculate guarantee
    for group in options.db.groups.get_groups():
        if group in common_groups:
            group_cpu_allocated = sum(x.power for x in group.get_kinda_busy_instances() if options.host_filter(x.host))
            filtered_instances_count = len([x for x in group.get_kinda_busy_instances() if options.host_filter(x.host)])
            group_memory_allocated = filtered_instances_count * group.card.reqs.instances.memory_guarantee.value / 1024. / 1024 / 1024
            metaprj_stats[group.card.tags.metaprj].update_allocated(cpu_allocated=group_cpu_allocated, memory_allocated=group_memory_allocated)
        elif group in ignore_groups:
            continue
        elif group in fullhost_groups:
            group_cpu_allocated = sum(x.power for x in group.getHosts() if options.host_filter(x))
            group_memory_allocated = sum(x.memory for x in group.getHosts() if options.host_filter(x))
            metaprj_stats[group.card.tags.metaprj].update_allocated(cpu_allocated=group_cpu_allocated, memory_allocated=group_memory_allocated)
        elif group in halfythost_groups:
            group_cpu_allocated = sum(x.power for x in group.getHosts() if options.host_filter(x)) / 2
            group_memory_allocated = sum(x.memory for x in group.getHosts() if options.host_filter(x)) / 2
            metaprj_stats[group.card.tags.metaprj].update_allocated(cpu_allocated=group_cpu_allocated, memory_allocated=group_memory_allocated)
            metaprj_stats['yt'].update_allocated(cpu_allocated=group_cpu_allocated, memory_allocated=group_memory_allocated)

    return common_groups, ignore_groups, fullhost_groups, halfythost_groups


def get_timestamp_db(ts):
    """Get most actual database at specified timestamp"""

    ts = ts - ts % (24 * 60 * 60)

    if ts not in get_timestamp_db.cached_dbs:
        xml_log = CURDB.get_repo().log_command_xml([], remote_url='svn+ssh://arcadia.yandex.ru/arc/tags/gencfg', no_merge_history=True)

        last_tag = None
        for elem in xml_log.findall('./logentry'):
            commit_dt = elem.find('date').text.partition('.')[0]
            commit_ts = time.mktime(time.strptime(commit_dt, '%Y-%m-%dT%H:%M:%S'))
            if commit_ts > ts:
                for path_elem in elem.findall('./paths/path'):
                    m = re.match('.*/(stable-(?:\d+)-r(?:\d+)).*', path_elem.text)
                    if m:
                        last_tag = m.group(1)
                        break
            else:
                break

        if last_tag:
            db = DB('tag@{}'.format(last_tag))
            get_timestamp_db.cached_dbs[ts] = db
            print '    Selected {} for timestamp {}'.format(last_tag, ts)
        else:
            db = CURDB
            print '    Selected CURDB for timestamp {}'.format(ts)

        return db
    else:
        return get_timestamp_db.cached_dbs[ts]
get_timestamp_db.cached_dbs = dict()


def main_addstats_at_ts(options, ts):
    """Add current usage stats"""
    print 'Processing timestamp {}'.format(ts)

    if options.use_timestamp_db:  # FIXME: (do not modify options.db)
        options.db = get_timestamp_db(ts)


    all_metaprjs = {x.card.tags.metaprj for x in options.db.groups.get_groups()}
    all_metaprjs.add('unknown')
    metaprj_stats = {x: TMetaprjStats(x) for x in all_metaprjs}


    # update allocated for all metaprjs
    common_groups, ignore_groups, fullhost_groups, halfythost_groups = update_guarantee_by_prj_at_ts(options, metaprj_stats)

    # update usage for all metaprjs
    if not options.skip_calculate_usage:
        hosts_power = {x.name: x.power for x in options.db.hosts.get_hosts()}
        group_metaprjs = {x.card.name: x.card.tags.metaprj for x in options.db.groups.get_groups()}
        update_usage_by_prj_at_ts(options, hosts_power, group_metaprjs, metaprj_stats, ts, common_groups, ignore_groups, fullhost_groups, halfythost_groups)

    if options.dry_run:
        for metaprj_name in sorted(metaprj_stats):
            print '    {}'.format(metaprj_stats[metaprj_name])
    else:
        # generate insert data elems
        insert_data = []
        for v in metaprj_stats.itervalues():
            insert_data.append(v.values_for_insert(ts))

        insert_data = ' '.join(insert_data)

        # generate query
        query = 'INSERT INTO metaprjusage ({}) VALUES {}'.format(', '.join(TMetaprjStats.fields_for_insert()), insert_data)

        print query

        gaux.aux_clickhouse.run_query(query)


def main_addstats(options):
    """Add statistics at current timestamp"""
    return main_addstats_at_ts(options, options.startts)


def main_fillstats(options):
    # find last timestamp with stats
    if options.startts is None:
        startts = int(gaux.aux_clickhouse.run_query('SELECT max(ts) FROM metaprjusage')[0][0]) + 1
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

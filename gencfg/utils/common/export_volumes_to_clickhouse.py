#!/skynet/python/bin/python
"""Export aggregated group volumes info to separate clickhouse table (RX-432)"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import time
import simplejson

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types

from gaux.aux_colortext import red_text
from gaux.aux_clickhouse import ts_to_event_date, run_query, run_query_dict
from gaux.aux_volumes import volumes_as_objects
from gaux.aux_utils import prettify_size


PERIOD = 60 * 60  # add group stats every hour


class EStorageType(object):
    SSD = 'ssd'
    HDD = 'hdd'
    ALL = [SSD, HDD]


EMPIRICAL_VOLUMES = dict(
    workdir_hdd_size=EStorageType.HDD,
    workdir_ssd_size=EStorageType.SSD,
    iss_resources_hdd_size=EStorageType.HDD,
    iss_resources_ssd_size=EStorageType.SSD,
    iss_shards_hdd_size=EStorageType.HDD,
    iss_shards_ssd_size=EStorageType.SSD,
    webcache_hdd_size=EStorageType.HDD,
    webcache_ssd_size=EStorageType.SSD,
    callisto_hdd_size=EStorageType.HDD,
    callisto_ssd_size=EStorageType.SSD,
    logs_hdd_size=EStorageType.HDD,
    logs_ssd_size=EStorageType.SSD,
)


class EActions(object):
    ADDSTATS = 'addstats'
    FILLSTATS = 'fillstats'
    ALL = [ADDSTATS, FILLSTATS]


class TGroupVolumeInfo(object):
    """Object with group volumes"""

    class TVolumeInfo(object):
        def __init__(self, name, tp):
            self.name = name
            self.tp = tp
            self.usage = 0
            self.guarantee = 0

        def __str__(self):
            return 'Volume <{}> of type <{}>: usage {}, guaratnee {}'.format(self.name, self.tp, prettify_size(self.usage), prettify_size(self.guarantee))


    def __init__(self, name):
        self.name = name
        self.volumes = dict()

        # init empirical volumes
        for volume_name, volume_type in EMPIRICAL_VOLUMES.iteritems():
            if volume_name not in self.volumes:
                self.volumes[volume_name]= TGroupVolumeInfo.TVolumeInfo(volume_name, volume_type)

        # init volumes of group
        if CURDB.groups.has_group(name):
            group = CURDB.groups.get_group(name)
            for volume_info in volumes_as_objects(group):
                if volume_info.guest_mp == '':
                    volume_name = '/workdir'
                else:
                    volume_name = volume_info.guest_mp

                if volume_info.host_mp_root == '/ssd':
                    volume_tp = EStorageType.SSD
                else:
                    volume_tp = EStorageType.HDD

                volume_guarantee = volume_info.quota.value * len(group.get_kinda_busy_instances())

                self.volumes[volume_name] = TGroupVolumeInfo.TVolumeInfo(volume_name, volume_tp)
                self.volumes[volume_name].guarantee = volume_guarantee

    def add_stats(self, data):
        # add info on empirical volumes
        for volume_name, volume_type in EMPIRICAL_VOLUMES.iteritems():
            self.volumes[volume_name].usage += float(data[volume_name])

        # add real volumes info
        for volume_name, volume_usage in simplejson.loads(data['volumes_size']).iteritems():
            if volume_name not in self.volumes:
                self.volumes[volume_name] = TGroupVolumeInfo.TVolumeInfo(volume_name, EStorageType.HDD)
            self.volumes[volume_name].usage += volume_usage

    def __str__(self):
        result = ['Group {}:'.format(self.name)]
        for volume_name in sorted(self.volumes):
            result.append('    {}'.format(self.volumes[volume_name]))
        return '\n'.join(result)


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


def have_stats_at(ts):
    """Check if have stats at specified timestamp"""
    ed = ts_to_event_date(ts)

    query = "SELECT count(*) FROM group_volumes_aggregated WHERE eventDate = '{ed}' and ts = {ts}".format(ed=ed, ts=ts)

    result = run_query(query)

    return len(result) > 0


def get_instanceusage_data_at(options, ts):
    """Get data from instanceusage_infreq_v2 at specified timestamp"""
    start_ts = ts - 24 * 60 * 60
    start_ed = ts_to_event_date(start_ts)
    end_ts = ts
    end_ed = ts_to_event_date(ts)

    # execute query
    fields = ['host', 'port', 'ts', 'group'] + EMPIRICAL_VOLUMES.keys() + ['volumes_size']
    query_tpl = ("SELECT {{fields}} FROM instanceusage_infreq_v2 WHERE eventDate >= '{start_ed}' AND "
                 "eventDate <= '{end_ed}' AND ts >= {start_ts} AND ts <= {end_ts}").format(start_ts=start_ts, end_ts=end_ts, start_ed=start_ed, end_ed=end_ed)
    raw_result = run_query_dict(query_tpl, fields)

    # filter out old values (only one value with same (host, port, ts, group)
    result = dict()
    for elem in raw_result:
        key = (elem['host'], elem['port'], elem['group'])
        if key not in result:
            result[key] = elem

        if elem['ts'] > result[key]['ts']:
            result[key] = elem

    return result.values()


def main_addstats_at(options, ts):
    """Add stats at specified timestamp"""
    print 'Adding stats at {}:'.format(ts)

    if have_stats_at(ts):
        print red_text('    Stats already in database, do not add')
        return

    usage_data = get_instanceusage_data_at(options, ts)

    # calculate per-group usage
    usage_by_group = dict()
    for elem in usage_data:
        groupname = elem['group']
        if groupname not in usage_by_group:
            usage_by_group[groupname] = TGroupVolumeInfo(groupname)
        usage_by_group[groupname].add_stats(elem)

    # insert data to table
    insert_data = []
    for group_usage in usage_by_group.itervalues():
        for volume_info in group_usage.volumes.itervalues():
            insert_data.append("({ts}, '{group}', '{eventDate}', '{volume_name}', '{volume_storage_type}', {volume_usage}, {volume_guarantee})".format(
                ts=ts, group=group_usage.name, eventDate=ts_to_event_date(ts), volume_name=volume_info.name, volume_storage_type=volume_info.tp,
                volume_usage=volume_info.usage, volume_guarantee=volume_info.guarantee
            ))
    insert_query = 'INSERT INTO group_volumes_aggregated VALUES {}'.format(', '.join(insert_data))
    run_query(insert_query)

    print '    Added {} documents'.format(len(insert_data))


def main_addstats(options):
    main_addstats_at(options, int(time.time()) / PERIOD * PERIOD)


def main_fillstats(options):
    if options.startts is None:
        options.startts = int(run_query('SELECT max(ts) FROM group_volumes_aggregated')[0][0]) + 1
    startts = options.startts + PERIOD - options.startts % PERIOD

    if options.endts is None:
        options.endts = int(time.time())
    endts = options.endts + PERIOD - options.endts % PERIOD

    add_stats_timestamps = range(startts, endts, PERIOD)

    for ts in add_stats_timestamps:
        main_addstats_at(options, ts)


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

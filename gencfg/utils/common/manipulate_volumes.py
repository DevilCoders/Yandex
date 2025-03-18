#!/skynet/python/bin/python

"""Script to manipulate groups volumes

Manipulate groups volumes:
    - <show>: show volumes for specified groups
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import json
import time
import datetime
from collections import defaultdict

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
import gaux.aux_volumes
import gaux.aux_clickhouse
import pytz


class EActions(object):
    SHOW = 'show'  # show groups volumes
    REMOVE = 'remove'  # remove specified volume
    MODIFY = 'modify'  # modify specified volume
    ADD = 'add'  # add new volume
    PUT = 'put'  # put volumes info from json
    KSTATS = 'kstats'  # clickhouse stats for group
    ALL = [SHOW, REMOVE, MODIFY, ADD, PUT, KSTATS]


def get_parser():
    parser = ArgumentParserExt(description='Perform various actions with groups volumes')
    parser.add_argument('-a', '--action', type=str, required=True,
                        choices=EActions.ALL,
                        help='Obligatory. Action to execute')
    parser.add_argument('-g', '--groups', type=argparse_types.groups, required=True,
                        help='Obligatory. List of groups to process')
    parser.add_argument('-o', '--volume', type=str, default=None,
                        help='Optional. Volume mount potin for actions <{EActions.REMOVE},{EActions.MODIFY},{EActions.ADD}>'.format(EActions=EActions))
    parser.add_argument('-p', '--params', type=argparse_types.kvlist, default=None,
                        help='Optional. Volume params to modify for actions <{EActions.MODIFY},{EActions.ADD}>'.format(EActions=EActions))
    parser.add_argument('-j', '--json-volumes', type=str, default=None,
                        help='Optional. Volumes info as json for actions <{EActions.PUT}>'.format(EActions=EActions))

    return parser


def normalize(options):
    if options.action in (EActions.REMOVE, EActions.MODIFY, EActions.ADD) and options.volume is None:
        raise Exception('You must specify <--volume> option with action <{}>'.format(options.action))
    if options.action in (EActions.MODIFY, EActions.ADD) and options.params is None:
        raise Exception('You must specify <--params> option with action <{}>'.format(options.action))
    if options.action == EActions.ADD:
        missing_keys = set(['host_mp_root', 'quota']) - set(options.params.keys())
        if missing_keys:
            raise Exception('Missing params <{}> in <--params> options'.format(','.join(missing_keys)))
    if (options.action == EActions.PUT) and (options.json_volumes is None):
        raise Exception('You must specfiy <--json> option with action <{}>'.format(options.action))

    if options.action == EActions.PUT:
        options.json_volumes = json.loads(options.json_volumes)


def main_remove(options):
    for group in options.groups:
        group_volumes = gaux.aux_volumes.volumes_as_objects(group)
        if options.volume not in (x.guest_mp for x in group_volumes):
            raise Exception('Can not remove volume <{}> from group <{}>: volume does not exists'.format(options.volume, group.card.name))

        remove_volume = [x for x in group_volumes if x.guest_mp == options.volume][0]

        group_volumes = [x for x in group_volumes if x != remove_volume]
        group.card.reqs.volumes = [x.to_card_node() for x in group_volumes]
        group.mark_as_modified()

        print 'Group {}:'.format(group.card.name)
        print '    Remove volume: {}'.format(remove_volume)

    CURDB.update(smart=True)


def main_modify(options):
    for group in options.groups:
        print 'Group {}:'.format(group.card.name)
        group_volumes = gaux.aux_volumes.volumes_as_objects(group)
        if options.volume not in (x.guest_mp for x in group_volumes):
            raise Exception('Can not modify volume <{}> from group <{}>: volume does not exists'.format(options.volume, group.card.name))

        remove_volume = [x for x in group_volumes if x.guest_mp == options.volume][0]
        print '    Modify volume {}:'.format(remove_volume.guest_mp)

        for k, v in options.params.iteritems():
            old_value = getattr(remove_volume, k)
            new_value = gaux.aux_volumes.TVolumeInfo.convert_value_from_str(k, v)
            setattr(remove_volume, k, new_value)
            print '        {}: {} -> {}'.format(k, old_value, new_value)

        group.card.reqs.volumes = [x.to_card_node() for x in group_volumes]
        gaux.aux_volumes.update_ssd_hdd_group_reqs(group)
        for volume in group.card.reqs.volumes:
            volume.uuid_generator_version = 2
        group.mark_as_modified()

    CURDB.update(smart=True)


def main_show(options):
    """Show volumes of specified groups"""
    for group in options.groups:
        group_volumes = gaux.aux_volumes.volumes_as_objects(group)
        print 'Group {}:'.format(group.card.name)
        for group_volume in group_volumes:
            print '    {}'.format(group_volume)


def main_add(options):
    volume_params = {k: gaux.aux_volumes.TVolumeInfo.convert_value_from_str(k, v) for k, v in options.params.iteritems()}
    volume_params['guest_mp'] = options.volume

    for group in options.groups:
        group_volumes = gaux.aux_volumes.volumes_as_objects(group)
        new_volume = gaux.aux_volumes.TVolumeInfo(**volume_params)
        group_volumes.append(new_volume)

        group.card.reqs.volumes = [x.to_card_node() for x in group_volumes]
        gaux.aux_volumes.update_ssd_hdd_group_reqs(group)
        for volume in group.card.reqs.volumes:
            volume.uuid_generator_version = 2
        group.mark_as_modified()

        print 'Group {}:'.format(group.card.name)
        print '     Add volume: {}'.format(new_volume)

    CURDB.update(smart=True)


def main_put(options):
    volumes = [gaux.aux_volumes.TVolumeInfo.from_json(x) for x in options.json_volumes]
    volumes.sort(key=lambda x: x.guest_mp)

    for group in options.groups:
        group.card.reqs.volumes = [x.to_card_node() for x in volumes]
        gaux.aux_volumes.update_ssd_hdd_group_reqs(group)
        group.mark_as_modified()

    CURDB.update(smart=True)


def group_kstats(by_volume_instance_data, group_name, options):
    # group values
    for volume_name in by_volume_instance_data:
        for host_port in by_volume_instance_data[volume_name]:
            l = by_volume_instance_data[volume_name][host_port]
            v = sum(l) / len(l)
            by_volume_instance_data[volume_name][host_port] = v

    # calculate 99 median
    result = dict()
    for volume_name in by_volume_instance_data:
        all_sizes = by_volume_instance_data[volume_name].values()
        all_sizes.sort()
        volume_size = all_sizes[int(0.99 * len(all_sizes))]
        result[volume_name] = volume_size

    # print result
    print 'Group {}:'
    for volume_name in sorted(result):
        if result[volume_name] > 0:
            print '   Volume {}: {:.2f} Gb'.format(volume_name, result[volume_name] / 1024. / 1024 / 1024)


def main_kstats(options):
    OLD_VOLUMES_NAMES = ('workdir_hdd_size', 'workdir_ssd_size', 'iss_resources_hdd_size', 'iss_resources_ssd_size', 'iss_shards_hdd_size',
                         'iss_shards_ssd_size', 'webcache_hdd_size', 'webcache_ssd_size', 'callisto_hdd_size', 'callisto_ssd_size', 'logs_hdd_size',
                         'logs_ssd_size')

    # create clickhouse query
    startt = int(time.time()) - 24 * 60 * 60
    start_event_date = datetime.datetime.fromtimestamp(startt, tz=pytz.timezone("Europe/Moscow")).strftime('%Y-%m-%d')
    group_in = '({})'.format(', '.join("'{}'".format(x.card.name) for x in options.groups))
    old_volumes_names_str = ', '.join(OLD_VOLUMES_NAMES)

    clickhouse_query = ("SELECT group, host, port, volumes_size, {old_volumes_names_str} "
                        "FROM instanceusage_infreq_v2 "
                        "WHERE eventDate == '{start_event_date}' AND group IN {group_in}").format(old_volumes_names_str=old_volumes_names_str,
                                                                                                  start_event_date=start_event_date, group_in=group_in)

    # run clickhouse query and load data
    clickhouse_result = gaux.aux_clickhouse.run_query(clickhouse_query)
    by_volume_instance_data = defaultdict(lambda: defaultdict(lambda: defaultdict(list)))

    for elem in clickhouse_result:
        group = elem[0]
        host = elem[1]
        port = elem[2]
        volumes_size = json.loads(elem[3])
        old_volumes = elem[4:]

        # process new volumes
        for volume_name, volume_size in volumes_size.iteritems():
            volume_name = 'volume_{}'.format(volume_name)
            by_volume_instance_data[group][volume_name][(host, port)].append(volume_size)

        # process old volumes
        for old_volume_name, old_volume_size in zip(OLD_VOLUMES_NAMES, old_volumes):
            by_volume_instance_data[group][old_volume_name][(host, port)].append(int(old_volume_size))

    for group in options.groups:
        group_kstats(by_volume_instance_data.get(group.card.name, dict()), group.card.name, options)


def main(options):
    if options.action == EActions.SHOW:
        main_show(options)
    elif options.action == EActions.REMOVE:
        main_remove(options)
    elif options.action == EActions.MODIFY:
        main_modify(options)
    elif options.action == EActions.ADD:
        main_add(options)
    elif options.action == EActions.PUT:
        main_put(options)
    elif options.action == EActions.KSTATS:
        main_kstats(options)
    else:
        raise Exception('Unknown action <{}>'.format(options.action))


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)

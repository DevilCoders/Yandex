#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
from core.db import CURDB, DB
from core.histdb.events import NewTagEvents, GroupEvents, HostEvents, EEventTypes, EGroupEvents, EHostEvents
from gaux.aux_colortext import red_text


def _parse_cmd():
    parser = ArgumentParser(description="Added events for new tag")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=["show", "apply"],
                        help="Obligatory. Action to execute")
    parser.add_argument("-p", "--tagpath", dest="tagpath", type=str, required=True,
                        help="Obligatory. Path to directory with tag")
    parser.add_argument("-t", "--timestamp", dest="timestamp", type=int, default=None,
                        help="Optional. Timestamp")
    parser.add_argument("-r", "--process", dest="process", type=str, default=','.join(EEventTypes.ALL),
                        choices=EEventTypes.ALL,
                        help="Optional. Process only specified types: comma-separated list of types: %s" % ','.join(
                            EEventTypes.ALL))

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    options.process = options.process.split(',')

    return options


def _add_tag_events(options, tagdb):
    tagname = tagdb.get_repo().get_current_tag()

    # =========================================================
    # create tag events
    # =========================================================
    if EEventTypes.TAG in options.process:
        if CURDB.histdb.has_events(event_type='TAG', event_object_id=tagname) > 0:
            raise Exception("Already have tag event with name %s" % tagname)
        NewTagEvents.new_tag(tagname, event_date=options.timestamp)

    # =========================================================
    # create group tag events
    # =========================================================
    if EEventTypes.GROUP in options.process:
        # FIXME: add remove event
        for group in tagdb.groups.get_groups():
            if group.card.master is not None:  # currently slave groups not supported
                continue
            if group.card.name in ['MSK_PPS_BASE', 'SAS_PPS_BASE']:  # some groups can not be properly converted
                continue
            if not CURDB.histdb.has_events(event_object_id=group.card.name, event_type=EEventTypes.GROUP):
                GroupEvents.group_added(group.card.name, event_date=options.timestamp)

            # check diff with previous stats
            last_stats = CURDB.histdb.get_events(event_object_id=group.card.name, event_type=EEventTypes.GROUP,
                                                 event_name=EGroupEvents.STATS, load=False)
            if len(last_stats):
                last_stats = last_stats[-1]
            else:
                last_stats = None

            new_stats = GroupEvents.group_stats_at_tag(group.card.name, tagname, tagdb, event_date=options.timestamp)

            # do not add statistics on groups that did not changed since previous tag
            if last_stats is not None and \
               new_stats.event_params['power'] == last_stats.event_params['power'] and \
               new_stats.event_params['instances'] == last_stats.event_params['instances'] and \
               new_stats.event_params['usedinstances'] == last_stats.event_params['usedinstances'] and \
               new_stats.event_params['disk'] == last_stats.event_params['disk'] and \
               new_stats.event_params['ssd'] == last_stats.event_params['ssd'] and \
               new_stats.event_params['memory'] == last_stats.event_params['memory']:
                new_stats.new_event = False

    # =========================================================
    # create host events
    # =========================================================
    if EEventTypes.HOST in options.process:
        # better load all events at the beginning
        CURDB.histdb.load_events()
        # find all existing hosts
        existing_hosts = set()
        for event in CURDB.histdb.get_events(event_type=EEventTypes.HOST, load=False):
            if event.event_name == EHostEvents.ADDED:
                #                if event.event_object.object_id in existing_hosts:
                #                    print "Host %s added twice" % event.event_object.object_id
                #                assert(event.event_object.object_id not in existing_hosts)
                existing_hosts.add(event.event_object.object_id)
            else:
                #                assert event.event_object.object_id in existing_hosts, "Host object id %s not in existing hosts (event_id %s)" % (event.event_object.object_id, event.event_id)
                if event.event_name == EHostEvents.REMOVED:
                    existing_hosts.add(event.event_object.object_id)
        # add existing hosts which somehow was not added
        for host in tagdb.hosts.get_hosts():
            if host.name not in existing_hosts:
                HostEvents.host_added(host, options.timestamp)
                HostEvents.host_hwfields_modified(host, options.timestamp)
        # add hwmidified to existing hosts
        for host in tagdb.hosts.get_hosts():
            last_hwmodified = CURDB.histdb.get_events(event_object_id=host.name, event_type=EEventTypes.HOST,
                                                      event_name=EHostEvents.HWMODIFIED, load=False)
            if len(last_hwmodified):
                last_hwmodified = last_hwmodified[-1]
            else:
                last_hwmodified = HostEvents.host_hwfields_modified(host, options.timestamp)
            modified = False
            for field in HostEvents.HOST_HWFIELDS:
                if last_hwmodified.event_params[field] != getattr(host, field, None):
                    modified = True
            if modified:
                HostEvents.host_hwfields_modified(host, options.timestamp)
        # change group host belongs to
        for host in tagdb.hosts.get_hosts():
            # find current group name, previous group name, compare and add event if they mismatch
            hostgroups = filter(lambda x: x.card.master is None, tagdb.groups.get_host_groups(host))
            hostgroups = filter(
                lambda x: x.card.properties.background_group == False and x.card.properties.fake_group == False,
                hostgroups)

            if len(hostgroups) == 0:
                print red_text("Host %s not in group" % host.name)
                continue
            curgroupname = hostgroups[0].card.name  # Can still have more than 1 master group

            events = CURDB.histdb.get_events(event_object_id=host.name, event_type=EEventTypes.HOST,
                                             event_name=EHostEvents.GROUPCHANGED, load=False)
            if len(events) == 0:
                lastgroupname = None
            else:
                lastgroupname = events[-1].event_params['groupname']

            if curgroupname != lastgroupname:
                HostEvents.host_group_changed(host, curgroupname, tagname, options.timestamp)

                # ===========================================================
                # create metagroup tag events
                # ===========================================================
                # temporary to not save metagroups events !!!
            #    if EEventTypes.METAGROUP in options.process:
            # add extra metagroups from main config
            #        metagroups_config = argparse_types.yamlconfig(os.path.join(CURDB.CONFIG_DIR, 'other', 'metagroups.yaml'))
            #        for metagroup_config in metagroups_config:
            #            events = CURDB.histdb.get_events(event_object_id = metagroup_config['descr'], flt = lambda x: x.event_name in [EMetaGroupEvents.ADDED, EMetaGroupEvents.REMOVED])
            #            if len(events) == 0 or events[-1].event_name == EMetaGroupEvents.REMOVED:
            #                MetaGroupEvents.metagroup_added(metagroup_config['descr'], metagroup_config['filters'], options.timestamp)

            # find all non-removed events
            #        metagroup_ids = set(map(lambda x: x.event_object.object_id, CURDB.histdb.get_events(event_type = EEventTypes.METAGROUP, event_name = EMetaGroupEvents.ADDED)))
            #        events_to_process = []
            #        for metagroup_id in metagroup_ids:
            #            events = CURDB.histdb.get_events(event_object_id = metagroup_id, event_type = EEventTypes.METAGROUP, flt = lambda x: x.event_name in [EMetaGroupEvents.ADDED, EMetaGroupEvents.REMOVED])
            #            if events[-1].event_name == EMetaGroupEvents.ADDED:
            #                events_to_process.append(events[-1])

            # run show metagroups power and save results
            #        config = map(lambda x: { 'descr' : x.event_object.object_id, 'filters' : x.event_params['filters'] }, events_to_process)
            #        params = { 'config' : config, 'ignore_intersection' : False, 'other_statistics' : True, 'verbose' : False, 'searcherlookup' : None }
            #        from utils.common import show_metagroups_power
            #        rr = show_metagroups_power.main(type("DummyShowMetagroupsPowerParams", (), params)(), tagdb, False)
            #        for k in rr['metagroups_stats']:
            #            MetaGroupEvents.metagroup_stats_at_tag(rr['metagroups_stats'][k], tagname, options.timestamp)

    if options.action == "show":
        CURDB.histdb.show_events(flt=lambda x: x.new_event == True, load=False)
    elif options.action == "apply":
        CURDB.histdb.update()


def main(options):
    # Init histdb. FIXME: get rid of this
    CURDB.histdb

    with DB(options.tagpath) as tagdb:
        if options.timestamp is None:
            options.timestamp = tagdb.get_repo().get_last_commit().date
        _add_tag_events(options, tagdb)


if __name__ == '__main__':
    options = _parse_cmd()
    main(options)

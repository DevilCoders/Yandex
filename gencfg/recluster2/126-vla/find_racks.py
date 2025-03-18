import collections
import non_moveable

import sys
sys.path.append('.')
import core.db

def filter_racks():
    db = core.db.CURDB
    vla_yt_rtc = db.groups.get_group('VLA_YT_RTC')

    # find groups on rack, filter by processor model
    rack_groups = collections.defaultdict(set)
    rack_hosts = collections.defaultdict(set)
    for group in vla_yt_rtc.slaves:
        for host in group.getHosts():
            if host.model == 'E5-2660v4':
                rack_groups[host.rack].add(group.card.name)
                rack_hosts[host.rack].add(host.name)

    # filter by non-moveable groups
    for rack, groups in rack_groups.iteritems():
        if non_moveable.groups.intersection(groups):
            del rack_hosts[rack]

    # filter by 36 hosts in rack (full search + arnold rack)
    full_racks = [rack for rack, hosts in rack_hosts.iteritems() if len(hosts) == 36]

    # take any 14 racks
    return {
        rack: {
            'hosts': rack_hosts[rack],
            'groups': rack_groups[rack]
        } for rack in full_racks[:14]
    }


def main():
    rack_hosts = filter_racks()
    for rack in rack_hosts:
        print rack;

    hosts = set()
    groups = set()
    for rack_details in rack_hosts.itervalues():
        hosts |= rack_details['hosts']
        groups |= rack_details['groups']


    with open('hosts.txt', 'w') as f:
        for host in sorted(hosts):
            print >> f, host

    with open('groups.txt', 'w') as f:
        for group in sorted(groups):
            print >> f, group

if __name__ == '__main__':
    main()

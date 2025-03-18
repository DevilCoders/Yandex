#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser
from collections import defaultdict
import re
import copy

import gencfg
from core.db import CURDB
from config import BRANCH_VERSION
from utils.mongo import show_hosts_from_instancestate


OMIT_BACKGROUND_GROUPS = [
    'SAS_YASM_YASMAGENT_PRESTABLE', 'ALL_YASM_YASMAGENT_STABLE', 'ALL_INFORMANT_PRODUCTION', 'MAN_YASM_YASMAGENT_UNISTAT_R1',
    'MSK_MAIL_LUCENE', 'SAS_MAIL_LUCENE', 'MAN_MAIL_LUCENE', 'MAN_YT_PROD_PORTOVM', 'MSK_RTC_SLA_TENTACLES_PROD',
    'SAS_RTC_SLA_TENTACLES_PROD', 'SAS_PSI_RESERVED_AGENTS', 'MAN_RTC_SLA_TENTACLES_PROD', 'VLA_RTC_SLA_TENTACLES_PROD',
    'VLA_PSI_RESERVED_AGENTS',
]


class TInstanceInfo(object):
    def __init__(self, host, port, major_tag, minor_tag, groupname, owners):
        self.host = host
        self.port = port
        self.major_tag = major_tag
        self.minor_tag = minor_tag
        self.groupname = groupname
        self.owners = owners

    @classmethod
    def create_from_instancestate(cls, data):
        """Create from instancestate"""
        instance, instance_props = data

        # parse host/port
        host, _, port = instance.partition(':')
        try:
            port = int(port)
        except:  # FIXME: bullshit
            port = 0

        # parse tags
        tv = instance_props.get('tv', 0)
        if tv > 1000000000:  # that big tv means, that instance is run from trunk rather than from tag
            major_tag = 0
            minor_tag = 0
        else:
            major_tag = instance_props.get('tv', 0) / 1000000
            minor_tag = instance_props.get('tv', 0) % 1000000

        # try to parse group
        m = re.search('.*a_topology_group-([^ ]+).*', instance_props.get('tags', ''))
        if m:
            groupname = m.group(1)
            if CURDB.groups.has_group(groupname):
                owners = copy.copy(CURDB.groups.get_group(groupname).card.owners)
            else:
                owners = []
        else:
            groupname = None
            owners = []

        return cls(host, port, major_tag, minor_tag, groupname, owners)

    def __str__(self):
        return '{host}:{port}: -> stable-{major_tag}/r{minor_tag} (group {groupname}, owners {owners})'.format(
                    host=self.host, port=self.port, major_tag=self.major_tag, minor_tag=self.minor_tag,
                    groupname=self.groupname, owners=','.join(self.owners)
               )


def print_verbose(options, message_verbose, message):
    if options.verbose >= message_verbose:
        print message


def parse_cmd():
    ACTIONS = ["show_brief", "show_busy_detailed", "show_busy_by_owner", "show_all"]

    import core.argparse.types as argparse_types  # have to import here

    parser = ArgumentParser(description="Show hosts with raised instances")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=ACTIONS,
                        help="Obligatory. Action to execute: %s" % ','.join(ACTIONS))
    parser.add_argument("-p", "--transport", type=str, default="instancestate",
                        choices=["instancestate", ],
                        help="Obligatory. Method to recieve instance data: by skynet or by reading instancestate info")
    parser.add_argument("-s", "--hosts", type=argparse_types.grouphosts, required=False,
                        help="Optional. List of groups or hosts to process, e.g. MSK_RESERVED,ws2-200.yandex.ru,MAN_RESERVED")
    parser.add_argument("-x", "--exclude-groups", type=argparse_types.groups, default=[],
                        help="Optional. List of groups to ignore when calculating used hosts")
    parser.add_argument("-t", "--timeout", type=int, default=50,
                        help="Optional. Skynet timeout")
    parser.add_argument("-f", "--filter", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Filter on hosts")
    parser.add_argument("-r", "--move-to-reserved", action="store_true", default=False,
                        help="Optional. Move all hosts to reserved (incompatable with --move-to-group)")
    parser.add_argument("--move-to-group", type=argparse_types.group, default=None,
                        help="Optional. Move all unused hosts to specified group (incompatable with --move-to-reserved)")
    parser.add_argument("--move-used-to-group", type=argparse_types.group, default=None,
                        help="Optional. Move all still used hosts to specified group")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Increase output verbosity (maximum 2)")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def get_result_with_instancestate(hosts):
    suboptions = {
        "action": "show_per_host",
        "groups": [],
        "hosts": hosts,
    }

    suboptions = show_hosts_from_instancestate.get_parser().parse_json(suboptions)
    subresult = show_hosts_from_instancestate.main(suboptions)

    import gencfg
    from core.db import CURDB

    for hostname in subresult:
        if subresult[hostname] is None:
            subresult[hostname] = None
        else:
            filtered = filter(lambda x: x[1].get('a', 0) == 1, subresult[hostname])
            subresult[hostname] = [TInstanceInfo.create_from_instancestate(x) for x in filtered]
    return subresult


def normalize(options):
    return options


def main(options):
    options = normalize(options)

    hosts = options.hosts
    hosts = filter(options.filter, hosts)
    if len(hosts) == 0:
        return dict()

    if options.transport == "instancestate":
        result = get_result_with_instancestate(hosts)
    else:
        raise Exception("Unknown transport %s" % options.transport)

    # filter result excluding instances of exclude groups
    exclude_group_names = set([x.card.name for x in options.exclude_groups] + OMIT_BACKGROUND_GROUPS)
    for hostname in result.keys():
        if result[hostname] is not None:
            result[hostname] = [x for x in result[hostname] if x.groupname not in exclude_group_names]

    # move all unused hosts to reserved group
    if options.move_to_reserved != False or options.move_to_group is not None:
        # find hosts to move to resered
        hosts_to_reserved = []
        for hostname, instances_info in result.iteritems():
            if instances_info is None:
                instances_info = []
            filtered_instances_info = []
            for instance_info in instances_info:
                if instance_info.groupname is None:
                    continue
                if CURDB.groups.has_group(instance_info.groupname) and CURDB.groups.get_group(instance_info.groupname).card.properties.background_group:
                    continue
                filtered_instances_info.append(instance_info)
            if len(filtered_instances_info) == 0:
                hosts_to_reserved.append(CURDB.hosts.get_host_by_name(hostname))

        if options.move_to_group:
            print_verbose(options, 1, "Move to group %s: %s" % (
            options.move_to_group.card.name, ' '.join(map(lambda x: x.name, hosts_to_reserved))))
            CURDB.groups.move_hosts(hosts_to_reserved, options.move_to_group)
            CURDB.update(smart=True)
        else:
            print_verbose(options, 1, "Move to reserved: %s" % (' '.join(map(lambda x: x.name, hosts_to_reserved))))
            for host in hosts_to_reserved:
                reserved_group = CURDB.groups.get_group("%s_RESERVED" % host.location.upper())
                CURDB.groups.move_host(host, reserved_group)
            CURDB.update(smart=True)

    # move all still used hosts to specified busy group
    if options.move_used_to_group is not None:
        hosts = map(lambda y: CURDB.hosts.get_host_by_name(y), filter(lambda x: result[x] != [], result))
        print_verbose(options, 1, "Move still used hosts to group %s: %s" % (
        options.move_used_to_group.card.name, ' '.join(map(lambda x: x.name, hosts))))
        CURDB.groups.move_hosts(hosts, options.move_used_to_group)
        CURDB.update(smart=True)

    return result


def is_old_topology_host(ilist):
    """Check ilist contains only instances with old topology or with trunk tag"""
    if ilist is None:
        return False
    if len(ilist) == 0:
        return False

    for instance_info in ilist:
        if instance_info.major_tag >= int(BRANCH_VERSION) - 4:
            return False

    return True


def print_result(options, result):
    failure_hosts = filter(lambda x: result[x] is None, result.keys())
    unused_hosts = filter(lambda x: result[x] == [], result.keys())
    old_topology_hosts = [x for (x, y) in result.iteritems() if is_old_topology_host(y)]
    used_hosts = list(set(result.keys()) - set(failure_hosts) - set(unused_hosts) - set(old_topology_hosts))

    if options.action in ["show_brief", "show_all"]:
        print "Failure hosts: %s" % ','.join(failure_hosts)
        print "Unused hosts: %s" % ','.join(unused_hosts)
        print "Old topology hosts: %s" % ','.join(old_topology_hosts)
        print "Used hosts: %s" % ','.join(used_hosts)

    if options.action in ["show_busy_detailed", "show_all"]:
        print "Old topology hosts:"
        for host in old_topology_hosts:
            print "    Host %s:" % host
            for instance_info in result[host]:
                print '        {}'.format(instance_info)
        print "Still used hosts:"
        for host in used_hosts:
            print "    Host %s:" % host
            for instance_info in result[host]:
                print '        {}'.format(instance_info)
    if options.action in ["show_busy_by_owner", "show_all"]:
        by_owners_data = defaultdict(list)
        for instance_info in sum(filter(lambda x: x is not None, result.itervalues()), []):
            if not instance_info.owners:
                continue
            by_owners_data[','.join(sorted(instance_info.owners))].append(instance_info)

        print "Used hosts by owners:"
        for owners in by_owners_data:
            owners_instances = ['{}:{}:({})'.format(x.host, x.port, x.groupname) for x in by_owners_data[owners]]
            print '    Owner {}: {}'.format(owners, ','.join(owners_instances))


if __name__ == '__main__':
    options = parse_cmd()
    result = main(options)
    print_result(options, result)

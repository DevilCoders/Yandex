#!/skynet/python/bin/python
"""Script to alloc hosts for 4rth MAIL_LUCENE replica (GENCFG-1015)"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from core.db import CURDB

from utils.pregen.find_most_memory_free_machines import HostInfo


def get_parser():
    parser = ArgumentParserExt(description="Generate extra replica for lucene")
    parser.add_argument("-g", "--group", type=argparse_types.group, required=True,
                        help="Obligatory. Group to add instnaces for")
    parser.add_argument("-t", "--tier", dest='shards_count', type=argparse_types.primus_int_count, required=True,
                        help="Obligatory. Tier for intlookup")
    parser.add_argument("-s", "--source-groups", type=argparse_types.groups, required=True,
                        help="Obligatory. List of groups to get hosts from")
    parser.add_argument("-x", "--skip-groups", type=argparse_types.groups, required=True,
                        help="Obligatory. List of groups to skip")
    parser.add_argument("-f", "--filter", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Function to filter hosts")

    return parser


def main(options):
    # apply basic filters
    source_hosts = sum((x.getHosts() for x in options.source_groups), [])
    print 'Source hosts: {}'.format(len(source_hosts))
    source_hosts = {x for x in source_hosts if options.filter(x)}
    print 'Source hosts after filter: {}'.format(len(source_hosts))
    skip_hosts = sum((x.getHosts() for x in options.skip_groups + [options.group]), [])
    source_hosts -= set(skip_hosts)
    print 'Source hosts after exclude: {}'.format(len(source_hosts))

    # filter hosts with enough memory
    required_memory = options.group.card.reqs.instances.memory_guarantee.gigabytes()
    source_hosts = {x: HostInfo(x) for x in source_hosts}
    affected_groups = set()
    for host in source_hosts:
        affected_groups |= set(CURDB.groups.get_host_groups(host))
    busy_instances = set()
    for group in affected_groups:
        busy_instances |= set(group.get_kinda_busy_instances())
    for instance in busy_instances:
        if instance.host in source_hosts:
            source_hosts[instance.host].append_instance(instance)
    source_hosts = [x for x in source_hosts.itervalues() if x.memory_left > required_memory]
    print 'Source hosts after memory filter: {}'.format(len(source_hosts))


    # check if we have enough hosts
    if len(source_hosts) < options.shards_count:
        raise Exception('Have only <{}> instances while need <{}>'.format(len(source_hosts), options.shards_count))

    # add needed hosts and add them to group
    source_hosts_by_dc = defaultdict(list)
    for host in source_hosts:
        source_hosts_by_dc[host.host.dc].append(host)
    for lst in source_hosts_by_dc.itervalues():
        lst.sort(key=lambda x: -x.memory_left)
    source_hosts = []
    for i in xrange(options.shards_count):
        for lst in source_hosts_by_dc.itervalues():
            if len(lst) > i:
                source_hosts.append(lst[i])
    source_hosts = source_hosts[:options.shards_count]
    source_hosts = [x.host for x in source_hosts]
    CURDB.groups.add_hosts(source_hosts, options.group)

    CURDB.update(smart=True)

if __name__ == '__main__':
    options = get_parser().parse_cmd()

    main(options)

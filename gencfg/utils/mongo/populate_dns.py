#!/skynet/python/bin/python

"""Upload new dns info to mongo (GENCFG-1761)"""

import os
import sys
import collections

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
from gaux.aux_hbf import generate_mtn_addr, generate_mtn_hostname
import gaux.aux_mongo
import core.argparse.types


class EActions(object):
    """All actions"""
    STATS = 'stats'
    EXPORT = 'export'
    EXPORT_TO_FILE = 'export-to-file'
    HOSTINFO = 'hostinfo'
    ALL = [STATS, EXPORT, EXPORT_TO_FILE, HOSTINFO]


Dns = collections.namedtuple('HostIp', ['hostname', 'ipv6addr'])
Record = collections.namedtuple('Record', ['dns', 'group', 'commit'])


def get_parser():
    parser = ArgumentParserExt(description='Upload dns info for hbf_mtn containers to mongo')
    parser.add_argument('-a', '--action', type=str, required=True,
                        choices=EActions.ALL,
                        help='Obligatory. Action to execute: one of <{}>'.format(','.join(EActions.ALL)))
    parser.add_argument('-s', '--hosts', type=core.argparse.types.comma_list, default=None,
                        help='Optional. List of hosts to get info for (for action <{}>)'.format(EActions.HOSTINFO))
    parser.add_argument('-f', '--target-file', type=str, required=False)

    return parser


def normalize(options):
    if options.action == EActions.HOSTINFO:
        if options.hosts is None:
            raise Exception('You must specify <--hosts> param in action <{}>'.format(EActions.HOSTINFO))


def get_mongo_data(options):
    mongo_data = []
    coll = gaux.aux_mongo.get_mongo_collection('gencfg_dns')
    for elem in coll.find():
        mongo_data.append(Record(
            dns=Dns(hostname=elem['hostname'], ipv6addr=elem['ipv6addr']),
            group=elem.get('group'),
            commit=elem.get('commit'),
        ))

    return mongo_data


def get_gencfg_data():
    last_commit_id = CURDB.get_repo().get_last_commit_id()

    gencfg_data = []
    for group in CURDB.groups.get_groups():
        for instance in group.get_kinda_busy_instances():
            vlan688_ipv6addr = None
            if 'vlan688' in instance.host.vlans:
                vlan688_ipv6addr = generate_mtn_addr(instance, group, 'vlan688')
                fqdn = generate_mtn_hostname(instance, group, '', vlan688_ipv6addr)
                gencfg_data.append(Record(
                    dns=Dns(hostname=fqdn, ipv6addr=vlan688_ipv6addr),
                    group=group.card.name,
                    commit=last_commit_id,
                ))
            if 'vlan788' in instance.host.vlans:
                vlan788_ipv6addr = generate_mtn_addr(instance, group, 'vlan788')
                fqdn = generate_mtn_hostname(instance, group, 'fb-', vlan688_ipv6addr if vlan688_ipv6addr else vlan788_ipv6addr)
                gencfg_data.append(Record(
                    dns=Dns(hostname=fqdn, ipv6addr=vlan788_ipv6addr),
                    group=group.card.name,
                    commit=last_commit_id,
                ))

    return gencfg_data


def get_diff(records_a, records_b):
    diff = []
    records_b_dns = {record.dns for record in records_b}
    for record in records_a:
        if record.dns not in records_b_dns:
            diff.append(record)
    return diff


def main_stats(options):
    """Compare data in mongo with data in gencfg"""
    # load mongo data
    mongo_data = get_mongo_data(options)

    hosts_by_addr = collections.defaultdict(list)
    addrs_by_host = collections.defaultdict(list)
    for record in mongo_data:
        hosts_by_addr[record.dns.ipv6addr].append(record.dns.hostname)
        addrs_by_host[record.dns.hostname].append(record.dns.ipv6addr)

    # find conflicts
    hosts_by_addr = {x: y for x, y in hosts_by_addr.iteritems() if len(y) > 1}
    addrs_by_host = {x: y for x, y in addrs_by_host.iteritems() if len(y) > 1}

    print 'Mongo:'
    print '    Total: {} entries'.format(len(mongo_data))
    if len(hosts_by_addr):
        print '    Hosts with same addr ({} total):'.format(len(hosts_by_addr))
        for addr, hosts in sorted(hosts_by_addr.items())[:30]:
            print '        Addr {}: hosts {}'.format(addr, ','.join(hosts))
    if len(addrs_by_host):
        print '    Addrs with same host ({} total):'.format(len(addrs_by_host))
        for host, addrs in sorted(addrs_by_host.items())[:30]:
            print '        Host {}: addrs {}'.format(host, ','.join(addrs))

    # load gencfg data
    gencfg_data = get_gencfg_data()
    print 'Gencfg:'
    print '    Total: {} entries'.format(len(gencfg_data))

    # find diff
    gencfg_extra = get_diff(gencfg_data, mongo_data)
    print 'Diff:'
    print '    Gencfg extra:'
    print '        Total: {} entries'.format(len(gencfg_extra))
    for record in gencfg_extra[:30]:
        print '        Host {}, addr {}'.format(record.dns.hostname, record.dns.ipv6addr)


def main_export_to_file(options):
    """Export dns data to file"""
    import json

    data = []
    seen_host_ips = set()
    for rec in gaux.aux_mongo.get_mongo_collection('gencfg_dns').find(
        {},
        {'_id': 0, 'hostname': 1, 'ipv6addr': 1}
    ).sort('commit', -1):
        if 'hostname' not in rec or 'ipv6addr' not in rec:
            continue
        hostname, ipv6addr = rec['hostname'].lower(), rec['ipv6addr']
        if hostname not in seen_host_ips and ipv6addr not in seen_host_ips:
            seen_host_ips |= {hostname, ipv6addr}
            data.append({"host": hostname, "ip6": ipv6addr})

    with open(options.target_file, 'w') as f:
        json.dump(data, f, indent=2)

    export_trunk_to_file('_trunk.txt')


def export_trunk_to_file(filename):
    with open(filename, 'w') as f:
        for d in get_gencfg_data():
            print >> f, d.dns.hostname, d.dns.ipv6addr


def main_export(options):
    """Export dns data to mongo"""
    mongo_data = get_mongo_data(options)
    gencfg_data = get_gencfg_data()

    export_data = get_diff(gencfg_data, mongo_data)
    if export_data:
        print 'Exporting {} items'.format(len(export_data))
        dns_coll = gaux.aux_mongo.get_mongo_collection('gencfg_dns')
        dns_inserter = dns_coll.initialize_unordered_bulk_op()

        for record in export_data:
            dns_inserter.insert(dict(
                hostname=record.dns.hostname,
                ipv6addr=record.dns.ipv6addr,
                commit=record.commit,
                group=record.group,
            ))

        dns_inserter.execute()
    else:
        print 'Nothing to export'


def main_hostinfo(options):
    """Get all info in dns table on specified hosts"""
    # load data from mongo
    mongo_data = []
    coll = gaux.aux_mongo.get_mongo_collection('gencfg_dns')
    for elem in coll.find({ 'hostname': { '$in' : options.hosts }}):
        mongo_data.append((elem['hostname'], elem['ipv6addr'], elem['commit']))

    # print info on all hosts
    for hostname in options.hosts:
        ipaddrs = [(y, z) for (x, y, z) in mongo_data if x == hostname]
        print 'Host {} has {} entries in db:'.format(hostname, len(ipaddrs))
        for ipaddr, commit in ipaddrs:
            print '    Addr {} in commit {}'.format(ipaddr, commit)


def main(options):
    if options.action == EActions.STATS:
        main_stats(options)
    elif options.action == EActions.EXPORT:
        main_export(options)
    elif options.action == EActions.HOSTINFO:
        main_hostinfo(options)
    elif options.action == EActions.EXPORT_TO_FILE:
        main_export_to_file(options)
    else:
        raise Exception('Unknown action <{}>'.format(options.action))


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)

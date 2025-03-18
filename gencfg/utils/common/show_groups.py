#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser
from collections import OrderedDict
import urllib2
import json

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types
from gaux.aux_colortext import red_text
from core.argparse.parser import ArgumentParserExt
from core.settings import SETTINGS
import core.card.node
import gaux.aux_staff
from gaux.aux_hbf import generate_mtn_hostname


def show_name(group):
    return group.card.name


def show_power(group):
    return str(sum([host.power for host in group.getHosts()]))


def show_ipower(group):
    return str(sum(map(lambda x: x.power, group.get_kinda_busy_instances())))


def show_master(group):
    return group.card.master.card.name if group.card.master is not None else '-'


def show_owners(group):
    return ','.join(group.card.owners)


def show_resolved_owners(group):
    return ','.join(gaux.aux_staff.unwrap_dpts(group.card.owners))


def show_guest_owners(group):
    owners = group.card.owners + group.card.guest.owners
    owners = sorted(set(gaux.aux_staff.unwrap_dpts(owners)))
    return ','.join(owners)

def show_watchers(group):
    return ','.join(group.card.watchers)


def show_expiration_date(group):
    return str(group.card.properties.expires)


def show_slaves(group):
    return ','.join([slave.card.name for slave in group.slaves])


def show_tags(group):
    return ','.join(
        ['a_ctype_%s' % group.card.tags.ctype, 'a_itype_%s' % group.card.tags.itype] + map(lambda x: 'a_prj_%s' % x,
                                                                                           group.card.tags.prj)
        + ['a_metaprj_%s' % group.card.tags.metaprj] + map(lambda x: 'itag_%s' % x, group.card.tags.itag))


def show_hosts_count(group):
    return str(len(group.getHosts()))


def show_instances_count(group):
    return str(len(group.get_kinda_busy_instances()))


def show_memory(group):
    return str(sum([host.memory for host in group.getHosts()]))


def show_imemory(group):
    if group.parent.db.version <= "2.2.21":
        m_per_instance = group.card.reqs.instances.memory.gigabytes()
    else:
        m_per_instance = group.card.reqs.instances.memory_guarantee.gigabytes()
    if m_per_instance == 0:
        print red_text("Group <%s> has zero memory requirements" % group.card.name)
    return '%.2f' % (m_per_instance * len(group.get_kinda_busy_instances()))


def show_disk(group):
    return str(sum([host.disk for host in group.getHosts()]))


def show_idisk(group):
    if len(group.card.intlookups) > 1:
        raise Exception("Not implemented")
    if len(group.card.intlookups) == 0:
        tier_size = 0
    elif len(group.card.intlookups) == 1:
        intlookup = group.parent.db.intlookups.get_intlookup(group.card.intlookups[0])
        if intlookup.tiers is None:
            tier_size = 0
        elif len(intlookup.tiers) == 1:
            tier = intlookup.tiers[0]
            tier_size = group.parent.db.tiers.get_tier(tier).disk_size
        else:
            raise Exception("Not implemented")
    else:
        raise Exception("Not implemented")

    s_host = group.card.properties.extra_disk_size
    s_instance = group.card.properties.extra_disk_size_per_instance
    s_shards = group.card.properties.extra_disk_shards
    disk_usage = len(group.getHosts()) * s_host + len(group.get_kinda_busy_instances()) * (
    s_instance + tier_size * s_shards)

    if disk_usage == 0:
        print red_text("Group <%s> has zero total disk usage" % group.card.name)

    return str(disk_usage)


def show_ssd(group):
    return str(sum([host.ssd for host in group.getHosts()]))


def show_issd(group):
    # FIXME: not implemented: can not detect if group should use ssd or disk
    return str(0)


def show_memory_guarantee(group):
    return group.card.reqs.instances.memory_guarantee.text


def show_memory_overcommit(group):
    return group.card.reqs.instances.memory_overcommit.text


def show_tiers(group):
    tiers = []
    if len(group.card.intlookups) > 0:
        for intlookup in (group.parent.db.intlookups.get_intlookup(x) for x in group.card.intlookups):
            if intlookup.tiers is not None:
                tiers.extend(intlookup.tiers)
    tiers = [x for x in tiers if x != None]

    if len(tiers):
        return ','.join(tiers)
    else:
        return "None"


def show_online_version(group):
    url = "%s/groups/%s/CURRENT" % (SETTINGS.services.clusterstate.rest.url, group.card.name)

    try:
        content = urllib2.urlopen(url).read()
    except Exception:
        return "None"

    jsoned = json.loads(content)

    def convert_version(version):
        if isinstance(version, int):
            if version < 1000000000:
                return "tag stable-%s/r%s" % (version / 1000000, version % 1000000)
            else:
                commit = version % 1000000000
                return "commit %s" % commit
        else:
            return str(version)

    return convert_version(jsoned["current"]["version"])


def show_hosts(group):
    return ','.join(sorted([x.name for x in group.get_kinda_busy_hosts()]))


def show_vm_hosts(group):
    vm_names = [generate_mtn_hostname(x, group, '') for x in group.get_kinda_busy_instances()]
    return ','.join(sorted(vm_names))


SHOW_FIELDS = OrderedDict([
    ("name", ("Group name", show_name)),
    ("master", ("Group Master", show_master)),
    ("owners", ("Group Owners", show_owners)),
    ("resolved_owners", ("Group Resolved Owners", show_resolved_owners)),
    ("guest_owners", ("Guest Group Owners", show_guest_owners)),
    ("watchers", ("Group Watchers", show_watchers)),
    ("expiration_date", ("Expiration Date", show_expiration_date)),
    ("slaves", ("Group Slaves", show_slaves)),
    ("tags", ("Group Tags", show_tags)),
    ("hosts_count", ("Group Hosts Count", show_hosts_count)),
    ("instances_count", ("Group Instances Count", show_instances_count)),
    ("power", ("Cpu Hosts Power", show_power)),
    ("ipower", ("Cpu Instances Power", show_ipower)),
    ("memory", ("Group Hosts Memory (in Gb)", show_memory)),
    ("imemory", ("Group Instances Memory (in Gb)", show_imemory)),
    ("disk", ("Group Hosts Disk (in Gb)", show_disk)),
    ("idisk", ("Group Instances Disk (in Gb)", show_idisk)),
    ("ssd", ("Group Hosts SSD (in Gb)", show_ssd)),
    ("issd", ("Group Instances SSD (in Gb)", show_issd)),
    ("memory_guarantee", ("Guaranted memory (in Gb)", show_memory_guarantee)),
    ("memory_overcommit", ("Overcommited memory (in Gb)", show_memory_overcommit)),
    ("online_version", ("Group online version", show_online_version)),
    ("hosts", ("Host list", show_hosts)),
    ("vm_hosts", ("Vm host list", show_vm_hosts)),
    ("tiers", ("Show tiers", show_tiers)),
])


def get_parser():
    parser = ArgumentParserExt("Script to show groups and some groups params (based on groups filter)")
    parser.add_argument("-i", "--show-fields", type=argparse_types.comma_list, required=True,
                        help="Obligatory. Predefined fiedls: %s. You can specify any card leaf value e. g. <reqs.instances.memory_guarantee>" % (",".join(SHOW_FIELDS.keys())))
    parser.add_argument("-E", "--show-expired-only", action='store_true', default=False,
                        help="Optional. Show only expired groups")
    parser.add_argument("-f", "--filter", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Applied filter")
    parser.add_argument("-g", "--groups", type=argparse_types.xgroups, default=None,
                        help="Optional. List of groups")
    parser.add_argument("--db", type=argparse_types.gencfg_db, default=CURDB,
                        help="Optional. Path to gencfg db")
    parser.add_argument("--hr", action='store_true', default=False,
                        help="Optional. Human-readable format")
    parser.add_argument('--delim', default='\t',
                        help='Optional. Field delimiter')
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Increase output verbosity (maximum 1)")

    return parser


def normalize(options):
    # check for invalid fields
    for field_name in options.show_fields:
        if field_name in SHOW_FIELDS:
            continue
        try:
            options.db.groups.get_scheme()._scheme.resolve_scheme_path(field_name.split('.'))
        except core.card.node.ResolveCardPathError:
            raise Exception("Field <%s> not found in group card" % field_name)

    if options.groups is None:
        options.groups = options.db.groups.get_groups()
        options.groups = sorted(options.groups, cmp = lambda x, y: cmp(x.card.name, y.card.name))


def main(options):
    filters = []
    if options.show_expired_only:
        filters.append(lambda group: group.is_expired())
    if options.filter:
        filters.append(options.filter)

    if filters:
        groups = [group for group in options.groups if all(f(group) for f in filters)]

    headers = []
    if options.verbose == 1:
        for field_name in options.show_fields:
            if field_name in SHOW_FIELDS:
                headers.append(SHOW_FIELDS[field_name][0])
            else:
                scheme_leaf = options.db.groups.get_scheme()._scheme.resolve_scheme_path(field_name.split('.'))
                headers.append(scheme_leaf.display_name)
        result = [headers]
    else:
        result = []

    for group in groups:
        cols = []
        for field_name in options.show_fields:
            if field_name in SHOW_FIELDS:
                cols.append(SHOW_FIELDS[field_name][1](group))
            else:
                value = group.card.get_card_value(field_name.split('.'))
                if isinstance(value, core.card.node.CardNode):
                    cols.append(json.dumps(value.as_dict()))
                else:
                    cols.append(str(group.card.get_card_value(field_name.split('.'))))
        result.append(cols)

    return result


def jsmain(d):
    options = get_parser().parse_json(d)
    normalize(options)
    return main(options)


def print_result(result):
    if options.hr:
        lengths = []
        for lst in map(list, zip(*result)):
            lengths.append(max(map(lambda x: len(x), lst)))

    for row in result:
        if row is None:
            line = "None"
        elif options.hr:
            line = ' '.join(' ' * (lengths[i] - len(col)) + '%s' % col for i, col in enumerate(row))
        else:
            line = options.delim.join(row)

        print line


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    result = main(options)

    print_result(result)

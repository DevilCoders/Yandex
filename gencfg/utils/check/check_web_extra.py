#!/skynet/python/bin/python
"""Various checks for jupiter, which can not be added to group card right now"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict
import time

import gencfg
from core.db import CURDB
from gaux.aux_colortext import red_text
from gaux.aux_utils import indent


def check_platinum_replicas_count():
    """Check production platinum replicas count

    Check if we have at least 10 production replicas in every location"""

    data = (
        # ('sas', ('SAS_WEB_BASE.PlatinumTier0',), 4, 1000),
        # ('man', ('MAN_WEB_BASE.PlatinumTier0',), 5, 1000),
        # ('man', ('MAN_WEB_BASE.WebTier1',), 2, 1000),
        # =================================== GENCFG-1788 START =====================================
        ('man', ('MAN_WEB_CALLISTO_CAM_BASE',), 6, 7),
        ('sas', ('SAS_WEB_CALLISTO_CAM_BASE',), 6, 6),
        ('vla', ('VLA_WEB_CALLISTO_CAM_BASE',), 6, 6),
        # =================================== GENCFG-1788 FINISH ====================================
        # =================================== GENCFG-1765 START =====================================
        # ('man', ('MAN_WEB_BASE.WebTier1',), 2, 1000),
        # ('vla', ('VLA_WEB_BASE.WebTier1',), 2, 1000),
        # ('vla', ('VLA_YT_RTC.WebTier1',), 2, 1000),
        # =================================== GENCFG-1765 FINISH ====================================
    )

    for location, intlookups, min_replicas, max_replicas in data:
        intlookups = [CURDB.intlookups.get_intlookup(x) for x in intlookups]

        intlookup_min_replicas = 0
        intlookup_max_replicas = 0
        for intlookup in intlookups:
            intlookup_min_replicas += min(len(intlookup.get_base_instances_for_shard(x)) for x in xrange(intlookup.get_shards_count()))
            intlookup_max_replicas += max(len(intlookup.get_base_instances_for_shard(x)) for x in xrange(intlookup.get_shards_count()))

        if intlookup_min_replicas < min_replicas:
            return False, 'Location <{}> (intlookups <{}>) has <{}> production replicas when requested at least <{}>'.format(
                    location, ','.join(x.file_name for x in intlookups), intlookup_min_replicas, min_replicas)
        if intlookup_max_replicas > max_replicas:
            return False, 'Location <{}> (intlookups <{}>) has <{}> production replicas when requested at most <{}>'.format(
                    location, ','.join(x.file_name for x in intlookups), intlookup_max_replicas, max_replicas)

    return True, None


def check_platinum_weights():
    """Check minimal platinum weights"""

    data = (
        # ('SAS_WEB_BASE.PlatinumTier0', 1),
        # ('MAN_WEB_BASE.PlatinumTier0', 1),
    )

    for intlookup, min_power in data:
        intlookup = CURDB.intlookups.get_intlookup(intlookup)
        bad_base_instances = [x for x in intlookup.get_base_instances() if x.power < min_power]
        if len(bad_base_instances):
            return False, 'Intlookup <{}> has <{}> instances with less than <{}> power'.format(intlookup.file_name, len(bad_base_instances), min_power)

    return True, None


# GENCFG-1839
def check_build_groups_have_ssd():
    """Check that all BUILD groups are on ssd machines"""
    groups = [x for x in CURDB.groups.get_groups() if x.card.name.endswith('_BUILD') and x.card.tags.metaprj == 'web']

    non_ssd_hosts = {}
    for group in groups:
        hosts = [x for x in group.getHosts() if x.ssd == 0 and x.ssd_size == 0]
        if hosts:
            non_ssd_hosts[group] = hosts

    if non_ssd_hosts:
        msg = []
        for group, hosts in non_ssd_hosts.iteritems():
            msg.append('Group {} has non-ssd hosts: {}'.format(group.card.name, ','.join(x.name for x in sorted(hosts))))
        msg = '\n'.join(msg)
        return False, msg

    return True, None


# GENCFG-1695
def check_single_ynode_on_host():
    """Check only one instance with itype <ytnode> on host"""
    groups = [x for x in CURDB.groups.get_groups() if x.card.tags.itype == 'ytnode']

    instances_by_host = defaultdict(list)
    for group in groups:
        for instance in group.get_kinda_busy_instances():
            instances_by_host[instance.host].append(instance)

    instances_by_host = [(x, y) for x, y in instances_by_host.iteritems() if len(y) > 1]

    if instances_by_host:
        msg = []
        for host, instances in instances_by_host:
            msg.append('Host {} has more than one instance with itype <ytnode>: {}'.format(host.name, ','.join(x.full_name() for x in instances)))
        msg = '\n'.join(msg)
        return False, msg

    return True, None


def check_no_psi_on_disallow_background_groups():
    """Check tags we to not have psi instances on hosts with allow_background_groups=False"""

    # find hosts we can not add psi to
    bad_groups = [x for x in CURDB.groups.get_groups() if x.card.properties.allow_background_groups == False or x.card.properties.nonsearch == True]
    bad_hosts = [x.getHosts() for x in bad_groups]
    bad_hosts = set(sum(bad_hosts, []))

    psi_groups = [x for x in CURDB.groups.get_groups() if x.card.tags.itype == 'psi' and x.card.properties.nonsearch == False]
    psi_groups.extend([CURDB.groups.get_group(x) for x in 'SAS_MAIL_LUCENE ALL_MAIL_LUCENE_EXTRA_REPLICA'.split()])
    succ, msg = True, []
    for group in psi_groups:
        intersect_hosts = set(group.getHosts()) & bad_hosts
        if intersect_hosts:
            succ = False
            msg.append('Group {} is raised {} hosts is not allowed on: {}'.format(group.card.name, len(intersect_hosts), ','.join(sorted(x.name for x in intersect_hosts))))

    msg = '\n'.join(msg)

    return succ, msg


def check_hosts_per_group():
    """Check that hosts_per_group in all intlookups is same"""

    SKIP_INTLOOKUPS = ['MAN_IMGS_BASE_PRISM', 'MAN_IMGS_CBIR_BASE_PRISM', 'VLA_WEB_PLATINUM_JUPITER_BASE_PIP', 'VLA_WEB_PLATINUM_JUPITER_BASE_TEST1', 'VLA_WEB_PLATINUM_JUPITER_BASE_TEST2', 'VLA_WEB_PLATINUM_JUPITER_BASE_RTHUB2', 'VLA_WEB_PLATINUM_JUPITER_BASE_RTHUB1']

    hosts_per_group_by_tier = defaultdict(lambda: defaultdict(list))
    for intlookup in CURDB.intlookups.get_intlookups():
        if not len(intlookup.get_int_instances()):
            continue
        if intlookup.tiers is None:
            continue
        if intlookup.file_name in SKIP_INTLOOKUPS:
            continue

        for tier_name in intlookup.tiers:
            hosts_per_group_by_tier[tier_name][intlookup.hosts_per_group].append(intlookup.file_name)

    succ = True
    msg = []
    for tier_name in sorted(hosts_per_group_by_tier):
        d = hosts_per_group_by_tier[tier_name]
        if len(d) > 1:
            succ = False
            msg_line = 'Tier {} has different hosts_per_groups:'
            for hosts_per_group in sorted(d):
                msg_line = '{} {}'.format(msg_line, '<{}> hosts per group in <{}>,'.format(hosts_per_group, ','.join(d[hosts_per_group])))
            msg.append(msg_line)
    msg = '\n'.join(msg)

    return succ, msg


def check_excessive_files():
    """Check that we do not have files GROUPNAME.hosts/GROUPNAME.instances for groups with host donor"""
    repo = CURDB.get_repo()

    msg = []
    for group in CURDB.groups.get_groups():
        if group.card.host_donor is None:
            continue

        for path in [os.path.join('groups', group.card.master.card.name, '{}.hosts'.format(group.card.name)),
                     os.path.join('groups', group.card.master.card.name, '{}.instances'.format(group.card.name)),]:
            if not os.path.exists(os.path.join(repo.path, path)):
                continue

            if repo.has_file(path):
                msg.append('Group {} with host donor {} has versioned file <{}>'.format(group.card.name, group.card.host_donor, path))

    if msg:
        msg = '\n'.join(msg)
        return False, msg
    else:
        return True, None


def main():
    checkers = [
        check_platinum_replicas_count,
        check_platinum_weights,
        check_build_groups_have_ssd,
        check_single_ynode_on_host,
        check_hosts_per_group,
        check_excessive_files,
#        check_no_psi_on_disallow_background_groups,
    ]

    status = 0
    for checker in checkers:
        is_ok, msg = checker()
        if not is_ok:
            status = 1
            print '{}:\n{}'.format(checker.__doc__.split('\n')[0], red_text(indent(msg)))

    return status


if __name__ == '__main__':
    status = main()

    sys.exit(status)

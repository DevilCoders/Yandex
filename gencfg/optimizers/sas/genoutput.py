from core.instances import TIntl2Group, TIntGroup, TMultishardGroup
from core.igroups import CIPEntry
from core.db import CURDB


def generate_intlookups(tiers, hgroups, group_name):
    # make new weights for instances
    CURDB.groups.get_group(group_name).custom_instance_power_used = True
    CURDB.groups.get_group(group_name).mark_as_modified()
    for hgroup in hgroups:
        # process hsots with not all instances used in optimization
        for host in hgroup.hosts:
            host_icount = len(hgroup.group.get_host_instances(host))
            if host_icount > len(hgroup.igroups):
                extra_power = sum(map(lambda x: x.power, hgroup.group.get_host_instances(host))) - hgroup.power
                assert (extra_power >= -0.01), "Extra power for host %s is %s" % (host.name, extra_power)
                for i in range(len(hgroup.igroups), host_icount):
                    instance = CURDB.groups.get_instance_by_N(host.name, hgroup.group.card.name, i)
                    instance.power = extra_power / (host_icount - len(hgroup.igroups))

        # created groups for used instances
        for i in range(len(hgroup.igroups)):
            igroup = hgroup.igroups[i]
            igroup.instances = []
            for host in hgroup.hosts:
                instance = CURDB.groups.get_instance_by_N(host.name, hgroup.group.card.name, i)
                instance.power = igroup.power
                igroup.instances.append(instance)
                cipentry = CIPEntry(instance.host, instance.port)
                CURDB.groups.get_group(instance.type).custom_instance_power[cipentry] = instance.power

    for tier in tiers:
        intlookup = CURDB.intlookups.create_tmp_intlookup()
        intlookup.hosts_per_group = tier.hosts_per_group
        intlookup.brigade_groups_count = tier.total_shards
        intlookup.tiers = [tier.tier_name]
        intlookup.base_type = group_name

        brigade_groups = []
        for shard in tier.shards:
            brigade_group = TMultishardGroup()
            for igroup in shard.assigned_igroups:
                brigade = TIntGroup(map(lambda x: [x], igroup.instances), [])
                brigade_group.brigades.append(brigade)
            brigade_groups.append(brigade_group)

        intlookup.intl2_groups.append(TIntl2Group(multishards=brigade_groups))

        if CURDB.intlookups.has_intlookup(tier.intlookup_name):
            CURDB.intlookups.remove_intlookup(tier.intlookup_name)
        CURDB.intlookups.rename_intlookup(intlookup.file_name, tier.intlookup_name)

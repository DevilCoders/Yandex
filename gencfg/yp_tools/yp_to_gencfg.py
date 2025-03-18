import sys
import core.db as db
from core.instances import TMultishardGroup, TIntGroup, TIntl2Group, Instance
import parse_allocator_result
import parse_yp_result
import make_recluster_request
import logging

def iter_instances(yp_group):
    for host, count in yp_group.iteritems():
        for i in range(count):
            yield host, i


def instances(yp_group):
    import random
    instances_list = list(iter_instances(yp_group))
    random.shuffle(instances_list)
    return instances_list


def build_t1_intlookup(base_group, base_port, base_power, base_allocation,
                       int_group, int_port, int_power, int_allocation,
                       replicas, hosts_per_group, brigade_groups_count):
    base_allocation = iter(base_allocation)
    int_allocation = iter(int_allocation)

    intl = db.CURDB.intlookups.create_tmp_intlookup()
    intl.base_type = base_group
    intl.file_name = base_group
    intl.hosts_per_group = hosts_per_group
    intl.brigade_groups_count = brigade_groups_count
    intl.tiers = ['WebTier1']

    intl2_group = TIntl2Group()
    for brigade in range(brigade_groups_count):
        ms = TMultishardGroup()
        for i in range(replicas):
            basesearchers = []
            for j in range(hosts_per_group):
                host, counter = next(base_allocation)
                basesearchers.append([Instance(db.CURDB.hosts.get_host_by_name(str(host)), float(base_power),
                                               base_port + 8 * counter, base_group, 0)])
            intsearchers = []
            for j in range(hosts_per_group):
                host, counter = next(int_allocation)
                intsearchers.append(Instance(db.CURDB.hosts.get_host_by_name(str(host)), float(int_power),
                                             int_port + 8 * counter, int_group, 0))
            ms.brigades.append(TIntGroup(basesearchers, intsearchers))
        intl2_group.multishards.append(ms)
    intl.intl2_groups = [intl2_group]  # flat intlookup

    try:
        next(base_allocation)
    except StopIteration:
        return intl
    else:
        raise RuntimeError('yp_allocation not exhausted')


def build_intlookup(group_name, port, tier, power, replicas, hosts_per_group, brigade_groups_count, yp_allocation):
    yp_allocation = iter(yp_allocation)

    intl = db.CURDB.intlookups.create_tmp_intlookup()
    intl.base_type = group_name
    intl.file_name = group_name
    intl.hosts_per_group = hosts_per_group
    intl.brigade_groups_count = brigade_groups_count
    intl.tiers = [tier] if tier else None

    intl2_group = TIntl2Group()
    for brigade in range(brigade_groups_count):
        ms = TMultishardGroup()
        for i in range(replicas):
            basesearchers = []
            for j in range(hosts_per_group):
                host, counter = next(yp_allocation)
                basesearchers.append([Instance(db.CURDB.hosts.get_host_by_name(str(host)), power, port + 8 * counter, group_name, 0)])
            int_group = TIntGroup(basesearchers, [])
            ms.brigades.append(int_group)
        intl2_group.multishards.append(ms)
    intl.intl2_groups = [intl2_group]  # flat intlookup

    try:
        next(yp_allocation)
    except StopIteration:
        return intl
    else:
        raise RuntimeError('{}: yp_allocation not exhausted'.format(group_name))


def gencfg_port(group):
    try:
        return int(db.CURDB.groups.get_group(group).card.legacy.funcs.instancePort.split('new')[1])
    except IndexError:
        return int(db.CURDB.groups.get_group(group).card.legacy.funcs.instancePort.split('old')[1])



def fix_power(group_spec):
    return group_spec.resources.power


def save_hosts(folder, name, instances):
    with open(folder + '/' + name + '.hosts', 'w') as f:
        for host in sorted(set(instance.host.name for instance in instances)):
            print >> f, host


def save_instances(folder, name, instances):
    with open(folder + '/' + name + '.instances', 'w') as f:
        for instance in sorted(instances):
            print >> f, '{}:{} {:0.3}'.format(instance.host.name, instance.port, instance.power)


def instances_file_needed(group_spec):
    # return True
    return db.CURDB.groups.get_group(group_spec.name).card.legacy.funcs.instanceCount != 'exactly1'


def intlookup_needed(group_spec):
    return db.CURDB.groups.get_group(group_spec.name).card.tags.itype not in ('int', 'intl2')


def save(group_spec, allocation, folder, name):
    if group_spec.tier is None:
        hosts_per_group = 1
    elif any(g in name for g in {'WEB_GEMINI_BASE', 'IMGS_THUMB_NEW', 'REMOTE_STORAGE',
                                 'IMGS_LARGE_THUMB', 'IMGS_RIM_3K',
                                 'INVERTED_INDEX', 'EMBEDDING', '_INT', 'BUILD', 'WEB_TIER0_BASE', 'TIER0_ATTRIBUTE_BASE', 'LUCENE', 'ARCNEWS_BASE', 'RQ2_BASE_PRIEMKA', 'ARCNEWS_STORY_BASE', 'WEB_TIER0'}):
        hosts_per_group = 1
    else:
        hosts_per_group = 18

    intlookup = build_intlookup(str(name), gencfg_port(name), group_spec.tier, fix_power(group_spec),
                                group_spec.replicas, hosts_per_group, group_spec.shards / hosts_per_group,
                                instances(allocation))
    save_hosts(folder, name, intlookup.get_base_instances())
    if instances_file_needed(group_spec):
        save_instances(folder, name, intlookup.get_base_instances())

    if intlookup_needed(group_spec):
        intlookup.write_intlookup_to_file_json(folder + '/' + name)


def rewrite_ints(intlookup, ints, ints_per_brigade, int_group, int_port, int_power):
    int_allocation = iter(ints)
    intl = db.CURDB.intlookups.get_intlookup(intlookup)
    intl2_group = TIntl2Group()
    for multishard in intl.get_multishards():
        for brigade in multishard.brigades:
            intsearchers = []
            for i in range(ints_per_brigade):
                host, counter = next(int_allocation)
                intsearchers.append(Instance(db.CURDB.hosts.get_host_by_name(str(host)), float(int_power),
                                             int_port + 8 * counter, int_group, 0))
            brigade.intsearchers = intsearchers

    folder = 'generated/yp'
    intl.write_intlookup_to_file_json(folder + '/' + intlookup)
    save_hosts(folder, int_group, intl.get_int_instances())
    save_instances(folder, int_group, intl.get_int_instances())


def main(file_name, outdir):
    logging.basicConfig(level=logging.DEBUG)
    logging.info('Converting %s', file_name)

    groups = parse_allocator_result.load_groups(file_name)

    for group in groups:
       group_spec = make_recluster_request.get_group_spec(group)
       logging.info('%s %s', group, group_spec)
       save(group_spec, groups[group], outdir, group)


if __name__ == '__main__':
    main(sys.argv[1], 'generated/yp')

from gaux.aux_colortext import green_text, red_text
from collections import defaultdict

from core.instances import TIntGroup
from random import Random

from core.db import CURDB


def optimize_by_net_usage(instances, hosts_per_group):
    instances_by_switch = defaultdict(list)
    for instance in instances:
        instances_by_switch[instance.host.switch].append(instance)

    groups_count = len(instances) / hosts_per_group

    # assign leader instances of groups
    switch_hosts_count = dict((x, len(y)) for (x, y) in instances_by_switch.items())
    result_groups = []
    for i in range(groups_count):
        result_groups.append([])
    leaders_count = defaultdict(int)
    for i in range(groups_count):
        next_leader = None
        next_leader_score = -1000000000

        for switch in switch_hosts_count:
            if switch_hosts_count[switch] > next_leader_score:
                next_leader = switch
                next_leader_score = switch_hosts_count[switch]

        leaders_count[next_leader] += 1
        switch_hosts_count[next_leader] -= hosts_per_group
        result_groups[i].append(instances_by_switch[next_leader].pop())

    # assign non-leader instances to leaders
    switch_hosts_count = dict((x, len(y)) for (x, y) in instances_by_switch.items())
    for i in range(hosts_per_group - 1):
        for j in range(groups_count):
            current_switch = result_groups[j][0].host.switch

            if len(instances_by_switch[current_switch]):
                result_groups[j].append(instances_by_switch[current_switch].pop())
                switch_hosts_count[current_switch] -= 1
                continue

            foreign_switch = None
            foreign_switch_score = 0
            for switch in switch_hosts_count:
                if switch_hosts_count[switch] / (leaders_count[switch] + 0.0001) > foreign_switch_score:
                    foreign_switch = switch
                    foreign_switch_score = switch_hosts_count[switch] / (leaders_count[switch] + 0.0001)
            result_groups[j].append(instances_by_switch[foreign_switch].pop())
            switch_hosts_count[foreign_switch] -= 1

    return result_groups, sum(instances_by_switch.values(), [])


def my_zip(lists):
    # takes [[a1], [b1,b2,b3], [c1,c2]]
    # returns [a1, b1, c1, b2, c2, b3]
    init_length = sum(len(x) for x in lists)
    result = []
    pos = 0
    while lists:
        next_lists = []
        for lst in lists:
            if len(lst) > pos:
                result.append(lst[pos])
                next_lists.append(lst)
        pos += 1
        lists = next_lists
    assert (len(result) == init_length)
    return result


def create_groups(base_instances, hosts_per_group, ints_per_group, base_type, min_groups_count, params,
                  preserved_hosts):
    myrand = Random()
    myrand.seed(12382709)

    is_multidc_groups = params.get('create_multidc_groups', False)
    is_multiqueue_groups = params.get('create_multiqueue_groups', False)
    if is_multidc_groups and is_multiqueue_groups:
        raise Exception("Both create_multidc_groups and create_multiqueue_groups specified")

    is_switchtype_groups = params.get('create_switchtype_groups', False)

    # if is_multiqueue_groups and is_switchtype_groups:
    #    raise Exception('Incompatible options "create_multiqueue_groups" and "create_switchtype_groups"')

    major_separate = []
    minor_separate = []
    display_group = []
    display_instance = []

    if is_multidc_groups:
        major_separate.append(lambda x: 'nodc')
        major_separate.append(lambda x: x.power)
        minor_separate.append(lambda x: 'nodc')
        display_group.append(lambda x: 'nodc')
        display_instance.append(lambda x: 'nodc')
    elif is_multiqueue_groups:
        major_separate.append(lambda x: x.host.dc)
        major_separate.append(lambda x: x.power)
        minor_separate.append(lambda x: x.host.dc)
        display_group.append(lambda x: x.dc)
        display_instance.append(lambda x: x.host.dc)
    else:
        major_separate.append(lambda x: x.host.queue)
        major_separate.append(lambda x: x.power)
        # FIXME: hack
        # if we split by switch_type, minor groups should be splitted by dc (not by queue)
        if is_switchtype_groups:
            minor_separate.append(lambda x: x.host.dc)
            display_group.append(lambda x: x.dc)
            display_instance.append(lambda x: x.host.dc)
        else:
            minor_separate.append(lambda x: x.host.queue)
            display_group.append(lambda x: x.queue)
            display_instance.append(lambda x: x.host.queue)

    if is_switchtype_groups:
        major_separate.append(lambda x: x.host.switch_type)
        minor_separate.append(lambda x: x.host.switch_type)
        display_group.append(lambda x: x.switch_type)
        display_instance.append(lambda x: x.host.switch_type)

    major_separate_func = lambda instance: tuple(map(lambda f: f(instance), major_separate))
    minor_separate_func = lambda instance: tuple(map(lambda f: f(instance), minor_separate))
    display_group_func = lambda instance: tuple(map(lambda f: f(instance), display_group))
    display_instance_func = lambda instance: tuple(map(lambda f: f(instance), display_instance))

    # stage 1
    # 1. separate by location
    # 2. separate by power
    # 3. create groups optimizing by network usage
    groups = []
    free_instances = []
    separated_instances = defaultdict(list)
    for instance in base_instances:
        separated_instances[major_separate_func(instance)].append(instance)
    for united_instances in separated_instances.values():
        assert (len(set(x.power for x in united_instances)) == 1)
        more_groups, more_free_instances = optimize_by_net_usage(united_instances, hosts_per_group + ints_per_group)
        groups.extend(more_groups)
        free_instances.extend(more_free_instances)

    # sort by power as we want combine basesearchers with the same power
    def cmp_power_ascending(i1, i2):
        if i1.power != i2.power:
            return cmp(i1.power, i2.power)
        if i1.port != i2.port:
            return cmp(i2.port, i1.port)
        return cmp(i1.host.name, i2.host.name)

    # stage 2
    # 1. separate by location
    # 2. create groups optimizing by power and after that optimizing by network
    free_instances = list(reversed(sorted(free_instances, cmp=cmp_power_ascending)))
    reserve_candidates = free_instances[:]
    base_instances = free_instances[:]
    free_instances = []
    separated_instances.clear()
    for instance in base_instances:
        separated_instances[minor_separate_func(instance)].append(instance)
    for united_instances in separated_instances.values():
        extra_groups = len(united_instances) / hosts_per_group
        for i in range(extra_groups):
            fst = i * hosts_per_group
            lst = (i + 1) * hosts_per_group
            group = united_instances[fst:lst]
            switch_load = defaultdict(int)
            for instance in group:
                switch_load[instance.host.switch] += 1
            group.sort(cmp=lambda x, y: cmp(switch_load[y.host.switch], switch_load[x.host.switch]))
            groups.append(group)
        free_instances.extend(united_instances[extra_groups * hosts_per_group:])

    brigades = []
    for group in groups:
        major_switch = group[0].host.switch
        switch_load = defaultdict(int)
        for instance in group:
            switch_load[instance.host.switch] += 1
        assert (max(switch_load.values()) == switch_load[major_switch])
        intsearchers = [CURDB.groups.get_instance_by_N(x.host.name, x.type + '_INT', x.N)
                        for x in group[:ints_per_group]]
        basesearchers = [[x] for x in group[ints_per_group:]]
        # we want to treat all shards equally => shuffle and remove cpu correlation
        myrand.shuffle(basesearchers)
        brigades.append(TIntGroup(basesearchers, intsearchers))
    groups = brigades
    del brigades

    print green_text('%d groups, %d free basesearcher, %f power (needed at least %d groups)' %
                     (len(groups), len(free_instances),
                      sum(map(lambda x: x.power, groups)), min_groups_count))

    # we have a lot of preserved hosts so
    # we use all instances from stage 2 to have more reserved candidates
    print 'Before preserved: %s' % len(reserve_candidates)
    reserve_candidates = [instance for instance in reserve_candidates if instance.host not in preserved_hosts]
    print 'After preserved: %s' % len(reserve_candidates)

    reserve_candidates.sort(cmp=cmp_power_ascending)
    reserve_candidates_weak = defaultdict(list)
    for instance in reserve_candidates:
        reserve_candidates_weak[display_instance_func(instance)].append(instance)

    # same instances by sorted in different order to provide diversity of reserved hosts
    diverse_key = lambda instance: instance.host.power
    reserve_candidates_by_diverse = defaultdict(list)
    for instance in reserve_candidates:
        reserve_candidates_by_diverse[diverse_key(instance)].append(instance)
    reserve_candidates = my_zip(reserve_candidates_by_diverse.values())
    # now split to destinations
    reserve_candidates_diversed = defaultdict(list)
    for instance in reserve_candidates:
        reserve_candidates_diversed[display_instance_func(instance)].append(instance)

    # count free power by cluster and log it
    tmp = free_instances
    free_instances = defaultdict(list)
    free_power = defaultdict(float)
    for instance in tmp:
        key = display_instance_func(instance)
        free_instances[key].append(instance)
        free_power[key] += instance.power

    group_free_power = defaultdict(float)
    for group in groups:
        group_free_power[display_group_func(group)] += group.get_sum_power() - hosts_per_group * group.power

    reserved_hosts = set()
    for ID in sorted(set(free_power.keys() + group_free_power.keys())):
        # we have two stages
        # on first stage we get diverse hosts
        # on second stage we get weakest hosts
        C = len(free_instances[ID])
        Cs = [C, 0]
        for stage, C in enumerate(Cs):
            if stage == 0:
                source = reserve_candidates_weak[ID]
            else:
                source = reserve_candidates_diversed[ID]
            for instance in source:
                if instance.host.name in reserved_hosts:
                    continue
                N = CURDB.groups.get_group(base_type).funcs.instanceCount(CURDB, instance.host)
                if N > C:
                    break
                C -= N
                reserved_hosts.add(instance.host.name)

        print green_text("Id %s: %d free instances, %f free power, %f group free power" % \
                         (ID, len(free_instances[ID]), free_power[ID], group_free_power[ID]))

    print red_text(
        "====================================== ADD TO RESERVED ==============================================")
    print ','.join(sorted(list(reserved_hosts)))
    print red_text(
        "=====================================================================================================")

    by_dc = defaultdict(int)
    by_power = defaultdict(int)
    by_memory = defaultdict(int)
    total_power = .0
    total_memory = 0
    for host in [CURDB.hosts.get_host_by_name(host) for host in reserved_hosts]:
        by_dc[host.dc] += 1
        by_power[host.power] += 1
        by_memory[host.memory] += 1
        total_power += host.power
        total_memory += host.memory
    print 'ADD TO RESERVED statistics'
    print 'By DC = %s' % dict(by_dc)
    print 'By power = %s' % dict(by_power)
    print 'By memory = %s' % dict(by_memory)
    print 'Total power = %s' % total_power
    print 'Total memory = %s' % total_memory

    if len(groups) < min_groups_count:
        raise Exception("Not enough groups for backup intlookups (have %d, needed %d)" % (len(groups), min_groups_count))

    return groups, free_instances, []

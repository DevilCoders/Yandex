import random
import math

from groupsinfo import InstancesGroup


def _sum_power(lst, indexes, needed_power=None):
    result = 0.
    for index in indexes:
        result += lst[index][0]
    return math.fabs(needed_power - result)


"""
    We have list of assigned power, e.g. [0.0, 11.0, 32.0, 1.0, 15.0, 11.0, 123.0] and want modified list with specified minimal
    power, e.g. <3.0> with the same total power, e. g. [3.0, 10.0, 31.0, 3.0, 14.0, 10.0, 122.0]
"""


def set_min_instance_power(data, min_power):
    total_power = sum(data)
    if total_power < len(data) * min_power:
        raise Exception("Min power %s is to high (sum power %s, number of instances %s)" % (min_power, total_power, len(data)))

    lacking_power = sum(map(lambda x: max(0, min_power - x), data))
    excessive_power = total_power - lacking_power

    for i in range(len(data)):
        instance_lacking_power = max(0, min_power - data[i])
        instance_excessive_power = max(0, data[i] - min_power)
        instance_excessive_power *= lacking_power / excessive_power

        data[i] += instance_lacking_power - instance_excessive_power

    return data


def find_candidate(N, shards, allow_use_not_all_slots):
    candidate = []
    used_slots = 0
    for i in range(N):
        index = random.randrange(0, len(shards))
        candidate.append(index)
        used_slots += shards[index][1].slot_size
        if used_slots >= N:
            break

    if used_slots == N:
        return True, candidate
    if allow_use_not_all_slots and len(candidate) > 1 and used_slots > N:
        candidate.pop()
        return True, candidate
    return False, None


def find_best_unassigned_shards(hgroup, needed_powers, steps, flt=None, allow_use_not_all_slots=False,
                                check_ssd_limits=True, check_same_shard_instanaces_on_host=True):
    if sum(map(lambda (x, y): y.slot_size, needed_powers)) <= hgroup.icount:
        return range(len(needed_powers)), _sum_power(needed_powers, range(len(needed_powers)), hgroup.power)

    best_solution = None
    best_result = None
    for i in range(steps):
        status, solution = find_candidate(hgroup.icount, needed_powers, allow_use_not_all_slots)
        if not status:
            continue
        if len(solution) != len(set(solution)):
            continue

        # check if no 2 replicas for any shard
        if check_same_shard_instanaces_on_host == True and len(solution) != len(
                set(map(lambda x: needed_powers[x][1], solution))):
            continue

        # check if custom filter passed
        if flt and not flt(hgroup, map(lambda x: needed_powers[x][1], solution)):
            continue

        # check if number of ssd replicas not more than expected
        if check_ssd_limits and hgroup.ssd is True:
            for shardinfo in map(lambda x: needed_powers[x][1], solution):
                if len(map(lambda x: x.hgroup.ssd is True, shardinfo.assigned_igroups)) > shardinfo.ssd_replicas:
                    continue

                #          this code for current recluster msk only
                #        if check_ssd_limits:
                #            hdc = hgroup.hosts[0].dc
                #            failed = False
                #            for shardinfo in map(lambda x: needed_powers[x][1], solution):
                #                if shardinfo.replicas > 1:
                #                    failed_by_replicas = len(filter(lambda x: x.hgroup.hosts[0].dc == hdc, shardinfo.assigned_igroups)) + 1 > shardinfo.replicas / 2
                #                    other_dc_power = sum(map(lambda x: x.power, filter(lambda x: x.hgroup.hosts[0].dc != hdc, shardinfo.assigned_igroups)))
                #                    my_dc_power = sum(map(lambda x: x.power, filter(lambda x: x.hgroup.hosts[0].dc == hdc, shardinfo.assigned_igroups))) + hgroup.power / len(solution)
                # if failed_by_replicas:
                #                    if failed_by_replicas or (my_dc_power > shardinfo.needed_power * 0.4 and shardinfo.needed_power > 100 and other_dc_power < shardinfo.needed_power / 2):
                #                        failed = True
                #                        break
                #            if failed:
                #                continue

        if best_result is None or _sum_power(needed_powers, solution, hgroup.power) < best_result:
            best_solution = solution
            best_result = _sum_power(needed_powers, solution, hgroup.power)

    return best_solution, best_result


def optimize(hgroups, shards, flt=None):
    for hgroup in hgroups:
        unassigned = sum(map(lambda x: x.get_needed_power(), shards), [])
        best_solution, best_result = find_best_unassigned_shards(hgroup, unassigned, 20000, flt)
        print "---1---"
        if best_solution is None:
            best_solution, best_result = find_best_unassigned_shards(hgroup, unassigned, 20000, flt, False, False)
            print "---2---"
        if best_solution is None:
            best_solution, best_result = find_best_unassigned_shards(hgroup, unassigned, 20000, flt, True, False)
            print "---3---"
        if best_solution is None:
            best_solution, best_result = find_best_unassigned_shards(hgroup, unassigned, 20000, None, True, False)
            print "---4---"
        if best_solution is None:
            best_solution, best_result = find_best_unassigned_shards(hgroup, unassigned, 20000, None, True, False, False)
            print "---5---"

        best_solution = map(lambda x: unassigned[x], best_solution)

        print map(lambda (power, shard_info): (power, shard_info.tier_name), best_solution)
        #        if len(best_solution) < 2:
        #            aaa = bbb

        coeff = hgroup.power / sum(map(lambda x: x[0], best_solution))
        best_solution = map(lambda (x, y): (x * coeff, y), best_solution)

        best_solution = zip(set_min_instance_power(map(lambda (x, y): x, best_solution), 1.0),
                            map(lambda (x, y): y, best_solution))

        for power, shard in best_solution:
            igroup = InstancesGroup(hgroup, power, shard.slot_size)
            hgroup.igroups.append(igroup)
            shard.assigned_igroups.append(igroup)


def find_best_ssd_replacement(my_shard, my_candidates, other_shard, other_candidates, old_best):
    my_hgroups = set(map(lambda x: x.hgroup, my_shard.assigned_igroups))
    other_hgroups = set(map(lambda x: x.hgroup, other_shard.assigned_igroups))

    new_best = old_best
    for my_candidate in my_candidates:
        for other_candidate in other_candidates:
            if my_candidate.hgroup in other_hgroups or other_candidate.hgroup in my_hgroups:
                continue

            if old_best is None or math.fabs(my_candidate.power - other_candidate.power) < math.fabs(
                            new_best[0].power - new_best[1].power):
                new_best = (my_candidate, other_candidate)
    return new_best


def optimize_ssd(shards):
    for shard_id, shard in enumerate(shards):
        if shard.ssd_replicas <= sum(map(lambda x: int(x.hgroup.ssd), shard.assigned_igroups)):
            continue

        for i in range(shard.ssd_replicas - sum(map(lambda x: int(x.hgroup.ssd), shard.assigned_igroups))):
            my_candidates = filter(lambda x: x.hgroup.ssd == 0, shard.assigned_igroups)
            best_replacement = None
            for help_shard in shards:
                if help_shard.ssd_replicas >= sum(map(lambda x: int(x.hgroup.ssd), help_shard.assigned_igroups)):
                    continue
                if help_shard.slot_size != shard.slot_size:
                    continue

                other_candidates = filter(lambda x: x.hgroup.ssd == True, help_shard.assigned_igroups)
                if len(other_candidates) == 0:
                    continue

                best_replacement = find_best_ssd_replacement(shard, my_candidates, help_shard, other_candidates,
                                                             best_replacement)

            if best_replacement is None:
                for other_shard in shards:
                    print other_shard
                raise Exception("Can not find replacement for tier (%d, %s)" % (shard_id, other_shard.tier_name))
            else:
                print "Best replacement for shard (%d, %s): %s <-> %s" % (
                shard_id, shard.tier_name, best_replacement[0].power, best_replacement[1].power)
            best_replacement[0].swap_data(best_replacement[1])

# break

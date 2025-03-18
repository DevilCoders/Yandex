#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types
from core.instances import TIntGroup, TMultishardGroup, TIntl2Group


class Shard(object):
    __slots__ = ['tier', 'N']

    def __init__(self, tier, N):
        self.tier = tier
        self.N = N


class WorkingHost(object):
    def __init__(self, host, options):
        self.host = host
        self.first_replica_group = options.group
        self.second_replica_group = options.backup_group
        self.research_group = options.research_group

        self.tiers_slot_sizes = dict(options.tiers_data)
        self.slots_left = host.memory / options.slot_size
        self.max_second_replica_cnt = options.max_second_replica_cnt

        self.first_slot_reserved_disk = self._first_slot_reserved_disk()
        self.disk_left = host.disk

        self.first_replica_shards = []
        self.second_replica_shards = []
        self.research_on_host = False

    def _first_slot_reserved_disk(self):
        pp = self.first_replica_group.card.properties
        return self.slots_left * (
        options.max_disk_per_slot + pp.extra_disk_size_per_instance * pp.extra_disk_shards) + pp.extra_disk_size

    def add_research_slot(self):
        self.research_on_host = True

        self.disk_left -= self.research_group.properties.extra_disk_size + self.research_group.properties.extra_disk_size_per_instance
        assert self.disk_left >= 0, "Can not add research slot to %s due to disk requirements" % self.host.name

        self.slots_left -= 1
        assert self.slots_left >= 1, "Can not add research slot to %s due to memory requirements" % self.host.name

    def try_add(self, shard, first_replica=True):
        if not first_replica:
            if shard in self.first_replica_shards:
                return False
            if len(self.second_replica_shards) == self.max_second_replica_cnt:
                return False

        # check if have enough slots
        if first_replica == True or len(self.second_replica_shards) == 0:
            needed_slots = self.tiers_slot_sizes[shard.tier]
        else:
            needed_slots = 0
        if needed_slots > self.slots_left:
            return False

        # check if have enough disk
        needed_disk = 0

        if first_replica:
            if len(self.first_replica_shards) == 0:
                needed_disk += self.first_replica_group.card.properties.extra_disk_size
            needed_disk += self.first_replica_group.card.properties.extra_disk_size_per_instance + \
                           CURDB.tiers.get_tier(
                               shard.tier).disk_size * self.first_replica_group.card.properties.extra_disk_shards
            if needed_disk > self.disk_left:
                return False
        else:
            if len(self.second_replica_shards) == 0:
                needed_disk += self.second_replica_group.card.properties.extra_disk_size
            needed_disk += self.second_replica_group.card.properties.extra_disk_size_per_instance + \
                           CURDB.tiers.get_tier(
                               shard.tier).disk_size * self.second_replica_group.card.properties.extra_disk_shards
            if needed_disk > (self.disk_left - self.first_slot_reserved_disk):
                return False

        # everything is ok, assigning shard
        self.disk_left -= needed_disk
        self.slots_left -= needed_slots
        if first_replica:
            self.first_replica_shards.append(shard)
            self.first_slot_reserved_disk = self._first_slot_reserved_disk()
        else:
            if len(self.second_replica_shards) == 0:
                self.first_slot_reserved_disk -= self.first_replica_group.card.properties.extra_disk_size_per_instance + \
                                                 CURDB.tiers.get_tier(
                                                     shard.tier).disk_size * self.first_replica_group.card.properties.extra_disk_shards
            self.second_replica_shards.append(shard)

        #        print "#####################################################################"
        #        print self.asstr()
        #        print "#####################################################################"

        return True

    def asstr(self):
        result = "Host %s: %s slots left, %s disk left, %s first slot reserved left, %s second slot reserved left\n" % (
        self.host.name, self.slots_left, self.disk_left, self.first_slot_reserved_disk,
        self.disk_left - self.first_slot_reserved_disk)
        result += "First replica: %s\n" % " ".join(map(lambda x: "(%s, %s)" % (x.tier, x.N), self.first_replica_shards))
        result += "Second replica: %s" % " ".join(map(lambda x: "(%s, %s)" % (x.tier, x.N), self.second_replica_shards))

        return result


def parse_cmd():
    parser = ArgumentParser(description="Allocate r1 or priemka hosts")
    parser.add_argument("-g", "--group", type=argparse_types.group, dest="group", required=True,
                        help="R1/priemka group")
    parser.add_argument("-f", "--from-groups", type=argparse_types.groups, dest="from_groups", required=True,
                        help="Group to get hosts from")
    parser.add_argument("-t", "--tiers-data", type=str, dest="tiers_data", default=None,
                        help="Number of instances")
    parser.add_argument("-p", "--research-power", type=float, dest="research_power", default=0.,
                        help="Research power")
    parser.add_argument("-s", "--slot-size", type=int, dest="slot_size", required=True,
                        help="Obligatory. Slot size (in gigabytes)")
    parser.add_argument("-e", "--min-second-replica-cnt", type=int, dest="min_second_replica_cnt", required=True,
                        help="Obligatory. Minimal number of second replica per host")
    parser.add_argument("-m", "--max-second-replica-cnt", type=int, dest="max_second_replica_cnt", required=True,
                        help="Obligatory. Maximal number of second replica per host")
    parser.add_argument("-l", "--host-filter", type=argparse_types.pythonlambda, dest="host_filter", default=None,
                        help="Optional. Filter")
    parser.add_argument("--prefer-amd", action="store_true", default=False,
                        help="Optional. First assign AMD hosts even if they are not as good as Intel ones")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    options.backup_group = CURDB.groups.get_group(options.group.card.name + '_BACKUP')
    if options.research_power > 0:
        options.research_group = CURDB.groups.get_group('MSK_WEB_RESEARCH_BASE_R1')
    else:
        options.research_group = None

    options.tiers_data = map(lambda x: (x.split(':')[0], int(x.split(':')[1])), options.tiers_data.split(','))
    #    options.tiers_data.sort(cmp = lambda (x1, y1), (x2, y2): cmp(CURDB.tiers.get_tier(x1).disk_size / y1 , CURDB.tiers.get_tier(x2).disk_size / y2))

    options.instances = sum(map(lambda (x, y): CURDB.tiers.get_total_shards(x) * y, options.tiers_data))

    options.max_disk_per_slot = max(
        map(lambda (x, y): CURDB.tiers.tiers[x].disk_size / float(y), options.tiers_data)) * 2

    return options


def get_best_candidate_from_unselected(candidates, first_replica=True):
    if len(candidates) == 0:
        raise Exception("No more hosts left")

    if first_replica:
        vfunc = lambda x: x.slots_left / x.host.power
    else:
        vfunc = lambda x: (x.disk_left - x.first_slot_reserved_disk) / x.host.power

    best_candidate, best_value = None, sys.float_info.min
    for new_candidate in candidates:
        new_value = vfunc(new_candidate)
        if vfunc(new_candidate) > best_value:
            best_candidate, best_value = new_candidate, new_value

    return best_candidate


def find_candidate_in_unselected(shard, candidate_hosts, first_replica=True, prefer_amd=False):
    if prefer_amd:
        amd_candidate_hosts = filter(lambda x: x.host.model.startswith('AMD'), candidate_hosts)
        if len(amd_candidate_hosts) > 0:
            best_candidate = get_best_candidate_from_unselected(amd_candidate_hosts, first_replica=first_replica)
            if best_candidate is not None and best_candidate.try_add(shard, first_replica=first_replica):
                return best_candidate

    best_candidate = get_best_candidate_from_unselected(candidate_hosts, first_replica=first_replica)
    if not best_candidate.try_add(shard, first_replica=first_replica):
        raise Exception("Can not add shard of tier %s to best host %s\n%s" % (shard.tier, best_candidate.host.name, best_candidate.asstr()))

    return best_candidate


def find_candidate_in_selected(shard, selected_hosts, first_replica):
    found = False
    for host in selected_hosts:
        if host.try_add(shard, first_replica=first_replica):
            found = True
            break
    return found


def write_intlookup(selected_hosts, group, tiers, first_replica=True):
    def ss(selected_host):
        if first_replica:
            return selected_host.first_replica_shards
        else:
            return selected_host.second_replica_shards

    if len(group.card.intlookups) > 0:
        intlookup = CURDB.intlookups.get_intlookup(group.card.intlookups[0])
    else:
        intlookup = CURDB.intlookups.create_empty_intlookup(group.card.name)
        intlookup.base_type = group.card.name
        group.card.intlookups.append(intlookup.file_name)

    intlookup.tiers = tiers
    intlookup.hosts_per_group = 1
    intlookup.brigade_groups_count = sum(map(lambda x: CURDB.tiers.get_tier(x).get_shards_count(), tiers))

    multishards = []
    for i in range(intlookup.brigade_groups_count):
        multishards.append(TMultishardGroup())

    tiers_start_count = {}
    c = 0
    for tier in tiers:
        tiers_start_count[tier] = c
        c += CURDB.tiers.get_tier(tier).get_shards_count()

    for selected_host in selected_hosts:
        for i, shard in enumerate(ss(selected_host)):
            shard_id = tiers_start_count[shard.tier] + shard.N
            instance = CURDB.groups.get_instance_by_N(selected_host.host.name, group.card.name, i)
            multishards[shard_id].brigades.append(TIntGroup([[instance]], []))

    intlookup.intl2_groups.append(TIntl2Group(multishards=multishards))


def main(options):
    needed_shards = []
    for tier, _ in options.tiers_data:
        needed_shards.extend(map(lambda x: Shard(tier, x), range(CURDB.tiers.tiers[tier].get_shards_count())))
    #    random.shuffle(needed_shards)

    from gaux.aux_shared import calc_source_hosts
    candidate_hosts = calc_source_hosts(options.from_groups)
    print "Unfiltered candidates total: %s" % len(candidate_hosts)
    candidate_hosts = filter(options.host_filter, candidate_hosts)
    print "Filtered candidates total: %s" % len(candidate_hosts)
    candidate_hosts = map(lambda x: WorkingHost(x, options), candidate_hosts)

    selected_hosts = map(lambda x: WorkingHost(x, options), options.group.getHosts())

    #    for candidate in candidate_hosts:
    #        print candidate.asstr()
    # ==================================================================
    # find research hosts
    # ==================================================================
    if options.research_power > 0:
        assert sum(map(lambda x: x.host.power,
                       candidate_hosts)) >= options.research_power, "Not enough power even for research"

        candidate_hosts.sort(cmp=lambda x, y: cmp(y.host.power, x.host.power))

        assigned_research_power = 0.
        for candidate_host in candidate_hosts:
            if candidate_host.slots_left < 2:  # R1 research hack (need at least 2 instances on machines with research slot)
                continue
            candidate_host.add_research_slot()
            assigned_research_power += candidate_host.host.power
            if assigned_research_power >= options.research_power:
                break

    candidate_hosts = set(candidate_hosts)

    for i, shard in enumerate(needed_shards):
        print "Run shard (%s, %s) (%d hosts left, %d shards left)" % (
        shard.tier, shard.N, len(candidate_hosts), len(needed_shards) - i)

        # ==============================================================
        # find hosts for first replica
        # ==============================================================
        if not find_candidate_in_selected(shard, selected_hosts, first_replica=True):
            best_candidate = find_candidate_in_unselected(shard, candidate_hosts, first_replica=True,
                                                          prefer_amd=options.prefer_amd)
            candidate_hosts.remove(best_candidate)
            selected_hosts.append(best_candidate)

        # ==============================================================
        # find hosts for second replica
        # ==============================================================
        if not find_candidate_in_selected(shard, selected_hosts, first_replica=False):
            best_candidate = find_candidate_in_unselected(shard, candidate_hosts, first_replica=False,
                                                          prefer_amd=options.prefer_amd)
            candidate_hosts.remove(best_candidate)
            selected_hosts.append(best_candidate)

    print "Total assgined power: %s" % sum(map(lambda x: x.host.power, selected_hosts))
    for host in selected_hosts:
        print host.asstr()

    tiers = map(lambda (tier, _): tier, options.tiers_data)

    to_add_hosts = filter(lambda x: not options.group.hasHost(x.host), selected_hosts)
    to_add_hosts = map(lambda x: x.host, to_add_hosts)

    if options.group.card.master is None:
        CURDB.groups.move_hosts(to_add_hosts, options.group)
    else:
        assert (options.group.card.host_donor is None)
        CURDB.groups.move_hosts(to_add_hosts, options.group.card.master)
        CURDB.groups.add_slave_hosts(to_add_hosts, options.group)

    # fill research power group before creating main intlookups
    if options.research_power > 0:
        CURDB.groups.add_slave_hosts(
            map(lambda x: x.host, filter(lambda y: y.research_on_host == True, selected_hosts)), options.research_group)
        research_intlookup = CURDB.intlookups.get_intlookup(options.research_group.intlookups[0])
        research_intlookup.tiers = ['ResearchTier0']

        multishard = TMultishardGroup()
        multishard.brigades = map(lambda x: TIntGroup([[x]], []), options.research_group.get_instances())
        research_intlookup.intl2_groups.append(TIntl2Group([multishard]))

    CURDB.groups.add_slave_hosts(
        map(lambda x: x.host, filter(lambda y: len(y.second_replica_shards) > 0, selected_hosts)), options.backup_group)
    write_intlookup(selected_hosts, options.backup_group, tiers, first_replica=False)

    options.group.refresh_after_card_update()
    write_intlookup(selected_hosts, options.group, tiers, first_replica=True)

    CURDB.update(smart=True)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)

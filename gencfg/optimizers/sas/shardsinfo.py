from collections import defaultdict

from core.db import CURDB


class TierInfo(object):
    def __init__(self, intlookup_name, tier_name, power, replicas, ssd_replicas, hosts_per_group, slot_size, no_extra_replicas):
        N = CURDB.tiers.primus_int_count(tier_name)[0]
        if N % hosts_per_group != 0:
            raise Exception("Incompatible tier shards count and hosts per group: %s and %s" % (N, hosts_per_group))

        self.intlookup_name = intlookup_name
        self.tier_name = tier_name
        self.power = float(power)
        self.total_shards = N / hosts_per_group
        self.replicas = int(replicas)
        self.ssd_replicas = int(ssd_replicas)
        self.hosts_per_group = hosts_per_group
        self.slot_size = slot_size
        self.no_extra_replicas = no_extra_replicas

        self.shards = []
        for i in range(self.total_shards):
            self.shards.append(ShardInfo(self.tier_name, self.power, self.replicas, self.ssd_replicas, self.slot_size))

    def append_other(self, other):
        assert self.intlookup_name == other.intlookup_name
        assert self.tier_name == other.tier_name
        assert self.total_shards == other.total_shards
        assert self.hosts_per_group == other.hosts_per_group

        for i in xrange(self.total_shards):
            self.shards[i].append_other(other.shards[i])


class ShardInfo(object):
    def __init__(self, tier_name, needed_power, replicas, ssd_replicas, slot_size):
        self.tier_name = tier_name
        self.needed_power = needed_power
        self.replicas = replicas
        self.ssd_replicas = ssd_replicas
        self.slot_size = slot_size

        self.assigned_igroups = []

    @property
    def real_power(self):
        return sum(map(lambda x: x.power, self.assigned_igroups))

    def get_needed_power(self):
        if len(self.assigned_igroups) >= self.replicas:
            return []

        P = self.needed_power - sum(map(lambda x: x.power, self.assigned_igroups))
        if P <= 0.:  # FIMXE: kinda hack
            P = 1.
        N = self.replicas - len(self.assigned_igroups)
        return [(P / N, self)] * N

    def append_other(self, other):
        assert self.tier_name == other.tier_name

        self.needed_power += other.needed_power
        self.replicas += other.replicas
        self.ssd_replicas += other.ssd_replicas
        self.assigned_igroups += other.assigned_igroups

    def __str__(self):
        if len(self.assigned_igroups) == 0:
            return "HAHAHA, Shard %s, needed power %s" % (self.tier_name, self.needed_power)
        else:
            return "Shard %s, needed power %s, real power %s, max %s, replicas %s, ssd %s" % (
                self.tier_name, self.needed_power, self.real_power, max(map(lambda x: x.power, self.assigned_igroups)),
                len(self.assigned_igroups), sum(map(lambda x: int(x.hgroup.ssd), self.assigned_igroups))
            )


def load_tiers(tiers_data, hosts_per_group):
    tiers = []
    for elem in tiers_data:
        tiers.append(TierInfo(elem.intlookup, elem.tier, elem.power, elem.replicas, elem.ssdreplicas, hosts_per_group,
                              elem.slot_size, elem.no_extra_replicas))
    return tiers


def check_and_add_replicas(tiers, sas_hgroups, mode, optimize_ssd):
    tiers_by_name = {x.tier_name: x for x in tiers}
    shards = sum(map(lambda x: x.shards, tiers), [])
    # fix power
    shards_power = sum(map(lambda x: x.needed_power, shards))
    hgroups_power = sum(map(lambda x: x.power, sas_hgroups))
    coeff = hgroups_power / shards_power
    for shard in shards:
        shard.needed_power *= coeff * 0.97

    # fix replicas count
    needed_replicas = sum(map(lambda x: x.replicas * x.slot_size, shards))
    have_replicas = sum(map(lambda x: x.icount, sas_hgroups))
    if needed_replicas > have_replicas:
        raise Exception("Not enough replicas: have %s, needed %s" % (have_replicas, needed_replicas))

    needed_ssd_replicas = sum(map(lambda x: x.ssd_replicas * x.slot_size, shards))
    have_ssd_replicas = sum(map(lambda x: x.icount, filter(lambda y: y.ssd == True, sas_hgroups)))
    if optimize_ssd and needed_ssd_replicas > have_ssd_replicas:
        raise Exception("Not enough ssd replicas: have %s, needed %s" % (have_ssd_replicas, needed_ssd_replicas))

    extra_replicas = have_replicas - needed_replicas
    if mode in ['max', 'min', 'uniform']:
        stop = False
        while not stop:
            if mode == 'min':
                c = min(map(lambda x: x.replicas, shards))
            elif mode == 'max':
                c = max(map(lambda x: x.replicas, shards))
            for shard in shards:
                if (mode == 'uniform' or shard.replicas == c) and (not tiers_by_name[shard.tier_name].no_extra_replicas):
                    if extra_replicas < shard.slot_size:
                        stop = True
                        break
                    shard.replicas += 1
                    extra_replicas -= shard.slot_size
    elif mode == 'fair':
        shards_by_replicas = defaultdict(list)
        for shard in shards:
            shards_by_replicas[shard.replicas].append(shard)

        all_before = 0
        all_after = 0
        for r in sorted(shards_by_replicas.keys()):
            my_before = r * len(shards_by_replicas[r])
            my_after = (my_before + all_before) * have_replicas / needed_replicas - all_after
            all_before += my_before
            all_after += my_after
            my_extra = my_after - my_before
            while my_extra > 0:
                for shard in shards_by_replicas[r]:
                    shard.replicas += 1
                    my_extra -= 1
                    if my_extra == 0:
                        break
    else:
        raise Exception("Unknown extra replicas distribution mode %s" % mode)


def show_shards(tiers, verbose):
    shards = sum(map(lambda x: x.shards, tiers), [])

    if verbose:
        for shard in shards:
            print "%s" % shard
    else:
        result = {}
        for shard in shards:
            if shard.tier_name in result:
                if shard.needed_power - shard.real_power > result[shard.tier_name].needed_power - result[
                    shard.tier_name].real_power:
                    result[shard.tier_name] = shard
            else:
                result[shard.tier_name] = shard
        for k in sorted(result.keys()):
            print result[k]

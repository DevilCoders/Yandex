# coding: utf-8
import collections
import copy
import random
from core.db import CURDB as db
from core.instances import TMultishardGroup, TIntl2Group, TIntGroup, Instance

tier1_intlookup = db.intlookups.get_intlookup('VLA_WEB_TIER1_JUPITER_BASE')
tier0_intlookup = db.intlookups.get_intlookup('VLA_YT_RTC.WebTier0')

tier0_byhost = {instance.host.name: instance for instance in tier0_intlookup.get_base_instances()}

# save tier1 instances
tier1_inst = []
for instance in tier1_intlookup.get_base_instances():
    tier1_inst.append(tier0_byhost[instance.host.name])
with open('tier1_instances', 'w') as f:
    for inst in tier1_inst:
        print >> f, inst.host.name, inst.power

# round power
bypower = collections.defaultdict(set)
for inst in tier1_inst:
    power = int(inst.power / 40) * 40
    bypower[power].add(inst.host.name)
with open('tier1_instances_rounded', 'w') as f:
    for power, hosts in bypower.iteritems():
        for host in hosts:
            print >>f, host, power

# cut down overpowered instances
bypower[1120] |= bypower[1720]
bypower[1120] |= bypower[1840]
bypower[1120] |= bypower[2040]
bypower[1120] |= bypower[2120]
bypower[1120] |= bypower[2200]
del bypower[1720]
del bypower[1840]
del bypower[2040]
del bypower[2120]
del bypower[2200]

# align groups (by 18)
l = list(bypower[1120])
random.shuffle(l)
extra = set(l[0:17])
bypower[1120] -= extra
bypower[320] += extra
bypower[320] |= extra
with open('tier1_instances_aligned', 'w') as f:
    for power, hosts in bypower.iteritems():
        for host in hosts:
            print >>f, host, power

# group by power
with open('tier1_instances_grouped', 'w') as f:
    for power, hosts in bypower.iteritems():
        for index, host in enumerate(hosts):
            if index % 18 == 0:
                print >>f, host, power

# run $python opt.py > optimized

# read optimization result
with open('optimized') as f:
    groups = []
    for line in f.readlines():
        fields = line.split()
        groups.append([int(float(fields[0])), int(float(fields[1])), int(float(fields[2]))])

def calc_instances(bypow):
    bypow = copy.deepcopy(bypow)
    grouped_instances = []
    for group in groups:
        grouped_instances.append([])
        for i in range(3):
            power = group[i]
            hosts = set(random.sample(bypow[power], 18))
            bypow[power] -= hosts
            grouped_instances[-1].append((hosts, power))
    return grouped_instances

r = calc_instances(bypower)

# save resulting instances
with open('result', 'w') as f:
    for group in r:
        for replica in group:
            hosts, power = replica
            for host in hosts:
                print >> f, host, power

# create and fill flat intlookup
intl = db.intlookups.create_empty_intlookup('TEST_INTL')
intl.hosts_per_group = 18
intl.brigade_groups_count = 82
intl.base_type = 'VLA_WEB_TIER1_JUPITER_BASE'
intl.tiers = ['WebTier1']

flatr = sum([x for x in r], [])
intl2_group = TIntl2Group()
intl.intl2_groups = [intl2_group]
for i in xrange(0, len(flatr), 3):
    mgroup = TMultishardGroup()
    for j in range(3):
        hosts, power = flatr[i + j]
        mgroup.brigades.append(TIntGroup([[Instance(db.hosts.get_host_by_name(host), float(power), 25130, 'VLA_WEB_TIER1_JUPITER_BASE', 0)] for host in hosts], []))
    intl2_group.multishards.append(mgroup)

intl.write_intlookup_to_file_json('test.intl')

#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import random
from collections import defaultdict

import gencfg
from core.db import CURDB

if __name__ == '__main__':
    random.seed(123123)

    # webs = ['intlookup-msk.py', 'intlookup-msk-platinum.py', 'intlookup-msk-geo.py', 'intlookup-msk-ukr.py']
    ils = [CURDB.intlookups.get_intlookup(il) for il in
           CURDB.groups.get_group('MSK_WEB_BASE').intlookups]  # if il != 'intlookup-msk-f.py']
    brigades = [brigade for il in ils for group in il.brigade_groups for brigade in group.brigades]
    remove = set()

    # we calc load for 1Gb switches only -- they are situated in the following dcs/queues:
    bad_dcs = ['myt']
    bad_queues = ['ugr1', 'ugr2', 'iva1', 'iva2', 'iva3', 'iva4']
    bad_switches = set()
    for host in CURDB.hosts.get_hosts():
        if host.dc in bad_dcs or host.queue in bad_queues:
            bad_switches.add(host.switch)

    switch_load = defaultdict(float)
    for brigade in brigades:
        ints_power = sum([i.power for i in brigade.intsearchers], .0)
        for intsearcher in brigade.intsearchers:
            int_switch = intsearcher.host.switch
            switch_load[int_switch] += brigade.power * intsearcher.power / ints_power
            for basesearher in [x[0] for x in brigade.basesearchers]:
                base_switch = basesearher.host.switch
                if int_switch == base_switch:
                    continue
                load = brigade.power * intsearcher.power / ints_power
                switch_load[base_switch] += load
                switch_load[int_switch] += load

    mmetas = CURDB.intlookups.get_intlookup('intlookup-msk-mmeta.py')
    mmetas = [brigade.basesearchers[0][0] for group in mmetas.brigade_groups for brigade in group.brigades]
    mmetas_power = 1. * len(mmetas)
    for meta in mmetas:
        meta_switch = meta.host.switch
        for brigade in brigades:
            ints_power = sum([i.power for i in brigade.intsearchers], .0)
            for intsearcher in brigade.intsearchers:
                int_switch = intsearcher.host.switch
                if int_switch == meta_switch:
                    continue
                load = brigade.power * intsearcher.power / ints_power / mmetas_power
                switch_load[meta_switch] += load
                switch_load[int_switch] += load

    switches = [switch for switch in switch_load.keys() if switch in bad_switches]
    switches.sort(cmp=lambda x, y: cmp(switch_load[y], switch_load[x]))
    n = 15
    print 'Most load %s switches: %s' % (n, ','.join(switches[:n]))

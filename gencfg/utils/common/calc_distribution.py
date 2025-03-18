#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from gaux.aux_utils import draw_hist

if __name__ == '__main__':
    intlookups = sys.argv[1:]
    if not intlookups:
        exit(0)
    intlookups = [CURDB.intlookups.get_intlookup(os.path.basename(x)) for x in intlookups]
    hosts = sum([[x.host for x in intlookup.get_base_instances()] for intlookup in intlookups], [])
    hosts = list(set(hosts))
    host_load = dict()
    for intlookup in intlookups:
        weight = intlookup.weight
        for shard in range(intlookup.hosts_per_group * intlookup.brigade_groups_count):
            brigades = intlookup.brigade_groups[shard / intlookup.hosts_per_group].brigades
            pos = shard % intlookup.hosts_per_group
            shard_power = sum([x.power for x in brigades], .0)
            for brigade in brigades:
                assert (len(brigade.basesearchers[pos]) == 1)
                host = brigade.basesearchers[pos][0].host
                if host.name not in host_load:
                    host_load[host.name] = .0
                host_load[host.name] += weight * brigade.power / shard_power / host.power

    median = sorted(host_load.values())[len(host_load) / 2]
    host_load = dict((x, min(1.0, y / median * .5)) for x, y in host_load.items())
    min_load = min(host_load.values())
    max_load = max(host_load.values())
    hosts_by_load = [y for x, y in sorted(map(lambda x: (x[1], x[0]), host_load.items()))]
    print 'min: %s, max %s' % (1.0, max_load / min_load)
    print 'low 10: %s' % ','.join(hosts_by_load[:10])
    print 'high 10: %s' % ','.join(hosts_by_load[-10:])
    draw_hist(host_load.values(), invert=True, header='heavy load hosts |--------> idle hosts; relative load;')
    # f = open('distribution.txt', 'w')
    # print >>f, '\n'.join('%s\t%s' % x for x in host_load.items())

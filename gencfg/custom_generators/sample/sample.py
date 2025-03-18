#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))  # add gencfg root to path

import gencfg
from core.db import CURDB


def main():
    if len(sys.argv) == 1:
        print "Usage: %s <group name>" % sys.argv[0]
        sys.exit(1)

    group = CURDB.groups.get_group(sys.argv[1])  # "group" object defined in <root>/core/igroups.py
    if len(group.intlookups):  # have intlookups
        print "Group %s with intlookups: %s" % (group.name, ','.join(group.intlookups))
        for intlookupname in group.intlookups:
            intlookup = CURDB.intlookups.get_intlookup(
                intlookupname)  # "intlookup" object defined in <root>/core/intlookups.py
            print "Processing intlookup %s" % (intlookup.file_name)
            for shard_id in range(intlookup.get_shards_count()):
                tier = intlookup.get_tier_for_shard(shard_id)
                primus = intlookup.get_primus_for_shard(shard_id)
                instances = intlookup.get_base_instances_for_shard(
                    shard_id)  # "instance" object defined in <root>/core/instances.py
                print "%s %s %s" % (tier, primus, ','.join(map(lambda x: str(x), instances)))
    else:  # do not have intlookups
        instances = group.get_instances()
        print ','.join(map(lambda x: str(x), instances))


if __name__ == '__main__':
    main()

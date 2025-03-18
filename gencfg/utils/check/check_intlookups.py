#!/skynet/python/bin/python

"""
    Check that we do not have repeating instances in intlookups
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
from collections import defaultdict

import gencfg
from core.db import CURDB
from gaux.aux_decorators import gcdisable

# SAS_WEB_CALLISTO_CAM_BASE contains non-trivial hamster intlookup
NOCHECK_GROUPS = ['MSK_WEB_NMETA', 'SAS_WEB_NMETA', 'SAS_WEB_CALLISTO_CAM_BASE']

@gcdisable
def main():
    intlookups = CURDB.intlookups.get_intlookups()
    intlookups = filter(lambda x: x.base_type not in NOCHECK_GROUPS, intlookups)

    instances_data = defaultdict(list)

    for intlookup in intlookups:
        for instance in intlookup.get_instances():
            instances_data[instance.name()].append(intlookup.file_name)

    failed = False
    for k in instances_data:
        if len(instances_data[k]) > 1:
            print "Instance %s in more than one intlookup: %s" % (k, ' '.join(instances_data[k]))
            failed = True

    if failed:
        sys.exit(1)

if __name__ == '__main__':
    main()

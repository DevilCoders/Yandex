#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
import config
from core.db import CURDB

if __name__ == '__main__':
    result = {}
    for group in CURDB.groups.get_groups():
        if group.card.master is not None:
            continue
        result[group.card.name] = sum(map(lambda x: x.power, group.get_instances()))

    fname = os.path.join(config.WEB_GENERATED_DIR, 'group.power')
    with open(fname, 'w') as f:
        for k in sorted(result.keys()):
            print >> f, "%s %s" % (k, result[k])

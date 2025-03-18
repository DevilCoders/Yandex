#!/skynet/python/bin/python
import os
import sys
import time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from argparse import ArgumentParser

import custom_generators.intmetasearchv2.generate_configs as intmetasearchv2


if __name__ == "__main__":
    parser = ArgumentParser(description="Script to precalc caches")
    parser.add_argument("--no-nanny", action="store_true", default=False, help="disable nanny services cache, needed for cauth export",
                        required=False)
    parser.add_argument("--no-configs", action="store_true", default=False, help="disable intmetasearch cache, needed for configs",
                        required=False)

    options = parser.parse_args()

    db = CURDB

    start_time = time.time()
    hosts = db.hosts.get_hosts_by_name([])
    print ('db.hosts.get_hosts_by_name([]) in {}s'.format(int(time.time() - start_time)))

    start_time = time.time()
    groups = db.groups.get_groups()
    print ('db.groups.get_groups() in {}s'.format(int(time.time() - start_time)))

    db.precalc_caches(options.no_nanny)

    if not options.no_configs:
        start_time = time.time()
        intmetasearchv2.jsmain({
            'action': intmetasearchv2.EActions.PRECALC_CACHES,
        })
        print ('intmetasearchv2 in {}s'.format(int(time.time() - start_time)))


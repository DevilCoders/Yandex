#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from config import MAIN_DIR, WEB_GENERATED_DIR
from core.searcherlookup import StrippedSearcherlookup


def _get_dublicates(l):
    seen = set()
    seen_add = seen.add
    seen_twice = set(x for x in l if x in seen or seen_add(x))
    return list(seen_twice)


def check_unique_hosts_in_slookup_line(ss):
    errors = []

    for shard_name in ss.slookup:
        dublicates = _get_dublicates(map(lambda x: x.name, ss.slookup[shard_name]))
        if len(dublicates) > 0:
            errors.append("Shard %s has dublicates %s" % (shard_name, ",".join(dublicates)))

    return errors


def check_unique_instances_in_ilookup_line(ss):
    errors = []

    for shard_name in ss.ilookup:
        dublicates = _get_dublicates(map(lambda x: "%s:%s" % (x.host.name, x.port), ss.ilookup[shard_name]))
        if len(dublicates) > 0:
            errors.append("Shard %s has dublicates %s" % (shard_name, ",".join(dublicates)))

    return errors


def check_unique_shard_for_instance(ss):
    errors = []

    all_instances_in_ilookup = []
    for l in ss.ilookup.values():
        all_instances_in_ilookup.extend(l)
    dublicates = _get_dublicates(map(lambda x: "%s:%s" % (x.host.name, x.port), all_instances_in_ilookup))
    if len(dublicates):
        errors.append("Found instances in multiple ilookup lines: %s" % ",".join(dublicates))

    return errors


CHECKERS = [
    (check_unique_hosts_in_slookup_line, "Every line in %slookup section consists of non-repeating"),
    (check_unique_instances_in_ilookup_line, "Every line in %ilookup section consists of non-repeating hosts"),
    (check_unique_shard_for_instance, "Every instance only in one line in %ilookup seciton"),
]

if __name__ == '__main__':
    fname = os.path.join(MAIN_DIR, WEB_GENERATED_DIR, 'searcherlookup.conf')

    slookup = StrippedSearcherlookup(fname)

    failed = 0
    for checker_func, checker_descr in CHECKERS:
        checker_errors = checker_func(slookup)
        if len(checker_errors):
            print "%s\n%s" % (checker_descr, "\n".join(map(lambda x: "    %s" % x, checker_errors)))
            failed = 1

    sys.exit(failed)

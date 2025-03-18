#!/skynet/python/bin/python
"""
    This script can be used to get rid of situation when two or more replicas located on the same host
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import math
from optparse import OptionParser

import gencfg
from core.db import CURDB


def parse_cmd():
    usage = "Usage %prog [options]"
    parser = OptionParser(usage=usage)
    parser.add_option("-i", "--intlookups", type="str", dest="intlookups", default=None,
                      help="Obligatory. Comma-separated list of intlookups to process")

    if len(sys.argv) == 1:
        sys.argv.append('-h')

    (options, args) = parser.parse_args()

    if args is not None and len(args):
        raise Exception("Unparsed args %s" % args)
    if options.intlookups is None:
        raise Exception("You must specify --intlookups option")

    options.intlookups = map(lambda x: CURDB.intlookups.get_intlookup(x), options.intlookups.split(','))

    return options


def check_brigades_hosts_intersection(intlookup):
    data = []

    # host check equlity
    intlookup.for_each_brigade(lambda x: data.append(set(map(lambda y: y.host.name, x.get_all_basesearchers()))))
    for i in range(len(data) - 1):
        for j in range(i + 1, len(data)):
            l = len(data[i] & data[j])
            if l != len(data[i]) and l != 0:
                raise Exception("Intlookup %s: Found group with strange intersection: '%s' and '%s'" % (intlookup.file_name, ' '.join(data[i]), ' '.join(data[j])))


def _id(brigade):
    return brigade.basesearchers[0][0].host.name


def _can_swap(brigade_group, brigade):
    if _id(brigade) in map(lambda x: _id(x), brigade_group.brigades):
        return False
    return True


def _need_swap(brigade_group, brigade):
    assert (brigade in brigade_group.brigades)
    if len(filter(lambda x: _id(x) == _id(brigade), brigade_group.brigades)) > 1:
        return True
    return False


def _find_best_swap(candidates, brigade):
    best_candidate, best_diff = candidates[0], math.fabs(candidates[0].power - brigade.power)
    for candidate in candidates[1:]:
        # do not replace instances with ssd with instances without it (for snippets)
        if candidate.basesearchers[0][0].host.ssd > 0 and brigade.basesearchers[0][0].host.ssd == 0:
            continue
        if candidate.basesearchers[0][0].host.ssd == 0 and brigade.basesearchers[0][0].host.ssd > 0:
            continue

        if math.fabs(candidate.power - brigade.power) < best_diff:
            best_candidate, best_diff = candidate, math.fabs(candidate.power - brigade.power)

    return best_candidate, best_diff


def swap_with_best(intlookup, brigade):
    candidates = []
    for brigade_group in intlookup.brigade_groups:
        if _can_swap(brigade_group, brigade):
            candidates.extend(brigade_group.brigades)

    if len(candidates) == 0:
        print "WARNING: can not swap %s:%s" % (brigade.basesearchers[0][0].host.name, brigade.basesearchers[0][0].port)
    else:
        best_candidate, best_diff = _find_best_swap(candidates, brigade)
        best_candidate.swap(brigade)


if __name__ == '__main__':
    options = parse_cmd()

    for intlookup in options.intlookups:
        check_brigades_hosts_intersection(intlookup)

    for intlookup in options.intlookups:
        for i in range(len(intlookup.brigade_groups)):
            brigade_group = intlookup.brigade_groups[i]
            for brigade in brigade_group.brigades:
                if _need_swap(brigade_group, brigade):
                    swap_with_best(intlookup, brigade)

    CURDB.update()

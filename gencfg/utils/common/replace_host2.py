#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import math
from collections import defaultdict
import logging
import re

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from core.instances import Instance
from core.igroups import CIPEntry
import core.card.types
from core.exceptions import UtilNormalizeException
from core.resources import TResources
from gaux.aux_utils import correct_pfname


class EAffinity(object):
    SAME = 0
    MORE = 1
    IGNORE = 2


def _compare_by_diff(v1, v2, mindiff):
    if math.fabs(v1 - v2) <= mindiff:
        return EAffinity.SAME
    elif v1 > v2:
        return EAffinity.MORE
    else:
        return EAffinity.IGNORE


class ICompare(object):
    def __init__(self, affinity, dcoeff):
        self.affinity = affinity
        self.dcoeff = dcoeff


class TCompareMemory(ICompare):
    NAME = 'mem'

    def __init__(self, affinity, dcoeff=1.):
        super(TCompareMemory, self).__init__(affinity, dcoeff)

    def match(self, candidate, badhost):
        return self.affinity >= _compare_by_diff(candidate.memory, badhost.memory, 0)

    def distance(self, candidate, badhost):
        return math.fabs(candidate.memory - badhost.memory) * self.dcoeff


class TComparePower(ICompare):
    NAME = 'power'

    def __init__(self, affinity, dcoeff=0.1):
        super(TComparePower, self).__init__(affinity, dcoeff)

    def match(self, candidate, badhost):
        return self.affinity >= _compare_by_diff(candidate.power, badhost.power, 0.01)

    def distance(self, candidate, badhost):
        return math.fabs(candidate.power - badhost.power) * self.dcoeff


class TCompareDisk(ICompare):
    NAME = 'disk'

    def __init__(self, affinity, dcoeff=0.01):
        super(TCompareDisk, self).__init__(affinity, dcoeff)

    def match(self, candidate, badhost):
        return self.affinity >= _compare_by_diff(candidate.hdd_size, badhost.hdd_size, 10)

    def distance(self, candidate, badhost):
        return math.fabs(candidate.hdd_size - badhost.hdd_size) * self.dcoeff


class TCompareSsd(ICompare):
    NAME = 'ssd'

    def __init__(self, affinity, dcoeff=0.1):
        super(TCompareSsd, self).__init__(affinity, dcoeff)

    def match(self, candidate, badhost):
        return self.affinity >= _compare_by_diff(candidate.ssd_size, badhost.ssd_size, 10)

    def distance(self, candidate, badhost):
        return math.fabs(candidate.ssd_size - badhost.ssd_size) * self.dcoeff


class TCompareQueue(ICompare):
    NAME = 'queue'

    def __init__(self, affinity, dcoeff=10):
        super(TCompareQueue, self).__init__(affinity, dcoeff)

    def match(self, candidate, badhost):
        if candidate.queue == badhost.queue:
            return True
        return self.affinity >= EAffinity.IGNORE

    def distance(self, candidate, badhost):
        if candidate.queue == badhost.queue:
            return 0
        return self.dcoeff


class TCompareDc(ICompare):
    NAME = 'dc'

    def __init__(self, affinity, dcoeff=20):
        super(TCompareDc, self).__init__(affinity, dcoeff)

    def match(self, candidate, badhost):
        if candidate.dc == badhost.dc:
            return True
        return self.affinity >= EAffinity.IGNORE

    def distance(self, candidate, badhost):
        if candidate.dc == badhost.dc:
            return 0
        return self.dcoeff


class TCompareLocation(ICompare):
    NAME = 'location'

    def __init__(self, affinity, dcoeff=1000):
        super(TCompareLocation, self).__init__(affinity, dcoeff)

    def match(self, candidate, badhost):
        if candidate.location == badhost.location:
            return True
        return self.affinity >= EAffinity.IGNORE

    def distance(self, candidate, badhost):
        if candidate.location == badhost.location:
            return 0
        return self.dcoeff


class TMultiCompare(object):
    MAPPING = {
        'mem': TCompareMemory,
        'power': TComparePower,
        'disk': TCompareDisk,
        'ssd': TCompareSsd,
        'queue': TCompareQueue,
        'dc': TCompareDc,
        'location': TCompareLocation,
    }

    def __init__(self, s):
        self.comparators = []

        elems = dict(map(lambda x: (x[:-1], x[-1]), s.split(',')))
        for k in self.MAPPING:
            comparator_class = self.MAPPING[k]
            if k not in elems:
                affinity = EAffinity.SAME
            else:
                affinity = {'=': EAffinity.SAME, '+': EAffinity.MORE, '-': EAffinity.IGNORE}[elems[k]]
            self.comparators.append(comparator_class(affinity))

    def match(self, candidate, badhost):
        for comparator in self.comparators:
            if not comparator.match(candidate, badhost):
                return False
        return True

    def distance(self, candidate, badhost):
        return sum(map(lambda x: x.distance(candidate, badhost), self.comparators))

    def failedcheck_distance(self, candidate, badhost):
        failed_comparators = filter(lambda x: not x.match(candidate, badhost), self.comparators)
        return sum(map(lambda x: x.distance(candidate, badhost), failed_comparators) + [0.])


def wrap_lambda(checking_func, checking_status, old_func):
    return lambda x, y: checking_func(x, y) >= checking_status and old_func(x, y)


def get_parser():
    parser = ArgumentParserExt("Script to replace machines one-by-one")

    DEFAULT_COMPARATOR = 'mem=,power=,disk=,ssd=,queue=,dc=,location='
    parser.add_argument("-c", "--comparator", dest="comparator", type=str, default=DEFAULT_COMPARATOR,
                        help="""Obligatory. Comma-separated list of checkers. Every checker looks like <param>{+|=|-}, where '=' means this
parameter should be equal in both bad and replacement host, '+' means replacement host better than bad host, '-' means ignore this parameter.
Full list of comparators: %s. Missing comparators treated as '-'. Default comparator: %s""" % (
                        ','.join(TMultiCompare.MAPPING.keys()), DEFAULT_COMPARATOR))

    parser.add_argument("-s", "--hosts", type=argparse_types.hosts, required=True,
                        help="Obligatory. List of hosts to replace")
    parser.add_argument("-r", "--src-groups", type=argparse_types.groups, default=[],
                        help="Optional. List of reserved groups (*_RESERVED by default)")
    parser.add_argument("-d", "--dest-group", type=argparse_types.group, required=True,
                        help="Optional. Destination group for replaced machines")
    parser.add_argument("-x", "--exclude-groups", type=argparse_types.groups, default=[],
                        help="Optional. List of excluded groups for replace")
    parser.add_argument("-f", "--filter", type=argparse_types.pythonlambda, default=None,
                        help="Optional. Filter on hosts")
    parser.add_argument("--skip-missing", action='store_true', default=False,
                        help="Optional. Do nothing with hosts with no replacement")
    parser.add_argument("-v", "--verbose", action='store_true', default=False,
                        help="Optional. Show more verbose output")
    parser.add_argument("-m", "--modify", action='store_true', default=False,
                        help="Optional. Modify groups/intlookups contents (option --apply include this option)")
    parser.add_argument("-y", "--apply", action='store_true', default=False,
                        help="Optional. Use this option to apply changes.")
    return parser


def normalize(options):
    if len(options.src_groups) == 0:
        options.src_groups = filter(lambda x: x.card.name.endswith('_RESERVED'), CURDB.groups.get_groups())

    options.comparator = TMultiCompare(options.comparator)

    if options.apply:
        options.modify = True

    return options


def show_replacements(replacements, verbose=False):
    def host_as_str(host, verbose):
        if host is None:
            return "None"
        if verbose:
            return "%s %s %s %s %s %s %s %s" % (
            host.name, host.model, host.memory, host.disk, host.ssd, host.queue, host.dc, host.location)
        else:
            return "%s" % host.name

    all_failed_checkers = set()
    for badhost in replacements:
        found, goodhost = replacements[badhost]
        if found:
            logging.info("%s -> %s" % (host_as_str(badhost, verbose), host_as_str(goodhost, verbose)))
        else:
            goodhost, failed_checkers = goodhost
            for failed_checker in failed_checkers:
                symbol = '+'
                checker = failed_checker.split(' ')[-1]

                if 'Disable' in failed_checker:
                    if '{}+'.format(checker) in all_failed_checkers:
                        all_failed_checkers.remove('{}+'.format(checker))
                    symbol = '-'
                all_failed_checkers.add('{}{}'.format(checker, symbol))

            logging.info("%s -> None (Candidate %s with new checkers %s)" % (
                host_as_str(badhost, verbose),
                host_as_str(goodhost, verbose),
                ",".join(map(lambda x: "<%s>" % x, failed_checkers))
            ))
    if all_failed_checkers:
        logging.error('ALL FAILED CHECKERS: {}\n'.format(','.join(all_failed_checkers)))


def main(options, from_cmd=True):
    """
        Result element format:
            host :
                replace_host : newhost or None
                affected_groups: set of affected groups
                replace_instances: list of pairs (old instance, new instance)
                bad_group: group
    """
    result = defaultdict(dict)

    # find available hosts (all hosts which are not in intlookups)
    extended_groups = sum(map(lambda x: [x] + x.slaves, options.src_groups), [])
    busy_hosts = sum(map(lambda x: list(x.get_busy_hosts()), extended_groups), [])
    avail_hosts = sum(map(lambda x: x.getHosts(), options.src_groups), [])
    avail_hosts = set(avail_hosts) - set(busy_hosts) - set(options.hosts)
    if options.filter:
        avail_hosts = filter(options.filter, avail_hosts)

    if len(avail_hosts) < len(options.hosts):
        raise Exception("Not enough hosts: needed %d, have %d" % (len(options.hosts), len(avail_hosts)))

    free_avail_host_resources = {x: TResources.free_resources(x) for x in avail_hosts}
    for host in avail_hosts:
        # add back resources from master group...
        group = CURDB.groups.get_host_master_group(host)
        for instance in group.get_host_instances(host):
            free_avail_host_resources[host] += TResources.from_instance(instance)
        # ...and backgrounds
        for group in CURDB.groups.get_host_groups(host):
            if group.card.properties.background_group:
                for instance in group.get_host_instances(host):
                    free_avail_host_resources[host] += TResources.from_instance(instance)

    # find replacements
    replacements = {}
    for badhost in options.hosts:
        best_repl, best_repl_distance = None, sys.float_info.max
        best_failchecked_repl, best_failchecked_repl_distance = None, sys.float_info.max

        badhost_used_resources = TResources.used_resources(badhost, ignore_fullhost=True)
        for candidate in avail_hosts:
            if not free_avail_host_resources[candidate].can_subtract(badhost_used_resources):
                logging.debug('Host {} does not fit'.format(candidate.name))
                logging.debug('\tresources being replaced: {}'.format(badhost_used_resources))
                logging.debug('\tcandidate resources: {}'.format(free_avail_host_resources[candidate]))
                continue
            else:
                logging.debug('Host %s fits', candidate.name)

            # check if we can replace host and satisfy group requirements like netcard regexp
            group_reqs_failed = False
            for group in CURDB.groups.get_host_groups(badhost):
                if group.card.reqs.hosts.netcard_regexp is not None:
                    if not re.match(group.card.reqs.hosts.netcard_regexp, candidate.netcard):
                        group_reqs_failed = True
                        break

            if group_reqs_failed:
                logging.debug('Host {} skipped by failed netcard'.format(candidate.name))
                continue

            candidate_matched = options.comparator.match(candidate, badhost)
            candidate_distance = options.comparator.distance(candidate, badhost)
            candidate_failedcheck_distance = options.comparator.failedcheck_distance(candidate, badhost)

            if candidate_matched:
                if candidate_distance < best_repl_distance:
                    best_repl, best_repl_distance = candidate, candidate_distance
            if candidate_failedcheck_distance < best_failchecked_repl_distance:
                best_failchecked_repl, best_failchecked_repl_distance = candidate, candidate_failedcheck_distance

        if best_repl is not None:
            avail_hosts.remove(best_repl)
            replacements[badhost] = (True, best_repl)
        elif best_failchecked_repl is not None:
            failed_checkers = filter(lambda x: not x.match(best_failchecked_repl, badhost),
                                     options.comparator.comparators)

            failed_checkers_msgs = []
            for failed_checker in failed_checkers:
                if failed_checker.affinity == EAffinity.SAME:
                    failed_checker.affinity = EAffinity.MORE

                    if failed_checker.match(best_failchecked_repl, badhost):
                        failed_checkers_msgs.append("Set <same or more> for checker %s" % failed_checker.NAME)
                    else:
                        failed_checkers_msgs.append("Disable checker %s" % failed_checker.NAME)

                    failed_checker.affinity = EAffinity.SAME
                elif failed_checker.affinity == EAffinity.MORE:
                    failed_checkers_msgs.append("Disable checker %s" % failed_checker.NAME)
                else:
                    raise Exception("Affinity <%s> can not fail" % failed_checker.affinity)

            replacements[badhost] = (False, (best_failchecked_repl, failed_checkers_msgs))
        else:
            raise Exception('Not found hosts with enough resources to replace {}: {}'.format(badhost.name, badhost_used_resources))

    for k in replacements:
        result[k]['replace_host'] = replacements[k]
        result[k]['bad_group'] = options.dest_group
        result[k]['replace_instances'] = list()
        result[k]['affected_groups'] = set()

    if from_cmd:
        show_replacements(replacements, options.verbose)

    if not options.skip_missing:
        fail_to_replace = filter(lambda x: replacements[x][0] == False, replacements)
        if len(fail_to_replace) > 0:
            raise Exception("Fail to replace: %s" % ','.join(map(lambda x: x.name, fail_to_replace)))

    # perform actual replacements
    for badhost in replacements:
        status, goodhost = replacements[badhost]
        if not status:
            continue

        if options.modify:
            badhost_groups = CURDB.groups.get_host_groups(badhost)
            badhost_groups = [x for x in badhost_groups if x not in options.exclude_groups]

            badhost_master_groups = filter(lambda x: x.card.master is None and x.card.on_update_trigger is None and x.card.properties.background_group == False, badhost_groups)
            assert len(badhost_master_groups), "Host <%s> in <%d> master groups: %s" % (
                badhost.name, len(badhost_master_groups), ",".join(map(lambda x: x.card.name, badhost_master_groups)))

            CURDB.groups.move_host(goodhost, None)
            badhost_master_groups[0].replace_host(badhost, goodhost)
            if badhost not in options.dest_group.getHosts():
                CURDB.groups.move_host(badhost, options.dest_group)

    if options.apply:
        if from_cmd:
            logging.info("Applying changes...")

        CURDB.update(smart=True)

    return result


def jsmain(d):
    options = get_parser().parse_json(d)
    normalize(options)
    return main(options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    normalize(options)
    if options.verbose:
        logging.basicConfig(level=logging.DEBUG)
    else:
        logging.basicConfig(level=logging.INFO)
    main(options, from_cmd=True)

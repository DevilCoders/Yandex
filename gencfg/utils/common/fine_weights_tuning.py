#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser
import hashlib
from collections import defaultdict, namedtuple
import tempfile
import random

import gencfg
from core.db import CURDB
from gaux.aux_utils import run_command, correct_pfname
import core.argparse.types as argparse_types
from core.igroups import CIPEntry
from core.exceptions import UtilNormalizeException


# check if brigade consists of ssd machines only
def _ssd_brigade(brigade):
    return not len(filter(lambda x: x.host.ssd == 0, brigade.get_all_basesearchers()))


# check constraints:
# - all intlookups have same group size
# - if two groups have same hosts they have all same hosts
# - hosts weight are the same
def check_intlookups(intlookups):
    hosts_per_group = None
    groups_by_host = defaultdict(dict)

    for intlookup in intlookups:
        if hosts_per_group is None:
            hosts_per_group = intlookup.hosts_per_group
        if hosts_per_group != intlookup.hosts_per_group:
            raise Exception("Have different hosts_per_group: %d and %d" % (hosts_per_group, intlookup.hosts_per_group))

        for brigade_group in intlookup.get_multishards():
            for brigade in brigade_group.brigades:
                instances = brigade.get_all_basesearchers()
                #                instances_weights = set(map(lambda x: x.power, instances))
                #                if len(instances_weights) > 1:
                #                    raise Exception("Have different weights in group %s: %s" % (brigade.basesearchers[0][0].name(), ", ".join(map(lambda x: str(x), instances_weights))))

                group_hash = brigade.calculate_unordered_hash()
                for instance in instances:
                    if 'hash' in groups_by_host[instance.host]:
                        if groups_by_host[instance.host]['hash'] != group_hash:
                            print groups_by_host[instance.host]
                            raise Exception("Found groups with different hosts: group starting with <%s> has different hash than groups starting with <%s>" % (instance.name(), groups_by_host[instance.host]['first_instance'].name()))
                    else:
                        groups_by_host[instance.host]['hash'] = group_hash
                    groups_by_host[instance.host]['first_instance'] = instances[0]


# class corresponding to group
# optimization can change its power
class FineOptimizationGroup(object):
    def __init__(self, group, parent, ssd_power):
        self.group = group
        self.group_id = hashlib.sha224(''.join(sorted(map(lambda x: x.host.name, group.get_all_basesearchers())))).hexdigest()
        self.parent = parent

        self.power = group.power - ssd_power
        self.ssd_power = ssd_power

    def __str__(self):
        basesearch = self.group.basesearchers[0][0]
        return "%s:%s:%s" % (basesearch.host.name, basesearch.port, int(self.power))


# class corresponding to brigade group
class FineOptimizationBrigadeGroup(object):
    def __init__(self, brigades, tier, needed_power, ssd_power_ratio=0.0):
        assert (0.0 <= ssd_power_ratio < 1.)

        self.needed_power = needed_power * (1 - ssd_power_ratio)
        self.needed_ssd_power = needed_power * ssd_power_ratio
        if ssd_power_ratio > 0:
            self.needed_ssd_power_per_brigade = self.needed_ssd_power / len(filter(lambda x: _ssd_brigade(x), brigades))
        else:
            self.needed_ssd_power_per_brigade = 0.0

        self.fine_optimization_groups = map(
            lambda x: FineOptimizationGroup(x, self, self.needed_ssd_power_per_brigade if _ssd_brigade(x) else 0.0),
            brigades)
        self.power = sum(map(lambda x: x.power, self.fine_optimization_groups))
        self.tier = tier
        self.needed_power = needed_power

    def update_power(self):
        self.power = sum(map(lambda x: x.power, self.fine_optimization_groups))

    def __str__(self):
        result = '%s ' % self.tier
        result += ' '.join(map(lambda x: str(x), self.fine_optimization_groups))
        result += ' power=%s needed=%s needed_ssd=%s diff=%s%%' % (
        self.power, self.needed_power, self.needed_ssd_power, int(100 * self.power / self.needed_power) - 100)
        return result


# Load all intlookups from configuration, then assign weight to every brigade in loaded intloklups.
def load_weights(options):
    if options.alternate_weights_file is not None:
        options.alternate_weights = defaultdict(list)
        for line in open(options.alternate_weights_file).readlines():
            line = line.strip()
            options.alternate_weights[line.split(' ')[0]].append(float(line.split(' ')[1]))
    else:
        options.alternate_weights = None

    options.intlookups = []
    for intlookup_info in options.config:
        intlookup = CURDB.intlookups.get_intlookup(intlookup_info.intlookup)
        weight = intlookup_info.power
        multishards = intlookup.get_multishards()
        if options.alternate_weights is None:
            for brigade_group in multishards:
                brigade_group.weight = weight
        else:
            assert (len(multishards) == len(options.alternate_weights[intlookup.file_name]))
            for i in range(len(mulishards)):
                multishards[i].weight = options.alternate_weights[intlookup.file_name][i]

        options.intlookups.append(intlookup)


def run_optimization_step(processed_groups, verbose_level, min_instance_power, max_instance_power_ratio):
    # fix situation when more than one of processed groups belongs to one shard
    processed_brigade_groups = []
    new_processed_groups = []
    for group in processed_groups:
        if group.parent not in processed_brigade_groups:
            processed_brigade_groups.append(group.parent)
            new_processed_groups.append(group)
    processed_groups = new_processed_groups

    if len(processed_brigade_groups) == 1:
        return

    if verbose_level >= 1:
        print "====================================================="
        print "Processing group_id %s" % processed_groups[0].group_id
        print "    Starting values"
        for x in processed_brigade_groups:
            print "        " + str(x)

    K = []
    D = []
    MP = []
    S = 0.
    for i in range(len(processed_groups)):
        K.append(1. / processed_brigade_groups[i].needed_power)
        D.append((processed_brigade_groups[i].power - processed_groups[i].power) / processed_brigade_groups[
            i].needed_power - 1)
        MP.append(processed_brigade_groups[i].needed_power * max_instance_power_ratio)
        S += processed_groups[i].power

    assert (S > 0), "Sum power of host %s instances below zero (may be due to ssd optiomization)" % str(
        processed_groups[0])

    if verbose_level >= 2:
        print "Value member", K
        print "Free memeber", D
        print "Instance power", map(lambda x: x.power, processed_groups)

    lp_input = "min: z;\n\n"
    for i in range(len(K)):
        lp_input += "x%s >= %s;\n" % (i, min_instance_power)
        lp_input += "x%s <= %s;\n" % (i, MP[i])
        if D[i] <= 0:
            lp_input += "%s x%s + %s - z <= 0;\n" % (K[i], i, D[i])
            lp_input += "%s x%s + %s + z >= 0;\n" % (K[i], i, D[i])
    lp_input += " + ".join(map(lambda x: "x%s" % x, range(len(K)))) + " = %s;\n" % S

    if verbose_level >= 2:
        print lp_input

    lp_file, lp_filename = tempfile.mkstemp()
    os.write(lp_file, lp_input)
    os.close(lp_file)
    ret, out, err = run_command(["lp_solve", lp_filename], sleep_timeout=0.01, raise_failed=False)
    if ret != 0:
        print "Failed to solve the following problem:"
        print "=================================================================="
        print lp_input
        print "=================================================================="
        print out
        print "=================================================================="
        print err
        print "=================================================================="
        raise Exception("Failed to solve lp problem")

    if verbose_level >= 2:
        print out

    i = 0
    for line in out.splitlines()[-len(K):]:
        processed_groups[i].power = float(line.rpartition(' ')[2])
        processed_brigade_groups[i].update_power()
        i += 1

    if verbose_level >= 1:
        print "    Finishing values"
        for x in processed_brigade_groups:
            print "       " + str(x)
        print "====================================================="


def filter_excluded_instances_or_hosts(brigade, excluded_instances_or_hosts):
    for basesearch in brigade.get_all_basesearchers():
        if (basesearch.host.name, None) in excluded_instances_or_hosts:
            return False
        if (basesearch.host.name, basesearch.port) in excluded_instances_or_hosts:
            return False
    return True


def load_fine_optimization_brigade_groups(intlookups, flt, excluded_instances_or_hosts, ssd_power_ratio):
    # calculate real power
    sum_power = 0.
    sum_intlookups_weight = 0.
    for intlookup in intlookups:
        for brigade_group in intlookup.get_multishards():
            sum_intlookups_weight += brigade_group.weight
            for brigade in brigade_group.brigades:
                if flt(brigade) and filter_excluded_instances_or_hosts(brigade, excluded_instances_or_hosts):
                    sum_power += brigade.power

    # create groups with correct powers
    fine_optimization_brigade_groups = []
    for intlookup in intlookups:
        for brigade_group in intlookup.get_multishards():
            needed_power = brigade_group.weight / sum_intlookups_weight * sum_power
            brigades = filter(lambda x: flt(x) and filter_excluded_instances_or_hosts(x, excluded_instances_or_hosts),
                              brigade_group.brigades)
            fine_optimization_brigade_groups.append(
                FineOptimizationBrigadeGroup(brigades, intlookup.tiers[0], needed_power, ssd_power_ratio))  # FIXME

    return fine_optimization_brigade_groups


def pretune_power(fine_optimization_brigade_groups):
    # combine groups with same hosts by id
    fine_optimization_groups_by_id = defaultdict(list)
    for fine_optimization_brigade_group in fine_optimization_brigade_groups:
        for fine_optimization_group in fine_optimization_brigade_group.fine_optimization_groups:
            fine_optimization_groups_by_id[fine_optimization_group.group_id].append(fine_optimization_group)

    for brigade_group in fine_optimization_brigade_groups:
        max_power = max(map(lambda x: x.power, brigade_group.fine_optimization_groups))
        if brigade_group.power - max_power > brigade_group.needed_power:
            coeff = brigade_group.needed_power / brigade_group.power
            print "Pretuning %s" % str(brigade_group)
            print "Coeff %s" % coeff

            processed_ids = set()
            for group in brigade_group.fine_optimization_groups:
                if group.group_id in processed_ids:  # in case we have to brigades with same hosts and same shard
                    continue
                processed_ids.add(group.group_id)

                step_groups = fine_optimization_groups_by_id[group.group_id]
                if len(step_groups) == 1:
                    continue
                print map(lambda x: x.power, step_groups)
                step_extra_power = group.power * (1 - coeff) / (len(step_groups) - 1)

                group.power *= coeff
                for step_group in step_groups:
                    if step_group != group:
                        step_group.power += step_extra_power


def optimize_power(fine_optimization_brigade_groups, options):
    # combine groups with same hosts by id
    fine_optimization_groups_by_id = defaultdict(list)
    for fine_optimization_brigade_group in fine_optimization_brigade_groups:
        for fine_optimization_group in fine_optimization_brigade_group.fine_optimization_groups:
            fine_optimization_groups_by_id[fine_optimization_group.group_id].append(fine_optimization_group)

    if options.verbose_level >= 1:
        print "Weights before optimizing"
        for x in fine_optimization_brigade_groups:
            print str(x)

    for i in range(options.steps):
        group_ids = fine_optimization_groups_by_id.keys()
        if options.use_random:
            random.shuffle(group_ids)
        for group_id in group_ids:
            run_optimization_step(fine_optimization_groups_by_id[group_id], options.verbose_level,
                                  options.min_instance_power, options.max_instance_power_ratio)
        #            break

    if options.verbose_level >= 1:
        print "Weights after optimizing"
        for x in fine_optimization_brigade_groups:
            print str(x)

        #    for id_ in fine_optimization_groups_by_id:
        #        total_power = sum(map(lambda x: x.power,  fine_optimization_groups_by_id[id_]))
        #        ssd_power = sum(map(lambda x: x.ssd_power, fine_optimization_groups_by_id[id_]))
        #        coeff = (total_power + ssd_power) / total_power

        #        for fine_optimization_group in fine_optimization_groups_by_id[id_]:
        #            fine_optimization_group.power *= coeff

    for x in sum(fine_optimization_groups_by_id.values(), []):
        x.power += x.ssd_power
    #        if x.ssd_power > 0:
    #            x.power /= (1 - options.ssd_power_ratio)

    if options.verbose_level >= 1:
        print "Weights after revertind ssd_power_ratio fix"
        for x in fine_optimization_brigade_groups:
            print str(x)


def save_optimized(fine_optimization_brigade_groups):
    for fine_optimization_brigade_group in fine_optimization_brigade_groups:
        for fine_optimization_group in fine_optimization_brigade_group.fine_optimization_groups:
            if fine_optimization_group.power < 0:
                raise Exception("Can not solve: got negative brigade power for %s" % str(fine_optimization_group))

            if fine_optimization_group.power != fine_optimization_group.group.power:
                fine_optimization_group.group.power = fine_optimization_group.power
                for basesearch in fine_optimization_group.group.get_all_basesearchers():
                    basesearch.power = fine_optimization_group.power
                    cipentry = CIPEntry(basesearch.host, basesearch.port)
                    group = CURDB.groups.get_group(basesearch.type)
                    group.custom_instance_power[cipentry] = fine_optimization_group.power
                    group.custom_instance_power_used = True


def parse_cmd():
    parser = ArgumentParser(description="Post-recluster fine weights tuning")
    parser.add_argument("-c", "--config", type=argparse_types.sasconfig, dest="config", default=None,
                        help="Optional. Config with intlookups and intlookups weights")
    parser.add_argument("-i", "--intlookup", type=argparse_types.intlookup, default=None,
                        help="Optional. Intlookup to process (in case we have only 1 intlookup with only 1 tier and want to make equal power")
    parser.add_argument("-s", "--steps", type=int, dest="steps", default=1,
                        help="Optional. Number of optimization steps (default: 1)")
    parser.add_argument("-r", "--use_random", action="store_true", dest="use_random", default=False,
                        help="Optional. Use random steps")
    parser.add_argument("-p", "--pre-tuning", action="store_true", dest="run_pretuning", default=False,
                        help="Optional. Run pretuning (to reduce power of groups with to much power")
    parser.add_argument("-f", "--filters", action="append", type=str, default=None,
                        help="Optional. Filter for hosts")
    parser.add_argument("-a", "--alternate-weights-file", type=str, dest="alternate_weights_file", default=None,
                        help="Optional. File with alternate weights")
    parser.add_argument("-x", "--excluded-instances-or-hosts", type=str, dest="excluded_instances_or_hosts",
                        default=None,
                        help="Optional. List of excluded hosts or instances.")
    parser.add_argument("-v", "--verbose", action="count", dest="verbose_level", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity.  The maximum is 3.")
    parser.add_argument("--snippets-power-ratio", type=float, dest="ssd_power_ratio", default=0.0,
                        help="Optional. Snippets power ratio (range [0.0, 1.0)). Value 0.5 means snippet requests need as much power as search requests. Assume all snippet requests goes to ssd")
    parser.add_argument("--min-instance-power", type=float, default=1.0,
                        help="Optional. Minimal instance power (1.0 by default)")
    parser.add_argument("--max-instance-power-ratio", type=float, default=100.0,
                        help="Optional. When we have N instances with total power P, one instance can not have more than P * max_instance_power_ratio weight")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if options.filters is None:
        options.filters = [lambda x: True]
    else:
        options.filters = map(lambda x: eval(x), options.filters)

    if options.excluded_instances_or_hosts:
        options.excluded_instances_or_hosts = set(
            map(lambda x: (x.split(':')[0], int(x.split(':')[1]) if x.find(':') > 0 else None),
                options.excluded_instances_or_hosts.split(',')))
    else:
        options.excluded_instances_or_hosts = set()

    return options


def normalize(options):
    if int(options.config is None) + int(options.intlookup is None) != 1:
        raise UtilNormalizeException(correct_pfname(__file__), ["config", "intlookup"],
                                     "Options --config and --intlookup are mutually exclusive")

    if options.intlookup is not None:
        if options.intlookup.tiers is None:
            raise UtilNormalizeException(correct_pfname(__file__), ["intlookup"],
                                         "Intlookup <%s> should have exactly one tier (have no tiers)" % options.intlookup.file_name)

        if len(options.intlookup.tiers) > 1:
            raise UtilNormalizeException(correct_pfname(__file__), ["intlookup"],
                                         "Intlookup <%s> should have exacly one tier (have tiers <%s>)" % (
                                         options.intlookup.file_name, ",".join(options.intlookup.tiers)))

    if options.intlookup is not None:
        result_type = namedtuple('SasConfigFromIntlookup',
                                 ['intlookup', 'tier', 'power', 'replicas', 'ssdreplicas', 'slot_size'])
        result = result_type(intlookup=options.intlookup.file_name, tier=options.intlookup.tiers[0], power=100.,
                             replicas=1, ssdreplicas=0, slot_size=1)
        options.config = [result]


def main(options):
    load_weights(options)

    check_intlookups(options.intlookups)

    for flt in options.filters:
        fine_optimization_brigade_groups = load_fine_optimization_brigade_groups(options.intlookups, flt,
                                                                                 options.excluded_instances_or_hosts,
                                                                                 options.ssd_power_ratio)
        if options.run_pretuning:
            pretune_power(fine_optimization_brigade_groups)
        optimize_power(fine_optimization_brigade_groups, options)
        save_optimized(fine_optimization_brigade_groups)

    for intlookup in options.intlookups:
        intlookup.mark_as_modified()
        CURDB.groups.get_group(intlookup.base_type).mark_as_modified()

    CURDB.intlookups.update(smart=1)
    CURDB.groups.update(smart=1)


if __name__ == '__main__':
    options = parse_cmd()
    normalize(options)
    main(options)

#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import math
import sqlite3
from argparse import ArgumentParser
from collections import defaultdict

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types
from core.card.node import save_card_node


def parse_cmd():
    parser = ArgumentParser(description="Update config for sas optimizer")
    parser.add_argument("-c", "--sasconfig-file", type=str, default=None, required=True,
                        help="Obligatory. Files with sas optimizer config")
    parser.add_argument("-d", "--datafile", dest="datafile", type=str, default=None, required=True,
                        help="Obligatory. Sql data with instances cpu usage")
    parser.add_argument("--use-all-instances-stats", action="store_true", default=False,
                        help="Optional. If this option is set, ge usage from all instances with same tier")
    parser.add_argument("-q", "--quiet", dest="quiet", action="store_true", default=False,
                        help="Optional. Quiet (no output)")
    parser.add_argument("-y", "--apply", dest="apply", action="store_true", default=False,
                        help="Optional. Apply result to card")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    options.sasconfig = argparse_types.sasconfig(options.sasconfig_file)
    for elem in options.sasconfig:
        elem.intlookup = CURDB.intlookups.get_intlookup(elem.intlookup)

    return options


def _print_diff(intlookup, oldpower, newpower):
    if newpower > oldpower:
        prefix = "\033[1;31m+"
    else:
        prefix = "\033[1;32m-"
    print "%s old=%.2f new=%.2f (%s%.2f%%\033[0m)" % (
    intlookup.file_name, oldpower, newpower, prefix, math.fabs(newpower / oldpower * 100. - 100.))


def _adjusted_usage(hostname, instance_cpu_usage, host_cpu_usage):
    #    return instance_cpu_usage

    host = CURDB.hosts.get_host_by_name(str(hostname))
    model = CURDB.cpumodels.get_model(host.model)

    host_cpu_usage *= model.ncpu
    instance_cpu_usage *= model.ncpu

    host_cpu_usage = max(min(host_cpu_usage, model.ncpu - 1), 0.0001)
    instance_cpu_usage = min(instance_cpu_usage, host_cpu_usage)

    i = int(host_cpu_usage)
    qpsbyload = model.consumption[-1].qps.tbon
    adjusted_usage = qpsbyload[i] * (host_cpu_usage - i) + (i + 1 - host_cpu_usage) * qpsbyload[i + 1]
    return instance_cpu_usage / host_cpu_usage * adjusted_usage


def main(options):
    tiers_to_intlookups = dict(map(lambda x: (x.intlookup.tiers[0], x.intlookup.file_name), options.sasconfig))

    conn = sqlite3.connect(options.datafile)
    c = conn.cursor()

    sqldata = c.execute(
        "SELECT host, port, intlookup_name, tier, instance_cpu_usage, host_cpu_usage FROM data WHERE instance_cpu_usage IS NOT NULL AND host_cpu_usage IS NOT NULL").fetchall()

    # check if data is valid and enough
    sqldata_as_dict = dict(map(lambda (host, port, intlookup_name, tier, instance_cpu_usage, host_cpu_usage): (
    '%s:%s' % (host, port), intlookup_name), sqldata))
    for elem in options.sasconfig:
        all_instances = elem.intlookup.get_base_instances()
        bad = 0
        for instance in all_instances:
            iname = '%s:%s' % (instance.host.name, instance.port)
            if iname not in sqldata_as_dict:
                bad += 1
            elif sqldata_as_dict[iname] != elem.intlookup.file_name:
                raise Exception("Instance %s belnogs to intlookup %s in sql data" % (iname, sqldata_as_dict[iname]))
        if not options.quiet:
            print "Intlookup %s, %d of %d bad instances" % (elem.intlookup.file_name, bad, len(all_instances))
        if bad / float(len(all_instances)) > 0.3:
            raise Exception("To many bad instances for intlookup %s (%d of %d)" % (elem.intlookup.file_name, bad, len(all_instances)))

    # get real usage
    usage = defaultdict(float)
    for host, port, intlookup_name, tier, instance_cpu_usage, host_cpu_usage in sqldata:
        instance_cpu_usage = max(0.0, instance_cpu_usage)  # FIXME: instance cpu usage somhow can be less than zero
        host_cpu_usage = max(0.0, host_cpu_usage)

        if options.use_all_instances_stats:
            usage[tiers_to_intlookups.get(tier, None)] += _adjusted_usage(host, instance_cpu_usage, host_cpu_usage)
        else:
            usage[intlookup_name] += _adjusted_usage(host, instance_cpu_usage, host_cpu_usage)
    for elem in options.sasconfig:
        usage[elem.intlookup.file_name] /= elem.intlookup.hosts_per_group * elem.intlookup.brigade_groups_count
    mult = options.sasconfig[0].power / usage[options.sasconfig[0].intlookup.file_name]
    for elem in options.sasconfig:
        usage[elem.intlookup.file_name] *= mult
        if not options.quiet:
            _print_diff(elem.intlookup, elem.power, usage[elem.intlookup.file_name])

    # write to file
    if options.apply:
        for elem in options.sasconfig:
            elem.power = usage[elem.intlookup.file_name]
            elem.intlookup = elem.intlookup.file_name
        if not options.quiet:
            print "!!!!!!!!!!!!!!!!! Applying changes !!!!!!!!!!!!!!!!!!!!!!!"
        save_card_node(options.sasconfig, options.sasconfig_file, os.path.join(CURDB.SCHEMES_DIR, 'sasconfig.yaml'))
        if not options.quiet:
            print "!!!!!!!!!!!!!!!!!!!!!! DONE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"


if __name__ == '__main__':
    options = parse_cmd()
    main(options)

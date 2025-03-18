#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
from collections import defaultdict
import tempfile

from argparse_types import argparse_hosts_or_instances

from gaux.aux_utils import run_command


def parse_cmd():
    usage = 'Usage %prog [options]'
    parser = ArgumentParser(usage=usage)
    parser.add_argument("-t", "--tiers-data", dest="tiers_data", type=str, default=None, required=True,
                        help="Obligatory. File with replicas and power data")
    parser.add_argument("-x", "--exclude", dest="exclude", type=argparse_hosts_or_instances, default=None,
                        help="Optional. List of hosts or instances to exclude")
    parser.add_argument("-c", "--cpuload", dest="cpuload", type=float, default=0.3,
                        help="Optional. Cpu load in our units. If not specified, suppose 0.3 of maximum")
    parser.add_argument("-r", "--report-type", dest="report_type", type=str, default=None, required=True,
                        choices=['xdistribution'],
                        help="Obligatory. Report type")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    options.intlookups = []
    for line in open(options.tiers_data).readlines():
        options.intlookups.append(CURDB.intlookups.get_intlookup(line.split(' ')[0]))
        options.intlookups[-1].power = float(line.split(' ')[2])

    all_instances = sum(map(lambda x: x.get_base_instances(), options.intlookups), [])
    options.cpuload = sum(map(lambda x: x.power, all_instances)) * options.cpuload

    return options


def calculate_load(options):
    hosts_load = defaultdict(float)

    total_config_power = sum(map(lambda x: x.power * x.get_shards_count(), options.intlookups))
    for intlookup in options.intlookups:
        shard_cpu_load = intlookup.power / total_config_power * options.cpuload
        for shard_id in range(intlookup.get_shards_count()):
            shard_instances = intlookup.get_base_instances_for_shard(shard_id)
            if options.exclude:
                filtered_shard_instances = filter(lambda x: not options.exclude.has_instance(x), shard_instances)
                if len(filtered_shard_instances) == 0:
                    print "!!!! Shard %s in intlookup %s has no working instances, skipping..." % (shard_id, intlookup)
                    continue
                shard_instances = filtered_shard_instances

            total_shards_power = sum(map(lambda x: x.power, shard_instances)) + 0.0001
            for instance in shard_instances:
                hosts_load[instance.host] += instance.power / total_shards_power * shard_cpu_load

    return hosts_load


def report(report_type, hosts_load):
    LN = 100
    if report_type == 'xdistribution':
        counts = [0] * (LN + 1)
        for host in hosts_load:
            c = int(hosts_load[host] / host.power * LN)
            if c > LN: c = LN
            counts[c] += 1

        dataf, dataname = tempfile.mkstemp()
        for i in range(len(counts)):
            os.write(dataf, '%s %s\n' % (i, counts[i]))
        os.close(dataf)

        plotf, plotname = tempfile.mkstemp()
        os.write(plotf, 'set terminal x11 persist;\n')
        os.write(plotf, "plot '%s' using ($1):($2) with lines;" % dataname)
        os.close(plotf)

        run_command(['gnuplot', plotname])

        os.remove(dataname)
        os.remove(plotname)

    else:
        raise Exception("Unknown report type %s" % report_type)


# for host in sorted(hosts_load.keys(), lambda x, y: cmp(x.name, y.name)):
#        print '%s %s %s' % (host.name, hosts_load[host], host.power)

if __name__ == '__main__':
    options = parse_cmd()

    hosts_load = calculate_load(options)
    report(options.report_type, hosts_load)

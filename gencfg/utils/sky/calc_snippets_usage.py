#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import random
from argparse import ArgumentParser
from collections import defaultdict

import api.cqueue
from kernel.util.errors import formatException

import gencfg
import core.argparse.types as argparse_types
from gaux.aux_utils import read_tail_lines


class EReportTypes(object):
    SHORT = 'short'
    BY_SHARD = 'by_shard'
    ALL = [SHORT, BY_SHARD]


class UsageExtractor(object):
    def __init__(self):
        pass

    def run(self, instances):

        def _search_duration(loglines):
            if len(loglines) < 2:
                return 1.0
            startt = int(loglines[0].split('\t')[1]) + float(loglines[0].split('\t')[2]) / 1000000
            endt = int(loglines[-1].split('\t')[1]) + float(loglines[-1].split('\t')[2]) / 1000000
            return endt - startt

        def _snippets_duration(loglines):
            if len(loglines) < 2:
                return 1.0
            startt = float(loglines[0].split('\t')[3]) / 1000000
            endt = float(loglines[-1].split('\t')[3]) / 1000000
            return endt - startt

        result = []
        for (host, port) in instances:
            search_log = "/usr/local/www/logs/current-loadlog-base-%s" % port  # FIXME: use instancestate
            loglines = read_tail_lines(search_log, 10000)
            search_time = sum(map(lambda x: int(x.partition('\t')[0]), loglines), 0.) / _search_duration(loglines)

            snippet_log = "/usr/local/www/logs/current-passagelog-base-%s" % port  # FIXME: use instancestate
            loglines = read_tail_lines(snippet_log, 10000)
            snippets_time = sum(map(lambda x: int(x.split('\t')[4]), loglines), 0.) / _snippets_duration(loglines)

            result.append(((host, port), {"search_time": search_time, "snippets_time": snippets_time}))

        return result


class InstanceStat(object):
    __slots__ = ['instance', 'search_time', 'snippets_time', 'finished']

    def __init__(self, instance):
        self.instance = instance

        self.search_time = 0.0
        self.snippets_time = 0.0
        self.finished = False


class IntlookupStat(object):
    def __init__(self, intlookup, shard_count, shard_ids):
        self.intlookup = intlookup

        if shard_ids is not None:
            assert (max(shard_ids)) < intlookup.get_shards_count()
            self.shard_ids = shard_ids
        else:
            assert (shard_count <= intlookup.get_shards_count())
            lst = list(range(intlookup.get_shards_count()))
            random.shuffle(lst)
            self.shard_ids = lst[:shard_count]
        self.shard_ids.sort()

        self.instances_stat = dict()
        for shard_id in self.shard_ids:
            istat = map(lambda x: InstanceStat(x), self.intlookup.get_base_instances_for_shard(shard_id))
            self.instances_stat[shard_id] = istat

    def get_instances(self):
        return sum(self.instances_stat.itervalues(), [])

    def update(self, calculated_stats):
        for instance_stat in self.get_instances():
            iid = (instance_stat.instance.host.name, instance_stat.instance.port)
            if iid in calculated_stats:
                instance_stat.search_time = calculated_stats[iid]["search_time"]
                instance_stat.snippets_time = calculated_stats[iid]["snippets_time"]
                instance_stat.finished = True

    def _report_filtered(self, shards):
        all_instances = sum(map(lambda x: self.instances_stat[x], shards), [])
        all_search_time = sum(map(lambda x: x.search_time, all_instances))
        all_snippets_time = sum(map(lambda x: x.snippets_time, all_instances))
        got_stats = sum(map(lambda x: int(x.finished), all_instances))

        shard_ids = sorted(shards)
        if len(shard_ids) >= 5:
            shard_ids = shard_ids[:5] + ["..."]
        shard_ids_as_str = "[" + ",".join(map(lambda x: str(x), shard_ids)) + "]"

        return "Intlookup %s, shards %s, instances %s(%s), snippet usage ratio %.4f" % \
               (self.intlookup.file_name, shard_ids_as_str, got_stats, len(all_instances),
                all_snippets_time / (all_search_time + all_snippets_time + 0.00000001))

    def generate_report(self, report_type):
        if report_type == EReportTypes.SHORT:
            return self._report_filtered(self.instances_stat.keys())
        elif report_type == EReportTypes.BY_SHARD:
            return "\n".join(map(lambda x: self._report_filtered([x]), sorted(self.instances_stat.keys())))


def parse_cmd():
    parser = ArgumentParser(description="Show snippet requests cpu usage (compare to search requests)")
    parser.add_argument("-i", "--intlookups", type=argparse_types.intlookups, required=True,
                        help="Obligatory. List of intlookups")
    parser.add_argument("-r", "--report-type", type=str, required=True,
                        choices=EReportTypes.ALL,
                        help="Obligatory. Report type")
    parser.add_argument("-d", "--shard-count", type=int, default=None,
                        help="Optional. Number of shards to process (choosen randomly), 1 by default (mutually exclusive with --shard-ids)")
    parser.add_argument("-s", "--shard-ids", type=str, default=None,
                        help="Optional. Comma-separated shard ids (mutally exclusive with --shard-count)")
    parser.add_argument("-t", "--timeout", type=int, default=50,
                        help="Optional. Skynet timeout")
    parser.add_argument("--seed", type=int, default=None,
                        help="Optional. Random seed (to work with same shards on different runs)")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Increase output verbosity (maximum 2)")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def normalize(options):
    if int(options.shard_count is None) + int(options.shard_ids is None) != 1:
        raise Exception("Options --shard-count and --shard-ids are mutually exclusive")

    if options.shard_ids is not None:
        options.shard_ids = map(lambda x: int(x), options.shard_ids.split(','))


def main(options):
    if options.seed:
        random.seed(options.seed)

    intlookups_stat = map(lambda x: IntlookupStat(x, options.shard_count, options.shard_ids), options.intlookups)

    all_instances_stat = sum(map(lambda x: x.get_instances(), intlookups_stat), [])

    by_host_params = defaultdict(list)
    for instance_stat in all_instances_stat:
        by_host_params[instance_stat.instance.host].append(instance_stat.instance)
    hostnames = map(lambda x: x.name, by_host_params.iterkeys())
    params = map(lambda x: map(lambda y: (y.host.name, y.port), x), by_host_params.itervalues())

    sky_result = dict()

    client = api.cqueue.Client('cqudp', netlibus=True)
    for hostname, host_result, failure in client.run(hostnames, UsageExtractor(), params).wait(options.timeout):
        if failure is not None:
            if options.verbose > 1:
                print "Failure on %s" % hostname
                print "Error message <%s>" % formatException(failure)
        else:
            sky_result.update(dict(host_result))

    for intlookup_stat in intlookups_stat:
        intlookup_stat.update(sky_result)

    return intlookups_stat


def print_result(intlookups_stat, options):
    for intlookup_stat in intlookups_stat:
        print intlookup_stat.generate_report(options.report_type)


if __name__ == '__main__':
    options = parse_cmd()
    normalize(options)
    result = main(options)
    print_result(result, options)

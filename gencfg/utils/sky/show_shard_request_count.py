#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

try:
    import gencfg
    from core.db import CURDB
    from tools.analyzer.runner import Runner
    import tools.analyzer.functions
    from gaux.aux_colortext import red_text
except ImportError:
    pass


class InstanceOrHost(object):
    def __init__(self, s):
        self.name = s.split(':')[0]
        self.port = int(s.split(':')[1]) if s.find(':') > 0 else None

    def findme(self, instances):
        for instance in instances:
            if instance.host.name == self.name and (self.port is None or instance.port == self.port):
                return instance
        return None


class LocalHost(object):
    """Class, behaving like gencfg host (needed for sky)"""
    def __init__(self, host):
        self.name = host.name

    def __eq__(self, other):
        return self.name == other.name


class LocalInstance(object):
    """Class, behaving like gencfg instance (needed for sky)"""
    def __init__(self, instance):
        self.host = LocalHost(instance.host)
        self.port = instance.port
        self.power = instance.power
        self.type = instance.type
        self.N = instance.N

    def __hash__(self):
        return hash(self.host.name) ^ self.port

    def __eq__(self, other):
        return self.host == other.host and \
               self.port == other.port and \
               self.power == other.power and \
               self.type == other.type and \
               self.N == other.N

    def name(self):
        return '{}:{}'.format(self.host.name, self.port)


def parse_cmd():
    parser = ArgumentParser(description="Show requests count to instances of specific shard")
    parser.add_argument("-i", "--intlookups", action="store", default=None,
                        help="Obligatory. Intlookup to analyze")
    parser.add_argument("-s", "--shard-id", action="store", type=int,
                        help="Optional. Shard id to analyze")
    parser.add_argument("-n", "--instance", action="store", type=str,
                        help="Optional. Instance to analyze")
    parser.add_argument("-p", "--primus_id", action="store", type=str, default=None,
                        help="Optional. Primus id to analyze")
    parser.add_argument("-t", "--timeout", action="store", type=int, default=60,
                        help="Optional. Timeout to skynet (default is 60)")
    parser.add_argument("-q", "--quiet", action="store_true", default=False,
                        help="Optional. Show statistics for only specified instances")
    parser.add_argument("-v", "--verbose", action="count", dest="verbose_level", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity. The maximum is 1.")
    parser.add_argument("-f", "--ifilter", action="store", dest="ifilter", default="lambda x: True",
                        help="Optional. Instances filter")
    parser.add_argument("--instance-type", type=str, default="base",
                        choices=["int", "base"],
                        help="Optional. Show statistics for intsearchers or basesearchers")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if (options.shard_id is not None) + (options.instance is not None) + (options.primus_id is not None) != 1:
        raise Exception("You should specify exactly one of --shard-id --instance --primus-id option")

    if options.intlookups is None:
        options.intlookups = CURDB.intlookups.get_intlookups()
    else:
        options.intlookups = map(lambda x: CURDB.intlookups.get_intlookup(os.path.basename(x)),
                                 options.intlookups.split(','))

    if options.shard_id is not None and len(options.intlookups) > 1:
        raise Exception("You can not specify shard id with multiple intlookups")

    options.instances_groups = []
    options.hosts = []
    if options.shard_id is not None:
        instances = options.intlookups[0].get_base_instances_for_shard(options.shard_id)
        instances = [LocalInstance(x) for x in instances]
        options.instances_groups.append((None, options.intlookups[0], options.shard_id, instances))
    elif options.primus_id is not None:
        for intlookup in options.intlookups:
            for i in range(intlookup.brigade_groups_count * intlookup.hosts_per_group):
                if options.primus_id == intlookup.get_primus_for_shard(i):
                    options.instances_groups.append((None, intlookup, i, [LocalInstance(x) for x in intlookup.get_base_instances_for_shard(i)]))
    else:
        instanceorhost = InstanceOrHost(options.instance)
        options.hosts = [CURDB.hosts.get_host_by_name(instanceorhost.name)]
        for intlookup in options.intlookups:
            if intlookup.file_name in ['intlookup-msk-xpacman-base-nidx.py', 'intlookup-msk-xpacman-base.py']:
                continue
            for i in range(intlookup.brigade_groups_count * intlookup.hosts_per_group):
                if options.instance_type == "base":
                    instances = [LocalInstance(x) for x in intlookup.get_base_instances_for_shard(i)]
                else:
                    if i % intlookup.hosts_per_group != 0:
                        continue
                    instances = [LocalInstance(x) for x in intlookup.get_int_instances_for_shard(i)]

                r = instanceorhost.findme(instances)
                if r:
                    options.instances_groups.append((r, intlookup, i, instances))

    options.ifilter = eval(options.ifilter)
    options.instances_groups = filter(lambda (x, y, z, t): options.ifilter(t[0]), options.instances_groups)

    options.instances = sum(map(lambda (x, y, z, t): t, options.instances_groups), [])

    if len(options.instances_groups) == 0:
        raise Exception("Valid instances not found")

    return options


def analyze_instance(instances_group, skynet_failure_instances, skynet_result, quiet):
    my_instance, intlookup, shard_id, neighbour_instances = instances_group

    def _str_instance(instance):
        if my_instance is not None and instance == my_instance:
            return red_text("% 10s:%s:%s" % (instance.host.name, instance.port, instance.type))
        else:
            return "% 10s:%s:%s" % (instance.host.name, instance.port, instance.type)

    if neighbour_instances[0].type.find('SNIPPETS') > 0:
        signal_name = 'instance_snippets_qps'
    else:
        signal_name = 'instance_qps'

    sum_power = sum(map(lambda x: x.power, options.instances))

    good_neighbour_instances = filter(lambda x: x not in skynet_failure_instances, neighbour_instances)
    good_neighbour_instances = filter(lambda x: skynet_result[x][signal_name] is not None, good_neighbour_instances)
    bad_neighbour_instances = list(set(neighbour_instances) - set(good_neighbour_instances))

    sum_qps = sum(map(lambda x: skynet_result[x][signal_name], good_neighbour_instances)) + 0.000001

    print "    Intlookup %s, shard %s: %s qps %s power" % (intlookup.file_name, shard_id, sum_qps, sum_power)
    for instance in neighbour_instances:
        if quiet and instance != my_instance:
            continue

        if instance in bad_neighbour_instances:
            print '        %s SKYNET FAILED' % _str_instance(instance)
        else:
            rel_power = float(instance.power) / sum_power * 100
            rel_qps = skynet_result[instance][signal_name] / sum_qps * 100
            print '        %s power %.2f(%3.2f%%) qps %.2f(%3.2f%%) ratio %3.2f' % (
            _str_instance(instance), instance.power, rel_power,
            skynet_result[instance][signal_name], rel_qps, rel_qps / (rel_power + sys.float_info.epsilon))


def analyze_host(host, skynet_result, skynet_failure_instances):
    failed_instances = filter(lambda x: x.host == host, skynet_failure_instances) + \
                       filter(lambda x: x.host == host and skynet_result[x]['instance_cpu_usage'] is None,
                              skynet_result.iterkeys())
    good_instances = filter(lambda x: x.host == host and skynet_result[x]['instance_cpu_usage'] is not None,
                            skynet_result.iterkeys())

    if len(good_instances) > 0:
        print map(lambda x: skynet_result[x], good_instances)
    else:
        pass

    print "    Host %s: %s power, %d good instances, %d bad instances" % (
    host.name, host.power, len(failed_instances), len(good_instances))
    for instance in good_instances:
        print "        Instance %s %.2f power %3.2f usage %3.2f ratio" % (
        instance.name(), instance.power, skynet_result[instance]['instance_cpu_usage'],
        instance.power / skynet_result[instance]['instance_cpu_usage'] / 1000.)
    for instance in failed_instances:
        print "        Instance %s SKYNET FAILED" % instance.name()


if __name__ == '__main__':
    options = parse_cmd()

    run_quiet = (options.verbose_level == 0)
    runner = Runner(quiet=run_quiet)

    skynet_failure_instances, skynet_result = runner.run_on_instances(options.instances,
                                                                      [tools.analyzer.functions.instance_snippets_qps,
                                                                       tools.analyzer.functions.instance_qps,
                                                                       tools.analyzer.functions.instance_cpu_usage],
                                                                      options.timeout)
    if len(skynet_failure_instances):
        print "Failure on %s" % (' '.join(map(lambda x: "%s:%s" % (x.host.name, x.port), skynet_failure_instances)))

    for x in skynet_result:
        for signal_name in ['instance_snippets_qps', 'instance_qps', 'instance_cpu_usage']:
            if skynet_result[x].get(signal_name, 'NULL') == 'NULL':
                skynet_result[x][signal_name] = 0

    print "Grouped by intlookup results:"
    for instances_group in options.instances_groups:
        analyze_instance(instances_group, skynet_failure_instances, skynet_result, options.quiet)

    print "Grouped by host results:"
    for host in options.hosts:
        analyze_host(host, skynet_result, skynet_failure_instances)

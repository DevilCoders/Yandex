#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict, OrderedDict
import json
import copy

import gencfg
from core.db import CURDB
from core.hosts import FakeHost
from core.instances import FakeInstance, Instance
from gaux.aux_utils import freq
import core.argparse.types as argparse_types
from core.argparse.parser import ArgumentParserExt

indent = 4 * ' '


class EReportTypes(object):
    TEXT = 'text'
    JSON = 'json'
    ALL = [TEXT, JSON]


class DummyGroup(object):
    class DummyCard(object):
        def __init__(self, name):
            self.name = name

    class DummyFuncs(object):
        def __init__(self):
            self.instanceCount = lambda x, y: 1

    def __init__(self, instances):
        self.card = DummyGroup.DummyCard("DUMMY")
        self.funcs = DummyGroup.DummyFuncs()
        self.instances = instances

    @classmethod
    def create_from_hosts(cls, hosts):
        return cls([Instance(x, x.power, 12345, "DUMMY", 0) for x in hosts])

    @classmethod
    def create_from_groups(cls, groups):
        return cls(sum([x.get_kinda_busy_instances() for x in groups], []))

    def getHosts(self):
        return list({x.host for x in self.instances})

    def get_instances(self):
        return self.instances

    def get_kinda_busy_instances(self):
        return self.get_instances()


def instances_stats_as_json(instances, hosts, is_detailed):
    """Return instances stats as json object"""
    ssd = [host.ssd for host in hosts]

    memory = 0
    memory_by_host = []
    for instance in instances:
        if instance.type in ('DUMMY', 'FAKE'):
            memory += instance.host.memory
            memory_by_host += [ instance.host.memory ]
        else:
            vl = CURDB.groups.get_group(instance.type).card.reqs.instances.memory_guarantee.gigabytes()
            memory += vl
            memory_by_host += [ vl ]


    hosts_power = defaultdict(float)
    for instance in instances:
        hosts_power[instance.host] += instance.power
    power = [int(x + .5) for x in hosts_power.values()]

    big_disk = [x.disk > 900 for x in hosts]
    disk = [x.disk / 100 * 100 for x in hosts]
    by_switch_count = defaultdict(int)
    for x in hosts:
        by_switch_count[x.switch] += 1

    jsoned = OrderedDict(
        hosts_count=len(hosts),
        instances_count=len(instances),
        instances_cpu=sum(power),
        instances_cpu_avg=int(sum(power) / len(power)) if power else 0,
        instances_memory=memory,
        instances_memory_avg=memory/ len(instances) if instances else 0,
    )

    if is_detailed and hosts:
        jsoned_detailed = dict(
            cpu=power,
            memory=memory_by_host,
            hdd=disk,
            ssd=ssd,
            hosts_per_swith=by_switch_count.values()
        )

        jsoned['detailed'] = jsoned_detailed

    return jsoned


def group_stats_as_json(group, options):
    hosts = set(group.getHosts())

    if options.host_filter:
        hosts = set(filter(lambda x: options.host_filter(x), hosts))

    if options.hosts_power:
        instances = [FakeInstance(FakeHost(x.name, x), 0, x.power) for x in hosts]
        for instance in instances:
            instance.host.memory = CURDB.hosts.get_host_by_name(instance.host.name).memory
    else:
        instances = filter(lambda x: x.host in hosts, group.get_kinda_busy_instances())

    result = dict()

    result['total'] = instances_stats_as_json(instances, hosts, options.verbose)

    if options.location_mode:
        result['by_location'] = dict()
        locations = sorted(list(set(map(lambda x: x.host.location, instances))))
        for location in locations:
            if len(filter(lambda x: x.host.location == location, instances)) > 0:
                result['by_location'][location] = instances_stats_as_json(filter(lambda x: x.host.location == location, instances),
                                                                          filter(lambda x: x.location == location, hosts), options.verbose)

    if options.dc_mode:
        result['by_dc'] = dict()
        dcs = sorted(list(set(map(lambda x: x.host.dc, instances))))
        for dc in dcs:
            if len(filter(lambda x: x.host.dc == dc, instances)) > 0:
                result['by_dc'][dc] = instances_stats_as_json(filter(lambda x: x.host.dc == dc, instances),
                                                    filter(lambda x: x.dc == dc, hosts), options.verbose)

    if options.queue_mode:
        result['by_queue'] = dict()
        prefixes = set(map(lambda x: x.queue, group.getHosts()))
        for prefix in sorted(list(prefixes)):
            reusult['by_queue'][prefix] = instances_stats_as_json(filter(lambda x: x.host.queue == prefix, instances),
                                                                         filter(lambda x: x.queue == prefix, hosts),
                                                                         options.verbose)
    if options.cluster_mode:
        result['by_cluster'] = dict()
        prefixes = set(map(lambda x: x.name.split('-')[0], group.getHosts()))
        for prefix in sorted(list(prefixes)):
            result['by_cluster'][prefix] = instances_stats_as_json(filter(lambda x: x.host.name.startswith('%s-' % prefix) or x.host.name == prefix, instances),
                                                                   filter(lambda x: x.name.startswith('%s-' % prefix) or x.name == prefix, hosts),
                                                                   options.verbose)
    if options.switch_mode:
        result['by_switch'] = dict()
        prefixes = set(map(lambda x: x.switch, group.getHosts()))
        for prefix in sorted(list(prefixes)):
            result['by_switch'][prefix] = instances_stats_as_json(filter(lambda x: x.host.switch == prefix, instances),
                                                                  filter(lambda x: x.switch == prefix, hosts),
                                                                  options.verbose)

    if options.cpumodel_mode:
        result['by_cpumodel'] = dict()
        models = set(map(lambda x: x.model, group.getHosts()))
        for model in sorted(list(models)):
            result['by_cpumodel'][model] = instances_stats_as_json(filter(lambda x: x.host.model == model, instances),
                                                                   filter(lambda x: x.model == model, hosts),
                                                                   options.verbose)

    return result


def get_parser():
    parser = ArgumentParserExt(description="Show power of group or list of hosts")
    parser.add_argument("--db", type=argparse_types.gencfg_db, default=CURDB,
                        help="Optional. Path to db")
    parser.add_argument('-t', '--report-type', type=str, default=EReportTypes.TEXT,
                        choices=EReportTypes.ALL,
                        help='Optional. Report type: one of <{}>'.format(','.join(EReportTypes.ALL)))
    parser.add_argument("-g", "--groups", type=argparse_types.xgroups, default=None,
                        help="Optional. Comma-separated list of groups to process")
    parser.add_argument("-o", "--hosts", type=argparse_types.hosts, default=None,
                        help="Optional. Comma-separated list of hosts to process")
    parser.add_argument("-l", "--location-mode", action="store_true", default=False,
                        help="Optional show by-location statistics")
    parser.add_argument("-d", "--dc-mode", action="store_true", dest="dc_mode", default=False,
                        help="Optional. Show by-dc statistics")
    parser.add_argument("-q", "--queue-mode", action="store_true", dest="queue_mode", default=False,
                        help="Optional. Show by-queue statistics")
    parser.add_argument("-c", "--cluster-mode", action="store_true", dest="cluster_mode", default=False,
                        help="Optional. Show by-cluster statistics")
    parser.add_argument("-s", "--switch-mode", action="store_true", dest="switch_mode", default=False,
                        help="Optional. Show by-switch statistics")
    parser.add_argument("-m", "--cpu-model-type", action="store_true", dest="cpumodel_mode", default=False,
                        help="Optional. Show by-cpumodel statistics")
    parser.add_argument("-f", "--host-filter", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Show only hosts by filter")
    parser.add_argument("-u", "--group-filter", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="optional. Show only groups by filter")
    parser.add_argument("-v", "--detailed", action="store_true", dest="verbose", default=False,
                        help="Optional. Show detailed cpu, memory, disk info")
    parser.add_argument("-j", "--join-groups", action="store_true", default=False,
                        help="Optional. Show joined statistics (with broken number of isntances)")
    parser.add_argument("--hosts-power", action="store_true", default=False,
                        help="Optional. Show hosts statistics instead of instances")
    parser.add_argument("--with-slaves", action="store_true", default=False,
                        help="Optional. Show info on slave groups as well")

    return parser


def normalize(options):
    if not (options.groups is None) ^ (options.hosts is None):
        raise Exception("You must exactly one of --groups or --hosts")

    if options.groups is None:
        options.groups = [DummyGroup.create_from_hosts(options.hosts)]

    options.groups = filter(options.group_filter, options.groups)
    if options.with_slaves:
        options.groups.extend(sum(map(lambda x: x.card.slaves, options.groups), []))

    if options.join_groups:
        options.groups = [DummyGroup.create_from_groups(options.groups)]


def main(options):
    jsoned = OrderedDict()
    for group in options.groups:
        jsoned[group.card.name] = group_stats_as_json(group, options)

    if options.report_type == EReportTypes.JSON:
        print json.dumps(jsoned, indent=5)
    elif options.report_type == EReportTypes.TEXT:
        for groupname, group_data in jsoned.iteritems():
            # print total
            d = copy.copy(group_data['total'])
            d['group'] = groupname
            print ('    Group {group}: {hosts_count} hosts, {instances_count} instances, {instances_cpu} power ({instances_cpu_avg:.2f} avg), '
                   '{instances_memory:.2f} Gb memory ({instances_memory_avg:.2f} Gb avg)').format(**d)

            # print group by
            group_by = [
                ('by_location', 'Location'),
                ('by_dc', 'Dc'),
                ('by_queue', 'Queue'),
                ('by_switch', 'Switch'),
                ('by_cpumodel', 'Cpu Model'),
            ]
            for group_key, descr in group_by:
                if group_key in group_data:
                    print '        Stats by {}:'.format(descr)
                    for prefix in group_data[group_key]:
                        d = copy.copy(group_data[group_key][prefix])
                        d['descr'] = descr
                        d['prefix'] = prefix
                        print ('            {descr} {prefix}: {hosts_count} hosts, {instances_count} instances, {instances_cpu} power ({instances_cpu_avg:.2f} avg), '
                               '{instances_memory:.2f} Gb memory ({instances_memory_avg:.2f} Gb avg)').format(**d)
    else:
        raise Exception('Unknown report type <{}>'.format(options.report_type))


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)

import os
import sys

sys.path.append(os.path.abspath(os.path.dirname(__file__)))

import copy
import math
import string

from collections import defaultdict

from core.db import CURDB
from core.intlookups import Intlookup
from core.instances import TMultishardGroup
from config_params import get_config_params
import generate_groups
from create_groups import create_groups


# TODO: divide IntlookupConfigurator to Intlookup and IntlookupConfigurator
class IntlookupConfigurator(object):
    def __init__(self, tree):
        # initialize empty params
        self.brigade_groups = []
        self.replaced_instances = []
        self.tiers = None
        self.temporary = False

        # fill common params
        if tree.find('file_name') is not None:
            self.file_name = tree.find('file_name').text.strip()
        if tree.find('hosts_per_group') is not None: self.hosts_per_group = int(tree.find('hosts_per_group').text)
        if tree.find('weight') is not None: self.weight = float(tree.find('weight').text)
        if tree.find('ints_per_group') is not None: self.ints_per_group = int(tree.find('ints_per_group').text)
        if tree.find('brigade_groups_count') is not None:
            self.brigade_groups_count, self.tiers = CURDB.tiers.primus_int_count(
                tree.find('brigade_groups_count').text.strip())
            if self.brigade_groups_count / self.hosts_per_group * self.hosts_per_group != self.brigade_groups_count:
                raise Exception("Incompatible brigade_groups_count %s and hosts_per_group %s" % (self.brigade_groups_count, self.hosts_per_group))
            self.brigade_groups_count /= self.hosts_per_group
        if tree.find('tiers') is not None and eval(tree.find('tiers').text.strip()) is not None:
            self.tiers = eval(tree.find('tiers').text.strip())
            total_shards = sum(CURDB.tiers.primus_int_count(x)[0] for x in self.tiers)
            self.brigade_groups_count = total_shards / self.hosts_per_group
            if self.brigade_groups_count * self.hosts_per_group != total_shards:
                raise Exception("Actual shards number %d is not multiple of hosts_per group %d" % (total_shards, self.hosts_per_group))

        # fill accepted host types
        if tree.find('base_type') is not None: self.base_type = tree.find('base_type').text.strip()
        if tree.find('meta_type') is not None: self.meta_type = tree.find('meta_type').text.strip()

        # find if we generate new ports
        if tree.find('newstyle_ports') is not None:
            self.newstyle_ports = bool(int(tree.find('newstyle_ports').text.strip()))
        else:
            self.newstyle_ports = False

        if tree.find('generate'):
            # fill generate method
            self._init_generate_method(tree.find('generate'))

        # load multiblocks info
        if tree.find('multi_blocks') is not None:
            self.multi_blocks = int(tree.find('multi_blocks').text.strip())
        else:
            self.multi_blocks = 1

        if tree.find('temporary') is not None:
            self.temporary = int(tree.find('temporary').text.strip())
        else:
            self.temporary = False

        # extra params (override global extra params)
        self.extra_params = dict()
        for elem in tree.findall('params/*'):
            self.extra_params[elem.tag] = string.strip(elem.text)

    def _init_generate_method(self, tree):
        self.generate_method = tree.find('method').text.strip()
        if self.generate_method == 'optimizer':
            # total power required by specific intlookup
            self.needed_power = self.weight * self.brigade_groups_count * self.hosts_per_group
            # this thing defines how much ints required for config
            # WTF: why to divide by hosts_per_group?
            # self.needed_ints = self.brigade_groups_count / float(self.hosts_per_group) * self.weight * self.ints_per_group
            self.functions = [x.text.strip() for x in tree.findall('optimizer/solver_functions/function')]
        elif self.generate_method == 'preserve':
            self.intlookup = Intlookup(CURDB, os.path.join(CURDB.INTLOOKUP_DIR, tree.find('file').text.strip()))
        else:
            raise Exception("Unknown generate method %s" % self.generate_method)

    def create_intlookup_from_optimizer(self, brigade_groups):
        intlookup = Intlookup(CURDB)
        intlookup.file_name = self.file_name
        intlookup.hosts_per_group = self.hosts_per_group
        intlookup.ints_per_group = self.ints_per_group
        intlookup.brigade_groups_count = self.brigade_groups_count
        intlookup.base_type = self.base_type
        intlookup.tiers = self.tiers
        intlookup.temporary = False
        intlookup.brigade_groups = []

        for row in brigade_groups:
            brigade_group = TMultishardGroup()
            brigade_group.brigades = row
            intlookup.brigade_groups.append(brigade_group)

        self.intlookup = intlookup


intlookup_configurators = []
extra_params = {}
postcommands = []
all_config_params = {}


def init_intlookups(tree):
    global intlookup_configurators
    global all_config_params
    global extra_params

    subtree = tree.find('intlookups')

    # initialize intlookups
    if subtree.find('adjust_power_file') is not None:
        all_config_params['adjust_power_file'] = subtree.find('adjust_power_file').text.strip()

    for elem in subtree.findall('intlookup'):
        intlookup_configurators.append(IntlookupConfigurator(elem))

    for elem in tree.findall('intlookups/params/*'):
        extra_params[elem.tag] = string.strip(elem.text)

    for elem in tree.findall('intlookups/postcommands/command'):
        postcommands.append((filter(lambda x: x != '', string.strip(elem.text).split(' ')), copy.copy(elem.attrib)))


def _generate_intlookups_for_base_type_and_group_size(intlookups, base_instances, params):
    print 'Start generation for intlookups %s with type %s and group size %d, base instances %d' % \
          (','.join(map(lambda x: x.file_name, intlookups)), intlookups[0].base_type, intlookups[0].hosts_per_group,
           len(base_instances))

    # find preserved hosts
    preserved_hosts = set()
    base_group = CURDB.groups.get_group(intlookups[0].base_type)
    for slave_group in base_group.getSlaves():
        if slave_group.hasHostDonor():
            continue
        for host in slave_group.getHosts():
            preserved_hosts.add(host)

    # create groups
    min_groups_count = sum(map(lambda x: x.brigade_groups_count, intlookups))
    groups, free_basesearchers, free_ints = \
        create_groups(base_instances, intlookups[0].hosts_per_group, intlookups[0].ints_per_group,
                      intlookups[0].base_type, min_groups_count, params, preserved_hosts)

    # create shard sizes
    shard_sizes = map(lambda x: (x.file_name, x.multi_blocks, [x.weight] * x.brigade_groups_count), intlookups)
    solvers = dict(map(lambda x: (x.file_name, x.functions), intlookups))

    # generate optimal config
    brigade_groups = generate_groups.generate_optimal_grouping(get_config_params()['solver_name'], params, groups,
                                                               shard_sizes, solvers)

    cur = 0
    # distribute groups
    for intlookup in intlookups:
        intlookup.create_intlookup_from_optimizer(brigade_groups[cur:cur + intlookup.brigade_groups_count])
        cur += intlookup.brigade_groups_count


def _divide_hosts_and_run_generator(intlookups, all_instances, generator, params):
    # get base and int instances
    print 'Start generation for intlookups %s with basetype %s' % (
    ','.join(map(lambda x: x.file_name, intlookups)), intlookups[0].base_type)
    base_instances = filter(lambda x: x.type == intlookups[0].base_type, all_instances)

    # split all basesearch instances proportionally to group power
    power_by_group_size = defaultdict(float)
    for intlookup in intlookups:
        power_by_group_size[intlookup.hosts_per_group] += intlookup.needed_power

    total_needed_power = sum((intlookup.needed_power for intlookup in intlookups), .0)

    # We'd prefer to unite some types (base_type) to have single intlookup config file,
    # single base_type with different weights for each intlookup.
    # But as far as implementation of algorithm for multiple different intlookup's hosts_per_group values
    # does not work well, we intentionally divide base_type to multiple types with the same hosts_per_group value.
    # In fact, now every type (and intlookup file) has the same hosts_per_group value for all intlookups.

    # from each cluster we get a proportional number of machines for each group
    # (Actually, 'cluster' is now deprecated, we use 'queue' to group hosts)
    # => as far as now there is 1 group all machines go to that group
    base_by_group_size = defaultdict(list)
    for queue in set(x.host.queue for x in base_instances):
        queue_base_instances = filter(lambda x: x.host.queue == queue, base_instances)
        queue_power = sum(x.power for x in queue_base_instances)
        for size, power in sorted(power_by_group_size.items(), cmp=lambda (x1, y1), (x2, y2): cmp(y1, y2)):
            cur_group_power = 0.
            while cur_group_power < queue_power * power / total_needed_power and len(queue_base_instances):
                cur_group_power += queue_base_instances[-1].power
                base_by_group_size[size].append(queue_base_instances.pop())

    # now we can generate configs for all group sizes
    for group_size in power_by_group_size.keys():
        group_intlookups = filter(lambda x: x.hosts_per_group == group_size, intlookups)
        generator(group_intlookups, base_by_group_size[group_size], params)


def _is_good_brigade(brigade, good_hosts):
    hosts = set(map(lambda x: x.name, brigade.intsearchers) + map(lambda t: t.name,
                                                                  reduce(lambda x, y: x + y, brigade.basesearchers,
                                                                         [])))

    print len(hosts), len(filter(lambda x: x in good_hosts, hosts))
    return len(hosts) == len(filter(lambda x: x in good_hosts, hosts))


def generate_intlookups():
    global extra_params

    print 'Start intlookups generation'

    all_instances = CURDB.groups.get_all_instances()

    hosts_instances = defaultdict(list)
    for instance in all_instances:
        hosts_instances[instance.host.name].append(instance)

    if os.getenv('DEBUG_HOST') is not None:
        iname = os.getenv('DEBUG_HOST')
        print "All instances for %s" % iname
        for instance in all_instances:
            if instance.host.name == iname:
                print "    %s:%s:%s:%s" % (instance.host.name, instance.port, instance.power, instance.type)

    # get only optimizer intlookups
    optimizer_intlookups = filter(lambda x: x.generate_method == 'optimizer', intlookup_configurators)
    # group optimizer intlookups by base type
    print 'Run optimizer intlookups: %s' % ', '.join(map(lambda x: x.file_name, optimizer_intlookups))
    for base_type in set(map(lambda x: x.base_type, optimizer_intlookups)):
        intlookups = filter(lambda x: x.base_type == base_type, optimizer_intlookups)
        params = copy.deepcopy(extra_params)
        for intlookup in intlookups:
            params.update(intlookup.extra_params)
        _divide_hosts_and_run_generator(intlookups, all_instances, _generate_intlookups_for_base_type_and_group_size,
                                        params)

    # write generated configs
    print 'Write generated configs'
    for intlookup in optimizer_intlookups:
        CURDB.intlookups.add_build_intlookup(intlookup.intlookup)

    print 'Finish intlookups generation'


def build_intlookup_from_config(filename):
    from xml.etree import cElementTree
    from templates import process_templates
    from config_params import init_config_params

    intlookup_configurators[:] = []

    tree = cElementTree.parse(open(filename))
    tree = process_templates(tree)
    init_config_params(tree)
    init_intlookups(tree)
    generate_intlookups()

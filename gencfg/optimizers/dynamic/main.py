#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import gencfg

from collections import namedtuple
from random import Random
import copy
import collections
from StringIO import StringIO
from collections import defaultdict
import time

from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
from core.card.node import Scheme
from core.igroups import CIPEntry
import utils.common.update_card as update_card
import utils.common.find_most_unused_port as find_most_unused_port
from gaux.aux_shared import create_host_filter
from gaux.aux_performance import perf_timer
import core.argparse.types as argparse_types
from gaux.aux_colortext import red_text
from gaux.aux_utils import correct_pfname
from core.exceptions import UtilNormalizeException

from aux_dynamic import calculate_free_resources, calc_instance_memory

ELEM_WIDTH = 10
ACTIONS = ["add", "remove", "remove_all", "show", "get_locations", "get_stats", "check"]
HOST_RESERVED_MEMORY = 2  # memory reserved in gigabytes


def filter_hosts_with_affinity_category(hosts, options, group):
    """Filter out hosts with bad affinity category (GENCFG-2391)"""
    if group.card.reqs.instances.affinity_category is None:
        return copy.copy(hosts)

    result_hosts = set(hosts)
    for slave_group in group.card.master.card.slaves:
        if (slave_group.card.reqs.instances.affinity_category is not None) and \
           (slave_group.card.reqs.instances.affinity_category != group.card.reqs.instances.affinity_category):
            result_hosts -= set(slave_group.getHosts())

    return result_hosts


def __filter_hosts_from_location(hosts, location):
    location = location.lower()
    if location == 'msk_iva':
        flt = lambda x: x.dc == 'iva'
    elif location == 'msk_myt':
        flt = lambda x: x.dc == 'myt'
    else:
        flt = lambda x: x.location == location

    filtered_hosts = filter(flt, hosts)

    return filtered_hosts


def filter_hosts(hosts, options, group):
    """Filter suitable hosts for allocation (RX-564)"""

    hosts = filter_hosts_with_affinity_category(hosts, options, group)

    if options.location is not None:
        # location is specified
        return __filter_hosts_from_location(hosts, options.location)
    elif group.card.reqs.hosts.location.location:
        result = set()
        for location in group.card.reqs.hosts.location.location:
            result |= set(__filter_hosts_from_location(hosts, location))
            return list(result)
    else:
        prefixes = ['MSK_IVA', 'MSK_MYT', 'SAS,', 'MAN', 'VLA']
        for prefix in prefixes:
            if group.card.name.startswith(prefix):
                return __filter_hosts_from_location(hosts, prefix)
        else:
            return copy.copy(hosts)


class TAllocationReport(object):
    """
        Sometimes we do not want add hosts to group immediately. This structure contain allocated resources and methos to apply/rollback changes
    """

    __slots__ = ['allocated_resources']

    def __init__(self, allocated_resources):
        """
            Load all allocated instances into internal structures

            :param allocated_resources(list of HostRoom): list of allocated instsnces
        """
        self.allocated_resources = allocated_resources

    def commit(self, group):
        """
            Apply changes to specified group.

            :type group: core.igroups.IGroup

            :param group: group to add hosts to
            :return None:
        """
        intlookups = [group.parent.db.intlookups.get_intlookup(intlookup) for intlookup in group.card.intlookups]
        old_instances = group.get_instances()

        if group.card.legacy.funcs.instancePort.startswith('old'):
            group.card.legacy.funcs.instancePort = "old%s" % group.card.reqs.instances.port
        else:
            group.card.legacy.funcs.instancePort = "new%s" % group.card.reqs.instances.port
        group.refresh_after_card_update()

        for old_instance in old_instances:
            new_instance = group.parent.get_instance_by_N(old_instance.host.name, old_instance.type, old_instance.N)
            old_instance.port = new_instance.port

        allocated_hosts = map(lambda x: x.host, self.allocated_resources)
        group.parent.db.groups.add_slave_hosts(allocated_hosts, group)

        for elem in self.allocated_resources:
            power_per_instance = float(elem.power // len(group.get_host_instances(elem.host)))
            for instance in group.get_host_instances(elem.host):
                instance.power = power_per_instance
                group.custom_instance_power[CIPEntry(elem.host, instance.port)] = power_per_instance

    def rollback(self, group):
        """
            Rollback changes that have previously been applied

            :type group: core.igroups.IGroup

            :param group: group to remove hosts from
            :return None:
        """

        allocated_hosts = map(lambda x: x.host, self.allocated_resources)
        group.parent.db.groups.remove_slave_hosts(allocated_hosts, group)

    def __str__(self):
        result = '\n'.join(('    {}'.format(x) for x in self.allocated_resources))
        result = 'Report:\n{}'.format(result)
        return result


class NoResourcesError(Exception): pass


class InvalidInputParams(Exception): pass


class CardSetter(object):
    FIELDS = {
        "itype": ["tags", "itype"],
        "ctype": ["tags", "ctype"],
        "prj": ["tags", "prj"],
        "metaprj": ["tags", "metaprj"],
        "itag": ["tags", "itag"],
        "min_power": ["reqs", "shards", "min_power"],
        "min_replicas": ["reqs", "shards", "min_replicas"],
        "max_replicas": ["reqs", "shards", "max_replicas"],
        "equal_instances_power": ["reqs", "shards", "equal_instances_power"],
        "memory": ["reqs", "instances", "memory_guarantee"],
        "disk": ["reqs", "instances", "disk"],
        "ssd": ["reqs", "instances", "ssd"],
        "net": ["reqs", "instances", "net_guarantee"],
        "affinity_category": ["reqs", "instances", "affinity_category"],
        #        "exclusive": ["reqs", "instances", "exclusive"],
        #        "high_io_load": ["reqs", "instances", "high_io_load"],
        #        "high_net_load": ["reqs", "instances", "high_net_load"],
    }

    STR_TO_TYPE = {
        "string": str,
        "int": int,
        "nonnegative int": int,
        "positive int": int,
        "byte size": str,
        "bool": bool,
        "list of string": str,
    }

    def get_scheme_node_by_path(self, main_node, path):
        node = main_node
        for item in path:
            node = node[item]
        return node

    def add_card_fields_to_parser(self, parser):
        group_scheme = Scheme(os.path.join(CURDB.SCHEMES_DIR, 'group.yaml'), CURDB.version)
        main_node = group_scheme.get_cached()

        for plain_field, card_field in CardSetter.FIELDS.items():
            node = self.get_scheme_node_by_path(main_node, card_field)
            field_description = node.description if node.description else node.display_name

            # temporary commented
            # type_info = Scheme.leaf_type_to_simple_form(node)
            # assert type_info["type"] in CardSetter.STR_TO_TYPE, "Failed to find a handle for '%s' type" % type_info["type"]

            # field_type = CardSetter.STR_TO_TYPE[ type_info["type"] ]
            parser.add_argument("--%s" % plain_field, dest=plain_field, type=str, required=False, default=None,
                                help="Optional. %s" % field_description)

    def update_group_fields(self, group, options):
        for plain_field, card_field in CardSetter.FIELDS.items():
            value = getattr(options, plain_field)
            if value is None:
                continue
            update_card.jsmain(dict(groups=[group.card.name], key='.'.join(card_field), value=value))

        if options.location:
            group.card.reqs.hosts.location.location = [options.location.lower()]

        # updates[tuple(card_field)] = value
        # has_updates, isOk, invalid_values = CardUpdater().update_group_card(group, updates)
        # if not isOk:
        #    raise Exception("Failed to set group settings, invalid values: %s" % invalid_values)


DRAW_PARAMS = ['power', 'memory', 'disk', 'ssd', 'net', 'exclusive']


class HostRoom(object):
    __slots__ = ['host', 'power', 'memory', 'disk', 'ssd', 'net', 'exclusive', 'weight']

    def __init__(self, **kwargs):
        members = ('host', 'power', 'memory', 'disk', 'ssd', 'net', 'exclusive',)
        for k in members:
            setattr(self, k, None)
        for k, v in kwargs.items():
            setattr(self, k, v)

    @staticmethod
    def from_instance(db, instance):
        group = db.groups.get_group(instance.type)
        room = HostRoom.from_reqs(group.card.reqs, instance.host)
        room.power = int(instance.power)
        room.memory = calc_instance_memory(db, instance) / 1024. / 1024 / 1024
        room.exclusive = 1
        return room

    @staticmethod
    def from_reqs(reqs, host):
        if reqs.shards.equal_instances_power and reqs.shards.max_replicas is not None:
            needed_power = reqs.shards.min_power / reqs.shards.max_replicas
        else:
            needed_power = 1

        return HostRoom(
            host=host,
            power=needed_power,  # to be filled later, but 1 is minimum
            memory=reqs.instances.memory_guarantee.gigabytes(),
            disk=reqs.instances.disk.gigabytes(),
            ssd=reqs.instances.ssd.gigabytes(),
            net=reqs.instances.net_guarantee.megabytes(),
            exclusive=1,
        )

    @staticmethod
    def from_host(host):
        avail_memory = host.get_avail_memory() / 1024. / 1024 / 1024
        return HostRoom(
            host=host,
            power=int(host.power),
            memory=avail_memory,
            disk=host.disk,
            ssd=host.ssd,
            net=host.net / 8.,
            exclusive=100,
        )

    @staticmethod
    def zero_room():
        return HostRoom(
            host=None,
            power=0,
            memory=0,
            disk=0,
            ssd=0,
            net=0,
            exclusive=0,
        )

    @staticmethod
    def free_room(host, db):
        free_room = HostRoom.from_host(host)

        for instance in db.groups.get_host_instances(host):
            group = db.groups.get_group(instance.type)
            if group.card.properties.fake_group:
                continue
            instance_room = HostRoom.from_instance(db, instance)
            free_room.subtract(instance_room)

        return free_room

    def subtract(self, other):
        assert (not self.host or not other.host or self.host == other.host)
        self.power -= other.power
        self.memory -= other.memory
        self.disk -= other.disk
        self.ssd -= other.ssd
        self.net -= other.net
        self.exclusive -= other.exclusive

    def add(self, other):
        assert (not self.host or not other.host or self.host == other.host)
        self.power += other.power
        self.memory += other.memory
        self.disk += other.disk
        self.ssd += other.ssd
        self.net += other.net
        self.exclusive += other.exclusive

    def can_subtract(self, other):
        assert (not self.host or not other.host or self.host == other.host)

        data = {
            'power': True,
            'memory': True,
            'hdd': True,
            'ssd': True,
            'net': True,
            'exclusive': True
        }
        if self.power - other.power < 0:
            data['power'] = False
        if self.memory - other.memory < 0:
            data['memory'] = False
        if self.disk - other.disk < 0:
            data['hdd'] = False
        if self.ssd - other.ssd < 0:
            data['ssd'] = False
        if self.net - other.net < 0:
            data['net'] = False
        if self.exclusive - other.exclusive < 0:
            data['exclusive'] = False

        return all(data.values()), data

    def max_put_count(self, other):
        """
            Check how many of 'other' room can be put in 'self' without exhausting of resources

            :type other: HostRoom

            :param other: room, representing some specific instance
            :return (int): number of 'other' rooms, which can be safely put in our room
        """

        N = 100
        if other.power > 0:
            N = min(N, self.power / other.power)
        if other.memory > 0:
            N = min(N, int(self.memory / other.memory))
        if other.disk > 0:
            N = min(N, int(self.disk / other.disk))
        if other.ssd > 0:
            N = min(N, int(self.ssd / other.ssd))
        if other.net > 0:
            N = min(N, int(self.net / other.net))
        if other.exclusive > 0:
            N = min(N, int(self.exclusive / other.exclusive))

        return max(N, 0)

    def assign_from(self, other):
        self.host = other.host
        self.power = other.power
        self.memory = other.memory
        self.disk = other.disk
        self.ssd = other.ssd
        self.net = other.net
        self.exclusive = other.exclusive

    def __str__(self):
        if self.host is None:
            prefix = "None "
        else:
            prefix = "Host %s " % self.host.name

        return "%spower=%s memory=%s disk=%s ssd=%s net=%s exclusive=%s" % (
        prefix, self.power, self.memory, self.disk, self.ssd, self.net, self.exclusive)


class CostCalculator(object):
    MAX_PUT = 100

    def __init__(self, master_hosts, db):
        many_power = int(db.cpumodels.get_model('E5-2660').power / 2)
        std_power = 100
        many_memory = 64
        std_memory = 12
        many_disk = 1024
        std_disk = 64
        big_power_instance = dict(power=many_power, memory=std_memory, disk=std_disk, ssd=0, net=0, exclusive=1, weight=1.0)
        big_memory_instance = dict(power=std_power, memory=many_memory, disk=std_disk, ssd=0, net=0, exclusive=1, weight=1.0)
        big_disk_instance = dict(power=std_power, memory=std_memory, disk=many_disk, ssd=0, net=0, exclusive=1, weight=1.0)
        idle_instance = dict(power=1, memory=std_memory, disk=std_disk, ssd=0, net=0, exclusive=1, weight=1.0)
        ninstances_reducer = dict(power=1, memory=0, disk=0, ssd=0, net=0, exclusive=1, weight=1.0)
        exclusive_instance = dict(power=std_power, memory=std_memory, disk=std_disk, ssd=0, net=0, exclusive=100, weight=1.0)
        self.std_types = []
        for src in [big_power_instance, big_memory_instance, big_disk_instance, idle_instance, ninstances_reducer,
                    exclusive_instance]:
            std_type = HostRoom()
            std_type.host = None
            std_type.power = src['power']
            std_type.memory = src['memory']
            std_type.disk = src['disk']
            std_type.ssd = src['ssd']
            std_type.net = src['net']
            std_type.exclusive = src['exclusive']
            std_type.weight = src['weight']
            self.std_types.append(std_type)

        self.weight_coeffs = [0.0, 1.0]
        for i in range(CostCalculator.MAX_PUT - 1):
            self.weight_coeffs.append(self.weight_coeffs[-1] * 0.8)
        for i in range(CostCalculator.MAX_PUT):
            self.weight_coeffs[i + 1] += self.weight_coeffs[i]

        self.free_rooms = [HostRoom.from_host(host) for host in master_hosts]
        for std_type in self.std_types:
            count_before = self.count(self.free_rooms, std_type)
            if not count_before:
                std_type.weight *= -1.0 / 1.0
            else:
                std_type.weight *= -1.0 / count_before

        self.rooms_before = [HostRoom.free_room(host, db) for host in master_hosts]

        self.room_by_host = {room.host:room for room in self.rooms_before}

        self.base_cost = 0.0
        for std_type in self.std_types:
            std_type_count = self.count(self.rooms_before, std_type)
            sum_weight = std_type_count * std_type.weight
            self.base_cost += sum_weight

        # if verbose:
        #    print 'Initial rooms:'
        #    print '\n'.join('\t%.4f: %s pow, %s mem, %s drive, %s' % (x.weight, x.power, x.memory, x.disk, 'exclusive' if x.exclusive > 1 else '') for x in self.std_types)

    def count(self, rooms, std_type):
        result = 0
        cur_room = HostRoom.zero_room()
        for room in rooms:
            cur_room.assign_from(room)
            put_count = cur_room.max_put_count(std_type)
            result += self.weight_coeffs[put_count]
        return result

    def calc_solution_cost(self, solution):
        new_cost = self.base_cost
        for room in solution:
            room_before = self.room_by_host[room.host]
            room_after = copy.copy(room_before)
            room_after.subtract(room)

            for std_type in self.std_types:
                std_type_count = self.count([room_before], std_type)
                sum_weight = std_type_count * std_type.weight
                new_cost -= sum_weight
            for std_type in self.std_types:
                std_type_count = self.count([room_after], std_type)
                sum_weight = std_type_count * std_type.weight
                new_cost += sum_weight

        return new_cost


class Engine(object):
    def __init__(self, db, hosts):
        self.db = db
        self.hosts = hosts
        self.cost_calculator = CostCalculator(self.hosts, self.db)

    def draw_solution(self, solution, options):
        solution_rooms = {room.host.name: room for room in solution}
        self.__custom_draw(options, solution_rooms)

    def draw_stock(self, options, solution_rooms, solution_in_free_room):
        self.__custom_draw(options, solution_rooms, solution_in_free_room)

    def __custom_draw(self, options, solution_rooms, solution_in_free_room=True):
        empty_rooms = {host.name: HostRoom.from_host(host) for host in self.hosts}
        free_rooms = {host.name: HostRoom.free_room(host, self.db) for host in self.hosts}
        free_rooms = {x: y for x, y in free_rooms.iteritems() if options.host_room_flt(y)}
        if empty_rooms:
            max_params = [max([getattr(room, param) for room in empty_rooms.values()]) for param in DRAW_PARAMS]
        else:
            max_params = [100 for param in DRAW_PARAMS]

        for host_name in sorted(free_rooms.keys()):
            empty_room = empty_rooms[host_name]
            free_room = free_rooms[host_name]
            sol_room = copy.copy(free_room)

            if host_name in solution_rooms:
                if solution_in_free_room:
                    sol_room.subtract(solution_rooms[host_name])
                else:
                    free_room.add(solution_rooms[host_name])

            buf = StringIO()
            print >> buf, '%10s:' % host_name.split('.')[0],
            for param, max_param in zip(DRAW_PARAMS, max_params):
                first_value = getattr(sol_room, param)
                second_value = getattr(free_room, param)
                third_value = getattr(empty_room, param)

                buf.write(' ')
                buf.write(param)
                if options.show_numeric:
                    s = "%d %d %d" % (int(first_value), (second_value - first_value), (third_value - second_value))
                    s = s[:options.elem_width]
                    s.rjust(options.elem_width)
                    buf.write(' %s' % s)
                else:
                    buf.write('[')
                    finished = False
                    for cell in range(options.elem_width + 1):
                        cell_value = max_param * (cell + 1) / options.elem_width
                        if cell_value <= first_value:
                            buf.write('.')
                        elif cell_value <= second_value:
                            buf.write('\033[91mX\033[0m')
                        elif cell_value <= third_value:
                            buf.write('X')
                        else:
                            if finished:
                                buf.write(' ')
                            else:
                                buf.write(']')
                                finished = True

            print buf.getvalue()

    def solve(self, reqs, options):
        # stage 0. build hosts filter (except dc, except queue, etc.)
        hosts_filter = create_host_filter(reqs)

        group = self.db.groups.get_group(options.group)
        group_switches = collections.defaultdict(int)
        for instance in group.get_instances():
            group_switches[instance.host.switch] += 1
        group_max_per_switch = group.card.reqs.hosts.max_per_switch or len(self.hosts)

        # stage 1. build available host rooms
        pool = []
        skiped = {
            'switch': 0,
            'switches_distribution': 0,
            'filter': 0,
            'overflow_switches': set()
        }
        for host in self.hosts:
            if not hosts_filter(host):
                skiped['filter'] += 1
                continue
            if group_switches.get(host.switch, 0) >= group_max_per_switch:
                skiped['switch'] += 1
                skiped['overflow_switches'].add(host.switch)
                continue
            room = HostRoom.free_room(host, self.db)
            fixed_resources_instance = HostRoom.from_reqs(reqs, host)
            status, data = room.can_subtract(fixed_resources_instance)
            if not status:
                reason = ','.join(sorted([k for k, v in data.items() if not v]))
                if reason not in skiped:
                    skiped[reason] = 0
                skiped[reason] += 1
                continue
            pool.append(room)

        pool_switches = collections.defaultdict(int)
        for item in pool:
            if pool_switches[item.host.switch] + group_switches.get(item.host.switch, 0) < group_max_per_switch:
                pool_switches[item.host.switch] += 1
            else:
                skiped['switches_distribution'] += 1
                skiped['overflow_switches'].add(item.host.switch)

        # check if we have enouth hosts in pool
        if sum(pool_switches.values()) < reqs.shards.min_replicas or len(pool) < reqs.shards.min_replicas:
            min_pool_size = min(sum(pool_switches.values()), len(pool))
            print('SKIP DUMP: {}'.format(skiped))
            return None, "have only {} hosts with enough resources, while required {}".format(
                min_pool_size, reqs.shards.min_replicas
            )

        # stage 2. find best subset of available rooms
        solution = self.__optimize(reqs, pool, group, options.without_reoptimize)
        if solution is None:
            return None, "unknown reason"
        assert (len(solution))

        # print smth
        if options.verbose:
            self.draw_solution(solution, options)

        return solution, None

    def __optimize(self, reqs, pool, group=None, without_reoptimize=False):
        if not pool:
            return None
        print('Pool: {}, Reqs: {}'.format(len(pool), reqs))
        generator = Random(123)

        # generate random solutions
        INITIAL_SIZE = 1000
        best_sol = None
        sol_key_size = reqs.shards.max_replicas if reqs.shards.max_replicas else len(pool)

        switches_distribution_base = defaultdict(int)
        if group is not None:
            for instance in group.get_instances():
                switches_distribution_base[instance.host.switch] += 1

        hosts_per_switch = None
        if group is not None:
            hosts_per_switch = group.card.reqs.hosts.max_per_switch or None

        sol_key = range(len(pool))
        solution = None
        filtered_reasons = None
        for i in range(INITIAL_SIZE):
            filtered_reasons = {'switch': 0, 'solution': 0}
            generator.shuffle(sol_key)

            switches_distribution = defaultdict(int)
            switches_distribution.update(switches_distribution_base)

            sol_key_prepared = []
            for index in sol_key:
                if hosts_per_switch and switches_distribution[pool[index].host.switch] >= hosts_per_switch:
                    filtered_reasons['switch'] += 1
                    continue

                sol_key_prepared.append(index)
                switches_distribution[pool[index].host.switch] += 1

                if len(sol_key_prepared) == sol_key_size:
                    break

            if sol_key_size != len(sol_key_prepared):
                continue

            solution = self.__key_to_solution(reqs, pool, sol_key_prepared)
            if solution is None:
                filtered_reasons['solution'] += 1
                continue
            cost = self.__calc_solution_cost(solution)
            if best_sol is None or best_sol[0] < cost:
                best_sol = (cost, sol_key_prepared)

        if not best_sol:
            print('Best solution not found: request {}, pool {}, filtered {}'.format(sol_key_size, len(sol_key), filtered_reasons))
            return None

        if without_reoptimize:
            return solution

        """
            We have best solution best_sol: list of indexes of host chosen for group. Now we move with small steps like "add one host", "remove one host",
            "replace one host with anothoer" to a better solution.
        """
        BUFFER_SIZE = 20
        sol_pool = [best_sol]
        MAX_ITERATIONS = 100
        for _ in range(MAX_ITERATIONS):
            if not sol_pool:
                break
            next_keys = [self.__gen_next_key(key, generator, len(pool)) for _, key in sol_pool]
            next_sols = []
            for key in next_keys:
                solution = self.__key_to_solution(reqs, pool, key)
                if solution is None:
                    continue
                cost = self.__calc_solution_cost(solution)
                next_sols.append((cost, key))

            next_sols = sol_pool + next_sols
            next_sols = sorted(next_sols, cmp=lambda x, y: cmp(x[0], y[0]))
            sol_pool = next_sols[:BUFFER_SIZE]

        if not sol_pool:
            print('Solution pool is empty')
            return None
        _, best_key = sol_pool[0]
        best_sol = self.__key_to_solution(reqs, pool, best_key)
        assert best_sol

        return best_sol

    def __gen_next_key(self, key, generator, max_size):
        ACTION_ADD = 0
        ACTION_REMOVE = 1
        ACTION_SWAP = 2

        actions = []
        if len(key) > 0:
            actions.append(ACTION_REMOVE)
        if len(key) < max_size:
            actions.append(ACTION_ADD)
        if len(actions) == 2:
            actions.append(ACTION_SWAP)

        assert actions
        action = generator.choice(actions)
        if action == ACTION_ADD:
            used = set(key)
            to_add = [x for x in range(max_size) if x not in used]
            assert to_add
            next_key = key[:] + [generator.choice(to_add)]
        elif action == ACTION_REMOVE:
            used = set(key)
            to_remove = [x for x in range(max_size) if x in used]
            assert to_remove
            to_remove = generator.choice(to_remove)
            next_key = [x for x in key if x != to_remove]
        elif action == ACTION_SWAP:
            used = set(key)
            to_add = [x for x in range(max_size) if x not in used]
            assert to_add
            to_add = generator.choice(to_add)
            to_remove = [x for x in range(max_size) if x in used]
            assert to_remove
            to_remove = generator.choice(to_remove)
            next_key = [x for x in key if x != to_remove] + [to_add]
        else:
            assert False

        return next_key

    def __key_to_solution(self, reqs, pool, key):
        """
            This function select candidate instances, which satisfy specified requirements.

            :param reqs(): group requirements, like total_power, min_replicas, memory_per_instance, ...
            :param pool(list of HostRoom): list of host resources infromation. Pool consists only of hosts, which satisfy memory/disk/... requirements
            :param key(list of int): indexes of hosts from pool, which should be used in creating HostRooms for instances

            :return (list of HostRoom): key-length list of HostRooms for allocated instances
        """

        if len(key) < reqs.shards.min_replicas:
            return None
        if (reqs.shards.max_replicas is not None) and (len(key) > reqs.shards.max_replicas):
            return None

        # print ', '.join('%s' % x for x in sorted(key))

        rooms = [pool[x] for x in key]
        rooms = sorted(rooms, cmp=lambda x, y: cmp(x.power, y.power))

        # check case when we want equal instances power
        if reqs.shards.equal_instances_power:
            lkey = len(key)
            for room in rooms:
                if room.power < reqs.shards.min_power / lkey:
                    return None
                if room.power < 1:
                    return None

        solution = []
        total_power = reqs.shards.min_power
        demand_power = reqs.shards.min_power
        for i, room in enumerate(rooms):
            if not total_power:
                instance_power = 0
            else:
                instance_power = demand_power / (len(rooms) - i)
            instance_power = max(1, instance_power)
            instance_power = min(instance_power, room.power)
            assert (instance_power >= 1)  # set 1 as minimum as zero weights break things in other projects

            instance = HostRoom.from_reqs(reqs, room.host)
            instance.power = instance_power
            solution.append(instance)

            demand_power -= instance.power

        if demand_power > 0:
            # not enough power
            return None
        return solution

    def __calc_solution_cost(self, solution):
        # DEBUG:
        # print 'calc_cost([%s])' % (','.join('%s' % x for x in solution))
        return self.cost_calculator.calc_solution_cost(solution)

    def calc_initial_cost(self):
        return self.cost_calculator.calc_solution_cost([])


class FastconfManager(object):
    def __init__(self, db, master_group):
        self.db = db
        self.master_group = db.groups.get_group(master_group)

        self.location_dcs = dict()
        for dc_info in db.settings.utils.optimizers.dynamic.locations:
            self.location_dcs[dc_info.name] = dc_info.dcs
            # self.location_dcs = {"MSK_UGRB": ["ugrb"], "MSK_FOL": ["fol"], "SAS": ["sas"], "MAN": ["man"]}

    def get_locations(self, options):
        return self.location_dcs.keys()

    def check_group_name(self, group, location):
        if location is None:
            return

        required_prefix = "%s_" % location
        if not group.startswith(required_prefix):
            raise Exception("Invalid group name '%s', group name should start with its location name, i.e. '%s'" % (
                            group, required_prefix))

        #        if itype is None:
        #            raise Exception("itype is not specified")
        # FIXME: temporary commented, do not know what to do
        #        if itype.lower() != "none":
        #            required_word = itype.upper()
        #            group_name_segments = group.split('_')
        #            if required_word not in group_name_segments:
        #                raise Exception("Invalid group name '%s', group name should contain its itype in capital letters, i.e. '%s'" % (group, required_word))

    def add_group(self, options):
        self.check_group_name(options.group, options.location)

        if options.add_to_existing:
            group = self.db.groups.get_group(options.group)
        elif options.copy_card_from:
            group = self.db.groups.copy_group(options.copy_card_from.card.name, options.group, options.master_group)
            if options.location is not None:
                group.card.reqs.hosts.location.location = [options.location.lower()]
            group.custom_instance_power_used = True
            # save options to card
            CardSetter().update_group_fields(group, options)
        else:
            group = self.db.groups.add_group(options.group,
                                             owners=options.owners,
                                             watchers=options.watchers,
                                             description=options.description,
                                             master=self.master_group.card.name,
                                             tags=dict(ctype=options.ctype))
            group.custom_instance_power_used = True

            try:
                # save options to card
                CardSetter().update_group_fields(group, options)
            except Exception:
                try:
                    self.db.groups.remove_group(group)
                except Exception:
                    pass
                raise

        group.card.properties.cpu_guarantee_set = True
        group.card.audit.cpu.last_modified = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())
        if group.get_instances():
            group.card.reqs.instances.port = min(x.port for x in group.get_instances())
        group.mark_as_modified()

        try:
            allocation_report = self.allocate_resources(options, group)
        except Exception:
            if not options.add_to_existing:
                try:
                    self.db.groups.remove_group(group)
                except Exception:
                    pass
            raise

        return group, allocation_report

    def allocate_resources(self, options, group):
        # finalize request
        request = self.__prepare_group_request(group, options)

        # filter out hosts we can not allocate hosts
        location_hosts = filter_hosts(group.card.master.getHosts(), options, group)
        location_hosts = filter(lambda x: x not in group.getHosts(), location_hosts)  # in case we add hosts

        # filter out hosts belonging to another groups
        exclude_groups = [x for x in self.master_group.parent.get_groups() if options.exclude_groups_flt(x)]
        exclude_hosts = set(sum([x.getHosts() for x in exclude_groups], []))
        location_hosts = [x for x in location_hosts if x not in exclude_hosts]

        # filter out hosts by custom host filter
        location_hosts = [x for x in location_hosts if options.hosts_flt(x)]

        solution, reason = Engine(self.db, location_hosts).solve(request, options)
        if solution is None:
            if request.hosts.location.location:
                raise NoResourcesError, ('No suitable hosts  in location <{}> found ({} candidate hosts).'
                                         'Reason: {}. Modify your request and try again or ask administrator to expand the host pool'
                                        ).format(request.hosts.location.location[0], len(location_hosts), reason)
            else:
                 raise NoResourcesError, ('No suitable hosts in group <{}> found ({} candidate hosts).'
                                          'Reason: {}. Modify your request and try again or ask administrator to expand the host pool'
                                         ).format(group.card.master.card.name, len(location_hosts), reason)

        # ===================================== RX-501 START (PISDEC) ================================================
        if options.min_power:
            for elem in solution:
                elem.power = int(options.min_power) / len(solution)
        # ===================================== RX-501 FINISH (PISDEC) ===============================================

        allocation_report = TAllocationReport(solution)

        # find port without intersections (GENCFG-2136)
        if (not options.add_to_existing) and (not options.copy_card_from):
            util_params = dict(strict=True, port_range=8, hosts=[x.host for x in allocation_report.allocated_resources])
            group_port = find_most_unused_port.jsmain(util_params).port
            group.card.reqs.instances.port = group_port
            group.card.legacy.funcs.instancePort = "new%s" % group_port
            # export_mtn_to_cauth to default for all newly created dynamic groups (osol@)
            group.card.properties.mtn.export_mtn_to_cauth = True

        if options.apply:
            allocation_report.commit(group)
            group.refresh_after_card_update()  # update function

        return allocation_report

    def __prepare_group_request(self, group, options):
        request = copy.deepcopy(group.card.reqs)

        locations = request.hosts.location.location

        if options.add_to_existing:
            request.shards.min_replicas = None if options.min_replicas is None else int(options.min_replicas)
            request.shards.max_replicas = None if options.max_replicas is None else int(options.max_replicas)
            request.shards.min_power = None if options.min_power is None else int(options.min_power)

            # ====================================== RX-501 START =======================================================
            for resource_name in 'memory_guarantee', 'ssd', 'disk', 'net_guarantee':
                getattr(request.instances, resource_name).value = 0

            influence_groups = [group] + [x for x in group.parent.get_groups() if x.card.host_donor == group.card.name]
            for influence_group in influence_groups:
                if influence_group.card.legacy.funcs.instanceCount.startswith('exactly'):
                    instances_per_host = int(influence_group.card.legacy.funcs.instanceCount[7:])
                else:
                    instances_per_host = 1

                for resource_name in 'memory_guarantee', 'ssd', 'disk', 'net_guarantee':
                    getattr(request.instances, resource_name).value = getattr(request.instances, resource_name).value + \
                                                                      getattr(influence_group.card.reqs.instances, resource_name).value * instances_per_host

                if (influence_group != group) and (request.shards.min_replicas is not None):
                    influence_group_instances = influence_group.get_kinda_busy_instances()
                    instance_power = 1 if not influence_group_instances else int(influence_group_instances[0].power)
                    request.shards.min_power += instances_per_host * instance_power  * request.shards.min_replicas
            # ====================================== RX-501 START =======================================================

        return request

    def __choose_emptiest_location(self):
        best_cost = None
        best_location = None
        for location, dcs in self.location_dcs.items():
            location_hosts = [host for host in self.master_group.getHosts() if host.dc in dcs]
            location_cost = Engine(self.db, location_hosts).calc_initial_cost()
            if best_cost is None or best_cost < location_cost:
                best_cost = location_cost
                best_location = location

        assert best_location
        return best_location

    def __build_location_requests(self, group):
        requests = []

        # TODO: defend from natural hosts death

        if not group.card.reqs.shards.location_fail.optimize:
            location = self.__choose_emptiest_location()
            power_per_location = {location: group.card.reqs.shards.min_power}
            replicas_per_location = {location: group.card.reqs.shards.min_replicas}
        else:
            power_per_location = self.__split_value_by_location(group.card.reqs.shards.min_power)
            replicas_per_location = self.__split_value_by_location(group.card.reqs.shards.min_replicas)

        for location in power_per_location:
            reqs = self.__copy_reqs(group.card.reqs)
            reqs.hosts.location.dc = self.location_dcs[location][:]
            reqs.shards.min_power = power_per_location[location]
            reqs.shards.min_replicas = max(replicas_per_location[location], reqs.shards.min_replicas_per_location)
            reqs.shards.max_replicas = max(reqs.shards.min_replicas, reqs.shards.max_replicas)
            requests.append(reqs)

        return requests

    def show(self, options):
        if not options.location:
            master_hosts = self.master_group.getHosts()
        else:
            host_dcs = set(self.location_dcs[options.location])
            master_hosts = [host for host in self.master_group.getHosts() if host.dc in host_dcs]
        master_hosts = filter(options.show_filter, master_hosts)

        if options.group is None:
            solution_rooms = {}
        else:
            slave_group = self.db.groups.get_group(options.group)
            if slave_group.master is None or slave_group.master.name != options.master_group:
                raise Exception("Group %s is not slave of %s" % (slave_group.name, options.master_group))

            solution_rooms = dict(
                map(lambda x: (x.host.name, HostRoom.from_instance(self.db, x)), slave_group.get_instances()))
            master_hosts = filter(lambda x: slave_group.hasHost(x), master_hosts)

        Engine(self.db, master_hosts).draw_stock(options, solution_rooms, False)

    def get_stats(self):
        reverted_locations_dcs = dict()
        for k, v in self.location_dcs.iteritems():
            for location in v:
                reverted_locations_dcs[location] = k

        StatsElemType = namedtuple('StatsElem', ['free_room', 'total_room'])

        result = dict(map(lambda x: (x, StatsElemType(free_room=HostRoom.zero_room(), total_room=HostRoom.zero_room())),
                          self.location_dcs))
        for host in self.master_group.getHosts():
            total_room = HostRoom.from_host(host)
            free_room = HostRoom.free_room(host, self.db)
            if free_room.power < 0:
                free_room.power = 0

            loc = reverted_locations_dcs[host.dc]
            result[loc].total_room.add(total_room)
            result[loc].free_room.add(free_room)

        return result

    def remove_group(self, options):
        group = self.db.groups.get_group(options.group)
        self.db.groups.remove_group(group)
        return None

    def remove_all(self, options):
        for slave in self.master_group.slaves[:]:
            self.db.groups.remove_group(slave)
        return None

    def __generate_free_port(self, group):
        min_port = 7000
        max_port = 16000
        port_step = 200
        original_port_index = hash(group.card.name) % ((max_port - min_port) / port_step)
        original_port = min_port + original_port_index * port_step

        # let's decide port base on group name
        used_ports = []
        for other_group in [self.master_group] + self.master_group.slaves:
            instance_ports = [x.port for x in other_group.get_instances()]
            if not instance_ports:
                continue
            min_other_port = min(instance_ports)
            max_other_port = max(instance_ports + [min_other_port + port_step - 1])

            # align by port_step
            min_aligned_port = (min_other_port / port_step) * port_step
            max_aligned_port = (max_other_port / port_step) * port_step

            used_ports.extend(range(min_aligned_port, max_aligned_port + port_step, port_step))

        used_ports = set(used_ports)
        while original_port in used_ports:
            original_port += port_step

        return original_port

    def check_something(self, host_value_func, instance_value_func, comment, check_zero=True):
        remaining_resources, zero_value_instances = calculate_free_resources(self.db, self.master_group.getHosts(),
                                                                             host_value_func, instance_value_func,
                                                                             check_zero=check_zero)
        invalid_hosts = filter(lambda (x, y): y < 0., remaining_resources.iteritems())

        success = True

        if len(invalid_hosts) > 0:
            print 'Following hosts have negative "%s" balance (value used more than capacity):' % comment
            print '\n'.join('\t%s %s' % (host.name, power) \
                            for (host, power) in sorted(invalid_hosts))
            success = False

        if not success:
            raise Exception("Some checks were broken")

    def check(self, options):
        # check power constraints
        self.check_something(
            lambda x: x.power,
            lambda x: x.power,
            "power",
            check_zero=True
        )

        # check memory constraints
        self.check_something(
            lambda x: x.get_avail_memory() / 1024. / 1024 / 1024,
            lambda x: self.db.groups.get_group(x.type).card.reqs.instances.memory_guarantee.gigabytes(),
            "memory",
            check_zero=True
        )

        return None


def get_parser():
    parser = ArgumentParserExt(description="Add extra replicas to intlookup (from free instances)")
    parser.add_argument("-m", "--master_group", dest="master_group", type=str, default=None,
                        help="Obligatory. Master group name")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=ACTIONS,
                        help="Optional. Action to execute")
    parser.add_argument("-g", "--group", dest="group", type=str,
                        help="Optional. Group name")
    parser.add_argument("-o", "--owners", dest="owners", type=argparse_types.comma_list, default=[],
                        help="Optional. Group owners")
    parser.add_argument("-w", "--watchers", dest="watchers", type=argparse_types.comma_list, default=[],
                        help="Optional. Group watchers")
    parser.add_argument("-d", "--description", dest="description", type=str,
                        help="Optional. Group description")
    parser.add_argument("--verbose", dest="verbose", action="store_true", default=False,
                        help="Optional. Print verbose info")
    parser.add_argument("--location", dest="location", default=None,
                        help="Optional. Location")
    parser.add_argument("--exclude-groups-flt", type=argparse_types.pythonlambda, default=lambda x: False,
                        help="Optional. Lambda function with filter on groups to be excluded when selecting hosts from free list")
    parser.add_argument('--hosts-flt', type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Lambda function with filter on hosts when selecting hosts from free list")
    parser.add_argument('--host-room-flt', type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Lambda function with filter on Free host rooms when showing statistics. E. g. 'lambda x: x.power > 500' to filter hosts with at least 500 cpu power available")
    parser.add_argument("--elem-width", type=int, default=ELEM_WIDTH,
                        help="Optional. Specific option of <--show>. Width of one element is %d by default" % ELEM_WIDTH)
    parser.add_argument("--show-filter", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Filter on hosts to show in \"show\" action")
    parser.add_argument("--add-to-existing", action="store_true", default=False,
                        help="Optional. For action 'add' move all allocated instances to already specified group")
    parser.add_argument("--copy-card-from", type=argparse_types.group,
                        help="Optional. Create group as copy from specified. Mutually exclusive with --add-to-existing")
    parser.add_argument("--show-numeric", action="store_true", default=False,
                        help="Optional. For action 'show' display numveriv values of parameters rather than stock-type")
    parser.add_argument("-y", "--apply", action="store_true", default=False,
                        help="Optional. Apply changes")
    parser.add_argument("--without_reoptimize", action="store_true", default=False,
                        help="Optional. Dispble second step of optimize solution.")
    CardSetter().add_card_fields_to_parser(parser)

    return parser


def parse_cmd():
    parser = get_parser()

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def parse_json(_request):
    parser = get_parser()

    request = copy.deepcopy(_request)

    options = parser.parse_json(request)

    return options


def gen_commit_message(options):
    if options.action == "add":
        return "In fastconf cluster %s added group %s" % (options.master_group, options.group)
    elif options.action == "remove":
        return "In fastconf cluster %s removed group %s" % (options.master_group, options.group)
    elif options.action == "remove_all":
        return "In fastconf cluster %s removed all groups" % options.master_group
    else:
        raise Exception("Unsupported action %s" % options.action)


def normalize(options):
    if options.action == "add":
        if options.add_to_existing:
            # check if groups exists and parameters same to paramers from options
            if not CURDB.groups.has_group(options.group):
                raise UtilNormalizeException(correct_pfname(__file__), ["group", "add_to_existing"],
                                             "Option <--add-to-existing> is enabled, but group <%s> does not exists" % (
                                             options.group))
            # update fields like location, master_group, etc.
            options.master_group = CURDB.groups.get_group(options.group).card.master.card.name

        if (options.add_to_existing == True) and (options.copy_card_from is not None):
            raise UtilNormalizeException(correct_pfname(__file__), ["add_to_existing", "copy_card_from"],
                                         "Options <--add-to-existing> and <--copy-card-from> are mutually exclusive")

        if not options.add_to_existing:
            if CURDB.groups.has_group(options.group):
                raise UtilNormalizeException(correct_pfname(__file__), ["group"], "Group <%s> already exists" % options.group)

    if options.master_group is None:
        raise UtilNormalizeException(correct_pfname(__file__), ["master_group"],
                                     "You must specify option --master-group")


@perf_timer
def main(options, db, from_cmd=True):
    man = FastconfManager(db, options.master_group)
    if options.action == "show":
        result = man.show(options)
    elif options.action == "get_locations":
        result = man.get_locations(options)
    elif options.action == "get_stats":
        result = man.get_stats()
    elif options.action == "add":
        group, allocation_report = man.add_group(options)
        if from_cmd:
            total_hosts = len(allocation_report.allocated_resources)
            total_power = sum(map(lambda x: x.power, allocation_report.allocated_resources))
            result = "Group %s: %d hosts, %f total power" % (group.card.name, total_hosts, total_power)
        else:
            result = group, allocation_report
    elif options.action == "remove":
        result = man.remove_group(options)
    elif options.action == "remove_all":
        result = man.remove_all(options)
    elif options.action == "check":
        result = man.check(options)
    else:
        raise Exception('Action "%s" is not supported' % options.action)

    if options.apply:
        CURDB.update(smart=True)
    else:
        if options.action in ["add", "remove", "remove_all"] and from_cmd:
            print red_text("Not updated!!! Add option -y to update.")

    return result


def jsmain(d, db, from_cmd=False):
    options = get_parser().parse_json(d)

    normalize(options)

    result = main(options, db, from_cmd=from_cmd)

    return result


def print_result(result, options):
    if result is None:
        return

    if options.action == "get_stats":
        for location in sorted(result.keys()):
            print "Location %s:\n    Free: %s\n    Total: %s\n" % (
            location, result[location].free_room, result[location].total_room)
    else:
        print result


if __name__ == '__main__':
    options = parse_cmd()

    normalize(options)

    result = main(options, CURDB)

    print_result(result, options)

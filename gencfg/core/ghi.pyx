# cython: profile=True, nocheck=True, boundscheck=False, wraparound=False, initializedcheck=False, cdivision=True, language_level=2, infer_types=True, c_string_type=bytes

"""
    GHI = Groups Hosts Instances. Time critical part of config generator. We try to use Cython here, because this module is almost independent
    from others
"""

import sys
import copy
import itertools
import math
import time
import traceback

from core.exceptions import TInstanceNotFoundException, TInstanceAlreadyInGHI

from core.hosts cimport Host
from core.instances cimport Instance

cdef class GHIGroup:
    def __cinit__(self, bytes name=None, set hosts=None, set instances=None, bytes donor=None, set acceptors=None, object ifunc=None):
        self.name = name
        self.hosts = hosts
        self.instances = instances
        self.donor = donor
        self.acceptors = acceptors
        self.ifunc = ifunc

    def __hash__(self):
        return hash(self.name)

    def __richcmp__(self, GHIGroup other, int op):
        if op == 2: # __eq__
            return self.name == other.name
        else:
            raise NotImplementedError("Operation <%s> is not implemented yet" % op)

cdef class GHI:
    """
        Structure with indexes for faster access to group instances, getting list of hosts groups, ...
    """
    def __cinit__(self, object db):
        self.db = db
        self.groups = {}
        self.host_groups = {}
        self.instances = {}
        self.instances_set = set()

    cdef bint has_group(self, bytes group_name):
        return group_name in self.groups

    cpdef add_group(self, bytes group_name, object instance_func, bytes donor_name):
        assert (not self.has_group(group_name)), "Group %s in ghi already" % group_name
        assert (donor_name is None or self.has_group(donor_name)), "Donor group <%s> for <%s> not found in our list" % (donor_name, group_name)

        cdef GHIGroup group = GHIGroup(name=group_name, hosts=set(), instances=None, ifunc=instance_func, donor=donor_name,
                                       acceptors=set())
        cdef GHIGroup donor_group
        if donor_name is not None:
            donor_group = self.groups[donor_name]
            donor_group.acceptors.add(group_name)
            group.hosts = donor_group.hosts
        self.groups[group_name] = group

    cpdef remove_group(self, bytes group_name):
        cdef GHIGroup group = self.groups.get(group_name)
        assert (group is not None)
        assert (not group.acceptors), "Group <%s> has acceptors <%s>, thus can not be removed" % (
            group.name, ",".join(group.acceptors))
        if group.instances is not None:
            self.__reset_group_instances(group)
        if group.donor:
            self.groups[group.donor].acceptors.discard(group.name)
            group.donor = None
        else:
            self.remove_group_hosts(group.name, list(group.hosts))
        del self.groups[group.name]

    cpdef rename_group(self, bytes old_name, bytes new_name):
        cdef GHIGroup group = self.groups.get(old_name)
        assert (group is not None)
        assert (new_name not in self.groups)

        del self.groups[old_name]
        group.name = new_name
        self.groups[new_name] = group

        if group.donor is not None:
            acceptors = (<GHIGroup>self.groups[group.donor]).acceptors
            acceptors.discard(old_name)
            acceptors.add(new_name)

        for acceptor in group.acceptors:
            (<GHIGroup>self.groups[<bytes>acceptor]).donor = new_name

        self.__remove_group_hosts_instances(group, list(group.hosts))

        if group.instances is not None:
            for instance in group.instances:
                instance.type = new_name

        group.instances = set()
        self.__add_group_hosts_instances(group, list(group.hosts))

    cpdef remove_host_donor(self, object igroup):
        """
            Unbind group from its donor, separate <group> hosts from <donor group> hosts.
        """
        cdef GHIGroup group = self.groups.get(igroup.card.name)
        cdef GHIGroup donor_group = self.groups.get(group.donor)
        donor_group.acceptors.remove(group.name)
        group.donor = None

        group.hosts = copy.copy(group.hosts)
        for host in group.hosts:
            if host not in self.host_groups:
                self.host_groups[host] = set()
            self.host_groups[host].add(group)

    cpdef bint has_host_donor(self, bytes group_name):
        cdef GHIGroup group = self.groups.get(group_name)
        assert (group_name is not None)
        return group.donor is not None

    cdef list __all_group_acceptors(self, GHIGroup group):
        cdef list extra
        cdef list queue = [group]
        cdef int pos = 0
        while True:
            extra = [self.groups[acceptor] for acceptor in (<GHIGroup>queue[pos]).acceptors]
            if len(extra) == 0:
                break
            queue.extend(extra)
            pos += 1
        return queue

    cdef list __all_host_group(self, Host host):
        return sum([self.__all_group_acceptors(group) for group in self.host_groups[host]], [])

    cpdef add_group_hosts(self, bytes group_name, list hosts):
        cdef GHIGroup group = self.groups.get(group_name)
        assert (group is not None)
        assert (group.donor is None)

        cdef set group_hosts = group.hosts
        cdef dict host_groups = self.host_groups
        cdef int before_hosts_count = len(group_hosts)
        cdef int new_hosts_count = 0
        for host in hosts:
            group_hosts.add(host)
            if host not in host_groups:
                host_groups[host] = set()
            host_groups[host].add(group)
            new_hosts_count += 1
        # all added hosts were not in the group before
        assert (len(group_hosts) == new_hosts_count + before_hosts_count), "Group <%s> hosts count is <%d> while new and before count <%d>" % (
                                                                           group.name, len(group_hosts), new_hosts_count + before_hosts_count)

        for acceptor in self.__all_group_acceptors(group):
            if (<GHIGroup>acceptor).instances is not None:
                self.__add_group_hosts_instances(<GHIGroup>acceptor, list(hosts))

    cpdef remove_group_hosts(self, bytes group_name, list hosts):
        cdef GHIGroup group = self.groups.get(group_name)
        assert (group is not None)
        assert (group.donor is None), "Group %s donor is %s (should be None)" % (group.name, group.donor)

        cdef set group_hosts = group.hosts
        cdef dict host_groups = self.host_groups
        for host in hosts:
            group_hosts.discard(host)
            host_groups[host].discard(group)
        # allow to remove unexisting hosts

        for acceptor in self.__all_group_acceptors(group):
            if (<GHIGroup>acceptor).instances is not None:
                self.__remove_group_hosts_instances(<GHIGroup>acceptor, hosts)

    cpdef bint is_host_in_group(self, bytes group_name, Host host):
        cdef GHIGroup group = self.groups.get(group_name)
        assert (group is not None)
        return host in group.hosts

    cpdef list get_group_hosts(self, bytes group_name):
        cdef GHIGroup group = self.groups.get(group_name)
        assert (group is not None)
        return list(group.hosts)

    cpdef list get_hosts(self):
        return self.host_groups.keys()

    cpdef list get_host_groups(self, Host host):
        cdef list groups = list(self.host_groups.get(host, []))
        groups = sum([self.__all_group_acceptors(<GHIGroup>group) for group in groups], [])
        return [x.name for x in groups]

    cpdef bytes get_group_donor(self, bytes group_name):
        cdef GHIGroup group = self.groups.get(group_name)
        assert (group is not None)
        return group.donor

    cpdef list get_group_acceptors(self, bytes group_name):
        cdef GHIGroup group = self.groups.get(group_name)
        assert (group is not None)
        return list(group.acceptors)

    cpdef change_group_instance_func(self, bytes group_name, object instance_func):
        cdef GHIGroup group = self.groups.get(group_name)
        assert (group is not None)
        if group.instances is not None:
            self.__reset_group_instances(group)
        group.ifunc = instance_func

    cpdef init_group_instances(self, GHIGroup group):
        if group.instances is None:
            group.instances = set()
            self.__add_group_hosts_instances(group, list(group.hosts))

    cpdef reset_all_groups_instances(self):
        self.instances = {}
        self.instances_set = set()
        for group in self.groups.values():
            group.instances = None

    cdef void __reset_group_instances(self, GHIGroup group):
        self.__remove_group_hosts_instances(group, list(group.hosts))
        group.instances = None

    cdef list __add_group_hosts_instances(self, GHIGroup group, list hosts):
        cdef object func = group.ifunc
        cdef set group_instances = group.instances
        cdef Instance instance
        cdef list host_instances
        cdef bytes key
        cdef bytes name_port_key

        for host in hosts:
            host_instances = [Instance(<Host>host, <float>power, <int>port, <bytes>(group.name), <int>N) for (host, power, port, type, N) in func(host)]
            for instance in host_instances:
                name_port_key = <bytes>('%s:%s' % (instance.host.name, instance.port))
                key = <bytes>('%s:%s' % (name_port_key, instance.type))

                if name_port_key in self.instances_set:
                    if self.db.version >= "2.2.27":
                        # find instance which is already in GHI when adding new one
                        for instance_key in self.instances:
                            hostname, port, type_ = instance_key.split(':')
                            port = int(port)
                            if hostname == instance.host.name and port == instance.port:
                                already_in_ghi_instance = instance_key

                        raise TInstanceAlreadyInGHI("%s:%s" % (instance.host.name, instance.port), already_in_ghi_instance, group.name)

                self.instances_set.add(name_port_key)
                self.instances[key] = instance
                group_instances.add(instance)

        return []

    cdef void __remove_group_hosts_instances(self, GHIGroup group, list hosts):
        cdef object func = group.ifunc
        cdef set group_instances = group.instances
        cdef Host host
        cdef Instance instance

        for host in hosts:
            host_instances = [Instance(*args) for args in func(host)]
            for instance in host_instances:
                key = '%s:%s:%s' % (instance.host.name, instance.port, instance.type)
                if key in self.instances:
                    self.instances_set.remove('%s:%s' % (instance.host.name, instance.port))
                    del self.instances[key]
                    group_instances.discard(instance)

    cpdef list get_all_instances(self):
        cdef list instances = []
        cdef GHIGroup group

        for group in self.groups.values():
            if group.instances is None:
                self.init_group_instances(group)
            instances.append(group.instances)
        return list(itertools.chain(instances))

    cpdef list get_group_instances(self, bytes group_name):
        cdef GHIGroup group = self.groups.get(group_name)
        assert (group is not None)
        if group.instances is None:
            self.init_group_instances(group)
        return list(group.instances)

    cpdef Instance get_instance(self, bytes group_name, bytes host, int port, power=None):
        cdef bytes key = <bytes>('%s:%s:%s' % (host, port, group_name))

        cdef GHIGroup group = self.groups.get(group_name)
        assert (group is not None), '%s:%s:%s' % (host, port, group_name)
        if group.instances is None:
            self.init_group_instances(group)

        cdef Instance instance = self.instances.get(key, None)

        if instance is None:
            instance_str = <bytes>('%s:%s:%s:%s' % (host, port, power if power is not None else '???', group.name))
            raise TInstanceNotFoundException(instance_str)
        if instance.type != group.name:
            instance_str = <bytes>('%s:%s:%s:%s' % (host, port, power if power is not None else '???', group.name))
            raise Exception('Error referenced instance %s have different group type %s' % (instance_str, instance.type))
        if power is not None and math.fabs(instance.power - float(power)) > 0.1:
            instance_str = <bytes>('%s:%s:%s:%s' % (host, port, power, group.name))
            sys.stderr.write("Instance with wrong power: %s %s %s\n" % (instance_str, power, instance.power))
        # instance.power = float(power) # FIXME: do not dow it
        #            raise Exception("Error in loading instance %s : found %s" % (instance_str, instance))

        return instance

    cpdef list get_group_host_instances(self, bytes group_name, Host host):
        cdef GHIGroup group = self.groups.get(group_name)
        assert (group is not None)
        return self.__get_group_host_instances(group, host)

    cdef list __get_group_host_instances(self, GHIGroup group, Host host):
        if group not in self.__all_host_group(host):
            return []
        if group.instances is None:
            self.init_group_instances(group)
        cdef object func = group.ifunc
        cdef list keys = ['%s:%s:%s' % (host.name, port, group.name) for (host, _, port, _, _) in func(host)]
        return [self.instances[key] for key in keys]

    cpdef list get_host_instances(self, Host host):
        return sum([self.__get_group_host_instances(group, host) for group in self.__all_host_group(host)], [])

    cpdef rename_host(self, bytes old_name, bytes new_name):
        #  update self.instnaces
        key_pairs = []
        for key in self.instances:
            host_name, _, rest = key.partition(':')
            if host_name == old_name:
                key_pairs.append((key, '{}:{}'.format(new_name, rest)))
        for old_key, new_key in key_pairs:
            self.instances[new_key] = self.instances.pop(old_key)


    cpdef list transf(self, group, set allowed_everywhere):
        lst_acceptors = self.__all_group_acceptors(self.groups[group])

        cdef list result = list()
        for acc in lst_acceptors:
            group_name = (<GHIGroup>acc).name
            if group_name not in allowed_everywhere:
                result.append(group_name)

        return result

    cpdef set calc_bad_group_intersections(self, set allowed_everywhere, list allowed_pairs):
        # we should be optimistic to decrease time of check in most cases!
        cdef bytes first_group
        cdef bytes second_group
        cdef list group_names
        cdef list new_group_names

        # let's normalize allowed_pairs
        cdef set norm_pairs = set()
        for pair in allowed_pairs:
            first, second = pair
            if first > second:
                first, second = second, first
            norm_pairs.add((first, second))

        cdef set intersections = set()
        for host, groups in self.host_groups.items():
            group_names = [(<GHIGroup>x).name for x in groups]
            intersections.add(tuple(group_names))

        cdef set pairs = set()
        for groups in intersections:
            groups = sum([self.transf(group, allowed_everywhere) for group in groups], [])

            for first in range(len(groups)):
                for second in range(first + 1, len(groups)):
                    first_group = groups[first]
                    second_group = groups[second]
                    if first_group > second_group:
                        first_group, second_group = second_group, first_group
                    pairs.add((first_group, second_group))

        cdef set bad_pairs = set()
        for pair in pairs:
            if pair not in norm_pairs:
                bad_pairs.add(pair)

        return bad_pairs

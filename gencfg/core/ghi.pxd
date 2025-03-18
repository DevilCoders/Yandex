from core.hosts cimport Host
from core.instances cimport Instance

cdef class GHIGroup:
    cdef public bytes name # group name
    cdef set hosts
    cdef public set instances
    cdef bytes donor
    cdef public set acceptors
    cdef object ifunc # instance func

cdef class GHI:
    cdef object db
    cdef public dict groups
    cdef dict host_groups
    cdef public dict instances
    cdef set instances_set

    cdef bint has_group(self, bytes group_name)
    cpdef add_group(self, bytes group_name, object instance_func, bytes donor_name)
    cpdef remove_group(self, bytes group_name)
    cpdef rename_group(self, bytes old_name, bytes new_name)
    cpdef remove_host_donor(self, object igroup)
    cpdef bint has_host_donor(self, bytes group_name)
    cdef list __all_group_acceptors(self, GHIGroup group)
    cdef list __all_host_group(self, Host host)
    cpdef add_group_hosts(self, bytes group_name, list hosts)
    cpdef remove_group_hosts(self, bytes group_name, list hosts)
    cpdef bint is_host_in_group(self, bytes group_name, Host host)
    cpdef list get_group_hosts(self, bytes group_name)
    cpdef list get_hosts(self)
    cpdef list get_host_groups(self, Host host)
    cpdef bytes get_group_donor(self, bytes group_name)
    cpdef list get_group_acceptors(self, bytes group_name)
    cpdef change_group_instance_func(self, bytes group_name, object instance_func)
    cpdef init_group_instances(self, GHIGroup group)
    cpdef reset_all_groups_instances(self)
    cdef void __reset_group_instances(self, GHIGroup group)
    cdef list __add_group_hosts_instances(self, GHIGroup group, list hosts)
    cdef void __remove_group_hosts_instances(self, GHIGroup group, list hosts)
    cpdef list get_all_instances(self)
    cpdef list get_group_instances(self, bytes group_name)
    cpdef Instance get_instance(self, bytes group_name, bytes host, int port, power=*)
    cpdef list get_group_host_instances(self, bytes group_name, Host host)
    cdef list __get_group_host_instances(self, GHIGroup group, Host host)
    cpdef list get_host_instances(self, Host host)
    cpdef list transf(self, group, set allowed_everywhere)
    cpdef set calc_bad_group_intersections(self, set allowed_everywhere, list allowed_pairs)
    cpdef rename_host(self, bytes old_name, bytes new_name)

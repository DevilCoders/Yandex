from collections import defaultdict


def calculate_free_resources(db, hosts, host_value_func, instance_value_func, check_zero=True):
    """
        This function calculates amount of free resources for every host from specified.

        :type db: core.db.DB
        :type hosts: list[core.hostsinfo.Host]
        :type host_value_func: T
        :type instance_value_func: T
        :type check_zero: bool

        :param db: Database to calculate resources for
        :param hosts: list of hosts to check. This hosts should belong to dynamic.
        :param host_value_func: function, calculating amount of resource for host, e. g. 'lambda x: x.power' to calculate cpu power of host
        :param instance_value_func: function, calculating amount of resorce for instance, e. g. 'lambda x: x.power' to calculate cpu power of instance
        :param check_zero: if param is True, check that instance consumes more than zero resource

        :return (dict of (core.hostsinfo.Host, float), list of core.instances.Instance): first elem is dict of remaining resources for specified host, second is list of instances with zero calculated resource
    """

    remaining_resources = defaultdict(float)
    zero_value_instances = []

    for host in hosts:
        remaining_resources[host] = host_value_func(host)
        host_instances = db.groups.get_host_instances(host)
        for instance in host_instances:
            instance_group = db.groups.get_group(instance.type)
            if instance_group.card.properties.fake_group:
                continue

            instance_value = instance_value_func(instance)
            if instance_value == 0 and check_zero:
                zero_value_instances.append(instance)
            remaining_resources[instance.host] -= instance_value

    return remaining_resources, zero_value_instances


def calc_instance_memory(db, instance):
    group = db.groups.get_group(instance.type)
    instance_memory = group.card.reqs.instances.memory_guarantee.value

    if hasattr(group.card.legacy.funcs, 'instanceMemory') and group.card.legacy.funcs.instanceMemory is not None:
        instance_memory = instance.host.get_avail_memory()
        for host_group in db.groups.get_host_groups(instance.host):
            if host_group == group:
                continue
            if host_group.card.properties.fake_group:
                continue
            instance_memory -= host_group.card.reqs.instances.memory_guarantee.value

    return int(instance_memory)


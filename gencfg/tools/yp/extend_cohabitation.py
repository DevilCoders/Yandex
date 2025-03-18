import core.db
import logging


MEMORY_RESERVE = 1 * 1024 ** 3
HDD_RESERVE = 50 * 1024 ** 3

class Resources(object):
    def __init__(self, mem, cpu, ssd, hdd):
        self.mem, self.cpu, self.ssd, self.hdd = mem, cpu, ssd, hdd

    def __repr__(self):
        return 'Resources(mem={}, cpu={}, ssd={}, hdd={}'.format(int(self.mem), int(self.cpu), int(self.ssd / 1024**3), int(self.hdd / 1024**3))

    def __le__(self, rhs):
        return self.mem <= rhs.mem and self.cpu <= rhs.cpu and self.ssd <= rhs.ssd and self.hdd <= rhs.hdd


def free_cpu(hosts, master_group):
    cpu = {host: host.power for host in hosts}
    for slave in master_group.slaves:
        for instance in slave.get_instances():
            if instance.host in cpu:
                cpu[instance.host] -= instance.power
    return cpu


def free_static_resources(hosts, master_group):
    resources = {host: Resources(host.get_avail_memory() - MEMORY_RESERVE,
                                 host.power,
                                 host.ssd * 1024. ** 3,
                                 host.disk * 1024. ** 3 - HDD_RESERVE) for host in hosts}
    for slave in master_group.slaves:
        for instance in slave.get_instances():
            if instance.host in resources:
                resources[instance.host].mem -= slave.card.reqs.instances.memory_guarantee.value
                resources[instance.host].cpu -= instance.power
                resources[instance.host].ssd -= slave.card.reqs.instances.ssd.value
                resources[instance.host].hdd -= slave.card.reqs.instances.disk.value
    return resources


def extend_hosts(group, master_group):
    hosts = {host for host in set(master_group.get_hosts()) - set(group.get_hosts()) if
             [host.location] == group.card.reqs.hosts.location.location}
    extended_to = set()
    reqs = Resources(group.card.reqs.instances.memory_guarantee.value,
                     0,  # cpu
                     group.card.reqs.instances.ssd.value,
                     group.card.reqs.instances.disk.value)
    for host, resources in free_static_resources(hosts, master_group).iteritems():
        if reqs <= resources:
            group.addHost(host)
            extended_to.add(host)

    logging.info('%s extended to %s hosts', group, len(extended_to))
    return extended_to


def extend_cpu(group, master_group):
     total_power = 0.0
     for host, power in free_cpu({host for host in group.get_hosts()}, master_group).iteritems():
        group.get_host_instances(host)[0].power += power
        total_power += power

     logging.info('%s extended by %s power units', group, total_power)


def extend(group, master_group, host_memory):
    extend_hosts(group, master_group, host_memory)
    extend_cpu(group, master_group)
    group.mark_as_modified()


def extend_old_blocks(db):
    all_dynamic = db.groups.get_group('ALL_DYNAMIC')
    extend(db.groups.get_group('SAS_DYNAMIC_YP_256_1'), all_dynamic, 256)
    extend(db.groups.get_group('SAS_DYNAMIC_YP_512_1'), all_dynamic, 512)
    extend(db.groups.get_group('MAN_DYNAMIC_YP_512_1'), all_dynamic, 512)
    extend(db.groups.get_group('MSK_IVA_DYNAMIC_YP_64_1'), all_dynamic, 64)
    extend(db.groups.get_group('MSK_IVA_DYNAMIC_YP_128_1'), all_dynamic, 128)
    extend(db.groups.get_group('MSK_IVA_DYNAMIC_YP_256_1'), all_dynamic, 256)
    extend(db.groups.get_group('MSK_MYT_DYNAMIC_YP_256_1'), all_dynamic, 256)
    extend(db.groups.get_group('VLA_DYNAMIC_YP_512_1'), all_dynamic, 512)
    db.update(smart=True)


def extend_hosts_new(db):
    group = db.groups.get_group('SAS_DYNAMIC_CPU')
    master = db.groups.get_group('ALL_DYNAMIC')

    inserted = 0
    for host in master.getHosts():
        if host.dc not in group.card.reqs.hosts.location.location:
            continue
        if group.hasHost(host):
            continue
        group.addHost(host)
        instance = group.get_host_instances(host)[0]
        group.custom_instance_power[instance] = 0.0
        inserted += 1

    logging.info('%s extended onto %s hosts', group, inserted)
    if inserted:
        group.custom_instance_power_used = True
        group.mark_as_modified()
        db.update(smart=True)


def extend_power_new(db):
    group = db.groups.get_group('SAS_DYNAMIC_CPU')
    master = db.groups.get_group('ALL_DYNAMIC')

    cpu = free_cpu(group.getHosts(), master)
    total_power = 0.0
    for host, power in cpu.iteritems():
        group.get_host_instances(host)[0].power += power
        total_power += power

    logging.info('Raised %s power units', total_power)

    if total_power > 0.0:
        group.mark_as_modified()
        db.update(smart=True)


def extend_new_blocks(db):
    all_dynamic = db.groups.get_group('ALL_DYNAMIC')
    groups = [
        # ssd
        #db.groups.get_group('SAS_DYNAMIC_SSD_4096'),
        #db.groups.get_group('SAS_DYNAMIC_SSD_2048'),
        #db.groups.get_group('SAS_DYNAMIC_SSD_1024'),
        #db.groups.get_group('SAS_DYNAMIC_SSD_512'),
        #db.groups.get_group('SAS_DYNAMIC_SSD_256'),
        # mem
        #db.groups.get_group('SAS_DYNAMIC_MEM_256'),
        #db.groups.get_group('SAS_DYNAMIC_MEM_128'),
        #db.groups.get_group('SAS_DYNAMIC_MEM_64'),
        #db.groups.get_group('SAS_DYNAMIC_MEM_32'),
        #db.groups.get_group('SAS_DYNAMIC_MEM_16'),
        # hdd
        db.groups.get_group('SAS_DYNAMIC_HDD_4096'),
        db.groups.get_group('SAS_DYNAMIC_HDD_2048'),
        db.groups.get_group('SAS_DYNAMIC_HDD_1024'),
        db.groups.get_group('SAS_DYNAMIC_HDD_512'),
        db.groups.get_group('SAS_DYNAMIC_HDD_256'),
    ]
    for group in groups:
        logging.info('Cleanup %s', group)
        db.groups.remove_hosts(group.getHosts(), group)

    for group in groups:
        extend_hosts(group, all_dynamic)

    db.update(smart=True)


def main(db):
    # extend_power_new(db)
    extend_new_blocks(db)


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    main(core.db.CURDB)

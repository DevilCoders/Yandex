# -*- coding: utf-8 -*-
"""
Utilities specific for Apache Kafka clusters.
"""

from ...providers.dns import Record
from ..utils import build_host_group, combine_sg_service_rules, HostGroup

ZK_ROLE_TYPE = 'zk'


def classify_host_map(hosts):
    """
    Classify dict of hosts on Kafka and ZooKeeper ones.
    """
    kafka_hosts = {}
    zk_hosts = {}
    for host, opts in hosts.items():
        if ZK_ROLE_TYPE in opts.get('roles', []):
            zk_hosts[host] = opts
        else:
            kafka_hosts[host] = opts

    return kafka_hosts, zk_hosts


def fill_conductor_group_id(host_group: HostGroup) -> None:
    if host_group.hosts:
        host_group.properties.conductor_group_id = next(iter(host_group.hosts.values()))['subcid']  # type: ignore


def build_kafka_host_groups(hosts, cfg_kafka, cfg_zk):
    """
    Get kafka and zk hosts groups
    """
    kafka_hosts, zk_hosts = classify_host_map(hosts)
    kafka_group = build_host_group(cfg_kafka, kafka_hosts)
    fill_conductor_group_id(kafka_group)
    zk_group = build_host_group(cfg_zk, zk_hosts)

    # Only one service SG per cluster is supported for now. So each host group should have rules
    # combined from all groups.
    sg_service_rules = combine_sg_service_rules(kafka_group, zk_group)
    kafka_group.properties.sg_service_rules = sg_service_rules
    zk_group.properties.sg_service_rules = sg_service_rules

    fill_conductor_group_id(zk_group)
    return kafka_group, zk_group


def get_cluster_dns_names(kafka_group, cid):
    dns_groups = []
    for dns_group in kafka_group.properties.group_dns:
        id_key = dns_group['id']
        if id_key != 'cid':
            raise RuntimeError(f"Can not use dns groups with id key {id_key}, only cid supported")
        group_name = dns_group['pattern'].format(id=cid)
        dns_groups.append(group_name)
    return dns_groups


def get_group_vtype(group):
    vtypes_set = set()
    for host in group.hosts.values():
        vtypes_set.add(host['vtype'])
    if len(vtypes_set) > 1:
        raise RuntimeError('Can not create cluster in multiple cloud types')
    return vtypes_set.pop()


def make_cluster_dns_record(executer, kafka_group):
    if not kafka_group.hosts:
        return
    vtype = get_group_vtype(kafka_group)

    # prepare IP adresses
    dns_records = []
    for host, opts in kafka_group.hosts.items():
        if vtype == 'compute':
            for address, version in executer.compute_api.get_instance_public_addresses(host):
                dns_records.append(
                    Record(
                        address=address,
                        record_type=('AAAA' if version == 6 else 'A'),
                    )
                )
        elif vtype == 'aws':
            addresses = executer.ec2.get_instance_addresses(opts['vtype_id'], opts['region_name'])
            dns_records.extend(addresses.public_records())

    # publish DNS records
    for group_name in get_cluster_dns_names(kafka_group, executer.task['cid']):
        if vtype == 'compute':
            executer.dns_api.set_records(group_name, dns_records)
        elif vtype == 'aws':
            executer.route53.set_records_in_public_zone(group_name, dns_records)


def delete_cluster_dns_record(executer, kafka_group):
    vtype = get_group_vtype(kafka_group)
    for group_name in get_cluster_dns_names(kafka_group, executer.task['cid']):
        if vtype == 'compute':
            executer.dns_api.set_records(group_name, [])
        elif vtype == 'aws':
            executer.route53.set_records_in_public_zone(group_name, [])

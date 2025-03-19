# -*- coding: utf-8 -*-
"""
DBaaS Internal API Hadoop cluster options getter
"""
from typing import Dict, List  # noqa

from ...core.exceptions import DbaasNotImplementedError, SubclusterNotExistsError
from ...utils.dataproc_manager import dataproc_manager_client
from ...utils.instance_group import InstanceGroup, InstanceGroupNotFoundError
from ...utils.filters_parser import Operator
from ...utils.host import get_host_objects
from ...utils import metadb
from .utils import get_autoscaling_config
from ...utils.pagination import Column, supports_pagination
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE
from .pillar import get_cluster_pillar
from .traits import HostHealth
from ...utils.iam_token import get_iam_client


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.INFO)
def hadoop_cluster_config(cluster: Dict, flavors=None) -> Dict:
    """
    Assemble Hadoop config object.
    """
    # flavors not used
    # pylint: disable=W0613
    pillar = get_cluster_pillar(cluster)
    return {
        'version_id': pillar.version_prefix,
        'hadoop': pillar.config,
    }


@register_request_handler(MY_CLUSTER_TYPE, Resource.SUBCLUSTER, DbaasOperation.LIST)
@supports_pagination(items_field='subclusters', columns=(Column(field='subcid', field_type=str),))
def hadoop_subclusters(cluster: Dict, page_token_subcid=None, limit=None, **_):
    """
    Return list of Hadoop subclusters.
    """
    pillar = get_cluster_pillar(cluster)
    all_subclusters = metadb.get_subclusters_by_cluster(cluster['cid'], page_token_subcid, limit)
    subclusters_info = []  # type: List[Dict]
    for subcluster_db_row in all_subclusters:
        cid = subcluster_db_row['cid']
        subcid = subcluster_db_row['subcid']
        subcluster = pillar.get_subcluster(subcid)
        if not subcluster:
            raise RuntimeError('Inconsistent pillar for cid {cid}, subcluster {subcid} not found')
        subcluster['created_at'] = subcluster_db_row['created_at']
        subcluster['cluster_id'] = cid
        instance_group_config = subcluster.get('instance_group_config')
        if instance_group_config:
            subcluster['instance_group_id'] = metadb.get_instance_group(subcid)['instance_group_id']
            subcluster['autoscaling_config'] = get_autoscaling_config(instance_group_config)
            subcluster['autoscaling_config']['decommission_timeout'] = subcluster.get('decommission_timeout')
        subclusters_info.append(subcluster)
    return subclusters_info


@register_request_handler(MY_CLUSTER_TYPE, Resource.SUBCLUSTER, DbaasOperation.INFO)
def hadoop_subcluster(cluster: Dict, subcid: str, **_) -> Dict:
    """
    Return Hadoop subcluster info.
    """
    pillar = get_cluster_pillar(cluster)
    subcluster = pillar.get_subcluster(subcid)
    if not subcluster:
        raise SubclusterNotExistsError(subcid)
    dbinfo = metadb.get_subcluster_info(subcid)
    instance_group_config = subcluster.get('instance_group_config')
    if instance_group_config:
        subcluster['instance_group_id'] = metadb.get_instance_group(subcid)['instance_group_id']
        subcluster['autoscaling_config'] = get_autoscaling_config(instance_group_config)
        subcluster['autoscaling_config']['decommission_timeout'] = subcluster.get('decommission_timeout')
    subcluster['cluster_id'] = dbinfo['cid']
    subcluster['created_at'] = dbinfo['created_at']
    return subcluster


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.HEALTH)
def hadoop_cluster_health(cluster):
    """
    Returns cluster health
    """
    return dataproc_manager_client().cluster_health(cluster['cid']).health


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.EXTRA_INFO)
def hadoop_cluster_extra_info(cluster: Dict) -> Dict:
    """
    Returns zone_id of Hadoop cluster.
    """
    pillar = get_cluster_pillar(cluster)
    extra = {
        'zone_id': pillar.zone_id,
        'service_account_id': pillar.service_account_id,
        'log_group_id': pillar.log_group_id,
        'ui_proxy': pillar.ui_proxy,
    }
    if pillar.user_s3_bucket:
        extra['bucket'] = pillar.user_s3_bucket
    return extra


def hadoop_host_extra_formatter(host: Dict, _) -> Dict:
    """
    Extra formatter for Hadoop host
    """
    result = {
        "subcluster_id": host['subcid'],
        "health": HostHealth.unknown,
    }
    if host.get('vtype_id'):
        result['compute_instance_id'] = host['vtype_id']
    result['role'] = host['roles'][0]
    return result


def parse_filters(filters):
    """
    Gets subcid from filters and validates
    """
    if not filters:
        return None
    if len(filters) > 1:
        raise DbaasNotImplementedError('Unsupported filter. Only simple filter')
    filter_obj = filters[0]
    if filter_obj.attribute != 'subcid':
        raise DbaasNotImplementedError('Unsupported filter field "{}"'.format(filter_obj.attribute))
    if filter_obj.operator != Operator.equals:
        raise DbaasNotImplementedError('Unsupported filter operator. Must be equals')
    subcid = filter_obj.value
    if not isinstance(subcid, str):
        raise DbaasNotImplementedError('Unsupported type of filter. Type must be a string')
    return subcid


def get_instance_groups_hosts(instance_groups, cluster, subcid):
    pillar = get_cluster_pillar(cluster)
    token = get_iam_client().issue_iam_token(service_account_id=pillar.service_account_id)
    instance_group_service = InstanceGroup(token=token)
    instance_groups_hosts = []
    for instance_group in instance_groups:
        instance_group_id = instance_group.get('instance_group_id')
        if not instance_group_id:
            continue
        try:
            instances = instance_group_service.list_instances(instance_group_id).data.instances
            if not subcid:
                subcid = metadb.get_subcluster_by_instance_group(instance_group_id)['subcid']
            for instance in instances:
                fqdn = instance.fqdn
                # we can not set custom fqdn field at the moment without a feature flag,
                # so we put mdb-style fqdn as an additional dns_record, considering it to be the main fqdn for dataproc
                if instance.network_interfaces:
                    additional_dns_records = instance.network_interfaces[0].primary_v4_address.dns_records
                    if additional_dns_records:
                        fqdn = additional_dns_records[0].fqdn.rstrip('.')
                instance_groups_hosts.append(
                    {
                        'name': fqdn,
                        'compute_instance_id': instance.instance_id,
                        'cluster_id': cluster['cid'],
                        'subcluster_id': subcid,
                        'role': pillar.get_subcluster(subcid)['role'],
                    }
                )
        except InstanceGroupNotFoundError:
            pass
    return instance_groups_hosts


@register_request_handler(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.LIST)
def list_hadoop_hosts(cluster, **kwargs):
    """
    Returns hosts of the Hadoop cluster.
    """
    subcid = parse_filters(kwargs.get('filters', []))
    hosts = get_host_objects(cluster, hadoop_host_extra_formatter, subcid)

    cid = str(cluster['cid'])

    if subcid:
        instance_groups = metadb.get_instance_group(subcid)
    else:
        instance_groups = metadb.get_instance_groups(cluster['cid'])

    if instance_groups:
        instance_groups_hosts = get_instance_groups_hosts(instance_groups, cluster, subcid)
        for instance_groups_host in instance_groups_hosts:
            hosts.append(instance_groups_host)

    fqdns = [host['name'] for host in hosts]
    hosts_health = dataproc_manager_client().hosts_health(cid, fqdns)

    for host in hosts:
        fqdn = host['name']
        health = hosts_health.get(fqdn)
        if health:
            host['health'] = health

    return {'hosts': hosts}

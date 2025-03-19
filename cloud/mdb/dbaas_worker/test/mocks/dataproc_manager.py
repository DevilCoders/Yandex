"""
Simple dataproc-manager mock
"""

import time

from yandex.cloud.priv.dataproc.manager.v1 import manager_service_pb2

from .utils import handle_action


def decommission(state, request, timeout):
    """
    Decommission mock
    """
    action_id = f'dataproc-manager-decommission-{request.cid}'
    handle_action(state, action_id)
    for host in request.yarn_hosts:
        state['dataproc-manager'][host] = manager_service_pb2.DECOMMISSIONED
    for host in state['dataproc-manager'].copy():
        if host not in request.yarn_hosts:
            del state['dataproc-manager'][host]

    return manager_service_pb2.DecommissionReply()


def cluster_health(state, request):
    """
    Cluster health mock
    """
    action_id = f'dataproc-manager-cluster-health-{request.cid}'
    handle_action(state, action_id)
    res = manager_service_pb2.ClusterHealthReply()
    res.cid = request.cid
    res.health = manager_service_pb2.ALIVE
    res.update_time = int(time.time()) + 1
    return res


def hosts_health(state, request):
    """
    Hosts health mock
    """
    action_id = f'dataproc-manager-hosts-health-{request.cid}'
    handle_action(state, action_id)
    res = manager_service_pb2.HostsHealthReply()
    instances = {i['fqdn'] for i in state['compute']['instances'].values()}
    for host in request.fqdns:
        host_health = res.hosts_health.add()
        host_health.fqdn = host
        if host not in instances:
            host_health.health = manager_service_pb2.HEALTH_UNSPECIFIED
        elif host in state['dataproc-manager']:
            host_health.health = state['dataproc-manager'][host]
        else:
            host_health.health = manager_service_pb2.ALIVE

        for service in [manager_service_pb2.YARN, manager_service_pb2.HDFS]:
            service_health = host_health.service_health.add()
            service_health.service = service
            service_health.health = host_health.health

    return res


def dataproc_manager(mocker, state):
    """
    Setup dataproc-manager mock
    """
    stub = mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.dataproc_manager.'
        'manager_service_pb2_grpc.DataprocManagerServiceStub'
    )
    stub.return_value.Decommission.side_effect = lambda request, timeout: decommission(state, request, timeout)
    stub.return_value.ClusterHealth.side_effect = lambda request: cluster_health(state, request)
    stub.return_value.HostsHealth.side_effect = lambda request: hosts_health(state, request)

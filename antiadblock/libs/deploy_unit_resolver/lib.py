import socket
from retry import retry

from infra.yp_service_discovery.python.resolver.resolver import Resolver
from infra.yp_service_discovery.api import api_pb2


# 2000 ~ 2 seconds
@retry(tries=5, delay=2, backoff=2, jitter=(0, 2), max_delay=30)
def get_deploy_unit_endpoints(deploy_stage, deploy_unit, cluster):
    resolver = Resolver(client_name='test:{}'.format(socket.gethostname()), timeout=5)
    request = api_pb2.TReqResolveEndpoints()
    request.cluster_name = cluster
    request.endpoint_set_id = '.'.join([deploy_stage, deploy_unit])
    response = resolver.resolve_endpoints(request)
    endpoints = sorted(response.endpoint_set.endpoints, key=lambda x: x.fqdn)
    return endpoints


def get_endpoints(deploy_stage, deploy_units, clusters):
    endpoints = []
    for unit in deploy_units:
        for cluster in clusters:
            endpoints += get_deploy_unit_endpoints(deploy_stage, unit, cluster)
    return endpoints


def get_fqdns(deploy_stage, deploy_units, clusters):
    return [ep.fqdn for ep in get_endpoints(deploy_stage, deploy_units, clusters)]


def get_ips(deploy_stage, deploy_units, clusters):
    return [ep.ip6_address for ep in get_endpoints(deploy_stage, deploy_units, clusters)]

"""
gRPC client to dataproc-manager
"""

import os
from typing import Dict, List

import grpc

from dbaas_common import retry
from yandex.cloud.priv.dataproc.manager.v1 import manager_service_pb2, manager_service_pb2_grpc

from ...modules.hadoop.traits import HostHealth
from ..types import ClusterHealth
from .health import ExtendedClusterHealth
from dbaas_common.tracing import grpc_channel_tracing_interceptor

STATUS_CONVERTER = {
    manager_service_pb2.HEALTH_UNSPECIFIED: ClusterHealth.unknown,
    manager_service_pb2.ALIVE: ClusterHealth.alive,
    manager_service_pb2.DEAD: ClusterHealth.dead,
    manager_service_pb2.DEGRADED: ClusterHealth.degraded,
}

HOST_STATUS_CONVERTER = {
    manager_service_pb2.HEALTH_UNSPECIFIED: HostHealth.unknown,
    manager_service_pb2.ALIVE: HostHealth.alive,
    manager_service_pb2.DEAD: HostHealth.dead,
    manager_service_pb2.DEGRADED: HostHealth.degraded,
}


def extract_code(rpc_error):
    """
    Extract grpc.StatusCode from grpc.RpcError
    """
    code = None
    # Workaround for too general error class in gRPC
    if hasattr(rpc_error, 'code') and callable(rpc_error.code):  # pylint: disable=E1101
        code = rpc_error.code()  # pylint: disable=E1101
    return code


def giveup(rpc_error):
    """
    Defines whether to retry operation on exception or give up
    """
    code = extract_code(rpc_error)
    if code == grpc.StatusCode.UNAVAILABLE:
        return False
    return True


class DataprocManagerClient:
    """
    gRPC client to dataproc-manager
    """

    def __init__(self, config) -> None:
        url = config['private_url']
        creds = self._get_ssl_creds(config)
        if config.get('insecure', False):
            channel = grpc.insecure_channel(url)
        else:
            server_name = config.get('server_name')
            options = [
                ('grpc.keepalive_time_ms', config.get('grpc_keepalive_time_ms', 11000)),
                ('grpc.keepalive_timeout_ms', config.get('grpc_keepalive_timeout_ms', 1000)),
                ('grpc.keepalive_permit_without_calls', config.get('grpc_keepalive_permit_without_calls', True)),
                ('grpc.http2.max_pings_without_data', config.get('grpc_max_pings_without_data', 0)),
            ]
            if server_name:
                options.append(('grpc.ssl_target_name_override', server_name))
            channel = grpc.secure_channel(url, creds, options)
        channel = grpc_channel_tracing_interceptor(channel)
        self.stub = manager_service_pb2_grpc.DataprocManagerServiceStub(channel)

    def _get_ssl_creds(self, config):
        cert_file = config.get('cert_file')
        if not cert_file or not os.path.exists(cert_file):
            return grpc.ssl_channel_credentials()
        with open(cert_file, 'rb') as file_handler:
            certs = file_handler.read()
        return grpc.ssl_channel_credentials(root_certificates=certs)

    @retry.on_exception((grpc.RpcError,), giveup=giveup, max_tries=3)
    def cluster_health(self, cid: str) -> ExtendedClusterHealth:
        """
        Returns health info of cluster and explanation if cluster is not Alive
        """
        req = manager_service_pb2.ClusterHealthRequest()
        req.cid = cid
        try:
            resp = self.stub.ClusterHealth(req)
            return ExtendedClusterHealth(
                health=STATUS_CONVERTER[resp.health],
                explanation=resp.explanation,
                hdfs_in_safemode=resp.hdfs_in_safemode,
            )
        except grpc.RpcError as err:
            code = extract_code(err)
            if code == grpc.StatusCode.NOT_FOUND:
                return ExtendedClusterHealth(
                    health=ClusterHealth.unknown,
                    explanation="there's no health report delivered from Data Proc Agent."
                    " Probably cluster doesn't have internet access.",
                    hdfs_in_safemode=False,
                )
            raise

    @retry.on_exception((grpc.RpcError,), giveup=giveup, max_tries=3)
    def hosts_health(self, cid: str, fqdns: List[str]) -> Dict[str, HostHealth]:
        """
        Returns health info of cluster's hosts
        """
        req = manager_service_pb2.HostsHealthRequest()
        req.cid = cid
        # pylint: disable=E1101
        req.fqdns.extend(fqdns)
        try:
            resp = self.stub.HostsHealth(req)
            return {host.fqdn: HOST_STATUS_CONVERTER[host.health] for host in resp.hosts_health}
        except grpc.RpcError as err:
            code = extract_code(err)
            if code == grpc.StatusCode.NOT_FOUND:
                return {}
            raise

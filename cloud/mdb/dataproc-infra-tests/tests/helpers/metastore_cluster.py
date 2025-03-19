import logging
import time

import grpc
from google.protobuf import json_format
from tests.helpers.step_helpers import (
    get_step_data,
    apply_overrides_to_cluster_config,
)
from yandex.cloud.priv.metastore.v1 import (
    cluster_service_pb2,
    cluster_service_pb2_grpc,
)

from tests.helpers.base_cluster import BaseCluster
from tests.helpers.metastore_payload import test_metastore
from tests.helpers import utils

LOG = logging.getLogger('metastore_cluster')


class MetastoreCluster(BaseCluster):
    def __init__(self, context):
        super().__init__(context)
        self.infratest_hostname = context.conf['compute_driver']['fqdn']
        self.username = 'root'
        host = context.conf['projects']['mdb_internal_api']['host']
        port = context.conf['projects']['mdb_internal_api']['port']
        self.url = f'{host}:{port}'
        self.channel = grpc.insecure_channel(self.url)
        self.cluster_service = cluster_service_pb2_grpc.ClusterServiceStub(self.channel)

    def load_cluster_into_context(self, cluster_id=None, name=None):
        if not cluster_id:
            if not name:
                raise ValueError('Load into context must specify name or cid')
            clusters = self.list_clusters()
            for cluster in clusters:
                if cluster['name'] == name:
                    cluster_id = cluster['id']
        self.context.cid = cluster_id
        if not cluster_id:
            raise ValueError('Cluster not found by name: ', name)
        self.context.cluster = self.get_cluster(cluster_id)

    def get_cluster(self, cluster_id, rewrite_context_response=False):
        request = cluster_service_pb2.GetClusterRequest(cluster_id=cluster_id)
        response = self._make_request(self.cluster_service.Get, request, rewrite_context_response)
        return json_format.MessageToDict(response)

    def list_clusters(self, folder_id: str = None) -> list:
        if not folder_id:
            folder_id = self.context.folder['folder_ext_id']
        request = cluster_service_pb2.ListClustersRequest(folder_id=folder_id)
        response = self._make_request(self.cluster_service.List, request, rewrite_context_with_response=False)
        return json_format.MessageToDict(response).get('clusters', [])

    def create_cluster(self, cluster_name):
        cluster_config = apply_overrides_to_cluster_config(self.context.cluster_config, get_step_data(self.context))
        cluster_config['name'] = cluster_name
        cluster_config['folderId'] = self.context.folder['folder_ext_id']

        request = cluster_service_pb2.CreateClusterRequest()
        json_format.ParseDict(cluster_config, request)
        response = self._make_request(self.cluster_service.Create, request)
        json_data = json_format.MessageToDict(response)
        self.context.operation_id = response.id
        self.context.cid = json_data['metadata']['clusterId']
        self.context.cluster = self.get_cluster(self.context.cid, rewrite_context_response=False)

        for _ in range(5):
            try:
                self.load_cluster_into_context(cluster_id=self.context.cid)
                return
            except Exception:
                pass
            time.sleep(1)

        return response

    def delete_cluster(self):
        request = cluster_service_pb2.DeleteClusterRequest(cluster_id=self.context.cid)
        return self._make_request(self.cluster_service.Delete, request)

    def test_metastore(self):
        if not hasattr(self.context, 'cluster'):
            self.load_cluster_into_context(name='test_cluster')

        metastore_endpoint_port = 9083
        tunnel_port = 27018
        gateway_hostname = self.context.conf['compute_driver']['fqdn']
        metastore_endpoint = self.context.cluster["endpointIp"]
        # for some reason open_tunnel (and ssh port forwarding from cli)
        # doesn't work -- established TCP connection immediately resets.
        # That is why socat is used (port 27018 is open in puncher for _DATAPROC_INFRA_TEST_)
        utils.ssh_async(
            gateway_hostname,
            [
                'pkill socat ;'
                f' socat TCP6-LISTEN:{tunnel_port},fork,reuseaddr TCP:{metastore_endpoint}:{metastore_endpoint_port}'
            ],
        )

        # wait tunnel availability
        timeout_seconds = 5
        deadline = time.time() + timeout_seconds
        while time.time() < deadline:
            return_code, stdout, stderr = utils.ssh(gateway_hostname, [f'ss -ntlp | grep {tunnel_port}'])
            if return_code == 0 and stdout.strip():
                break
            time.sleep(0.1)

        try:
            result = test_metastore(
                metastore_hostname=gateway_hostname,
                metastore_port=tunnel_port,
            )
        finally:
            utils.ssh(gateway_hostname, ['pkill socat || true'])

        return result

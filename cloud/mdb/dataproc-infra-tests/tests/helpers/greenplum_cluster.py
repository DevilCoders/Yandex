"""
Utilities for dealing with Greenplum cluster
"""

import logging
import time

import grpc
from google.protobuf import json_format


from tests.helpers.step_helpers import (
    fill_update_mask,
    get_step_data,
    apply_overrides_to_cluster_config,
)
from yandex.cloud.priv.mdb.greenplum.v1 import (
    cluster_service_pb2,
    cluster_service_pb2_grpc,
)

from tests.helpers.base_cluster import BaseCluster


LOG = logging.getLogger('greenplum_cluster')


class GreenplumCluster(BaseCluster):
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

    def update_cluster(self):
        cluster_config = get_step_data(self.context)
        if 'cluster_id' not in cluster_config:
            cluster_config['cluster_id'] = self.context.cid

        request = cluster_service_pb2.UpdateClusterRequest()
        fill_update_mask(request, cluster_config)
        json_format.ParseDict(cluster_config, request)
        return self._make_request(self.cluster_service.Update, request)

    def delete_cluster(self):
        request = cluster_service_pb2.DeleteClusterRequest(cluster_id=self.context.cid)
        return self._make_request(self.cluster_service.Delete, request)

    def add_hosts(self):
        cluster_config = get_step_data(self.context)
        if 'cluster_id' not in cluster_config:
            cluster_config['cluster_id'] = self.context.cid

        request = cluster_service_pb2.AddClusterHostsRequest()
        json_format.ParseDict(cluster_config, request)
        return self._make_request(self.cluster_service.AddHosts, request)

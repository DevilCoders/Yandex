"""
Utilities for dealing with SQL Server cluster
"""

import copy
import datetime
from google.protobuf import json_format

from yandex.cloud.priv.mdb.sqlserver.v1 import (
    cluster_service_pb2,
    cluster_service_pb2_grpc,
    database_service_pb2,
    database_service_pb2_grpc,
    user_service_pb2,
    user_service_pb2_grpc,
)
from tests.helpers.base_cluster import BaseCluster
from tests.helpers.go_internal_api import snake_case_to_camel_case, get_wrapped_service
from tests.helpers.step_helpers import (
    fill_update_mask,
    get_controlplane_time,
    get_step_data,
    apply_overrides_to_cluster_config,
)
from tests.helpers.utils import merge


class SqlServerCluster(BaseCluster):
    def __init__(self, context):
        super().__init__(context)
        self.cluster_service = get_wrapped_service(context, cluster_service_pb2_grpc.ClusterServiceStub)
        self.database_service = get_wrapped_service(context, database_service_pb2_grpc.DatabaseServiceStub)
        self.user_service = get_wrapped_service(context, user_service_pb2_grpc.UserServiceStub)

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

    def load_cluster_hosts_into_context(self, cluster_id=None):
        if not cluster_id:
            cluster_id = self.context.cid
            if not cluster_id:
                raise ValueError('Load into context must specify cid')
        self.context.hosts = self.list_hosts(cluster_id)

    def restore_default_args(self, backup_id='LATEST', pitr_time=None):
        if backup_id == 'LATEST':
            backups = self.list_backups(self.context.cid)
            backup_id = backups[0]['id']
        if pitr_time is None:
            pitr_time = get_controlplane_time(self.context)
        if isinstance(pitr_time, (float, int)):
            pitr_time = datetime.datetime.fromtimestamp(pitr_time).astimezone(datetime.timezone.utc).isoformat()
        return backup_id, pitr_time

    def get_cluster(self, cluster_id, rewrite_context_response=False):
        request = cluster_service_pb2.GetClusterRequest(cluster_id=cluster_id)
        response = self._make_request(self.cluster_service.Get, request, rewrite_context_response)
        return json_format.MessageToDict(response)

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

    def restore_cluster(self, backup_id='LATEST', pitr_time=None):
        # prepare config:
        restore_arguments = get_step_data(self.context)
        default_cluster_config = self.context.conf['test_cluster_configs']['sqlserver'][self.context.config_type]
        cluster_template = merge(copy.deepcopy(default_cluster_config), copy.deepcopy(restore_arguments))
        backup_id, pitr_time = self.restore_default_args(backup_id, pitr_time)

        cluster_config = {
            'backupId': backup_id,
            'time': pitr_time,
            'folderId': self.context.folder['folder_ext_id'],
        }
        # copy only supported in Restore operation fields:
        for field in [
            'backup_id',
            'time',
            'name',
            'description',
            'labels',
            'environment',
            'config_spec',
            'host_specs',
            'network_id',
            'folder_id',
            'security_group_ids',
            'deletion_protection',
            'host_group_ids',
        ]:
            camel_case_field = snake_case_to_camel_case(field, first_lower=True)
            if field in cluster_template:
                cluster_config[camel_case_field] = cluster_template[field]
            if camel_case_field in cluster_template:
                cluster_config[camel_case_field] = cluster_template[camel_case_field]
        request = cluster_service_pb2.RestoreClusterRequest()
        json_format.ParseDict(cluster_config, request)
        response = self._make_request(self.cluster_service.Restore, request)
        json_data = json_format.MessageToDict(response)
        self.context.operation_id = response.id
        self.context.cid = json_data['metadata']['clusterId']

    def list_clusters(self, folder_id=None) -> list:
        if not folder_id:
            folder_id = self.context.folder['folder_ext_id']
        request = cluster_service_pb2.ListClustersRequest(folder_id=folder_id)
        response = self._make_request(self.cluster_service.List, request, rewrite_context_with_response=False)
        return json_format.MessageToDict(response).get('clusters', [])

    def list_hosts(self, cluster_id=None) -> list:
        if not cluster_id:
            cluster_id = self.context.cid
        request = cluster_service_pb2.ListClusterHostsRequest(cluster_id=cluster_id)
        response = self._make_request(self.cluster_service.ListHosts, request, rewrite_context_with_response=False)
        return json_format.MessageToDict(response).get('hosts', [])

    def create_backup(self, cluster_id: str):
        request = cluster_service_pb2.BackupClusterRequest(cluster_id=cluster_id)
        # response is Operation
        return self._make_request(self.cluster_service.Backup, request)

    def list_backups(self, cluster_id: str) -> list:
        # There are 2 ways to list backups:
        # * BackupService - works with folder_id
        # * ClusterService - works with cluster_id
        request = cluster_service_pb2.ListClusterBackupsRequest(cluster_id=cluster_id)
        response = self._make_request(self.cluster_service.ListBackups, request, rewrite_context_with_response=False)
        # response is ListClusterBackupsResponse
        return json_format.MessageToDict(response).get('backups', [])

    def get_latest_backup(self, cluster_id: str):
        backups = self.list_backups(cluster_id)
        return backups[0]

    def failover(self, cluster_id: str, host: str):
        request = cluster_service_pb2.StartClusterFailoverRequest(cluster_id=cluster_id, host_name=host)
        return self._make_request(self.cluster_service.StartFailover, request)

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

    def export_database_backup(self, req):
        if 'cluster_id' not in req:
            req['cluster_id'] = self.context.cid
        request = database_service_pb2.ExportDatabaseBackupRequest()
        json_format.ParseDict(req, request)
        return self._make_request(self.database_service.ExportBackup, request)

    def import_database_backup(self, req):
        if 'cluster_id' not in req:
            req['cluster_id'] = self.context.cid
        request = database_service_pb2.ImportDatabaseBackupRequest()
        json_format.ParseDict(req, request)
        return self._make_request(self.database_service.ImportBackup, request)

    def create_database(self, dbname):
        req = {'cluster_id': self.context.cid, 'database_spec': {'name': dbname}}
        request = database_service_pb2.CreateDatabaseRequest()
        json_format.ParseDict(req, request)
        return self._make_request(self.database_service.Create, request)

    def restore_database(self, dbname, fromname, backup_id='LATEST', pitr_time=None):
        backup_id, pitr_time = self.restore_default_args(backup_id, pitr_time)
        req = {
            'cluster_id': self.context.cid,
            'database_name': dbname,
            'from_database': fromname,
            'backup_id': backup_id,
            'time': pitr_time,
        }
        request = database_service_pb2.RestoreDatabaseRequest()
        json_format.ParseDict(req, request)
        return self._make_request(self.database_service.Restore, request)

    def delete_database(self, dbname):
        req = {
            'cluster_id': self.context.cid,
            'database_name': dbname,
        }
        request = database_service_pb2.DeleteDatabaseRequest()
        json_format.ParseDict(req, request)
        return self._make_request(self.database_service.Delete, request)

    def create_user(self, user_name, password, db_name=None, roles=None):
        req = {
            'cluster_id': self.context.cid,
            'user_spec': {
                "name": user_name,
                "password": password,
                "permissions": [],
                'server_roles': ['MDB_MONITOR'],
            },
        }
        if db_name and roles:
            req['user_spec']['permissions'].append(
                {
                    "database_name": db_name,
                    "roles": roles or [],
                }
            )
        request = user_service_pb2.CreateUserRequest()
        json_format.ParseDict(req, request)
        return self._make_request(self.user_service.Create, request)

    def delete_user(self, user_name):
        req = {
            'cluster_id': self.context.cid,
            'user_name': user_name,
        }
        request = user_service_pb2.DeleteUserRequest()
        json_format.ParseDict(req, request)
        return self._make_request(self.user_service.Delete, request)

    def grant_permission(self, user_name, db_name, roles):
        req = {
            'cluster_id': self.context.cid,
            'user_name': user_name,
            'permission': {
                'database_name': db_name,
                'roles': roles,
            },
        }
        request = user_service_pb2.GrantUserPermissionRequest()
        json_format.ParseDict(req, request)
        return self._make_request(self.user_service.GrantPermission, request)

    def revoke_permission(self, user_name, db_name, roles):
        req = {
            'cluster_id': self.context.cid,
            'user_name': user_name,
            'permission': {
                'database_name': db_name,
                'roles': roles,
            },
        }
        request = user_service_pb2.RevokeUserPermissionRequest()
        json_format.ParseDict(req, request)
        return self._make_request(self.user_service.RevokePermission, request)

"""
Utilities for dealing with Go Internal API
"""
import grpc
from google.protobuf import field_mask_pb2
from google.protobuf.json_format import MessageToDict, ParseDict

from tests.helpers import docker
from tests.helpers.internal_api import get_iam_token
from yandex.cloud.priv.mdb.clickhouse.v1 import cluster_service_pb2 as ch_specs
from yandex.cloud.priv.mdb.clickhouse.v1 import \
    database_service_pb2 as ch_database_specs
from yandex.cloud.priv.mdb.clickhouse.v1 import \
    format_schema_service_pb2 as ch_format_schema_specs
from yandex.cloud.priv.mdb.clickhouse.v1 import \
    ml_model_service_pb2 as ch_ml_model_specs
from yandex.cloud.priv.mdb.clickhouse.v1 import \
    user_service_pb2 as ch_user_specs
from yandex.cloud.priv.mdb.clickhouse.v1.cluster_service_pb2_grpc import \
    ClusterServiceStub as CHClusterServiceStub
from yandex.cloud.priv.mdb.clickhouse.v1.database_service_pb2_grpc import \
    DatabaseServiceStub as CHDatabaseServiceStub
from yandex.cloud.priv.mdb.clickhouse.v1.format_schema_service_pb2_grpc import \
    FormatSchemaServiceStub as CHFormatSchemaServiceStub
from yandex.cloud.priv.mdb.clickhouse.v1.ml_model_service_pb2_grpc import \
    MlModelServiceStub as CHMlModelServiceStub
from yandex.cloud.priv.mdb.clickhouse.v1.user_service_pb2_grpc import \
    UserServiceStub as CHUserServiceStub
from yandex.cloud.priv.mdb.elasticsearch.v1 import \
    cluster_service_pb2 as es_specs
from yandex.cloud.priv.mdb.elasticsearch.v1 import \
    user_service_pb2 as es_user_specs
from yandex.cloud.priv.mdb.elasticsearch.v1.cluster_service_pb2_grpc import \
    ClusterServiceStub as ESClusterServiceStub
from yandex.cloud.priv.mdb.elasticsearch.v1.console import \
    cluster_service_pb2 as es_console_specs  # pylint: disable=import-error, no-name-in-module
from yandex.cloud.priv.mdb.elasticsearch.v1.console.cluster_service_pb2_grpc import \
    ClusterServiceStub as ESConsoleClusterServiceStub
from yandex.cloud.priv.mdb.elasticsearch.v1.user_service_pb2_grpc import \
    UserServiceStub as ESUserServiceStub
from yandex.cloud.priv.mdb.greenplum.v1 import cluster_service_pb2 as gp_specs
from yandex.cloud.priv.mdb.greenplum.v1.cluster_service_pb2_grpc import \
    ClusterServiceStub as GPClusterServiceStub
from yandex.cloud.priv.mdb.mongodb.v1 import \
    cluster_service_pb2 as mongodb_specs
from yandex.cloud.priv.mdb.mongodb.v1.cluster_service_pb2_grpc import \
    ClusterServiceStub as MongoDBClusterServiceStub
from yandex.cloud.priv.mdb.opensearch.v1 import cluster_service_pb2 as os_specs
from yandex.cloud.priv.mdb.opensearch.v1.cluster_service_pb2_grpc import \
    ClusterServiceStub as OSClusterServiceStub
from yandex.cloud.priv.mdb.redis.v1 import \
    backup_service_pb2 as redis_backup_specs
from yandex.cloud.priv.mdb.redis.v1 import \
    cluster_service_pb2 as redis_cluster_specs
from yandex.cloud.priv.mdb.redis.v1.backup_service_pb2_grpc import \
    BackupServiceStub as RedisBackupServiceStub
from yandex.cloud.priv.mdb.redis.v1.cluster_service_pb2_grpc import \
    ClusterServiceStub as RedisClusterServiceStub
from yandex.cloud.priv.mdb.v1 import operation_service_pb2 as op_spec
from yandex.cloud.priv.mdb.v1.operation_service_pb2_grpc import \
    OperationServiceStub

GRPC_DEFS = {
    'ClickHouse': {
        'cluster_service': {
            'stub': CHClusterServiceStub,
            'requests': {
                'Create': ch_specs.CreateClusterRequest,
                'Delete': ch_specs.DeleteClusterRequest,
                'Backup': ch_specs.BackupClusterRequest,
                'ListBackups': ch_specs.ListClusterBackupsRequest,
                'Restore': ch_specs.RestoreClusterRequest,
                'AddHosts': ch_specs.AddClusterHostsRequest,
                'DeleteHosts': ch_specs.DeleteClusterHostsRequest,
                'CreateShardGroup': ch_specs.CreateClusterShardGroupRequest,
                'UpdateShardGroup': ch_specs.UpdateClusterShardGroupRequest,
                'DeleteShardGroup': ch_specs.DeleteClusterShardGroupRequest,
                'get': ch_specs.GetClusterRequest,
                'list': ch_specs.ListClustersRequest,
                'list_hosts': ch_specs.ListClusterHostsRequest,
                'AddShard': ch_specs.AddClusterShardRequest,
                'DeleteShard': ch_specs.DeleteClusterShardRequest,
                'CreateExternalDictionary': ch_specs.CreateClusterExternalDictionaryRequest,
                'UpdateExternalDictionary': ch_specs.UpdateClusterExternalDictionaryRequest,
                'DeleteExternalDictionary': ch_specs.DeleteClusterExternalDictionaryRequest,
            },
        },
        'user_service': {
            'stub': CHUserServiceStub,
            'requests': {
                'Create': ch_user_specs.CreateUserRequest,
                'Update': ch_user_specs.UpdateUserRequest,
                'Delete': ch_user_specs.DeleteUserRequest,
                'GrantPermission': ch_user_specs.GrantUserPermissionRequest,
            },
        },
        'database_service': {
            'stub': CHDatabaseServiceStub,
            'requests': {
                'Create': ch_database_specs.CreateDatabaseRequest,
                'Delete': ch_database_specs.DeleteDatabaseRequest,
            },
        },
        'ml_model_service': {
            'stub': CHMlModelServiceStub,
            'requests': {
                'Create': ch_ml_model_specs.CreateMlModelRequest,
                'Update': ch_ml_model_specs.UpdateMlModelRequest,
                'Delete': ch_ml_model_specs.DeleteMlModelRequest,
            },
        },
        'format_schema_service': {
            'stub': CHFormatSchemaServiceStub,
            'requests': {
                'Create': ch_format_schema_specs.CreateFormatSchemaRequest,
                'Update': ch_format_schema_specs.UpdateFormatSchemaRequest,
                'Delete': ch_format_schema_specs.DeleteFormatSchemaRequest,
            },
        },
    },
    'elasticsearch': {
        'cluster_service': {
            'stub': ESClusterServiceStub,
            'requests': {
                'Create': es_specs.CreateClusterRequest,
                'Delete': es_specs.DeleteClusterRequest,
                'get': es_specs.GetClusterRequest,
                'list': es_specs.ListClustersRequest,
                'list_hosts': es_specs.ListClusterHostsRequest,
                'Update': es_specs.UpdateClusterRequest,
                'AddHosts': es_specs.AddClusterHostsRequest,
                'DeleteHosts': es_specs.DeleteClusterHostsRequest,
                'ListBackups': es_specs.ListClusterBackupsRequest,  # pylint: disable=no-member
                'Backup': es_specs.BackupClusterRequest,
                'Restore': es_specs.RestoreClusterRequest,
            },
        },
        'user_service': {
            'stub': ESUserServiceStub,
            'requests': {
                'Create': es_user_specs.CreateUserRequest,
                'Delete': es_user_specs.DeleteUserRequest,
                'Update': es_user_specs.UpdateUserRequest,
            },
        },
        'console_service': {
            'stub': ESConsoleClusterServiceStub,
            'requests': {
                'Get': es_console_specs.GetElasticsearchClustersConfigRequest,
            },
        },
    },
    'opensearch': {
        'cluster_service': {
            'stub': OSClusterServiceStub,
            'requests': {
                'Create': os_specs.CreateClusterRequest,
                'Delete': os_specs.DeleteClusterRequest,
                'Update': os_specs.UpdateClusterRequest,
                'get': os_specs.GetClusterRequest,
                'list': os_specs.ListClustersRequest,
                'list_hosts': os_specs.ListClusterHostsRequest,
                'AddHosts': os_specs.AddClusterHostsRequest,
                'DeleteHosts': os_specs.DeleteClusterHostsRequest,
            },
        },
    },
    'mongodb': {
        'cluster_service': {
            'stub': MongoDBClusterServiceStub,
            'requests': {
                'resetup_hosts': mongodb_specs.ResetupHostsRequest,
                'stepdown_hosts': mongodb_specs.StepdownHostsRequest,  # pylint: disable=no-member
            },
        },
    },
    'redis': {
        'backup_service': {
            'stub': RedisBackupServiceStub,
            'requests': {
                'List': redis_backup_specs.ListBackupsRequest,
                'Get': redis_backup_specs.GetBackupRequest,
            },
        },
        'cluster_service': {
            'stub': RedisClusterServiceStub,
            'requests': {
                'StartFailover': redis_cluster_specs.StartClusterFailoverRequest,
                'ListHosts': redis_cluster_specs.ListClusterHostsRequest,
                'Rebalance': redis_cluster_specs.RebalanceClusterRequest,
                'ListBackups': redis_cluster_specs.ListClusterBackupsRequest,
                'Backup': redis_cluster_specs.BackupClusterRequest,
            },
        },
    },
    'Greenplum': {
        'cluster_service': {
            'stub': GPClusterServiceStub,
            'requests': {
                'Create': gp_specs.CreateClusterRequest,
                'Delete': gp_specs.DeleteClusterRequest,
                'get': gp_specs.GetClusterRequest,
                'list': gp_specs.ListClustersRequest,
                'list_hosts': gp_specs.ListClusterHostsRequest,
                'Update': gp_specs.UpdateClusterRequest,
                'ListBackups': gp_specs.ListClusterBackupsRequest,
                'Restore': gp_specs.RestoreClusterRequest,
            },
        },
    },
}


class GoInternalAPIError(RuntimeError):
    """
    General Go API error exception
    """


def init_service_and_message(cluster_type, method, channel, data, service='cluster_service'):
    """
    Instantiate grpc service and message.
    """
    srv = GRPC_DEFS[cluster_type][service]['stub'](channel)
    msg_cls = GRPC_DEFS[cluster_type][service]['requests'][method]
    # dont use json field mask syntax as it does not allow fields like elasticsearch_config_7
    # https://developers.google.com/protocol-buffers/docs/reference/java/com/google/protobuf/FieldMask
    if 'update_mask' in data:
        update_mask = field_mask_pb2.FieldMask(**data['update_mask'])
        msg = msg_cls(update_mask=update_mask)
        del data['update_mask']
    else:
        msg = msg_cls()
    print('parsing dict {} to msg {}'.format(data, msg))
    ParseDict(data, msg)
    return srv, msg


def msg_to_dict(msg):
    """
    Convert protobuf message to dict.
    """
    return MessageToDict(msg, including_default_value_fields=True, preserving_proto_field_name=True)


def check_response(resp, error, key=None):
    """
    Check grpc response.
    """

    def _check_msg():
        try:
            return resp['response']['message'] == 'OK'
        except KeyError:
            return False

    pred = key and key in resp
    if not (pred or _check_msg()):
        raise GoInternalAPIError(error + ':{0}'.format(resp))


def get_channel(context):
    """
    Get gRPC connection to Go MDB Internal API.
    """
    host, port = docker.get_exposed_port(
        docker.get_container(context, 'go-internal-api01'),
        context.conf['projects']['go-internal-api']['expose']['grpc'])
    return grpc.insecure_channel(f'{host}:{port}')


def get_metadata(context):
    """
    Get gRPC authorization metadata.
    """
    return [('authorization', f'Bearer {get_iam_token(context)}')]


def get_hosts(context, cluster_type, cluster_id):
    """
    Get cluster hosts.
    """
    data = {'cluster_id': cluster_id, 'page_size': 100}
    with get_channel(context) as channel:
        srv, msg = init_service_and_message(cluster_type, 'list_hosts', channel, data)
        if cluster_type == 'Greenplum':
            resp = srv.ListMasterHosts(msg, metadata=get_metadata(context))
        else:
            resp = srv.ListHosts(msg, metadata=get_metadata(context))

    resp = msg_to_dict(resp)
    check_response(resp, 'could not list hosts', key='hosts')

    return resp['hosts']


def get_cluster(context, cluster_type, cluster_name, folder_id):
    """
    Get cluster by name.
    """
    cluster_id = _get_cluster_id(context, cluster_type, cluster_name, folder_id)
    data = {'cluster_id': cluster_id}
    with get_channel(context) as channel:
        srv, msg = init_service_and_message(cluster_type, 'get', channel, data)
        resp = srv.Get(msg, metadata=get_metadata(context))

    resp = msg_to_dict(resp)
    check_response(resp, 'Failed to get cluster', key='id')

    return resp


def _get_cluster_id(context, cluster_type, cluster_name, folder_id):
    """
    Get cluster id by name.
    """
    for cluster in get_clusters(context, cluster_type, folder_id):
        if cluster['name'] == cluster_name:
            return cluster['id']

    raise GoInternalAPIError('cluster {0} not found'.format(cluster_name))


def get_clusters(context, cluster_type, folder_id):
    """
    Get clusters.
    """
    data = {'folder_id': folder_id, 'page_size': 100}
    with get_channel(context) as channel:
        srv, msg = init_service_and_message(cluster_type, 'list', channel, data)
        resp = srv.List(msg, metadata=get_metadata(context))

    resp = msg_to_dict(resp)
    check_response(resp, 'could not list clusters', key='clusters')

    return resp['clusters']


def load_cluster_into_context(context, cluster_type, cluster_name=None, with_hosts=True):
    """
    Reload cluster with related objects and update corresponding context data.
    """
    if not cluster_name:
        cluster_name = context.cluster['name']

    context.cluster = get_cluster(context, cluster_type, cluster_name, context.folder['folder_ext_id'])

    if with_hosts:
        context.hosts = get_hosts(context, cluster_type, context.cluster['id'])


def get_task(context):
    """
    Get current task
    """
    msg = op_spec.GetOperationRequest()
    ParseDict({'operation_id': context.operation_id}, msg)

    with get_channel(context) as channel:
        op_service = OperationServiceStub(channel)
        resp = op_service.Get(msg, metadata=get_metadata(context))

    return msg_to_dict(resp)


def load_cluster_type_info_into_context(context, cluster_type):
    """
    Load cluster type information and update corresponding context data.
    """
    if 'console_service' not in GRPC_DEFS.get(cluster_type, {}):
        return

    data = {'folder_id': context.folder['folder_ext_id']}
    with get_channel(context) as channel:
        srv, msg = init_service_and_message(cluster_type, 'Get', channel, data, service='console_service')
        resp = srv.Get(msg, metadata=get_metadata(context))

    resp = msg_to_dict(resp)
    check_response(resp, 'could not get cluster config', key='versions')

    context.versions = resp['versions']

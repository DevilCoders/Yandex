# -*- coding: utf-8 -*-
"""
DBaaS Internal API for objects (hosts, databases and users) management.
"""

from flask import g
from flask.views import MethodView

from . import API, marshal, parse_kwargs
from ..core.auth import check_auth
from ..utils import metadb
from ..utils.cluster.get import get_cluster, lock_cluster
from ..utils.idempotence import supports_idempotence
from ..utils.identity import get_folder_by_cluster_id
from ..utils.register import DbaasOperation, Resource, get_request_handler
from ..utils.types import ClusterStatus
from ..utils.validation import check_cluster_not_in_status
from .operations import render_operation_v1
from .schemas.cluster import ListClusterHostsRequestSchemaV1
from .schemas.common import FilteredListRequestSchemaV1, ListRequestSchemaV1, ListRequestLargeOutputSchemaV1
from .schemas.operations import OperationSchemaV1


RESOURCES = {
    Resource.USER: {
        'url': 'users',
        'tag': 'User',
        'id_key': 'user_name',
        'operations': {
            DbaasOperation.INFO,
            DbaasOperation.LIST,
            DbaasOperation.CREATE,
            DbaasOperation.MODIFY,
            DbaasOperation.GRANT_PERMISSION,
            DbaasOperation.REVOKE_PERMISSION,
            DbaasOperation.DELETE,
        },
    },
    Resource.DATABASE: {
        'url': 'databases',
        'tag': 'Database',
        'id_key': 'database_name',
        'operations': {
            DbaasOperation.INFO,
            DbaasOperation.LIST,
            DbaasOperation.CREATE,
            DbaasOperation.MODIFY,
            DbaasOperation.DELETE,
        },
    },
    Resource.ML_MODEL: {
        'url': 'mlModels',
        'tag': 'MlModel',
        'id_key': 'ml_model_name',
        'operations': {
            DbaasOperation.INFO,
            DbaasOperation.LIST,
            DbaasOperation.CREATE,
            DbaasOperation.MODIFY,
            DbaasOperation.DELETE,
        },
    },
    Resource.FORMAT_SCHEMA: {
        'url': 'formatSchemas',
        'tag': 'FormatSchema',
        'id_key': 'format_schema_name',
        'operations': {
            DbaasOperation.INFO,
            DbaasOperation.LIST,
            DbaasOperation.CREATE,
            DbaasOperation.MODIFY,
            DbaasOperation.DELETE,
        },
    },
    Resource.HOST: {
        'url': 'hosts',
        'tag': 'Cluster',
        'id_key': 'host_name',
        'operations': {
            DbaasOperation.LIST,
            DbaasOperation.BATCH_CREATE,
            DbaasOperation.BATCH_MODIFY,
            DbaasOperation.BATCH_DELETE,
        },
        'operations_schemas': {
            DbaasOperation.LIST: ListClusterHostsRequestSchemaV1,
        },
    },
    Resource.SHARD: {
        'url': 'shards',
        'tag': 'Cluster',
        'id_key': 'shard_name',
        'operations': {
            DbaasOperation.INFO,
            DbaasOperation.LIST,
            DbaasOperation.CREATE,
            DbaasOperation.MODIFY,
            DbaasOperation.DELETE,
        },
    },
    Resource.SUBCLUSTER: {
        'url': 'subclusters',
        'tag': 'Cluster',
        'id_key': 'subcluster_name',
        'operations': {
            DbaasOperation.INFO,
            DbaasOperation.LIST,
            DbaasOperation.CREATE,
            DbaasOperation.MODIFY,
            DbaasOperation.DELETE,
        },
        'operations_schemas': {
            DbaasOperation.LIST: ListRequestSchemaV1,
        },
    },
    Resource.HADOOP_JOB: {
        'url': 'jobs',
        'tag': 'Cluster',
        'id_key': 'job_id',
        'operations': {
            DbaasOperation.INFO,
            DbaasOperation.LIST,
            DbaasOperation.CREATE,
            DbaasOperation.CANCEL,
            DbaasOperation.GET_HADOOP_JOB_LOG,
        },
        'operations_schemas': {
            DbaasOperation.LIST: FilteredListRequestSchemaV1,
            DbaasOperation.GET_HADOOP_JOB_LOG: ListRequestLargeOutputSchemaV1,
        },
    },
    Resource.HADOOP_UI_LINK: {
        'url': 'ui_links',
        'tag': 'Cluster',
        'id_key': 'ui_link_id',
        'operations': {
            DbaasOperation.LIST,
        },
        'operations_schemas': {
            DbaasOperation.LIST: ListRequestSchemaV1,
        },
    },
    Resource.CLUSTER: {
        'tag': 'Cluster',
        'operations': {
            DbaasOperation.ENABLE_SHARDING,
            DbaasOperation.ADD_ZOOKEEPER,
            DbaasOperation.CREATE_DICTIONARY,
            DbaasOperation.DELETE_DICTIONARY,
            DbaasOperation.START_FAILOVER,
            DbaasOperation.REBALANCE,
        },
    },
    Resource.ALERT_GROUP: {
        'url': 'alert-group',
        'tag': 'AlertGroup',
        'id_key': 'alert_group_id',
        'operations': {
            DbaasOperation.CREATE,
            DbaasOperation.MODIFY,
            DbaasOperation.DELETE,
        },
    },
}

OPERATIONS = {
    DbaasOperation.INFO: {
        'method': 'get',
        'on_instance': True,
        'status_blacklist': (),
    },
    DbaasOperation.LIST: {
        'method': 'get',
        'on_instance': False,
        'status_blacklist': (),
    },
    DbaasOperation.CREATE: {
        'method': 'post',
        'on_instance': False,
        'status_blacklist': (ClusterStatus.stopped,),
    },
    DbaasOperation.BATCH_CREATE: {
        'method': 'post',
        'on_instance': False,
        'url_suffix': ':batchCreate',
        'status_blacklist': (ClusterStatus.stopped,),
    },
    DbaasOperation.MODIFY: {
        'method': 'patch',
        'on_instance': True,
        'status_blacklist': (ClusterStatus.stopped,),
    },
    DbaasOperation.BATCH_MODIFY: {
        'method': 'post',
        'on_instance': False,
        'url_suffix': ':batchUpdate',
        'status_blacklist': (ClusterStatus.stopped,),
    },
    DbaasOperation.GRANT_PERMISSION: {
        'method': 'post',
        'on_instance': True,
        'url_suffix': ':grantPermission',
        'status_blacklist': (ClusterStatus.stopped,),
    },
    DbaasOperation.REVOKE_PERMISSION: {
        'method': 'post',
        'on_instance': True,
        'url_suffix': ':revokePermission',
        'status_blacklist': (ClusterStatus.stopped,),
    },
    DbaasOperation.DELETE: {
        'method': 'delete',
        'on_instance': True,
        'status_blacklist': (ClusterStatus.stopped,),
    },
    DbaasOperation.BATCH_DELETE: {
        'method': 'post',
        'on_instance': False,
        'url_suffix': ':batchDelete',
        'status_blacklist': (ClusterStatus.stopped,),
    },
    DbaasOperation.ENABLE_SHARDING: {
        'method': 'post',
        'on_instance': True,
        'url_suffix': ':enableSharding',
        'status_blacklist': (ClusterStatus.stopped,),
    },
    DbaasOperation.ADD_ZOOKEEPER: {
        'method': 'post',
        'on_instance': True,
        'url_suffix': ':addZookeeper',
        'status_blacklist': (ClusterStatus.stopped,),
    },
    DbaasOperation.CREATE_DICTIONARY: {
        'method': 'post',
        'on_instance': True,
        'url_suffix': ':createExternalDictionary',
        'status_blacklist': (ClusterStatus.stopped,),
    },
    DbaasOperation.DELETE_DICTIONARY: {
        'method': 'post',
        'on_instance': True,
        'url_suffix': ':deleteExternalDictionary',
        'status_blacklist': (ClusterStatus.stopped,),
    },
    DbaasOperation.START_FAILOVER: {
        'method': 'post',
        'on_instance': True,
        'url_suffix': ':startFailover',
        'status_blacklist': (ClusterStatus.stopped,),
    },
    DbaasOperation.REBALANCE: {
        'method': 'post',
        'on_instance': True,
        'url_suffix': ':rebalance',
        'status_blacklist': (ClusterStatus.stopped,),
    },
    DbaasOperation.GET_HADOOP_JOB_LOG: {
        'method': 'get',
        'on_instance': True,
        'url_suffix': ':logs',
        'status_blacklist': (),
    },
    DbaasOperation.CANCEL: {
        'method': 'post',
        'on_instance': True,
        'url_suffix': ':cancel',
        'status_blacklist': (),
    },
}


def generate_resource_handlers(resources):
    """
    Generates and register API handlers for passed in resource descriptors.
    """
    for resource, options in resources.items():
        for operation in options['operations']:
            add_handler(resource, operation)


def add_handler(resource, operation):
    """
    Generate and register a single API handler.
    """
    resource_options = RESOURCES[resource]
    operation_options = OPERATIONS[operation]
    method = operation_options['method']
    id_key = resource_options.get('id_key')
    child = bool(id_key)

    if method == 'get':
        marshal_deco = marshal.with_resource(resource, operation)
        lock = False
    else:
        marshal_deco = marshal.with_schema(OperationSchemaV1)
        lock = True

    parse_kwargs_deco = None
    if method in ['get', 'delete']:
        schema = resource_options.get('operations_schemas', {}).get(operation)
        if schema:
            parse_kwargs_deco = parse_kwargs.with_schema(schema)
    else:
        parse_kwargs_deco = parse_kwargs.with_resource(resource, operation)

    def _handler(_self, **kwargs):
        cluster_type = kwargs.pop('cluster_type')
        cluster_id = kwargs.pop('cluster_id')

        cluster = lock_cluster(cluster_id, cluster_type) if lock else get_cluster(cluster_id, cluster_type)
        request_handler = get_request_handler(cluster_type, resource, operation)

        if operation_options['status_blacklist']:
            check_cluster_not_in_status(cluster, *operation_options['status_blacklist'])
        if child and operation_options['on_instance']:
            resource_id = kwargs.pop(id_key, None)
            result = request_handler(cluster, resource_id, **kwargs)
        else:
            result = request_handler(cluster, **kwargs)

        if lock:
            metadb.complete_cluster_change(cluster_id)
            g.metadb.commit()
            result = render_operation_v1(result)

        return result

    if method != 'get':
        _handler = supports_idempotence(_handler)
    _handler = check_auth(
        folder_resolver=get_folder_by_cluster_id,
        resource=resource,
        operation=operation,
    )(_handler)
    _handler = marshal_deco(_handler)
    if parse_kwargs_deco is not None:
        _handler = parse_kwargs_deco(_handler)

    api_resource = type('AutoOperationHandler-{0}-{1}'.format(resource, operation), (MethodView,), {method: _handler})

    API.add_resource(api_resource, format_url(resource, operation))


def format_url(resource, operation):
    """
    Format URL for a given operation.
    """
    operation_options = OPERATIONS[operation]
    resource_options = RESOURCES[resource]
    id_key = resource_options.get('id_key')
    child = bool(id_key)

    url = '/mdb/<ctype:cluster_type>/<version>/clusters/<string:cluster_id>'
    if child:
        url = '{0}/{1}'.format(url, resource_options['url'])

        if operation_options['on_instance']:
            url = '{0}/<{1}>'.format(url, id_key)

    suffix = operation_options.get('url_suffix')
    if suffix:
        url += suffix

    return url


generate_resource_handlers(RESOURCES)

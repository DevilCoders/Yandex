import importlib
import logging
import time

import grpc
from google.protobuf.json_format import MessageToDict, ParseDict

from yandex.cloud.priv.mdb.v1 import operation_service_pb2 as op_spec
from yandex.cloud.priv.mdb.v1.operation_service_pb2_grpc import OperationServiceStub

from .grpcutil.service import WrappedGRPCService

logger = logging.getLogger()


def get_cluster_hosts(context, hosts, host_type=None, geo=None):
    """
    Get real hosts of cluster given cluster name and properties
    """
    hostnames = []
    for host in hosts:
        if host_type:
            if host.get('type') != host_type:
                continue
        if geo:
            host_zone_id = host.get('zoneId', host.get('zone_id'))
            if host_zone_id != geo:
                continue

        hostnames.append(host['name'])
    return hostnames


class GoInternalAPIError(RuntimeError):
    """
    General Go API error exception
    """


def snake_case_to_camel_case(s, first_lower=False):
    splitted = s.split('_')
    return ''.join(
        map(
            lambda x: x[1] if first_lower and x[0] == 0 else x[1].capitalize(),
            enumerate(splitted),
        )
    )


def init_service_and_message(context, cluster_type, method, data, service_type='cluster', version='v1'):
    """
    Instantiate grpc service and message.
    """
    module = importlib.import_module(f'yandex.cloud.priv.mdb.{cluster_type}.{version}.{service_type}_service_pb2_grpc')
    srv = getattr(module, snake_case_to_camel_case('_'.join([service_type, 'service', 'stub'])))
    spec_module = importlib.import_module(f'yandex.cloud.priv.mdb.{cluster_type}.{version}.{service_type}_service_pb2')
    msg = getattr(spec_module, snake_case_to_camel_case('_'.join([method, 'request'])))()
    ParseDict(data, msg)
    return get_wrapped_service(context, srv), msg


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


def get_wrapped_service(context, service_stub):
    api_conf = context.conf['projects']['mdb_internal_api']
    url = '{host}:{port}'.format(host=api_conf['host'], port=api_conf['port'])
    # TODO: replace with secure connection
    # ca_path = context.conf['compute']['ca_path']
    # with open(ca_path, 'rb') as f:
    #     creds = grpc.ssl_channel_credentials(f.read())
    channel = grpc.insecure_channel(url)
    grpc_timeout = int(api_conf['grpc_timeout'])
    get_token = lambda: context.conf['compute_driver']['compute']['token']
    return WrappedGRPCService(logger, channel, service_stub, grpc_timeout, get_token)


def get_hosts(context, cluster_type, cluster_id):
    """
    Get cluster hosts.
    """
    data = {'cluster_id': cluster_id, 'page_size': 100}
    srv, msg = init_service_and_message(context, cluster_type, 'list_cluster_hosts', data)
    # This IF is for Greenplum only because of we didn't have ListHosts attribute
    if cluster_type == 'greenplum':
        resp = srv.ListMasterHosts(msg)
    else:
        resp = srv.ListHosts(msg)
    resp = msg_to_dict(resp)
    check_response(resp, 'could not list hosts', key='hosts')

    return resp['hosts']


def get_cluster(context, cluster_type, folder_id=None, cluster_name=None, cluster_id=None):
    """
    Get cluster by name.
    """
    if cluster_id is None:
        cluster_id = _get_cluster_id(context, cluster_type, cluster_name, folder_id)
    data = {'cluster_id': cluster_id}
    srv, msg = init_service_and_message(context, cluster_type, 'get_cluster', data)
    resp = srv.Get(msg)

    resp = msg_to_dict(resp)
    check_response(resp, 'Failed to get cluster', key='id')

    return resp


def get_backups(context, cluster_type, cluster_id):
    """
    Get backups by cluster id.
    """
    data = {'cluster_id': cluster_id}
    srv, msg = init_service_and_message(context, cluster_type, 'list_cluster_backups', data)
    resp = srv.ListBackups(msg)

    resp = msg_to_dict(resp)
    check_response(resp, 'Failed to get backups', key='backups')

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
    srv, msg = init_service_and_message(context, cluster_type, 'list_clusters', data)
    resp = srv.List(msg)

    resp = msg_to_dict(resp)
    check_response(resp, 'could not list clusters', key='clusters')

    return resp['clusters']


def load_cluster_into_context(context, cluster_type, cluster_name=None, with_hosts=True):
    """
    Reload cluster with related objects and update corresponding context data.
    """
    if not cluster_name:
        cluster_name = context.cluster['name']

    context.cluster = get_cluster(context, cluster_type, context.folder['folder_ext_id'], cluster_name)

    if with_hosts:
        context.hosts = get_hosts(context, cluster_type, context.cluster['id'])


def get_task(context):
    """
    Get current task
    """
    msg = op_spec.GetOperationRequest()
    ParseDict({'operation_id': context.operation_id}, msg)

    op_service = get_wrapped_service(context, OperationServiceStub)
    resp = op_service.Get(msg)

    resp = msg_to_dict(resp)
    check_response(resp, 'Could not get task')

    return resp


def check_task_finished(context, deadline, error, finished_successfully):
    """
    Wait for task completion and return results
    """
    op_service = get_wrapped_service(context, OperationServiceStub)
    request = op_spec.GetOperationRequest()
    request.operation_id = context.operation_id
    while time.time() < deadline:
        resp = op_service.Get(request)
        task = MessageToDict(resp)
        if not task.get('done', False):
            time.sleep(1)
            continue
        logger.debug('Task is done: %r', task)
        finished_successfully = 'error' not in task
        if not finished_successfully:
            error = task['error']
        break
    return error, finished_successfully


def create_cluster(context, cluster_type, cluster_config):
    srv, msg = init_service_and_message(context, cluster_type, 'create_cluster', cluster_config)
    return srv.Create(msg)


def restore_cluster(context, cluster_type, cluster_config):
    srv, msg = init_service_and_message(context, cluster_type, 'restore_cluster', cluster_config)
    return srv.Restore(msg)

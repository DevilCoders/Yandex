"""
Helpers for working with yandexcloud managed clickhouse
"""

import grpc
import logging

from typing import List
from retrying import retry
from . import utils

import yandex.cloud.mdb.clickhouse.v1.cluster_service_pb2 as cs
import yandex.cloud.mdb.clickhouse.v1.cluster_pb2 as cluster
import yandex.cloud.mdb.clickhouse.v1.database_pb2 as db
import yandex.cloud.mdb.clickhouse.v1.user_pb2 as user

from yandex.cloud.mdb.clickhouse.v1.cluster_service_pb2_grpc import ClusterServiceStub


LOG = logging.getLogger('mdb_clickhouse')


class MDBClickhouseException(Exception):
    """
    Common Exception for helper
    """


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def operation_wait(ctx: dict, operation: str, response_type=None, meta_type=None, timeout=None):
    """
    Block and wait until operation completes
    """
    sdk = ctx.state['yandexsdk']
    return sdk.wait_operation_and_get_result(
        operation,
        response_type=response_type,
        meta_type=meta_type,
        timeout=timeout,
        logger=LOG,
    )


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def cluster_create(ctx: dict,
                   name: str):
    """
    Create managed clickhouse
    """
    conf = ctx.conf
    sdk = ctx.state['yandexsdk']
    service = sdk.client(ClusterServiceStub)

    return service.Create(cs.CreateClusterRequest(
        folder_id=conf['environment']['folder-id'],
        name=name,
        labels=ctx.labels,
        environment=conf['mdb']['clickhouse']['environment'],
        network_id=conf['environment']['network_id'],
        config_spec=cs.ConfigSpec(
            clickhouse=cs.ConfigSpec.Clickhouse(
                resources=cluster.Resources(
                    resource_preset_id='b2.micro',
                    disk_size=10 * (1024 ** 3),
                    disk_type_id='network-ssd'),
            ),
        ),
        host_specs=[
            cs.HostSpec(
                zone_id=conf['environment']['zone'],
                type='CLICKHOUSE',
                subnet_id=conf['environment']['subnet_id'],
                assign_public_ip=False)],
        database_specs=[db.DatabaseSpec(name=conf['mdb']['clickhouse']['db'])],
        user_specs=[
            user.UserSpec(
                name=conf['mdb']['clickhouse']['user'],
                password=conf['mdb']['clickhouse']['password'],
                permissions=[
                    user.Permission(database_name=conf['mdb']['clickhouse']['db']),
                ],
            ),
        ],
    ))


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def cluster_wait_create(ctx: dict, operation: str):
    """
    Wait operation of cluster creating
    """
    return operation_wait(ctx,
                          operation,
                          response_type=cluster.Cluster,
                          meta_type=cs.CreateClusterMetadata)


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def _list_clusters_page(service: any, folder_id: str, page_size: int, page_token: str):
    """
    Request for retrying
    """
    try:
        response = service.List(cs.ListClustersRequest(
            folder_id=folder_id,
            page_size=page_size,
            page_token=page_token,
        ))
        return response
    except grpc.RpcError:
        raise


def clusters_list(ctx: dict, page_size: int = 1000):
    """
    List all clusters
    """
    sdk = ctx.state['yandexsdk']
    folder_id = ctx.conf['environment']['folder-id']
    page_token = None
    service = sdk.client(ClusterServiceStub)
    items = []
    while True:
        response = _list_clusters_page(service, folder_id, page_size, page_token)
        items.extend(response.clusters)
        page_token = response.next_page_token
        if not page_token:
            break
    return items


def cluster_by_name(ctx: dict, name: str):
    """
    Return clsuter by name
    """
    items = clusters_list(ctx)
    for item in items:
        if item.name == name:
            return item
    raise MDBClickhouseException(f'Cluster with name {name} not found')


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def _cluster_delete(service: any, cluster_id: str):
    """
    Async request to delete cluster
    """
    try:
        return service.Delete(cs.DeleteClusterRequest(cluster_id=cluster_id))
    except grpc.RpcError as err:
        code = utils.extract_code(err)
        if code == grpc.StatusCode.NOT_FOUND:
            LOG.warn(f'MDB Clickhouse {cluster.id} already deleted')
        else:
            raise


def clusters_delete(ctx: dict,
                    cluster_ids: List[str]):
    """
    Delete managed clickhouse clusters
    """
    sdk = ctx.state['yandexsdk']
    service = sdk.client(ClusterServiceStub)
    operations = [_cluster_delete(service, cluster_id) for cluster_id in cluster_ids]

    for operation in operations:
        if not operation:
            continue
        operation_wait(ctx,
                       operation,
                       meta_type=cs.DeleteClusterMetadata,
                       )


def cluster_delete(ctx: dict,
                   cluster_id: str):
    """
    Delete managed clickhouse cluster
    """
    return clusters_delete(ctx, [cluster_id])


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def cluster_hosts(ctx: dict, cluster_id: str) -> List[str]:
    """
    Return fqdns of clickhouse cluster
    """
    sdk = ctx.state['yandexsdk']
    service = sdk.client(ClusterServiceStub)
    return service.ListHosts(cs.ListClusterHostsRequest(cluster_id=cluster_id)).hosts


def feature_clean(ctx: dict):
    """
    Clean context and resources after feature
    """
    if hasattr(ctx, 'clickhouse_cluster_id'):
        cluster_delete(ctx, ctx.clickhouse_cluster_id)
        ctx.clickhouse_cluster_name = None
        ctx.clickhouse_cluster_id = None
        ctx.clickhouse_hosts = None
        ctx.clickhouse_operation_id = None

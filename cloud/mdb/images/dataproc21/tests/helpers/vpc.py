"""
Helpers for working with yandexcloud sdk
"""

import grpc
import logging

from . import utils
from retrying import retry

from yandex.cloud.vpc.v1.network_pb2 import Network
from yandex.cloud.vpc.v1.network_service_pb2 import (
    CreateNetworkRequest,
    CreateNetworkMetadata,
    ListNetworksRequest,
    DeleteNetworkRequest,
    DeleteNetworkMetadata,
)
from yandex.cloud.vpc.v1.network_service_pb2_grpc import NetworkServiceStub

from yandex.cloud.vpc.v1.subnet_pb2 import Subnet, DhcpOptions
from yandex.cloud.vpc.v1.subnet_service_pb2 import (
    CreateSubnetRequest,
    CreateSubnetMetadata,
    GetSubnetRequest,
    ListSubnetsRequest,
    DeleteSubnetRequest,
    DeleteSubnetMetadata,
)
from yandex.cloud.vpc.v1.subnet_service_pb2_grpc import SubnetServiceStub


LOG = logging.getLogger('vpc')


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def network_create(ctx, name):
    """
    Create VPC network
    """
    conf = ctx.conf
    sdk = ctx.state['yandexsdk']
    network_id = None
    try:
        folder_id = conf['environment']['folder-id']
        service = sdk.client(NetworkServiceStub)
        operation = service.Create(CreateNetworkRequest(
            folder_id=folder_id,
            name=name,
            labels=ctx.labels,
        ))
        operation_result = sdk.wait_operation_and_get_result(
            operation,
            response_type=Network,
            meta_type=CreateNetworkMetadata,
            logger=LOG,
        )
        network_id = operation_result.response.id
        return network_id
    finally:
        if network_id:
            ctx.state['resources']['networks'][name] = network_id


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def network_delete(ctx, name):
    """
    Delete VPC network
    """
    sdk = ctx.state['yandexsdk']
    network_id = ctx.state['resources']['networks'].get(name)
    if not network_id:
        network_id = next(x.id for x in networks_list(ctx) if x.name == name)
        if not network_id:
            LOG.warn(f'VPC network {name} not found')
            return
    service = sdk.client(NetworkServiceStub)
    try:
        operation = service.Delete(DeleteNetworkRequest(
            network_id=network_id,
        ))
        sdk.wait_operation_and_get_result(
            operation,
            response_type=Network,
            meta_type=DeleteNetworkMetadata,
            logger=LOG,
        )
    except grpc.RpcError as err:
        code = utils.extract_code(err)
        if code == grpc.StatusCode.NOT_FOUND:
            LOG.warn(f'VPC network {network_id} already deleted')
        else:
            raise
    finally:
        ctx.state['resources']['networks'].pop(name, None)


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def _list_network_page(service, folder_id, page_size, page_token):
    """
    Request for retrying list network page
    """
    response = service.List(ListNetworksRequest(
        folder_id=folder_id,
        page_size=page_size,
        page_token=page_token,
    ))
    return response


def networks_list(ctx, page_size=1000):
    """
    List all networks
    """
    sdk = ctx.state['yandexsdk']
    folder_id = ctx.conf['environment']['folder-id']
    page_token = None
    service = sdk.client(NetworkServiceStub)
    ret = []
    while True:
        response = _list_network_page(service, folder_id, page_size, page_token)
        ret.extend(response.networks)
        page_token = response.next_page_token
        if not page_token:
            break
    return ret


def _get_dhcp_options(ctx):
    """
    Return DHCP Options for VPC subnet
    """
    conf = ctx.conf.get('vpc', {}).get('dhcp_options', {})
    if not conf:
        return None
    return DhcpOptions(
        domain_name_servers=conf.get('domain_name_servers', ['77.88.8.8']),
        domain_name=conf.get('domain_name', 'example.com'),
    )


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def subnet_create(ctx, name, zone_id, network_id, v4_cidr_block):
    """
    Create VPC subnet
    """
    conf = ctx.conf
    sdk = ctx.state['yandexsdk']
    subnet_id = None
    try:
        folder_id = conf['environment']['folder-id']
        service = sdk.client(SubnetServiceStub)
        operation = service.Create(CreateSubnetRequest(
            folder_id=folder_id,
            name=name,
            network_id=network_id,
            zone_id=zone_id,
            v4_cidr_blocks=[v4_cidr_block],
            dhcp_options=_get_dhcp_options(ctx),
            labels=ctx.labels,
        ))
        operation_result = sdk.wait_operation_and_get_result(
            operation,
            response_type=Subnet,
            meta_type=CreateSubnetMetadata,
            logger=LOG,
        )
        subnet_id = operation_result.response.id
        return subnet_id
    finally:
        if subnet_id:
            ctx.state['resources']['subnets'][name] = subnet_id


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def subnet_info(ctx, subnet_id: str):
    """
    Returns VPC subnet object
    """
    sdk = ctx.state['yandexsdk']
    service = sdk.client(SubnetServiceStub)
    return service.Get(GetSubnetRequest(subnet_id=subnet_id))


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def subnet_delete(ctx, name):
    """
    Delete VPC subnet
    """
    sdk = ctx.state['yandexsdk']
    subnet_id = ctx.state['resources']['subnets'].get(name)
    if not subnet_id:
        subnet_id = next(x.id for x in subnets_list(ctx) if x.name == name)
        if not subnet_id:
            LOG.warn(f'VPC subnet {name} not found')
            return
    service = sdk.client(SubnetServiceStub)
    try:
        operation = service.Delete(DeleteSubnetRequest(
            subnet_id=subnet_id,
        ))
        sdk.wait_operation_and_get_result(
            operation,
            response_type=Subnet,
            meta_type=DeleteSubnetMetadata,
            logger=LOG,
        )
    except grpc.RpcError as err:
        code = utils.extract_code(err)
        if code == grpc.StatusCode.NOT_FOUND:
            LOG.warn(f'VPC subnet {subnet_id} already deleted')
        else:
            raise
    finally:
        ctx.state['resources']['subnets'].pop(name, None)


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def _list_subnet_page(service, folder_id, page_size, page_token):
    """
    Request for retrying get subnet page
    """
    response = service.List(ListSubnetsRequest(
        folder_id=folder_id,
        page_size=page_size,
        page_token=page_token,
    ))
    return response


def subnets_list(ctx, page_size=1000):
    """
    List all subnets
    """
    sdk = ctx.state['yandexsdk']
    folder_id = ctx.conf['environment']['folder-id']
    page_token = None
    service = sdk.client(SubnetServiceStub)
    ret = []
    while True:
        response = _list_subnet_page(service, folder_id, page_size, page_token)
        ret.extend(response.subnets)
        page_token = response.next_page_token
        if not page_token:
            break
    return ret

"""
Helpers for working with yandexcloud sdk
"""

import sh
import grpc
import time
import logging

from retrying import retry
from typing import Optional
from . import utils

from yandex.cloud.compute.v1.image_service_pb2 import GetImageLatestByFamilyRequest
from yandex.cloud.compute.v1.image_service_pb2_grpc import ImageServiceStub
from yandex.cloud.compute.v1.instance_pb2 import IPV4, Instance
from yandex.cloud.compute.v1.instance_service_pb2 import (
    CreateInstanceRequest,
    ResourcesSpec,
    AttachedDiskSpec,
    NetworkInterfaceSpec,
    PrimaryAddressSpec,
    OneToOneNatSpec,
    DeleteInstanceRequest,
    CreateInstanceMetadata,
    DeleteInstanceMetadata,
    ListInstancesRequest,
    GetInstanceRequest,
    StartInstanceRequest,
    StartInstanceMetadata,
    StopInstanceRequest,
    StopInstanceMetadata,
)
from yandex.cloud.compute.v1.instance_service_pb2_grpc import InstanceServiceStub


LOG = logging.getLogger('compute')


class ComputeException(Exception):
    """
    Common Exception for compute helper
    """


class InstanceNotFound(ComputeException):
    """
    Instance not found
    """


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def get_image_by_family(ctx, family: str) -> str:
    """
    Get latest image by image_family
    """
    sdk = ctx.state['yandexsdk']
    service = sdk.client(ImageServiceStub)
    image = service.GetLatestByFamily(
        GetImageLatestByFamilyRequest(
            folder_id=ctx.conf['environment']['folder-id'],
            family=family,
        ),
    )
    return image.id


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def operation_wait(ctx, operation, response_type=None, meta_type=None, timeout=None):
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
def instance_create(ctx: dict,
                    name: str,
                    image_id: Optional[str] = None,
                    subnet_id: Optional[str] = None,
                    metadata: Optional[dict] = None,
                    ipv6: Optional[bool] = False):
    """
    Create instance vm
    """
    conf = ctx.conf
    sdk = ctx.state['yandexsdk']
    folder_id = conf['environment']['folder-id']
    instance_service = sdk.client(InstanceServiceStub)
    network_interface_specs = [
        NetworkInterfaceSpec(
            subnet_id=subnet_id or conf['environment']['subnet_id'],
            primary_v4_address_spec=PrimaryAddressSpec(one_to_one_nat_spec=OneToOneNatSpec(ip_version=IPV4)),
        ),
    ]
    if ipv6:
        network_interface_specs = [
            NetworkInterfaceSpec(
                subnet_id=subnet_id or conf['environment']['subnet_id'],
                primary_v4_address_spec=PrimaryAddressSpec(),
                primary_v6_address_spec=PrimaryAddressSpec(),
            ),
        ]

    return instance_service.Create(CreateInstanceRequest(
        folder_id=folder_id,
        name=name,
        hostname=name,
        service_account_id=conf['compute']['service_account_id'],
        resources_spec=ResourcesSpec(
            memory=conf['compute']['memory'],
            cores=conf['compute']['cores'],
            core_fraction=conf['compute']['core_fraction'],
        ),
        zone_id=conf['environment']['zone'],
        platform_id=conf['compute']['platform_id'],
        boot_disk_spec=AttachedDiskSpec(
            auto_delete=True,
            disk_spec=AttachedDiskSpec.DiskSpec(
                type_id=conf['compute']['root_disk_type'],
                size=conf['compute']['root_disk_size'],
                image_id=image_id or conf['compute']['image_id'],
            ),
        ),
        network_interface_specs=network_interface_specs,
        metadata=metadata or {},
        labels=ctx.labels,
    ))


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def instance_wait_create(ctx, operation):
    """
    Wait operation of compute instance creating
    """
    instance_id, name = None, None
    try:
        operation_result = operation_wait(ctx,
                                          operation,
                                          response_type=Instance,
                                          meta_type=CreateInstanceMetadata)
        instance_id = operation_result.response.id
        name = operation_result.response.name
    finally:
        if instance_id and name:
            ctx.state['resources']['instances'][name] = instance_id


def wait_ssh_available(ctx, user, hostname, timeout=300.0):
    """
    Wait for hostname to become available over ssh
    """
    start_ts = time.time()
    utils.wait_tcp_conn(hostname, 22, timeout=timeout)
    exception = ComputeException(f'{hostname} tcp/22 unreachable')
    while time.time() - start_ts <= timeout:
        try:
            sh.ssh(*utils.ssh_options(ctx), f'{user}@{hostname}', 'uptime', _timeout=15.0)
            return
        except (sh.TimeoutException, sh.ErrorReturnCode) as exc:
            exception = exc
    raise exception


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def instance_get(ctx, instance_id):
    """
    Get instance
    """
    sdk = ctx.state['yandexsdk']
    service = sdk.client(InstanceServiceStub)
    response = service.Get(GetInstanceRequest(
        instance_id=instance_id,
    ))
    return response


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def _list_instances_page(service, folder_id, page_size, page_token):
    """
    Request for retrying
    """
    try:
        response = service.List(ListInstancesRequest(
            folder_id=folder_id,
            page_size=page_size,
            page_token=page_token,
        ))
        return response
    except grpc.RpcError:
        raise


def instances_list(ctx, page_size=1000):
    """
    List all instances
    """
    sdk = ctx.state['yandexsdk']
    folder_id = ctx.conf['environment']['folder-id']
    page_token = None
    service = sdk.client(InstanceServiceStub)
    instances = []
    while True:
        response = _list_instances_page(service, folder_id, page_size, page_token)
        instances.extend(response.instances)
        page_token = response.next_page_token
        if not page_token:
            break
    return instances


def get_instance_by_name(ctx, name):
    """
    Return instance by name
    """
    for instance_name, instance_id in ctx.state['resources']['instances'].items():
        if instance_name == name:
            return instance_get(ctx, instance_id)

    instances = instances_list(ctx)
    for instance in instances:
        if instance.name == name:
            return instance
    raise InstanceNotFound(f'Instance with name {name} not found')


def get_public_ip_address(ctx, name):
    """
    Get public address for instance with name
    """
    dnscache = ctx.state['dnscache']
    addr = dnscache.get(name)
    if addr:
        return addr
    instance = get_instance_by_name(ctx, name)
    for iface in instance.network_interfaces:
        addr = iface.primary_v6_address.address or iface.primary_v4_address.one_to_one_nat.address
        if addr:
            break
    if not addr:
        raise ComputeException(f'Not found usable address for instance {addr} {name},'
                               f'ifaces: {instance.network_interfaces[0]}')
    dnscache[name] = addr
    return addr


def instance_fqdn(ctx, name):
    """
    Get instance internal fqdn
    """
    instance = get_instance_by_name(ctx, name)
    return instance.fqdn


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def instance_delete(ctx, name=None, instance_id=None, wait=True):
    """
    Delete compute instance
    """
    if not (name or instance_id):
        raise Exception("You must provide one of name or instance_id")
    if name:
        try:
            instance_id = get_instance_by_name(ctx, name).id
        except InstanceNotFound:
            LOG.warn(f'instance with {name} already deleted')
            return
    try:
        sdk = ctx.state['yandexsdk']
        instance_service = sdk.client(InstanceServiceStub)
        operation = instance_service.Delete(DeleteInstanceRequest(instance_id=instance_id))
        if wait is False:
            return operation
        operation_wait(ctx,
                       operation,
                       meta_type=DeleteInstanceMetadata,
                       )

    except grpc.RpcError as err:
        code = utils.extract_code(err)
        if code == grpc.StatusCode.NOT_FOUND:
            LOG.warn(f'Compute instance {instance_id} already deleted')
        else:
            raise
    finally:
        ctx.state['resources']['instances'].pop(name, None)
        ctx.state['dnscache'].pop(name, None)


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def instance_stop(ctx, name=None, instance_id=None, wait=True):
    """
    Stop compute instance
    """
    if not (name or instance_id):
        raise Exception("You must provide one of name or instance_id")
    if name:
        instance_id = get_instance_by_name(ctx, name).id
    sdk = ctx.state['yandexsdk']
    instance_service = sdk.client(InstanceServiceStub)
    operation = instance_service.Stop(StopInstanceRequest(instance_id=instance_id))
    del ctx.state['dnscache'][name]
    if not wait:
        return operation
    operation_wait(ctx,
                   operation,
                   meta_type=DeleteInstanceMetadata,
                   )


@retry(stop_max_attempt_number=3, retry_on_exception=utils.retry_if_grpc_error)
def instance_start(ctx, name=None, instance_id=None, wait=True):
    """
    Start compute instance
    """
    if not (name or instance_id):
        raise Exception("You must provide one of name or instance_id")
    if name:
        instance_id = get_instance_by_name(ctx, name).id
    sdk = ctx.state['yandexsdk']
    instance_service = sdk.client(InstanceServiceStub)
    operation = instance_service.Start(StartInstanceRequest(instance_id=instance_id))
    if not wait:
        return operation
    operation_wait(ctx,
                   operation,
                   meta_type=StartInstanceMetadata,
                   )


def instances_delete(ctx, names):
    """
    Concurrently delete a set of compute instances by their names
    """
    operations = []
    for name in names:
        operation = instance_delete(ctx, name=name, wait=False)
        if operation:
            operations.append(operation)
    for operation in operations:
        operation_wait(ctx, operation, meta_type=DeleteInstanceMetadata)


def instances_stop(ctx, names):
    """
    Concurrently stops a set of compute instances by their names
    """
    operations = []
    for name in names:
        LOG.info('Stopping instance {name}')
        operation = instance_stop(ctx, name=name, wait=False)
        if operation:
            operations.append(operation)
    for operation in operations:
        operation_wait(ctx, operation, meta_type=StopInstanceMetadata)


def instances_start(ctx, names):
    """
    Concurrently starts a set of compute instances by their names
    """
    operations = []
    for name in names:
        operation = instance_start(ctx, name=name, wait=False)
        if operation:
            operations.append(operation)
    for operation in operations:
        operation_wait(ctx, operation, meta_type=StopInstanceMetadata)

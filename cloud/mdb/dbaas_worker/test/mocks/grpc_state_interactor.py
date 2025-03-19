# coding: utf-8

import logging
from urllib.parse import parse_qs

from mock import Mock

from cloud.mdb.internal.python.grpcutil.exceptions import NotFoundError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter
from yandex.cloud.priv.compute.v1 import (
    disk_pb2,
    image_pb2,
    instance_pb2,
    disk_placement_group_pb2,
    placement_group_pb2,
)
from yandex.cloud.priv.operation import operation_pb2
from .utils import handle_action

log = MdbLoggerAdapter(logging.getLogger(__name__), {})


def _apply_filters(filters, sequence):
    """
    Apply filters to sequence and get list of matched elements
    """
    ret = []
    for element in sequence:
        matched = True
        for key, value in filters.items():
            if element[key] != value:
                matched = False
                break
        if matched:
            ret.append(element)
    return ret


def _disk_from_state_to_grpc(state_obj) -> disk_pb2.Disk:
    result = disk_pb2.Disk()
    prop_map = {
        'id': 'id',
        'zoneId': 'zone_id',
        'typeId': 'type_id',
        'size': 'size',
        'folderId': 'folder_id',
    }
    for source, dest in prop_map.items():
        setattr(result, dest, state_obj[source])
    result.status = getattr(disk_pb2.Disk.Status, state_obj['status'])
    return result


def _image_from_state_to_grpc(state_obj) -> image_pb2.Image:
    result = image_pb2.Image()
    prop_map = {
        'id': 'id',
        'description': 'description',
        'folderId': 'folder_id',
        'minDiskSize': 'min_disk_size',
    }
    for source, dest in prop_map.items():
        setattr(result, dest, state_obj[source])
    result.status = getattr(image_pb2.Image.Status, state_obj['status'])
    return result


def _disk_placement_group_from_state_to_grpc(state_obj) -> disk_placement_group_pb2.DiskPlacementGroup:
    result = disk_placement_group_pb2.DiskPlacementGroup()
    result.id = state_obj['id']
    return result


def _placement_group_from_state_to_grpc(state_obj) -> placement_group_pb2.PlacementGroup:
    result = placement_group_pb2.PlacementGroup()
    result.id = state_obj['id']
    return result


def __handle_disk(container, disk_state):
    container.auto_delete = disk_state['autoDelete']
    container.device_name = disk_state['deviceName']
    container.disk_id = disk_state['diskId']
    container.mode = getattr(instance_pb2.AttachedDisk.Mode, disk_state['mode'])
    container.status = getattr(instance_pb2.AttachedDisk.Status, disk_state['status'])


def _instance_from_state_to_grpc(state_obj: dict) -> instance_pb2.Instance:
    result = instance_pb2.Instance()
    prop_map = {
        'id': 'id',
        'fqdn': 'fqdn',
        'folderId': 'folder_id',
        'name': 'name',
        'platformId': 'platform_id',
        'zoneId': 'zone_id',
    }
    for kv_container in ['labels', 'metadata']:
        cnt = getattr(result, kv_container)
        for key, value in state_obj[kv_container].items():
            cnt[key] = value
    for source, dest in prop_map.items():
        setattr(result, dest, state_obj[source])
    try:
        result.status = getattr(instance_pb2.Instance.Status, state_obj['status'])
    except KeyError:
        raise Exception(state_obj)
    result.network_settings.type = getattr(instance_pb2.NetworkSettings.Type, state_obj['networkSettings']['type'])

    resources = state_obj['resources']
    result.resources.memory = resources['memory']
    result.resources.cores = resources['cores']
    result.resources.gpus = resources['gpus']
    result.resources.core_fraction = resources['coreFraction']
    result.resources.nvme_disks = resources['nvmeDisks']

    disk_state = state_obj['bootDisk']
    __handle_disk(result.boot_disk, disk_state)

    for disk_state in state_obj['secondaryDisks']:
        disk = instance_pb2.AttachedDisk()
        __handle_disk(disk, disk_state)
        result.secondary_disks.extend([disk])

    for net_state in state_obj['networkInterfaces']:
        net_interface = instance_pb2.NetworkInterface()
        net_interface.index = str(net_state['index'])
        net_interface.subnet_id = net_state['subnetId']
        if 'primaryV4Address' in net_state:
            net_interface.primary_v4_address.address = net_state['primaryV4Address']['address']
        if 'primaryV6Address' in net_state:
            net_interface.primary_v6_address.address = net_state['primaryV6Address']['address']
        result.network_interfaces.extend([net_interface])

    return result


def gen_element_get_grpc(state, collection_type):
    """
    Generate element get function
    """

    def get_by_id(id):
        element_obj = state['compute'][collection_type].get(id)

        if element_obj is None:
            err = NotFoundError(
                log,
                f'worker mock for {collection_type}:{id} not found error',
                err_type='NOT_FOUND',
                code=0,
            )
            raise err
        return element_obj

    def disk(request, **grpc_kwargs):
        element_id = request.disk_id
        handle_action(state, f'compute-{collection_type}-get-{element_id}')
        element_obj = get_by_id(element_id)
        return _disk_from_state_to_grpc(element_obj)

    def instance(request, **grpc_kwargs):
        """
        Get element from collection by id
        """
        element_id = request.instance_id
        handle_action(state, f'compute-{collection_type}-get-{element_id}')
        element_obj = get_by_id(element_id)
        return _instance_from_state_to_grpc(element_obj)

    def image(request, **grpc_kwargs):
        element_id = request.image_id
        handle_action(state, f'compute-{collection_type}-get-{element_id}')
        element_obj = get_by_id(element_id)
        return _image_from_state_to_grpc(element_obj)

    def operation(request, **grpc_kwargs) -> operation_pb2.Operation:
        element_id = request.operation_id
        handle_action(state, f'compute-{collection_type}-get-{element_id}')
        element_obj = get_by_id(element_id)
        result = operation_pb2.Operation(id=element_id, done=True, response=element_obj['response'])
        return result

    def disk_placement_group(request, **grpc_kwargs) -> disk_placement_group_pb2.DiskPlacementGroup:
        element_id = request.disk_placement_group_id
        handle_action(state, f'compute-{collection_type}-get-{element_id}')
        element_obj = get_by_id(element_id)
        return _disk_placement_group_from_state_to_grpc(element_obj)

    def placement_group(request, **grpc_kwargs) -> placement_group_pb2.PlacementGroup:
        element_id = request.placement_group_id
        handle_action(state, f'compute-{collection_type}-get-{element_id}')
        element_obj = get_by_id(element_id)
        return _placement_group_from_state_to_grpc(element_obj)

    transform_map = {
        'disks': disk,
        'instances': instance,
        'images': image,
        'operations': operation,
        'disk_placement_groups': disk_placement_group,
        'placement_groups': placement_group,
    }

    if collection_type in transform_map:
        return transform_map[collection_type]
    raise NotImplementedError('implement yourself')


def gen_element_list_grpc(state, collection_type):
    """
    Generate elements list function
    """

    def generated_instance(request, **grpc_kwargs):
        """
        List elements matching filters
        """
        filters = parse_qs(request.filter)
        for key, value in filters.items():
            if len(value) > 1:
                raise RuntimeError('we do not expect multiple values')
            filters[key] = value[0].strip('"')
        if request.folder_id:
            filters['folderId'] = request.folder_id
        if request.name:
            filters['name'] = request.name

        filters_action = '-'.join(sorted([f'{key}-{value}' for key, value in filters.items()]))

        handle_action(state, f'compute-{collection_type}-get-{filters_action}')

        items = _apply_filters(filters, filter(lambda x: x is not None, state['compute'][collection_type].values()))

        transform_map = {
            'images': _image_from_state_to_grpc,
            'instances': _instance_from_state_to_grpc,
        }

        if collection_type in transform_map:
            items = map(transform_map[collection_type], items)

        response_kwargs = {
            collection_type: items,
            'next_page_token': '',
        }

        return Mock(**response_kwargs)

    return generated_instance

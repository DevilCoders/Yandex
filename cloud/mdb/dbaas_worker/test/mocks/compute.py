"""
Simple compute mock
"""

import logging
from ipaddress import IPv4Network, IPv6Network

from google.protobuf import any_pb2

from yandex.cloud.priv.compute.v1 import disk_service_pb2, instance_pb2, instance_service_pb2
from yandex.cloud.priv.operation import (
    operation_pb2,
)
from .grpc_state_interactor import (
    gen_element_get_grpc,
    gen_element_list_grpc,
)
from .utils import handle_action


def new_disk_id(state):
    return f'disk-{max([int(x.split("-")[1]) for x in state["compute"]["disks"]] + [0]) + 1}'


def disk_from_spec_grpc(spec):
    result = {
        'size': spec.disk_spec.size,
        'typeId': spec.disk_spec.type_id,
        'diskSpec': dict(
            auto_delete=spec.auto_delete,
        ),
    }
    if spec.disk_id:
        result['diskSpec']['disk_id'] = spec.disk_id
    return result


def attach_disk_from_spec_grpc(state, spec, instance=None):
    """
    Create or take disk from spec
    """
    disk_id = spec['diskSpec'].get('disk_id')
    if disk_id:
        if disk_id not in state['compute']['disks']:
            raise NotImplementedError(f'disk {disk_id} not found')
        if state['compute']['disks'][disk_id]['instanceIds']:
            raise NotImplementedError('disk already attached')
        state['compute']['disks'][disk_id]['instanceIds'] = [instance['id']]
    else:
        disk_id = new_disk_id(state)
        disk_info = spec
        disk_info.update(
            {
                'folderId': instance['folderId'] if instance else spec['diskSpec']['folderId'],
                'zoneId': instance['zoneId'] if instance else spec['diskSpec']['zoneId'],
                'id': disk_id,
                'instanceIds': [instance['id']] if instance else [],
                'status': 'READY',
            }
        )
        state['compute']['disks'][disk_id] = disk_info

    if instance:
        instance_disk_info = {
            'autoDelete': spec['diskSpec']['auto_delete'],
            'deviceName': disk_id,
            'diskId': disk_id,
            'mode': 'READ_WRITE',
            'status': 'ATTACHED',
        }
    else:
        instance_disk_info = None

    return None, disk_id, instance_disk_info


def ret_operation_base(state, metadata, response):
    operation_id = f'operation-{max([int(x.split("-")[1]) for x in state["compute"]["operations"]] + [0]) + 1}'
    state['compute']['operations'][operation_id] = {
        'id': operation_id,
        'metadata': metadata,
        'response': response,
        'done': True,
    }
    return operation_id


def action_id_create_instance(fqdn):
    return f'compute-instance-create-{fqdn}'


def instance_id_seq(state):
    return f'instance-{max([int(x.split("-")[1]) for x in state["compute"]["instances"]] + [0]) + 1}'


def create_handle_network_spec_grpc(state, instance, spec):
    """
    Attach interface to instance based on spec
    """
    subnet = state['compute']['subnets'][spec.subnet_id]
    address_index = int(instance['id'].split('-')[1])
    interface = {
        'index': len(instance['networkInterfaces']),
        'subnetId': spec.subnet_id,
        'securityGroupIds': spec.security_group_ids,
    }
    if subnet.get('v4CidrBlock'):
        address_spec = spec.primary_v4_address_spec
        address = str(IPv4Network(subnet['v4CidrBlock'][0])[address_index])
        interface['primaryV4Address'] = {'address': address}
        if address_spec.one_to_one_nat_spec.ip_version == instance_pb2.IPV4:
            interface['oneToOneNat'] = {'address': str(IPv4Network('203.0.113.0/24')[address_index])}
    if subnet.get('v6CidrBlock'):
        address = str(IPv6Network(subnet['v6CidrBlock'][0])[address_index])
        interface['primaryV6Address'] = {'address': address}

    instance['networkInterfaces'].append(interface)

    return None


def ret_grpc_operation(state, metadata=None, response=None):
    metadata_message = any_pb2.Any()
    if metadata is not None and not isinstance(metadata, dict):
        metadata_message.Pack(metadata)
    response_message = any_pb2.Any()
    if response is not None:
        response_message.Pack(response)
    operation = operation_pb2.Operation(
        id=ret_operation_base(state, metadata=metadata or {}, response=response_message),
        metadata=metadata_message,
        response=response_message,
        done=True,
    )
    return operation


def resource_spec_to_dict(resources_spec):
    result = {}
    for key in ('memory', 'cores', 'gpus'):
        result[key] = getattr(resources_spec, key, None)

    result['coreFraction'] = resources_spec.core_fraction
    result['nvmeDisks'] = resources_spec.nvme_disks
    return result


def compute(mocker, state):
    def create_instance(request, **kwargs):
        response = instance_pb2.Instance()

        handle_action(state, action_id_create_instance(request.fqdn))
        instance = dict(
            folderId=request.folder_id,
            fqdn=request.fqdn,
            name=request.name,
            platformId=request.platform_id,
            zoneId=request.zone_id,
            metadata={},
            labels={},
            networkSettings={'type': 'STANDARD'},
            status='RUNNING',
        )
        if request.service_account_id:
            instance['serviceAccountId'] = request.service_account_id

        instance['id'] = instance_id_seq(state)

        instance['resources'] = resource_spec_to_dict(request.resources_spec)

        ret, _, disk_info = attach_disk_from_spec_grpc(state, disk_from_spec_grpc(request.boot_disk_spec), instance)
        if ret:
            return ret
        instance['bootDisk'] = disk_info

        instance['secondaryDisks'] = []
        for secondary_spec in request.secondary_disk_specs:
            ret, disk_id, disk_info = attach_disk_from_spec_grpc(state, disk_from_spec_grpc(secondary_spec), instance)
            if ret:
                return ret
            instance['secondaryDisks'].append(disk_info)
            response_disk_spec = response.secondary_disks.add()
            response_disk_spec.disk_id = disk_id

        instance['networkInterfaces'] = []
        for network_spec in request.network_interface_specs:
            create_handle_network_spec_grpc(state, instance, network_spec)

        state['compute']['instances'][instance['id']] = instance

        metadata = instance_service_pb2.CreateInstanceMetadata()
        metadata.instance_id = instance['id']

        operation = ret_grpc_operation(state, metadata, response=response)

        logging.getLogger(__name__).info('created %s: "%s"', operation, operation.error)
        return operation

    def start_instance(request, **kwargs):
        instance_id = request.instance_id
        handle_action(state, f'compute-start-{instance_id}')

        instance = state['compute']['instances'][instance_id]

        if instance['status'] != 'STOPPED':
            raise NotImplementedError()

        instance['status'] = 'RUNNING'

        # Make decommissioned hosts healthy again
        if instance['fqdn'] in state['dataproc-manager']:
            del state['dataproc-manager'][instance['fqdn']]
        return ret_grpc_operation(state, {'instanceId': instance_id})

    def stop_instance(request, **kwargs):
        instance_id = request.instance_id
        handle_action(state, f'compute-stop-{instance_id}')

        instance = state['compute']['instances'].get(instance_id)

        if instance['status'] != 'RUNNING':
            raise NotImplementedError('implement yourself')

        instance['status'] = 'STOPPED'

        return ret_grpc_operation(state, {'instanceId': instance_id})

    def delete_instance(request, **kwargs):
        instance_id = request.instance_id
        handle_action(state, f'compute-delete-{instance_id}')

        instance = state['compute']['instances'].get(instance_id)
        if not instance:
            raise NotImplementedError

        if instance['bootDisk']['autoDelete']:
            del state['compute']['disks'][instance['bootDisk']['diskId']]
        else:
            state['compute']['disks'][instance['bootDisk']['diskId']]['instanceIds'] = []

        for disk in instance['secondaryDisks']:
            if disk['autoDelete']:
                del state['compute']['disks'][disk['diskId']]
            else:
                state['compute']['disks'][disk['diskId']]['instanceIds'] = []

        del state['compute']['instances'][instance_id]

        return ret_grpc_operation(state, {'instanceId': instance_id})

    def update_instance_metadata(request, **kwargs):
        instance_id = request.instance_id
        handle_action(state, f'compute-update-metadata-{instance_id}')

        instance = state['compute']['instances'].get(instance_id)

        if not instance:
            raise NotImplementedError('implement yourself')

        instance['metadata'].update(dict(request.upsert))

        return ret_grpc_operation(state, {'instanceId': instance_id})

    def update_network_interface(request, **kwargs):
        instance_id = request.instance_id
        network_id = request.network_interface_index
        handle_action(state, f'compute-update-metadata-{instance_id}-{network_id}')

        instance = state['compute']['instances'].get(instance_id)

        if not instance:
            raise RuntimeError(f'instance {instance_id} not exist')

        for d in instance['networkInterfaces']:
            if str(d['index']) == network_id:
                d['securityGroupIds'] = request.security_group_ids
                break
        else:
            raise RuntimeError(f'instance {instance_id} not have network interface {network_id}, instance {instance}')

        return ret_grpc_operation(state, {'instanceId': instance_id})

    def modify_instance(request, **kwargs):
        instance_id = request.instance_id
        handle_action(state, f'compute-modify-{instance_id}')

        instance = state['compute']['instances'].get(instance_id)

        if not instance:
            raise NotImplementedError('implement yourself')

        if request.boot_disk_spec.disk_id:
            if request.boot_disk_spec.disk_id != instance['bootDisk']['diskId']:
                raise Exception(
                    '{} boot disk replace from {} to {} is not implemented in mock'.format(
                        instance['fqdn'],
                        instance['bootDisk']['diskId'],
                        request.boot_disk_spec.disk_id,
                    )
                )
            instance['bootDisk']['autoDelete'] = request.boot_disk_spec.auto_delete

        field_mapping = dict(
            metadata='metatada',
            labels='labels',
            platformId='platform_id',
            serviceAccountId='service_account_id',
        )
        for key in field_mapping:
            field_value = getattr(request, field_mapping[key], None)
            if isinstance(field_value, (str, int, dict)):
                instance[key] = field_value

        if request.resources_spec:
            instance['resources'].update(resource_spec_to_dict(request.resources_spec))

        condition = (
            isinstance(request.network_settings.type, int)
            and request.network_settings.type != instance_pb2.NetworkSettings.Type.TYPE_UNSPECIFIED
        )
        if condition:
            if request.network_settings.type == instance_pb2.NetworkSettings.Type.SOFTWARE_ACCELERATED:
                ns = 'SOFTWARE_ACCELERATED'
            elif request.network_settings.type == instance_pb2.NetworkSettings.Type.STANDARD:
                ns = 'STANDARD'
            else:
                raise NotImplementedError(
                    'implement it for {}'.format(
                        request.network_settings.type,
                    )
                )
            instance['networkSettings'] = {'type': ns}

        return ret_grpc_operation(state, {'instanceId': instance_id})

    def delete_disk(request, **kwargs):
        disk_id = request.disk_id
        handle_action(state, f'compute-disk-delete-{disk_id}')

        disk = state['compute']['disks'].get(disk_id)

        if not disk:
            raise NotImplementedError

        if disk['instanceIds']:
            raise NotImplementedError(f'Disk is attached to instance {disk["instanceIds"][0]}')

        del state['compute']['disks'][disk_id]

        return ret_grpc_operation(state, {'diskId': disk_id})

    def modify_disk(request, **kwargs):
        disk_id = request.disk_id
        handle_action(state, f'compute-disk-modify-{disk_id}')
        disk = state['compute']['disks'].get(disk_id)
        if not disk:
            raise NotImplementedError('No such disk')
        if request.size < disk['size']:
            raise NotImplementedError('Disk downscale is not supported')

        disk['size'] = request.size
        return ret_grpc_operation(state, {'diskId': disk_id})

    def create_disk(request, **kwargs):
        handle_action(state, f'compute-disk-create-{request.zone_id}-{request.type_id}')

        ext_spec = {
            'folderId': request.folder_id,
            'zoneId': request.zone_id,
        }

        new_disk_spec = {
            'diskSpec': ext_spec,
            'size': request.size,
            'typeId': request.type_id,
        }

        _, disk_id, _ = attach_disk_from_spec_grpc(state, new_disk_spec)

        metadata = disk_service_pb2.CreateDiskMetadata()
        metadata.disk_id = disk_id

        return ret_grpc_operation(state, metadata)

    def attach_disk(request, **kwargs):
        instance_id = request.instance_id
        disk_spec = disk_from_spec_grpc(request.attached_disk_spec)
        handle_action(state, f'compute-attach-{instance_id}-{disk_spec["diskSpec"]["disk_id"]}')

        instance = state['compute']['instances'].get(instance_id)

        if not instance:
            raise NotImplementedError('not found')

        ret, _, disk_info = attach_disk_from_spec_grpc(state, disk_spec, instance)

        instance['secondaryDisks'].append(disk_info)

        return ret_grpc_operation(state)

    def detach_disk(request, **kwargs):
        instance_id = request.instance_id
        disk_id = request.disk_id
        handle_action(state, f'compute-detach-{instance_id}-{disk_id}')

        instance = state['compute']['instances'].get(instance_id)

        if not instance:
            raise NotImplementedError('not found instance')

        for index, disk in enumerate(instance['secondaryDisks']):
            if disk['diskId'] == disk_id:
                disk_data = state['compute']['disks'][disk['diskId']]
                disk_data['instanceIds'] = []
                instance['secondaryDisks'].pop(index)
                return ret_grpc_operation(state)

        raise NotImplementedError('unable to find disk on instance')

    stub = mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.compute.InstancesClient._instance_service',
    )
    stub.Create.side_effect = create_instance
    stub.Start.side_effect = start_instance
    stub.Stop.side_effect = stop_instance
    stub.Delete.side_effect = delete_instance
    stub.UpdateMetadata.side_effect = update_instance_metadata
    stub.UpdateNetworkInterface.side_effect = update_network_interface
    stub.Update.side_effect = modify_instance
    stub.Get.side_effect = gen_element_get_grpc(state, 'instances')
    stub.List.side_effect = gen_element_list_grpc(state, 'instances')
    stub.AttachDisk.side_effect = attach_disk
    stub.DetachDisk.side_effect = detach_disk

    stub = mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.compute.DisksClient._disk_service',
    )
    stub.Get = gen_element_get_grpc(state, 'disks')
    stub.Delete = delete_disk
    stub.Update = modify_disk
    stub.Create = create_disk

    stub = mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.compute.ImagesClient._image_service',
    )
    stub.Get.side_effect = gen_element_get_grpc(state, 'images')
    stub.List.side_effect = gen_element_list_grpc(state, 'images')

    stub = mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.compute.OperationsClient._operation_service',
    )
    stub.Get = gen_element_get_grpc(state, 'operations')

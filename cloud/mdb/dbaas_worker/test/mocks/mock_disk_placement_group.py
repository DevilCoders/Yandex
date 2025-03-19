"""
disk placement group mock
"""

from .compute import ret_grpc_operation
from yandex.cloud.priv.compute.v1 import (
    disk_placement_group_pb2,
)
from .grpc_state_interactor import (
    gen_element_get_grpc,
)
from .utils import handle_action


def disk_placement_group_id_seq(state):
    return (
        f'diskplacementgroup-{max([int(x.split("-")[1]) for x in state["compute"]["disk_placement_groups"]] + [0]) + 1}'
    )


def disk_placement_groups(mocker, state):
    state_endpoint = state['compute']['disk_placement_groups']

    def create_disk_placement_group(request, **kwargs):
        handle_action(state, f'disk_placement_group.{request.name}.create')

        id = disk_placement_group_id_seq(state)
        state_entity = dict(
            disks=[],
            id=id,
            name=request.name,
            folder_id=request.folder_id,
            zone=request.zone_id,
        )

        state_endpoint[id] = state_entity

        response_model = disk_placement_group_pb2.DiskPlacementGroup()
        response_model.id = id

        return ret_grpc_operation(state, response=response_model)

    def delete_disk_placement_group(request, **kwargs):
        group_id = request.disk_placement_group_id
        handle_action(state, f'disk_placement_group.{group_id}.delete')

        group = state_endpoint.get(group_id)

        if not group:
            raise NotImplementedError

        if group['disks']:
            raise NotImplementedError(f'Group is not empty {group["disks"]}')

        del state_endpoint[group_id]

        return ret_grpc_operation(state)

    stub = mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.disk_placement_group.DiskPlacementGroupClient._disk_placement_group_service',
    )
    stub.Delete.side_effect = delete_disk_placement_group
    stub.Create.side_effect = create_disk_placement_group
    stub.Get.side_effect = gen_element_get_grpc(state, 'disk_placement_groups')

    stub = mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.disk_placement_group.OperationsClient._operation_service',
    )
    stub.Get = gen_element_get_grpc(state, 'operations')

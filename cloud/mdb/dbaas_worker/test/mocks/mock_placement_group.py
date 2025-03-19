"""
placement group mock
"""

from .compute import ret_grpc_operation
from yandex.cloud.priv.compute.v1 import placement_group_pb2
from .grpc_state_interactor import (
    gen_element_get_grpc,
)
from .utils import handle_action


def placement_group_id_seq(state):
    return f'placementgroup-{max([int(x.split("-")[1]) for x in state["compute"]["placement_groups"]] + [0]) + 1}'


def placement_groups(mocker, state):
    state_endpoint = state['compute']['placement_groups']

    def create_placement_group(request, **kwargs):
        handle_action(state, f'placement_group.{request.name}.create')

        id = placement_group_id_seq(state)
        state_entity = dict(
            id=id,
            name=request.name,
            folder_id=request.folder_id,
        )

        state_endpoint[id] = state_entity

        response_model = placement_group_pb2.PlacementGroup()
        response_model.id = id

        return ret_grpc_operation(state, response=response_model)

    def delete_placement_group(request, **kwargs):
        group_id = request.placement_group_id
        handle_action(state, f'placement_group.{group_id}.delete')

        group = state_endpoint.get(group_id)

        if group:
            del state_endpoint[group_id]

        return ret_grpc_operation(state)

    stub = mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.placement_group.PlacementGroupClient._placement_group_service',
    )
    stub.Delete.side_effect = delete_placement_group
    stub.Create.side_effect = create_placement_group
    stub.Get.side_effect = gen_element_get_grpc(state, 'placement_groups')

    stub = mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.placement_group.OperationsClient._operation_service',
    )
    stub.Get = gen_element_get_grpc(state, 'operations')

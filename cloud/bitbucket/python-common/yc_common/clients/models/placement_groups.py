"""Instance placement groups"""

from yc_common.clients.models.base import BasePublicObjectModelV1Beta1, BaseListModel, BasePublicModel
from yc_common.clients.models.operations import OperationV1Beta1, OperationMetadataV1Beta1
from yc_common.fields import IntType, BooleanType, StringType, ListType, ModelType


class SpreadPlacementStrategy(BasePublicModel):
    max_instances_per_fault_domain = IntType()
    max_instances_per_node = IntType()
    best_effort = BooleanType()


class PlacementGroup(BasePublicObjectModelV1Beta1):
    spread_placement_strategy = ModelType(SpreadPlacementStrategy)


class PlacementGroupList(BaseListModel):
    placement_groups = ListType(ModelType(PlacementGroup), default=list)


class PlacementGroupMetadata(OperationMetadataV1Beta1):
    placement_group_id = StringType()


class PlacementGroupOperation(OperationV1Beta1):
    metadata = ModelType(PlacementGroupMetadata)

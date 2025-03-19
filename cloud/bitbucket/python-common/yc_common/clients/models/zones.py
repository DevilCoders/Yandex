from schematics import types as schematics_types

from yc_common import models as common_models
from yc_common import validation

from . import base as base_models
from . import operations as operations_models


class Zone(base_models.BasePublicModel):

    class Status:
        UP = "up"
        DOWN = "down"

        ALL = [UP, DOWN]

    id = schematics_types.StringType(required=True)
    status = common_models.StringEnumType(required=True)

    # Special fields for console, identity

    default = schematics_types.BooleanType()
    weight = schematics_types.IntType()


class ZonesList(base_models.BaseListModel):
    zones = schematics_types.ListType(schematics_types.ModelType(Zone), required=True, default=list)


class ZoneMetadata(operations_models.OperationMetadataV1Beta1):
    zone_id = schematics_types.StringType()


class ZoneOperation(operations_models.OperationV1Beta1):
    metadata = schematics_types.ModelType(ZoneMetadata)

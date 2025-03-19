"""Public Image models"""

from schematics import types as schematics_types

from yc_common import models as common_models
from yc_common.clients.models import base as base_models
from yc_common.clients.models import operations as operations_models
from yc_common.clients.models import operating_system as os_models


class Image(base_models.BasePublicObjectModelV1Beta1):
    class Status:
        CREATING = "creating"
        READY = "ready"
        ERROR = "error"
        DELETING = "deleting"

        ALL = [CREATING, READY, ERROR, DELETING]

    status = common_models.StringEnumType(required=True)
    storage_size = schematics_types.IntType(default=0)
    min_disk_size = schematics_types.IntType()
    family = schematics_types.StringType()
    product_ids = schematics_types.ListType(schematics_types.StringType)
    os = schematics_types.ModelType(os_models.OperatingSystem)

    # Special properties for internal users
    requirements = schematics_types.DictType(schematics_types.StringType)


# FIXME: Our server code drops empty image list, so we need default=list here
class ImageList(base_models.BaseListModel):
    images = schematics_types.ListType(schematics_types.ModelType(Image), required=True, default=list)


class ImageMetadata(operations_models.OperationMetadataV1Beta1):
    image_id = schematics_types.StringType()


class ImageOperation(operations_models.OperationV1Beta1):
    metadata = schematics_types.ModelType(ImageMetadata)
    response = schematics_types.ModelType(Image)


class DeleteImageOperation(operations_models.OperationV1Beta1):
    metadata = schematics_types.ModelType(ImageMetadata)

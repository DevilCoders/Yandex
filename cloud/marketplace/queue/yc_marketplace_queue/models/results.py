from schematics.types import StringType

from yc_common.clients.models.images import Image
from yc_common.models import Model
from yc_common.models import StringEnumType
from yc_common.validation import ResourceIdType


class MoveImageResult(Model):
    new_image_id = ResourceIdType(required=True)


class PublishLogoResult(Model):
    url = StringType(required=True)


class UpdateImagePoolSizeResult(Model):
    operation_id = ResourceIdType(required=True)


class FinalizeImageResult(Model):
    image_id = ResourceIdType(required=True)
    status = StringEnumType(choices=Image.Status.ALL, required=True)

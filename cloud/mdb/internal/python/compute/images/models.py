from datetime import datetime
from enum import Enum
from typing import NamedTuple

from yandex.cloud.priv.compute.v1 import image_pb2

from cloud.mdb.internal.python.compute.models import enum_from_api


class ImageStatus(Enum):
    UNKNOWN = image_pb2.Image.Status.STATUS_UNSPECIFIED
    CREATING = image_pb2.Image.Status.CREATING
    READY = image_pb2.Image.Status.READY
    ERROR = image_pb2.Image.Status.ERROR
    DELETING = image_pb2.Image.Status.DELETING

    @staticmethod
    def from_api(raw_data):
        return enum_from_api(raw_data, ImageStatus)


class ImageModel(NamedTuple):
    id: str
    name: str
    description: str
    folder_id: str
    family: str
    min_disk_size: int
    status: ImageStatus
    created_at: datetime
    status: ImageStatus
    labels: dict

    @staticmethod
    def from_api(raw_image: image_pb2.Image):
        return ImageModel(
            id=raw_image.id,
            name=raw_image.name,
            folder_id=raw_image.folder_id,
            family=raw_image.family,
            labels=raw_image.labels,
            description=raw_image.description,
            min_disk_size=raw_image.min_disk_size,
            created_at=raw_image.created_at.ToDatetime(),
            status=ImageStatus.from_api(raw_image.status),
        )

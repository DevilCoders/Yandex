"""Solomon API models"""

from yc_common.models import Model, ModelType, ListType, StringType


class ResourceType:
    INSTANCE = "instance"
    IMAGE = "image"
    DISK = "disk"

    ALL = [INSTANCE, IMAGE, DISK]


class ResourceTypesList(Model):
    types = ListType(StringType(), required=True)


class Resource(Model):
    id = StringType(required=True)
    cloud_id = StringType()
    folder_id = StringType()
    name = StringType()  # Name is an optional attribute


class ResourceList(Model):
    page_token = StringType()
    resources = ListType(ModelType(Resource), required=True)

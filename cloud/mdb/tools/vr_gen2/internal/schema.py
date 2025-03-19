from marshmallow import Schema
from marshmallow_dataclass import class_schema
from yaml import load, FullLoader

from .models import ResourceDefSchema


class BaseSchema(Schema):
    pass


def load_resource_definitions(string) -> ResourceDefSchema:
    loaded = load(string, Loader=FullLoader)
    ResourceDefinitionSchema = class_schema(ResourceDefSchema, base_schema=BaseSchema)
    result = ResourceDefinitionSchema().load(loaded)
    return result

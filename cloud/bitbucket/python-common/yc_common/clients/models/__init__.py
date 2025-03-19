from schematics.models import ModelMeta
from schematics.types import BaseType
from schematics.types.serializable import Serializable

from yc_common.models import Model, MetadataOptions
from yc_common.formatting import underscore_to_lowercamelcase


class Api2ModelMeta(ModelMeta):
    def __new__(cls, name, bases, namespace, **kargs):
        for field_name, field_type in namespace.items():
            if isinstance(field_type, BaseType):
                if field_type.serialized_name is None:
                    field_type.serialized_name = underscore_to_lowercamelcase(field_name)
            elif isinstance(field_type, Serializable):
                if field_type.type.serialized_name is None:
                    field_type.type.serialized_name = underscore_to_lowercamelcase(field_name)

        return super().__new__(cls, name, bases, namespace, **kargs)


class BasePublicModel(Model, metaclass=Api2ModelMeta):
    """Basic class for all public models"""

    @property
    def is_internal(self):
        for k, v in self.__class__.fields.items():
            if v.metadata.get(MetadataOptions.INTERNAL) and self.get(k) is not None:
                return True

        return False

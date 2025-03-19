from abc import ABC
from abc import ABCMeta
from abc import abstractmethod
from typing import Type
from typing import TypeVar

from schematics import ModelMeta

from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from yc_common import logging
from yc_common.clients.models import BasePublicModel
from yc_common.misc import timestamp
from yc_common.models import IsoTimestampType
from yc_common.models import Model
from yc_common.validation import ResourceIdType

log = logging.get_logger(__name__)


class AbstractMetaClass(ABCMeta, ModelMeta):
    pass


BasePublicModelDescendant = TypeVar("BasePublicModelDescendant", bound=BasePublicModel)


class AbstractMktBase(ABC, Model, metaclass=AbstractMetaClass):
    def __init__(self, *args, strict=False, **kwargs):
        super().__init__(*args, **kwargs, strict=False)

    @property
    @abstractmethod
    def id(self) -> ResourceIdType:
        return ResourceIdType(required=True)

    @property
    @abstractmethod
    def created_at(self) -> IsoTimestampType:
        return IsoTimestampType(required=True)

    @property
    @abstractmethod
    def PublicModel(self) -> Type[BasePublicModel]:
        return BasePublicModel

    @classmethod
    def db_fields(cls, table_name="") -> str:
        if table_name:
            table_name += "."

        cols = []
        for key, value in cls.fields.items():
            if value.serialized_name is not None:
                cols.append(table_name + value.serialized_name)
            else:
                cols.append(table_name + key)

        return ",".join(cols)

    @classmethod
    def new(cls, *args, id: ResourceIdType = None, created_at: int = None, **kwargs):
        return super().new(
            id=generate_id() if id is None else id,
            created_at=timestamp() if created_at is None else created_at,
            *args,
            **kwargs,
        )

    def to_public(self) -> BasePublicModelDescendant:
        return self.PublicModel(self.to_primitive())

    Filterable_fields = None


class MktBasePublicModel(BasePublicModel):

    def __init__(self, *args, strict=False, **kwargs):
        super().__init__(*args, **kwargs, strict=False)

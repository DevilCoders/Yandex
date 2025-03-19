from typing import List
from typing import Tuple
from typing import Union

from yc_common.clients.kikimr.client import KikimrTableSpec
from cloud.marketplace.common.yc_marketplace_common.utils.errors import MarketplaceBaseError


class DBError(MarketplaceBaseError):
    message = "Invalid data model"


class DataModel:
    def __init__(self, tables):
        self.tables = []
        for table in tables:
            self._validate_table(table)
            if not self._contain(table):
                self.tables.append(table)
            else:
                raise DBError()

    @staticmethod
    def _validate_table(table):
        if not isinstance(table, Table):
            raise DBError()

    def _contain(self, table) -> bool:
        for t in self.tables:
            if t.name == table.name:
                return True
        return False

    def serialize_to_common(self) -> List[Tuple[str, KikimrTableSpec]]:
        return list(map(lambda table: table.serialize_to_common(), self.tables))


class Table:
    name = ""
    spec = None

    def __init__(self, name: str, spec: Union[dict, KikimrTableSpec]) -> None:
        self.name = name
        if not isinstance(spec, KikimrTableSpec):
            self.spec = KikimrTableSpec(**spec)
        else:
            self.spec = spec

    def serialize_to_common(self) -> Tuple[str, KikimrTableSpec]:
        return (
            self.name,
            self.spec,
        )

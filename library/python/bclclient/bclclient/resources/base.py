from datetime import datetime
from typing import Optional, Union

from ..http import Connector


class ApiEntity:
    """Сущность  API."""


class ApiResource:
    """База для ресурсов API."""

    def __init__(self, connector: Connector):
        self._conn = connector

    @classmethod
    def _cast_date(cls, val: Optional[Union[datetime, str]]) -> str:

        if not val:
            return ''

        if isinstance(val, datetime):
            out = val.strftime('%Y-%m-%d')

        else:
            out = val

        return out

    @classmethod
    def _cast_bool(cls, val: Optional[bool]) -> str:
        return 'true' if val else 'false'

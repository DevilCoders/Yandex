import typing
from datetime import datetime
from dataclasses import dataclass

from cloud.ai.lib.python.serialization import OrderedYsonSerializable


# common params for whole markup session
@dataclass
class MarkupParams(OrderedYsonSerializable):
    markup_id: str
    params: dict
    received_at: datetime

    @staticmethod
    def primary_key() -> typing.List[str]:
        return ['markup_id']

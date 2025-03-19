import typing

from dataclasses import dataclass
from datetime import datetime

from cloud.ai.lib.python.serialization import OrderedYsonSerializable
from .common_markup import MarkupStep


@dataclass
class MarkupPool(OrderedYsonSerializable):
    id: str
    markup_id: str
    markup_step: MarkupStep
    params: dict
    toloka_environment: str
    received_at: datetime

    @staticmethod
    def primary_key() -> typing.List[str]:
        return ['id']

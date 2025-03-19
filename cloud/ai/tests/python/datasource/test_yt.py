import typing
import unittest
from dataclasses import dataclass
from datetime import datetime
from enum import Enum

from cloud.ai.lib.python.datasource.yt.model import generate_attrs
from cloud.ai.lib.python.serialization import YsonSerializable, OrderedYsonSerializable


class TestYT(unittest.TestCase):
    def test_generate_attrs(self):
        @dataclass
        class Part(YsonSerializable):
            name: str

        class Kind(Enum):
            OK = 'OK'

        @dataclass
        class Entry(OrderedYsonSerializable):
            price: int
            volume: float
            name: str
            checksum: bytes
            ready: bool
            part: Part
            id: str
            tags: typing.List[str]
            kind: typing.Optional[Kind]
            received_at: datetime

            @staticmethod
            def serialization_additional_fields():
                return {
                    'version': 'v1',
                    'version_ordinal': 1,
                }

            @staticmethod
            def primary_key() -> typing.List[str]:
                return ['id', 'name']

        self.assertEqual(
            {
                'schema': {
                    '$value': [
                        {
                            'name': 'id',
                            'type': 'string',
                            'sort_order': 'ascending',
                            'required': True,
                        },
                        {
                            'name': 'name',
                            'type': 'string',
                            'sort_order': 'ascending',
                            'required': True,
                        },
                        {
                            'name': 'price',
                            'type': 'int64',
                        },
                        {
                            'name': 'volume',
                            'type': 'double',
                        },
                        {
                            'name': 'checksum',
                            'type': 'string',
                        },
                        {
                            'name': 'ready',
                            'type': 'boolean',
                        },
                        {
                            'name': 'part',
                            'type': 'any',
                        },
                        {
                            'name': 'tags',
                            'type': 'any',
                        },
                        {
                            'name': 'kind',
                            'type': 'string',
                        },
                        {
                            'name': 'received_at',
                            'type': 'string',
                            'required': True,
                        },
                        {
                            'name': 'version',
                            'type': 'string',
                        },
                        {
                            'name': 'version_ordinal',
                            'type': 'int64',
                        },
                    ],
                    '$attributes': {'strict': True, 'unique_keys': True},
                }
            },
            generate_attrs(Entry, required={'received_at'}),
        )

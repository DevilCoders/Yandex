import re
from dataclasses import dataclass
from datetime import datetime
from enum import Enum
import typing

from cloud.ai.lib.python.serialization import YsonSerializable, OrderedYsonSerializable
from cloud.ai.speechkit.stt.lib.data.model.common.id import generate_id
from .records import Record


class RecordTagAction(Enum):
    ADD = 'ADD'
    DEL = 'DEL'

    def __lt__(self, other: 'RecordTagAction'):
        return self.value < other.value


# IMPORTANT: enum members order declares sort order, so append new types only to the end of list!
# If change of existing order is necessary, then composite tags migration will be required.
class RecordTagType(Enum):
    # Baskets
    KPI = 'KPI'  # KPI records basket

    # Source
    CLIENT = 'CLIENT'  # YC API client
    FOLDER = 'FOLDER'  # YC folder id
    IMPORT = 'IMPORT'  # Imported from some data source
    EXEMPLAR = 'EXEMPLAR'  # Exemplar records with ground truth joins
    MARKUP = 'MARKUP'  # Markup from Speechkit PRO

    # Subsource
    MODE = 'MODE'  # Cloud recognition mode – short, long, stream.
    CONVERT = 'CONVERT'  # Converted from some data storage format
    APP = 'APP'  # Source Yandex app
    DATASET = 'DATASET'  # Dataset ID, from which record was imported

    # Attributes
    LANG = 'LANG'
    ACOUSTIC = 'ACOUSTIC'

    # Temporal
    PERIOD = 'PERIOD'  # Describes time period when record was received

    # Misc
    OTHER = 'OTHER'

    # Attributes
    REGION = 'REGION'  # Region of toloka performer (for voice recorder tasks)


@dataclass
class RecordTagData(YsonSerializable):
    type: RecordTagType
    value: str

    @staticmethod
    def from_str(data_str: str) -> 'RecordTagData':
        parts = data_str.split(':')
        if len(parts) != 2:
            raise ValueError(f'Invalid tag data: {data_str}')
        tag_type_str, tag_value = parts
        tag_type = RecordTagType(tag_type_str)
        return RecordTagData(type=tag_type, value=tag_value)

    @staticmethod
    def create_period(received_at: datetime) -> 'RecordTagData':
        return RecordTagData(
            type=RecordTagType.PERIOD,
            value=f'{received_at.year}-{received_at.month:02d}',
        )

    @staticmethod
    def create_lang(language_code: str) -> 'RecordTagData':
        normalized_language_code = {
            'ru-ru': 'ru-RU',
            'en-us': 'en-US',
            'tr-tr': 'tr-TR',
            'kk-kk': 'kk-KK',
            'fr-fr': 'fr-FR',
            'de-de': 'de-DE',
            'sv-sv': 'sv-SV',
            'fi-fi': 'fi-FI',
            'az-az': 'az-AZ',
            'he-he': 'he-HE'
        }.get(language_code.lower())
        if normalized_language_code is None:
            raise ValueError(f'Unexpected language code: {language_code}')
        return RecordTagData(
            type=RecordTagType.LANG,
            value=normalized_language_code,
        )

    @staticmethod
    def create_lang_ru() -> 'RecordTagData':
        return RecordTagData(
            type=RecordTagType.LANG,
            value='ru-RU',
        )

    def to_str(self) -> str:
        return '%s:%s' % (self.type.value, self.value)


@dataclass
class RecordTag(OrderedYsonSerializable):
    id: str
    action: RecordTagAction
    record_id: str
    data: RecordTagData
    received_at: datetime

    @staticmethod
    def primary_key() -> typing.List[str]:
        return ['record_id', 'id', 'action']

    @staticmethod
    def add(record: Record, data: RecordTagData, received_at: datetime) -> 'RecordTag':
        return RecordTag.add_by_record_id(record_id=record.id, data=data, received_at=received_at)

    @staticmethod
    def add_by_record_id(record_id: str, data: RecordTagData, received_at: datetime) -> 'RecordTag':
        return RecordTag(
            id=generate_id(), action=RecordTagAction.ADD, record_id=record_id, data=data, received_at=received_at
        )

    @staticmethod
    def sanitize_value(raw_value: str):
        updated_value = raw_value.lower().strip().replace(" ", "-")
        match = re.match('^[a-z0-9_/-]+$', updated_value)

        if match is not None:
            return updated_value
        else:
            raise ValueError(f'Tag value contains incorrect characters: {updated_value}')

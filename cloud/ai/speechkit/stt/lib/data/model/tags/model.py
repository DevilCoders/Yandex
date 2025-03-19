import typing
from dataclasses import dataclass

from cloud.ai.speechkit.stt.lib.data.model.dao.records_tags import RecordTagData, RecordTagType


@dataclass
class RecordTagDataRequest:
    """
Tag conjunction entry.

Use cases:
1. Request records with specified tag type and tag value
CLIENT:mtt
2. Request records with specified tag
CLIENT
3. Request records without tags defined by 1, 2
~CLIENT:mtt
~CLIENT
    """
    type: RecordTagType
    value: typing.Optional[str]
    negation: bool

    @staticmethod
    def from_str(data_str: str) -> 'RecordTagDataRequest':
        negation = False
        if data_str.startswith('~'):
            negation = True
            data_str = data_str[1:]
        parts = data_str.split(':')
        if len(parts) > 1:
            tag_data = RecordTagData.from_str(data_str)
            tag_type = tag_data.type
            tag_value = tag_data.value
        else:
            tag_type = RecordTagType(data_str)
            tag_value = None
        return RecordTagDataRequest(
            type=tag_type,
            value=tag_value,
            negation=negation,
        )

    def to_yql_filter(self, tags_column: str):
        if self.value is None:
            operator = '=' if self.negation else '>'
            return f'$tags_count_with_type({tags_column}, "{self.type.value}") {operator} 0'
        else:
            operator = 'NOT ' if self.negation else ''
            tag_data = RecordTagData(type=self.type, value=self.value)
            return f'{operator}ListHas({tags_column}, "{tag_data.to_str()}")'

    def __hash__(self):
        return hash((self.type, self.value, self.negation))

    def __lt__(self, other: 'RecordTagDataRequest'):
        if self.type.value == other.type.value:
            if (self.value or '') == (other.value or ''):
                return self.negation < other.negation
            else:
                return (self.value or '') < (other.value or '')
        else:
            return self.type.value < other.type.value

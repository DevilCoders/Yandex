import abc
import sys
import typing
from datetime import datetime

from dataclasses import dataclass
from enum import Enum

from cloud.ai.lib.python.datasource.yt.ops import TableMeta
from cloud.ai.lib.python.serialization import YsonSerializable
from cloud.ai.speechkit.stt.lib.data.model.dao import TextComparisonStopWordsArcadiaSource
from cloud.ai.speechkit.stt.lib.data.ops.yt import get_tables_dir
from cloud.ai.speechkit.stt.lib.text.text_comparison_stop_words import get_text_comparison_stop_words_from_arcadia
from ..application import PreparedSlice


class PredicateType(Enum):
    CONTAIN_TEXTS = 'contain-texts'
    LENGTH = 'length'
    CUSTOM = 'custom'
    EXPRESSION = 'expression'


class Predicate(YsonSerializable):
    @abc.abstractmethod
    def to_eval_str(self, arcadia_token: str) -> str:
        pass


@dataclass
class PredicateContainTexts(Predicate):
    texts_source: TextComparisonStopWordsArcadiaSource
    words: bool
    negation: bool

    def to_eval_str(self, arcadia_token: str) -> str:
        texts = get_text_comparison_stop_words_from_arcadia(
            self.texts_source.topic,
            self.texts_source.lang,
            self.texts_source.revision,
            arcadia_token,
        )
        assert len(texts) > 0, 'texts list must not be empty'

        texts_str = ', '.join(f'"{t}"' for t in texts)
        negation = 'not ' if self.negation else ''
        if self.words:
            return f'{negation}any(word in {{{texts_str}, }} for word in text.split(" "))'
        else:
            return f'{negation}any(t in text for t in ({texts_str}, ))'

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'type': PredicateType.CONTAIN_TEXTS.value}


@dataclass
class PredicateLength(Predicate):
    min_length: typing.Optional[int]
    max_length: typing.Optional[int]

    def to_eval_str(self, arcadia_token: str) -> str:
        assert self.min_length is not None or self.max_length is not None
        min_length = self.min_length or 0
        max_length = self.max_length or sys.maxsize
        assert min_length >= 0
        assert min_length <= max_length
        expressions = []
        if min_length > 0:
            expressions.append(f'len(text) >= {min_length}')
        if max_length < sys.maxsize:
            expressions.append(f'len(text) <= {max_length}')
        return ' and '.join(expressions)

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'type': PredicateType.LENGTH.value}


@dataclass
class PredicateCustom(Predicate):
    eval_str: str

    def to_eval_str(self, arcadia_token: str) -> str:
        return self.eval_str

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'type': PredicateType.CUSTOM.value}


@dataclass
class PredicateExpression(Predicate):
    operator: str  # and / or
    predicates: typing.List[Predicate]

    def to_eval_str(self, arcadia_token: str) -> str:
        assert len(self.predicates) > 0
        return '(' + f' {self.operator} '.join(
            predicate.to_eval_str(arcadia_token) for predicate in self.predicates
        ) + ')'

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'type': PredicateType.EXPRESSION.value}

    @staticmethod
    def deserialization_overrides() -> typing.Dict[str, typing.Callable]:
        return {
            'predicates': get_predicate,
        }


def get_predicate(fields: dict) -> Predicate:
    try:
        return {
            PredicateType.CONTAIN_TEXTS: PredicateContainTexts,
            PredicateType.LENGTH: PredicateLength,
            PredicateType.CUSTOM: PredicateCustom,
            PredicateType.EXPRESSION: PredicateExpression,
        }[PredicateType(fields['type'])].from_yson(fields)
    except (KeyError, ValueError):
        raise ValueError(f'unsupported predicate: {fields}')


PredicateExpression._deserialization_overrides = {
    'predicates': get_predicate,
}


@dataclass
class SliceDescriptor(YsonSerializable):
    name: str
    predicate: Predicate
    received_at: datetime

    def prepare(self, arcadia_token: str) -> PreparedSlice:
        return PreparedSlice(
            name=self.name,
            predicate=self.predicate.to_eval_str(arcadia_token),
        )

    @staticmethod
    def deserialization_overrides() -> typing.Dict[str, typing.Callable]:
        return {
            'predicate': get_predicate,
        }


table_slices_meta = TableMeta(
    dir_path=get_tables_dir('slices'),
    attrs={
        'schema': {
            '$value': [
                {
                    'name': 'name',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'predicate',
                    'type': 'any',
                },
                {
                    'name': 'received_at',
                    'type': 'string',
                    'required': True,
                },
            ],
            '$attributes': {'strict': True, 'unique_keys': True},
        }
    },
)

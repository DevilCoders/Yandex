import typing
from datetime import datetime

from dataclasses import dataclass
from enum import Enum

from cloud.ai.lib.python.datetime import format_datetime, parse_datetime
from cloud.ai.lib.python.serialization import YsonSerializable
from .common_markup import MarkupData, MarkupStep


class MarkupAssignmentDataVersions(Enum):
    V1 = 'v1'


class MarkupAssignmentStatus(Enum):
    ACTIVE = 'ACTIVE'
    SUBMITTED = 'SUBMITTED'
    ACCEPTED = 'ACCEPTED'
    REJECTED = 'REJECTED'
    SKIPPED = 'SKIPPED'
    EXPIRED = 'EXPIRED'


@dataclass
class MarkupAssignmentDataV1:
    source_id: str
    task_suite_id: str
    user_id: str
    status: MarkupAssignmentStatus
    reward: float
    mixed: bool
    auto_merged: bool
    owner_id: str
    created_at: typing.Optional[datetime]
    submitted_at: typing.Optional[datetime]
    accepted_at: typing.Optional[datetime]
    rejected_at: typing.Optional[datetime]
    skipped_at: typing.Optional[datetime]
    expired_at: typing.Optional[datetime]

    def to_yson(self) -> dict:
        return {
            'version': MarkupAssignmentDataVersions.V1.value,
            'source_id': self.source_id,
            'task_suite_id': self.task_suite_id,
            'user_id': self.user_id,
            'status': self.status.value,
            'reward': self.reward,
            'mixed': self.mixed,
            'auto_merged': self.auto_merged,
            'owner_id': self.owner_id,
            'created_at': format_datetime(self.created_at),
            'submitted_at': format_datetime(self.submitted_at),
            'accepted_at': format_datetime(self.accepted_at),
            'rejected_at': format_datetime(self.rejected_at),
            'skipped_at': format_datetime(self.skipped_at),
            'expired_at': format_datetime(self.expired_at),
        }

    @staticmethod
    def from_yson(fields: dict) -> 'MarkupAssignmentDataV1':
        return MarkupAssignmentDataV1(
            source_id=fields['source_id'],
            task_suite_id=fields['task_suite_id'],
            user_id=fields['user_id'],
            status=MarkupAssignmentStatus(fields['status']),
            reward=fields['reward'],
            mixed=fields['mixed'],
            auto_merged=fields['auto_merged'],
            owner_id=fields['owner_id'],
            created_at=parse_datetime(fields['created_at']),
            submitted_at=parse_datetime(fields['submitted_at']),
            accepted_at=parse_datetime(fields['accepted_at']),
            rejected_at=parse_datetime(fields['rejected_at']),
            skipped_at=parse_datetime(fields['skipped_at']),
            expired_at=parse_datetime(fields['expired_at']),
        )


class HoneypotAcceptanceStrategy(Enum):
    DEFAULT = 'default'
    TRANSCRIPT_V1 = 'transcript-v1'


@dataclass
class SolutionFieldDiff:
    field_name: str
    expected_value: typing.Union[str, int, bool, Enum]
    actual_value: typing.Union[str, int, bool, Enum]
    extra: typing.Any

    def to_yson(self) -> dict:
        return {
            'field_name': self.field_name,
            'expected_value': self.expected_value,
            'actual_value': self.actual_value,
            'extra': self.extra,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'SolutionFieldDiff':
        return SolutionFieldDiff(
            field_name=fields['field_name'],
            expected_value=fields['expected_value'],
            actual_value=fields['actual_value'],
            extra=fields['extra'],
        )


@dataclass
class SolutionAcceptanceResult:
    accepted: bool
    diff_list: typing.List[SolutionFieldDiff]

    def to_yson(self) -> dict:
        return {
            'accepted': self.accepted,
            'diff_list': [diff.to_yson() for diff in self.diff_list],
        }

    @staticmethod
    def from_yson(fields: dict) -> 'SolutionAcceptanceResult':
        return SolutionAcceptanceResult(
            accepted=fields['accepted'],
            diff_list=[SolutionFieldDiff.from_yson(diff) for diff in fields['diff_list']],
        )

    def __eq__(self, other: 'SolutionAcceptanceResult'):
        return self.accepted == other.accepted and self.diff_list == other.diff_list


@dataclass
class SolutionHoneypotAcceptanceData:
    task_id: str
    known_solutions: typing.List[SolutionAcceptanceResult]  # for each known solution contains list of fields diffs

    def to_yson(self) -> dict:
        return {
            'task_id': self.task_id,
            'known_solutions': [solution.to_yson() for solution in self.known_solutions],
        }

    @staticmethod
    def from_yson(fields: dict) -> 'SolutionHoneypotAcceptanceData':
        return SolutionHoneypotAcceptanceData(
            task_id=fields['task_id'],
            known_solutions=[SolutionAcceptanceResult.from_yson(solution) for solution in fields['known_solutions']],
        )


@dataclass
class TextComparisonStopWordsArcadiaSource(YsonSerializable):
    topic: str
    lang: str
    revision: int

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'source': 'arcadia'}


class MarkupAssignmentValidationType(Enum):
    HONEYPOTS_ACCEPTANCE = 'honeypots-acceptance'
    TOLOKA_AUTO = 'toloka-auto'
    FEEDBACK_LOOP = 'feedback-loop'


@dataclass
class HoneypotsAcceptanceValidationData:
    accepted: bool
    correct_solutions: typing.List[SolutionHoneypotAcceptanceData]
    incorrect_solutions: typing.List[SolutionHoneypotAcceptanceData]
    min_correct_solutions: int
    acceptance_strategy: HoneypotAcceptanceStrategy
    rejection_comment: typing.Optional[str]

    def to_yson(self) -> dict:
        return {
            'type': MarkupAssignmentValidationType.HONEYPOTS_ACCEPTANCE.value,
            'accepted': self.accepted,
            'correct_solutions': [s.to_yson() for s in self.correct_solutions],
            'incorrect_solutions': [s.to_yson() for s in self.incorrect_solutions],
            'min_correct_solutions': self.min_correct_solutions,
            'acceptance_strategy': self.acceptance_strategy.value,
            'rejection_comment': self.rejection_comment,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'HoneypotsAcceptanceValidationData':
        return HoneypotsAcceptanceValidationData(
            accepted=fields['accepted'],
            correct_solutions=[SolutionHoneypotAcceptanceData.from_yson(s) for s in fields['correct_solutions']],
            incorrect_solutions=[SolutionHoneypotAcceptanceData.from_yson(s) for s in fields['incorrect_solutions']],
            min_correct_solutions=fields['min_correct_solutions'],
            acceptance_strategy=HoneypotAcceptanceStrategy(fields['acceptance_strategy']),
            rejection_comment=fields['rejection_comment'],
        )


@dataclass
class TolokaAutoValidationData(YsonSerializable):
    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'type': MarkupAssignmentValidationType.TOLOKA_AUTO.value}


# to get more data about specific tasks evaluations, join corresponding records_bits_markups
@dataclass
class FeedbackLoopValidationData(YsonSerializable):
    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'type': MarkupAssignmentValidationType.FEEDBACK_LOOP.value}


MarkupAssignmentValidationData = typing.Union[
    HoneypotsAcceptanceValidationData,
    TolokaAutoValidationData,
    FeedbackLoopValidationData,
]


def get_markup_assignment_validation_data(fields: dict) -> MarkupAssignmentValidationData:
    t = fields['type']
    if t == MarkupAssignmentValidationType.HONEYPOTS_ACCEPTANCE.value:
        return HoneypotsAcceptanceValidationData.from_yson(fields)
    elif t == MarkupAssignmentValidationType.TOLOKA_AUTO.value:
        return TolokaAutoValidationData()
    elif t == MarkupAssignmentValidationType.FEEDBACK_LOOP.value:
        return FeedbackLoopValidationData()
    else:
        raise ValueError(f'Unexpected markup assignment validation data: {fields}')


@dataclass
class MarkupAssignment:
    id: str
    markup_id: str
    markup_step: MarkupStep
    pool_id: str
    data: typing.Union[MarkupAssignmentDataV1]
    tasks: typing.List[MarkupData]
    validation_data: typing.List[MarkupAssignmentValidationData]
    received_at: datetime
    other: typing.Any

    def __lt__(self, other: 'MarkupAssignment'):
        return self.id < other.id

    def to_yson(self) -> dict:
        return {
            'id': self.id,
            'markup_id': self.markup_id,
            'markup_step': self.markup_step.value,
            'pool_id': self.pool_id,
            'data': self.data.to_yson(),
            'tasks': [x.to_yson() for x in self.tasks],
            'validation_data': [d.to_yson() for d in self.validation_data],
            'received_at': format_datetime(self.received_at),
            'other': self.other,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'MarkupAssignment':
        assignment_version = fields['data']['version']
        if assignment_version == MarkupAssignmentDataVersions.V1.value:
            data = MarkupAssignmentDataV1.from_yson(fields['data'])
        else:
            raise ValueError(f'Unsupported markup assignment data: {fields["data"]}')

        return MarkupAssignment(
            id=fields['id'],
            markup_id=fields['markup_id'],
            markup_step=MarkupStep(fields['markup_step']),
            pool_id=fields['pool_id'],
            data=data,
            tasks=[MarkupData.from_yson(x) for x in fields['tasks']],
            validation_data=[get_markup_assignment_validation_data(d) for d in fields['validation_data']],
            received_at=parse_datetime(fields['received_at']),
            other=fields['other'],
        )

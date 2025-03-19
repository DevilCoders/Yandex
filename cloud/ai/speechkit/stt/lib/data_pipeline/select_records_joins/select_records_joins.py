import typing
from collections import defaultdict
from datetime import datetime
from enum import Enum
import abc
from dataclasses import dataclass

from cloud.ai.lib.python.datetime import format_datetime
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    Recognition,
    JoinData,
    Mark,
    JoinDataExemplar,
    MultiChannelJoinData,
    AudioEncoding,
    get_channels_texts,
    RecordTagData,
    RecordTagType,
)
from cloud.ai.speechkit.stt.lib.data.model.tags import (
    parse_tag_conjunctions,
    validate_tag_conjunctions,
    filter_tags_by_tag_conjunction,
    prepare_tags_for_view,
    RecordTagDataRequest,
)
from cloud.ai.speechkit.stt.lib.data.ops.queries import (
    SelectRecordsJoinsByTagsRow,
    select_records_joins_by_tags,
    select_records_bits_joins_by_tags,
)


class PickingStrategyStatus(Enum):
    SUCCESS = 'success'
    FAILED = 'failed'


class BasePickingRecordJoinStrategy(abc.ABC):
    @abc.abstractmethod
    def pick_join(
        self,
        record_id: str,
        joins: typing.List[typing.Tuple[Recognition, JoinData, datetime]]
    ) -> typing.Tuple[PickingStrategyStatus, Recognition]:
        raise NotImplementedError

@dataclass
class PickingConcreteJoinStrategy(BasePickingRecordJoinStrategy):
    join_type: JoinData

    def is_concrete_join(self, join_data: JoinData) -> bool:
        if isinstance(join_data, MultiChannelJoinData):
            return all(
                isinstance(channel.join_data, self.join_type) for channel in join_data.channels
            )
        else:
            return isinstance(join_data, self.join_type)

    def pick_join(
        self,
        record_id: str,
        joins: typing.List[typing.Tuple[Recognition, JoinData, datetime]]
    ) -> typing.Tuple[PickingStrategyStatus, Recognition]:
        concrete_joins = [j for j in joins if self.is_concrete_join(j[1])]
        if len(concrete_joins) > 1:
            raise ValueError(f'Multiple {self.join_type} joins for record {record_id}')
        elif len(concrete_joins) == 1:
            print(f'{self.join_type} join is chosen')
            return PickingStrategyStatus.SUCCESS, concrete_joins[0][0]

        return PickingStrategyStatus.FAILED, None


@dataclass
class PickingLatestRecordJoinStrategy(BasePickingRecordJoinStrategy):
    reverse: bool

    def pick_join(
        self,
        record_id: str,
        joins: typing.List[typing.Tuple[Recognition, JoinData, datetime]]
    ) -> typing.Tuple[PickingStrategyStatus, Recognition]:
        join = sorted(joins, key=lambda r: r[2], reverse=self.reverse)[0]
        print(
            f'First join from list of sorted joins (reverse={self.reverse}) is chosen, received at {format_datetime(join[2])}')

        return PickingStrategyStatus.SUCCESS, join[0]


available_strategies = {
    'EXEMPLAR': PickingConcreteJoinStrategy(JoinDataExemplar),
    'LATEST': PickingLatestRecordJoinStrategy(reverse=True),
    'EARLIEST': PickingLatestRecordJoinStrategy(reverse=False)
}


def select_records_joins(
    tags: typing.List[str],
    mark: typing.Optional[Mark],
    bits: bool,
    join_received_after: typing.Optional[datetime],
    join_received_before: typing.Optional[datetime],
    duration_limit_minutes_per_tag: typing.Optional[float],
    filter_tags: bool,
    compose_tags: bool,
    picking_strategies: typing.List[BasePickingRecordJoinStrategy],
) -> typing.List[dict]:
    tag_conjunctions = parse_tag_conjunctions(tags)
    validate_tag_conjunctions(tag_conjunctions)
    if bits:
        selector = select_records_bits_joins_by_tags
    else:
        selector = select_records_joins_by_tags
    rows = selector(tag_conjunctions, mark, join_received_after, join_received_before)
    return produce_output(rows, tag_conjunctions, duration_limit_minutes_per_tag, filter_tags, compose_tags, picking_strategies)


def produce_output(
    rows: typing.List[SelectRecordsJoinsByTagsRow],
    tag_conjunctions: typing.List[typing.Set[RecordTagDataRequest]],
    duration_limit_minutes_per_tag: typing.Optional[float],
    filter_tags: bool,
    compose_tags: bool,
    picking_strategies: typing.List[BasePickingRecordJoinStrategy],
) -> typing.List[dict]:
    result = []

    duration_limit_seconds_per_tag = None
    if duration_limit_minutes_per_tag is not None and duration_limit_minutes_per_tag > 0:
        duration_limit_seconds_per_tag = duration_limit_minutes_per_tag * 60

    # Earliest join received_at is the only way to make deterministic subsample with limited duration.
    # Currently there is no sort order while selecting records for markup, so order defaults to records ID order,
    # which is random and do not correspond to receiving date or something else. But if some sort order for markup
    # records will appear, then our way to arrange subsample by join received_at may lead to unwanted side effects
    # (i.e. subsample will contain earliest received records). In this case we should create truly random subsample
    # by assigning some tag to records.
    rows.sort(key=lambda r: get_earliest_join_received_at(r))
    tag_to_duration_seconds = defaultdict(float)

    for row in rows:
        tags = row.tags
        if filter_tags:
            tags = filter_tags_by_tag_conjunction(tags, tag_conjunctions)
        tags = tuple(prepare_tags_for_view(tags, compose=compose_tags))

        if (
            duration_limit_seconds_per_tag is not None and
            tag_to_duration_seconds[tags] > duration_limit_seconds_per_tag
        ):
            continue
        tag_to_duration_seconds[tags] += row.duration

        channels_texts = choose_join(row.record_id, row.joins, picking_strategies)
        encoding = row.req_params.get_audio_encoding()
        spec = {
            'audio_encoding': encoding.value,
            'audio_channel_count': len(channels_texts),
        }
        if encoding == AudioEncoding.LPCM:
            spec['sample_rate_hertz'] = row.req_params.get_sample_rate_hertz()
        spec['lang'] = get_lang(row)

        result.append(
            {
                'id': row.record_id,
                's3_obj': row.s3_obj.to_yson(),
                'tags': tags,
                'spec': spec,
                'ref': channels_texts,
                'duration': row.duration,
            }
        )
    return result


def get_earliest_join_received_at(row: SelectRecordsJoinsByTagsRow) -> datetime:
    dates = [join[2] for join in row.joins]
    return sorted(dates)[0]


def choose_join(
    record_id: str,
    joins: typing.List[typing.Tuple[Recognition, JoinData, datetime]],
    picking_strategies: typing.List[BasePickingRecordJoinStrategy]
) -> typing.List[str]:
    assert len(joins) > 0

    if len(joins) == 1:
        return get_channels_texts(joins[0][0])

    print(f'Record {record_id} has {len(joins)} joins')

    for strategy in picking_strategies:
        status, recognition = strategy.pick_join(record_id, joins)
        if status == PickingStrategyStatus.SUCCESS:
            return get_channels_texts(recognition)

    raise RuntimeError(f'Unable to choose join for record {record_id}: all picking strategies failed!')


def get_lang(row: SelectRecordsJoinsByTagsRow) -> str:
    for tag_str in row.tags:
        tag = RecordTagData.from_str(tag_str)
        if tag.type == RecordTagType.LANG:
            return tag.value
    raise ValueError(f'record {row.record_id} has no LANG tag')

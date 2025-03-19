import os
import typing
from collections import defaultdict
from datetime import datetime

from cloud.ai.speechkit.stt.lib.data.model.dao import (
    JoinDataNoSpeechAllBits,
    JoinDataDpM3NaiveDev,
    RecordBitMarkup,
    BitDataTimeInterval,
    RecordJoin,
    RecognitionPlainTranscript,
    MarkupDataVersions,
    JoinDataOKCheckTranscriptMV,
    MultiChannelRecognition,
    MultiChannelJoinData,
    ChannelRecognition,
    ChannelJoinData,
    MarkupStep,
    OverlapStrategy,
)
import cloud.ai.speechkit.stt.lib.data_pipeline.transcript_feedback_loop as transcript
from cloud.ai.speechkit.stt.lib.data_pipeline.files import get_name_for_record_bit_audio_file_from_record_bit
from .common import RecordMarkupData, RecordChannelMarkupData, RecordBitMarkupData, group_records_bits_markups
from .io import call_markups_joiner, text_fetchers
from .majority import produce_majority_vote, MajorityVoteResult

# We have to filter empty texts and low confidence texts before sending them to markup joiner.
# If transcript markups have majority votes, then it is no speech decision. If check transcript markups
# have no majority votes, then transcript hypothesis has low confidence (but it still can be used if we want).
# Also, if markup contains empty text, regardless of the markup type, it will also be filtered.
#
# Note: check transcript markups will be preserved with their overlap count, may be it is
# not ok (transcript markups with overlap=1 in the middle of record vs check transcript markups
# with overlap>1 in the middle of record, may be not fair).

filter_joiner_input_strategies = {
    MarkupDataVersions.PLAIN_TRANSCRIPT: lambda majority_vote_result: majority_vote_result is None,
    MarkupDataVersions.TRANSCRIPT_AND_TYPE: lambda majority_vote_result: majority_vote_result is None,
    MarkupDataVersions.CHECK_TRANSCRIPT: lambda majority_vote_result: majority_vote_result is not None
    and isinstance(majority_vote_result.join_data, JoinDataOKCheckTranscriptMV),
}


def produce_majority_votes_joins_and_filter_joiner_input(
    records_markups_data: typing.List[RecordMarkupData],
    markup_step_to_overlap_strategy: typing.Dict[MarkupStep, OverlapStrategy],
) -> typing.Tuple[
    typing.List[RecordMarkupData],
    typing.Dict[str, MajorityVoteResult],
]:
    bit_id_to_majority_vote_result = {}
    filtered_records_markups_data = []
    for record_markups_data in records_markups_data:
        filtered_record_markups_data = RecordMarkupData(
            record_id=record_markups_data.record_id,
            channels=[],
        )
        for record_channel_markups_data in record_markups_data.channels:
            filtered_record_channel_bits_markups_data = []
            for record_bit_markup_data in record_channel_markups_data.bits_markups:
                version_to_filtered_markups = {}
                for markup_version, markups in record_bit_markup_data.version_to_markups.items():
                    assert len({markup.markup_step for markup in markups}) == 1
                    markup_step = markups[0].markup_step
                    overlap_strategy = markup_step_to_overlap_strategy.get(markup_step)
                    majority_vote_result = produce_majority_vote(markups, overlap_strategy)
                    if filter_joiner_input_strategies[markup_version](majority_vote_result):
                        not_empty_text_markups = [
                            markup for markup in markups if len(text_fetchers[markup_version](markup.markup_data)) > 0
                        ]
                        if len(not_empty_text_markups) > 0:
                            version_to_filtered_markups[markup_version] = not_empty_text_markups
                    if majority_vote_result is not None:
                        if markups[0].bit_id not in bit_id_to_majority_vote_result:
                            bit_id_to_majority_vote_result[markups[0].bit_id] = majority_vote_result
                if len(version_to_filtered_markups) > 0:
                    filtered_record_channel_bits_markups_data.append(
                        RecordBitMarkupData(
                            bit_data=record_bit_markup_data.bit_data,
                            version_to_markups=version_to_filtered_markups,
                        )
                    )
            if len(filtered_record_channel_bits_markups_data) > 0:
                filtered_record_markups_data.channels.append(
                    RecordChannelMarkupData(
                        channel=record_channel_markups_data.channel,
                        bits_markups=filtered_record_channel_bits_markups_data,
                    )
                )
        if len(filtered_record_markups_data.channels) > 0:
            filtered_records_markups_data.append(filtered_record_markups_data)

    return filtered_records_markups_data, bit_id_to_majority_vote_result


def join_markups(
    markups_with_bit_data: typing.List[typing.Tuple[RecordBitMarkup, typing.Union[BitDataTimeInterval]]],
    markup_step_to_overlap_strategy: typing.Dict[MarkupStep, OverlapStrategy],
    markup_joiner_executable_path: str,
    external_records_bits_joins: typing.List[transcript.RecordBitJoins],
    bit_offset: int,
    received_at: datetime,
) -> typing.List[RecordJoin]:
    if len(external_records_bits_joins) > 0:
        # transcript step joins are already obtained using an external algorithm (i.e. feedback loop)
        # so we collect majority vote joins only for markups occurred before transcript step (i.e. check asr)
        # old way chooses best bit text which has min distance to other bit texts
        # in feedback loop we already has text confidences and can select best bit text
        audio_name_to_best_text = {
            get_name_for_record_bit_audio_file_from_record_bit(j.record_bit): j.joins[0].recognition.text
            for j in external_records_bits_joins
        }
        markups_with_bit_data = [
            m for m in markups_with_bit_data if
            m[0].markup_step == MarkupStep.CHECK_ASR_TRANSCRIPT or
            m[0].markup_step == MarkupStep.TRANSCRIPT and
            audio_name_to_best_text[
                os.path.basename(m[0].markup_data.input.audio_s3_obj.key)
            ] == m[0].markup_data.solution.text
        ]

    records_markups_data = group_records_bits_markups(markups_with_bit_data)
    filtered_records_markups_data, bit_id_to_majority_vote_result = \
        produce_majority_votes_joins_and_filter_joiner_input(records_markups_data, markup_step_to_overlap_strategy)

    joins_fields = []

    joined_by_mv_entities_ids = set()
    for bit_id, majority_vote_result in bit_id_to_majority_vote_result.items():
        recognition = RecognitionPlainTranscript(text=majority_vote_result.text)
        join_data = majority_vote_result.join_data
        joins_fields.append((False, bit_id, recognition, join_data))
        joined_by_mv_entities_ids.add(bit_id)

    records_ids_to_channels_count = defaultdict(int)
    bits_ids = set()
    for markup_with_bit_data in markups_with_bit_data:
        record_id = markup_with_bit_data[0].record_id
        bit_id = markup_with_bit_data[0].bit_id
        channel = markup_with_bit_data[1].channel
        bits_ids.add(bit_id)
        # TODO: weird way to get channel count, may be pass records array here
        if channel > records_ids_to_channels_count[record_id]:
            records_ids_to_channels_count[record_id] = channel

    joiner_joined_entity_id_to_channel_texts = call_markups_joiner(
        markup_joiner_executable_path, bit_offset, filtered_records_markups_data
    )

    bit_id_to_external_joins = {j.record_bit.id: j.joins for j in external_records_bits_joins}

    for bit_id in bits_ids:
        if bit_id in joined_by_mv_entities_ids:
            # Check transcript majority votes texts will be returned again like they were joined.
            continue
        if bit_id in bit_id_to_external_joins:
            joins = bit_id_to_external_joins[bit_id]
            multiple_per_moment = len(joins) > 1
            for j in joins:
                joins_fields.append((multiple_per_moment, bit_id, j.recognition, j.join_data))
        else:
            # old way chooses best bit text which has min distance to other bit texts
            text = joiner_joined_entity_id_to_channel_texts[bit_id][1]
            recognition = RecognitionPlainTranscript(text=text)
            join_data = JoinDataDpM3NaiveDev()
            joins_fields.append((False, bit_id, recognition, join_data))

    for record_id, channels_count in records_ids_to_channels_count.items():
        assert record_id not in joined_by_mv_entities_ids
        recognition_channels = []
        join_data_channels = []
        for channel in range(1, channels_count + 1):
            if (
                record_id in joiner_joined_entity_id_to_channel_texts and
                channel in joiner_joined_entity_id_to_channel_texts[record_id]
            ):
                text = joiner_joined_entity_id_to_channel_texts[record_id][channel]
                join_data = JoinDataDpM3NaiveDev()
            else:
                # Bit texts for this channel not passed to joiner, therefore all bit texts where empty
                text = ''
                join_data = JoinDataNoSpeechAllBits()
            recognition_channels.append(ChannelRecognition(
                channel=channel,
                recognition=RecognitionPlainTranscript(text=text),
            ))
            join_data_channels.append(ChannelJoinData(
                channel=channel,
                join_data=join_data,
            ))

        if channels_count > 1:
            recognition = MultiChannelRecognition(channels=recognition_channels)
            join_data = MultiChannelJoinData(channels=join_data_channels)
        else:
            # Simplify data structure
            recognition = recognition_channels[0].recognition
            join_data = join_data_channels[0].join_data

        joins_fields.append((False, record_id, recognition, join_data))

    joins = []
    for multiple_per_moment, entity_id, recognition, join_data in joins_fields:
        joins.append(
            RecordJoin.create_by_record_id(
                record_id=entity_id,
                recognition=recognition,
                join_data=join_data,
                received_at=received_at,
                multiple_per_moment=multiple_per_moment,
                other=None,
            )
        )

    return joins

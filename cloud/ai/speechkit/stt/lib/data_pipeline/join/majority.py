from dataclasses import dataclass
import typing

from cloud.ai.speechkit.stt.lib.data.model.dao import (
    RecordBit,
    RecordBitMarkup,
    MarkupDataVersions,
    MarkupTranscriptType,
    JoinDataNoSpeechTranscriptMV,
    JoinDataOKCheckTranscriptMV,
    JoinDataNotOKCheckTranscriptNoSpeechMV,
    JoinData,
    MajorityVote,
    BitDataTimeInterval,
    OverlapStrategy,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.files import get_name_for_record_bit_audio_file

from .common import group_records_bits_markups

majority_vote_strategies = {
    MarkupDataVersions.PLAIN_TRANSCRIPT: [
        (
            lambda solution: solution.misheard or solution.text in ['', '-', '?'],
            lambda solution, input, votes, overlap_strategy: (JoinDataNoSpeechTranscriptMV(votes, overlap_strategy), ''),
        )
    ],
    MarkupDataVersions.TRANSCRIPT_AND_TYPE: [
        (
            lambda solution: solution.type != MarkupTranscriptType.SPEECH,
            lambda solution, input, votes, overlap_strategy: (JoinDataNoSpeechTranscriptMV(votes, overlap_strategy), ''),
        )
    ],
    MarkupDataVersions.CHECK_TRANSCRIPT: [
        (
            lambda solution: solution.ok,
            lambda solution, input, votes, overlap_strategy: (JoinDataOKCheckTranscriptMV(votes, overlap_strategy), input.text),
        ),
        (
            lambda solution: not solution.ok and solution.type != MarkupTranscriptType.SPEECH,
            lambda solution, input, votes, overlap_strategy: (JoinDataNotOKCheckTranscriptNoSpeechMV(votes, overlap_strategy), ''),
        ),
    ],
}


@dataclass
class MajorityVoteResult:
    join_data: JoinData
    text: str


# Check if users markups match enough to make decision about record bit text
# without additional transcript task or without algorithmic text joining.
def produce_majority_vote(
    markups: typing.List[RecordBitMarkup],
    overlap_strategy: typing.Optional[OverlapStrategy],
) -> typing.Optional[MajorityVoteResult]:
    # Markups during vote should always belong to only one type and to one bit.
    if len({m.markup_data.version for m in markups}) > 1:
        raise ValueError('Different markup versions')
    if len({m.bit_id for m in markups}) > 1:
        raise ValueError('Markups for different bits')
    first_markup_data = markups[0].markup_data
    version = first_markup_data.version
    if version not in majority_vote_strategies:
        return None
    for predicate, result_func in majority_vote_strategies[version]:
        matched_predicate_count = len([m for m in markups if predicate(m.markup_data.solution)])
        votes = MajorityVote(majority=matched_predicate_count, overall=len(markups))
        if overlap_strategy is None:
            is_majority = votes.majority > votes.overall // 2
        else:
            is_majority = overlap_strategy.check_majority_vote(votes)
        if is_majority:
            join_data, text = result_func(first_markup_data.solution, first_markup_data.input, votes, overlap_strategy)
            return MajorityVoteResult(join_data, text)
    return None


# We can omit transcription tasks for records bits with majority votes on markups.
# This function filters obfuscated records bits S3 URLs.
def filter_records_bits_to_transcript(
    markups_with_bit_data: typing.List[typing.Tuple[RecordBitMarkup, typing.Union[BitDataTimeInterval]]],
    records_bits_s3_urls: typing.Dict[str, str],
    overlap_strategy: typing.Optional[OverlapStrategy],
) -> typing.Dict[str, str]:
    filtered_records_bits_s3_urls = {}
    records_markups_data = group_records_bits_markups(markups_with_bit_data)
    for record_markup_data in records_markups_data:
        for record_channel_markup_data in record_markup_data.channels:
            for record_bit_markup_data in record_channel_markup_data.bits_markups:
                if all(
                    produce_majority_vote(markups, overlap_strategy) is None
                    for markups in record_bit_markup_data.version_to_markups.values()
                ):
                    bit_name = get_name_for_record_bit_audio_file(
                        record_id=record_markup_data.record_id,
                        channel=record_bit_markup_data.bit_data.channel,
                        start_ms=record_bit_markup_data.bit_data.start_ms,
                        end_ms=record_bit_markup_data.bit_data.end_ms,
                    )
                    filtered_records_bits_s3_urls[bit_name] = records_bits_s3_urls[bit_name]
    return filtered_records_bits_s3_urls


def combine_markups_with_bits_data(
    records_bits_markups: typing.List[RecordBitMarkup],
    records_bits: typing.List[RecordBit],
) -> typing.List[typing.Tuple[RecordBitMarkup, typing.Union[BitDataTimeInterval]]]:
    bit_id_to_bit = {b.id: b for b in records_bits}
    markups_with_bit_data = []
    for markup in records_bits_markups:
        bit_data = bit_id_to_bit[markup.bit_id].bit_data
        markups_with_bit_data.append((markup, bit_data))
    return markups_with_bit_data

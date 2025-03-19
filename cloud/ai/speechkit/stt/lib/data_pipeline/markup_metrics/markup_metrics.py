import operator
import typing
from collections import defaultdict
from datetime import datetime
import itertools
import numpy as np

import dataforge.feedback_loop as feedback_loop

from cloud.ai.speechkit.stt.lib.data.model.dao import (
    MetricMarkup, MarkupStep, RecordBitMarkup, MarkupSolution, MarkupSolutionCheckTranscript,
    MarkupAssignment, MarkupPool, MetricMarkupStep, MarkupAssignmentStatus, MarkupMetadata,
    MarkupType, RecordTagData, RecordTagType, Record, OverlapStrategy, RecordBit, RecordsBitsStats,
    percentiles,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.files import (
    get_name_for_audio_file_from_s3_key,
    get_record_id_and_channel_and_start_ms_and_end_ms_by_record_bit_filename,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.markup_params import quality_evaluation_overlap_strategy
from cloud.ai.speechkit.stt.lib.data_pipeline.markup_quality import HoneypotsQuality


def calculate_metrics(
    markup_metadata: MarkupMetadata,
    lang: str,
    markup_step_to_assignments: typing.Dict[MarkupStep, typing.List[MarkupAssignment]],
    markup_step_to_pool: typing.Dict[MarkupStep, typing.Optional[MarkupPool]],
    markup_step_to_honeypots_quality: typing.Dict[MarkupStep, typing.Optional[HoneypotsQuality]],
    filtered_transcript_bits_ratio: float,
    transcript_feedback_loop: typing.Optional[feedback_loop.Metrics],
    quality_evaluation_markups: typing.List[RecordBitMarkup],
    records: typing.List[Record],
    records_bits: typing.List[RecordBit],
    regular_tags: typing.List[RecordTagData],
    received_at: datetime,
    nv_context: typing.Any,
) -> MetricMarkup:
    assert len(quality_evaluation_markups) > 0

    step_to_metrics = {}
    assignment_id_to_bits_duration_sum_minutes_without_honeypots = {}
    assignment_id_to_duration_minutes = {}
    record_id_to_bits_duration_sum_minutes_in_assignment = {}
    for markup_step in markup_step_to_assignments.keys():
        pool = markup_step_to_pool[markup_step]
        if pool is None:
            step_to_metrics[markup_step] = None
            continue

        assignments = markup_step_to_assignments[markup_step]

        if transcript_feedback_loop is not None and markup_step in [MarkupStep.CHECK_TRANSCRIPT,
                                                                    MarkupStep.TRANSCRIPT]:
            # feedback loop can run with dynamic reward strategy, so cost can't be calculated just as
            # len(submitted_assignments) * assignment_cost
            if markup_step == MarkupStep.TRANSCRIPT:
                markup_cost = transcript_feedback_loop.cost_markup
            else:
                markup_cost = transcript_feedback_loop.cost_check
        else:
            assignment_cost = pool.params['reward_per_assignment']
            markup_cost = assignment_cost * len(
                [assignment for assignment in assignments if assignment.data.status == MarkupAssignmentStatus.ACCEPTED]
            )

        toloker_speeds = []
        for assignment in assignments:
            assignment_duration_minutes = get_assignment_duration_minutes(assignment)
            assignment_bits_duration_sum_minutes = 0
            assignment_bits_duration_sum_minutes_without_honeypots = 0
            for task in assignment.tasks:
                is_honeypot = len(task.known_solutions) > 0
                bit_filename = get_name_for_audio_file_from_s3_key(task.input.audio_s3_obj.key)
                record_id, _, start_ms, end_ms = \
                    get_record_id_and_channel_and_start_ms_and_end_ms_by_record_bit_filename(bit_filename)
                task_audio_duration_minutes = float(end_ms - start_ms) / 1000.0 / 60.0
                assignment_bits_duration_sum_minutes += task_audio_duration_minutes
                if not is_honeypot:
                    assignment_bits_duration_sum_minutes_without_honeypots += task_audio_duration_minutes
                    if record_id not in record_id_to_bits_duration_sum_minutes_in_assignment:
                        record_id_to_bits_duration_sum_minutes_in_assignment[record_id] = defaultdict(float)
                    record_id_to_bits_duration_sum_minutes_in_assignment[record_id][assignment.id] += \
                        task_audio_duration_minutes
            toloker_speeds.append(assignment_bits_duration_sum_minutes / assignment_duration_minutes)
            assignment_id_to_bits_duration_sum_minutes_without_honeypots[assignment.id] = \
                assignment_bits_duration_sum_minutes_without_honeypots
            assignment_id_to_duration_minutes[assignment.id] = assignment_duration_minutes

        toloker_speed_mean = sum(toloker_speeds) / len(toloker_speeds)

        honeypots_quality = markup_step_to_honeypots_quality[markup_step]
        all_assignments_accuracy_ratio = None if honeypots_quality is None else honeypots_quality.all_assignments_accuracy_ratio
        accepted_assignments_accuracy_ratio = None if honeypots_quality is None else honeypots_quality.accepted_assignments_accuracy_ratio

        step_metrics = MetricMarkupStep(
            pool_id=pool.id,
            cost=markup_cost,
            toloker_speed_mean=toloker_speed_mean,
            all_assignments_honeypots_quality_mean=all_assignments_accuracy_ratio,
            accepted_assignments_honeypots_quality_mean=accepted_assignments_accuracy_ratio,
        )
        step_to_metrics[markup_step] = step_metrics

    record_id_to_duration_minutes = {r.id: r.audio_params.duration_seconds / 60.0 for r in records}

    records_markup_speeds = []
    for record_id, assignment_id_to_bits_durations_sum_minutes in record_id_to_bits_duration_sum_minutes_in_assignment.items():
        record_markup_duration_minutes = 0
        for assignment_id, bits_durations_sum_minutes in assignment_id_to_bits_durations_sum_minutes.items():
            assignment_all_bits_duration_sum_minutes_without_honeypots = \
                assignment_id_to_bits_duration_sum_minutes_without_honeypots[assignment_id]
            assignment_duration_minutes = assignment_id_to_duration_minutes[assignment_id]
            record_ratio_in_assignment = bits_durations_sum_minutes / assignment_all_bits_duration_sum_minutes_without_honeypots
            record_markup_duration_minutes += assignment_duration_minutes * record_ratio_in_assignment
        records_markup_speeds.append(record_markup_duration_minutes / record_id_to_duration_minutes[record_id])

    accuracy, not_evaluated_records_ratio = calculate_quality_evaluation_values(
        quality_evaluation_markups, quality_evaluation_overlap_strategy)

    records_durations_sum_minutes = sum(s.duration_seconds for s in markup_metadata.tags_stats) / 60.0

    markup_duration_minutes = (received_at - markup_metadata.started_at).total_seconds() / 60.0

    metrics = MetricMarkup(
        markup_id=markup_metadata.markup_id,
        markup_type=infer_markup_type_from_metadata(markup_metadata, regular_tags),
        markup_metadata=markup_metadata,
        language=lang,
        records_durations_sum_minutes=records_durations_sum_minutes,
        markup_duration_minutes=markup_duration_minutes,
        filtered_transcript_bits_ratio=filtered_transcript_bits_ratio,
        accuracy=accuracy,
        not_evaluated_records_ratio=not_evaluated_records_ratio,
        record_markup_speed_mean=sum(records_markup_speeds) / len(records_markup_speeds),
        step_to_metrics=step_to_metrics,
        transcript_feedback_loop=transcript_feedback_loop,
        bits_stats=generate_records_bits_stats(records, records_bits),
        nirvana_workflow_url=nv_context.get_meta().workflow_url,
        received_at=received_at,
    )

    return metrics


def get_assignment_duration_minutes(assignment: MarkupAssignment) -> float:
    return float((assignment.data.submitted_at - assignment.data.created_at).total_seconds()) / 60.0


def generate_records_bits_stats(
    records: typing.List[Record],
    records_bits: typing.List[RecordBit],
) -> RecordsBitsStats:
    def record_id_key(record_bit: RecordBit) -> str:
        return record_bit.record_id

    durations_seconds = []
    overlaps = []

    id_to_record = {record.id: record for record in records}
    records_bits = sorted(records_bits, key=record_id_key)
    for _, bits in itertools.groupby(records_bits, key=record_id_key):
        bits = list(bits)
        record = id_to_record[bits[0].record_id]
        bits_durations_seconds = [(bit.bit_data.end_ms - bit.bit_data.start_ms) / 1000. for bit in bits]
        durations_seconds += bits_durations_seconds
        overlap = sum(bits_durations_seconds) / record.audio_params.duration_seconds / float(record.audio_params.channel_count)
        overlaps += [overlap] * len(bits)  # consider bits overlaps proportionally to record length

    durations_seconds = np.array(durations_seconds)
    overlaps = np.array(overlaps)

    durations_seconds_percentiles = {p: np.percentile(durations_seconds, p) for p in percentiles}
    overlaps_percentiles = {p: np.percentile(overlaps, p) for p in percentiles}

    return RecordsBitsStats(durations_seconds_percentiles, overlaps_percentiles)


def infer_markup_type_from_metadata(md: MarkupMetadata, regular_tags: typing.List[RecordTagData]) -> MarkupType:
    tags: typing.List[typing.List[RecordTagData]] = []
    for stats in md.tags_stats:
        tags.append([RecordTagData.from_str(tag) for tag in stats.tags])

    def regular_markup_predicate(tag_data: RecordTagData) -> bool:
        return tag_data.type == RecordTagType.CLIENT or tag_data.type == RecordTagType.MODE or tag_data in regular_tags

    def external_markup_predicate(tag_data: RecordTagData) -> bool:
        return tag_data.type == RecordTagType.MARKUP

    def tags_meet_predicate(
        predicate: typing.Callable[[RecordTagData], bool],
    ) -> bool:
        return all(
            any(predicate(tag_data) for tag_data in tags_conjunction)
            for tags_conjunction in tags
        )

    if tags_meet_predicate(regular_markup_predicate):
        return MarkupType.REGULAR

    if tags_meet_predicate(external_markup_predicate):
        return MarkupType.EXTERNAL

    return MarkupType.CUSTOM


# poor code starts from here
# TODO: refactor accuracy calculation, reusing grouping and majority vote code from join module
def calculate_quality_evaluation_values(
    markups: typing.List[RecordBitMarkup],
    overlap_strategy: OverlapStrategy,
) -> typing.Tuple[float, float]:
    assert len(markups) > 0

    bit_id_to_markups = defaultdict(list)
    for markup in markups:
        bit_id_to_markups[markup.bit_id].append(markup)

    accurate_joins = 0
    evaluated_joins = 0
    total_joins = 0
    for bit_markups in bit_id_to_markups.values():
        total_joins += 1
        solution, count = get_majority_solution(bit_markups)
        if count >= overlap_strategy.min_majority:
            assert isinstance(solution, MarkupSolutionCheckTranscript)
            if solution.ok:
                accurate_joins += 1
            evaluated_joins += 1

    if evaluated_joins == 0:
        accuracy = -1
    else:
        accuracy = accurate_joins / evaluated_joins
    not_evaluated_records_ratio = (total_joins - evaluated_joins) / total_joins

    return accuracy, not_evaluated_records_ratio


def get_majority_solution(markups: typing.List[RecordBitMarkup]) -> typing.Tuple[MarkupSolution, int]:
    solution_to_count = defaultdict(int)
    assert len({markup.markup_data.version for markup in markups}) == 1
    for markup in markups:
        solution = markup.markup_data.solution
        assert isinstance(solution, MarkupSolutionCheckTranscript)
        solution_to_count[(solution.ok, solution.type)] += 1
    solution_fields, count = max(solution_to_count.items(), key=operator.itemgetter(1))
    ok, type = solution_fields
    return MarkupSolutionCheckTranscript(ok, type), count

import typing
from collections import defaultdict
from datetime import datetime, timedelta

import yt.wrapper as yt
from dataclasses import dataclass

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.lib.python.datetime import parse_datetime
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    MetricMarkup, MarkupMetadata, MarkupPriorities, RecordTagType, TagsMarkupStats, MarkupData,
    MetricMarkupStep, MarkupStep,
)
from cloud.ai.speechkit.stt.lib.data.model.tags import prepare_tags_for_view
from cloud.ai.speechkit.stt.lib.data.ops.yt import table_metrics_markup_meta
from cloud.ai.speechkit.stt.lib.data_pipeline.files import (
    get_name_for_audio_file_from_s3_key,
    get_record_id_and_channel_and_start_ms_and_end_ms_by_record_bit_filename,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.markup_metrics import infer_markup_type_from_metadata

dry_run = False


@dataclass
class Assignment:
    tasks: typing.List[MarkupData]
    duration_minutes: float
    created_at: datetime


def main():
    yt.config['proxy']['url'] = 'hahn'
    print('loading data...', flush=True)
    record_id_to_duration_seconds, markup_id_to_metadata = load_records_data()
    markup_id_to_assignments = load_assignments_data()
    for table_name in yt.list(table_metrics_markup_meta.dir_path):
        print(f'processing table {table_name}...', flush=True)
        recalc_table(
            table_name,
            record_id_to_duration_seconds,
            markup_id_to_metadata,
            markup_id_to_assignments,
        )


def recalc_table(
    table_name: str,
    record_id_to_duration_seconds: typing.Dict[str, float],
    markup_id_to_metadata: typing.Dict[str, MarkupMetadata],
    markup_id_to_assignments: typing.Dict[str, typing.List[Assignment]],
):
    table = Table(meta=table_metrics_markup_meta, name=table_name)
    metrics = []
    for row in yt.read_table(table.path):
        markup_id = row['markup_id']
        if markup_id not in markup_id_to_metadata or markup_id not in markup_id_to_assignments:
            print(f'markup {markup_id} not found in data, removing it from metrics')
            continue
        metric = recalc_markup(
            row,
            record_id_to_duration_seconds,
            markup_id_to_metadata[markup_id],
            markup_id_to_assignments[markup_id],
        )
        metrics.append(metric)
        print(f'\tmarkup: {markup_id}\n'
              f'\trecords markups speed: {metric.record_markup_speed_mean}\n'
              f'\tsession speed: {metric.records_durations_sum_minutes / metric.markup_duration_minutes}\n'
              f'\ttype: {metric.markup_type.value}', flush=True)

    if dry_run:
        return

    yt.remove(table.path)
    table.append_objects(metrics)


def recalc_markup(
    metrics_row: dict,
    record_id_to_duration_seconds: typing.Dict[str, float],
    metadata: MarkupMetadata,
    assignments: typing.List[Assignment],
) -> MetricMarkup:
    assignment_id_to_bits_duration_sum_minutes_without_honeypots = {}
    assignment_id_to_duration_minutes = {}
    record_id_to_bits_duration_sum_minutes_in_assignment = {}

    for assignment_id, assignment in enumerate(assignments):
        assignment_bits_duration_sum_minutes_without_honeypots = 0
        for task in assignment.tasks:
            is_honeypot = len(task.known_solutions) > 0
            bit_filename = get_name_for_audio_file_from_s3_key(task.input.audio_s3_obj.key)
            record_id, _, start_ms, end_ms = \
                get_record_id_and_channel_and_start_ms_and_end_ms_by_record_bit_filename(bit_filename)
            task_audio_duration_minutes = float(end_ms - start_ms) / 1000.0 / 60.0
            if not is_honeypot:
                assignment_bits_duration_sum_minutes_without_honeypots += task_audio_duration_minutes
                if record_id not in record_id_to_bits_duration_sum_minutes_in_assignment:
                    record_id_to_bits_duration_sum_minutes_in_assignment[record_id] = defaultdict(float)
                record_id_to_bits_duration_sum_minutes_in_assignment[record_id][assignment_id] += \
                    task_audio_duration_minutes
        assignment_id_to_bits_duration_sum_minutes_without_honeypots[assignment_id] = \
            assignment_bits_duration_sum_minutes_without_honeypots
        assignment_id_to_duration_minutes[assignment_id] = assignment.duration_minutes

    records_markup_speeds = []
    has_invalid_record_ids = False
    for record_id, assignment_id_to_bits_durations_sum_minutes in record_id_to_bits_duration_sum_minutes_in_assignment.items():
        record_markup_duration_minutes = 0
        for assignment_id, bits_durations_sum_minutes in assignment_id_to_bits_durations_sum_minutes.items():
            assignment_all_bits_duration_sum_minutes_without_honeypots = \
                assignment_id_to_bits_duration_sum_minutes_without_honeypots[assignment_id]
            assignment_duration_minutes = assignment_id_to_duration_minutes[assignment_id]
            record_ratio_in_assignment = bits_durations_sum_minutes / assignment_all_bits_duration_sum_minutes_without_honeypots
            record_markup_duration_minutes += assignment_duration_minutes * record_ratio_in_assignment
        if record_id not in record_id_to_duration_seconds:
            # old markup has invalid audio s3 URLs
            print(f'\tmock record markup speed')
            has_invalid_record_ids = True
            break
        records_markup_speeds.append(record_markup_duration_minutes / (record_id_to_duration_seconds[record_id] / 60.0))

    first_assignment_started_at = min(assignment.created_at for assignment in assignments)
    # 2 hours is mean time passed from graph start to first Run Markup operation start for last markup sessions
    metadata.started_at = first_assignment_started_at - timedelta(hours=2)

    received_at = parse_datetime(metrics_row['received_at'])
    if received_at > metadata.started_at:
        markup_duration_minutes = (received_at - metadata.started_at).total_seconds() / 60.0
    else:
        # old markup has mocked received_at
        print(f'\tmock markup duration')
        markup_duration_minutes = metrics_row['records_durations_sum_minutes'] / 2

    if metrics_row['markup_id'] == 'b5dc1c89-c977-4bc8-908e-b26098914061':
        markup_duration_minutes *= 10

    return MetricMarkup(
        markup_id=metrics_row['markup_id'],
        markup_type=infer_markup_type_from_metadata(metadata),
        markup_metadata=metadata,
        language='ru-RU',
        records_durations_sum_minutes=metrics_row['records_durations_sum_minutes'],
        markup_duration_minutes=markup_duration_minutes,
        filtered_transcript_bits_ratio=metrics_row['filtered_transcript_bits_ratio'],
        accuracy=metrics_row['accuracy'],
        not_evaluated_records_ratio=metrics_row['not_evaluated_records_ratio'],
        record_markup_speed_mean=20.0 if has_invalid_record_ids else
        sum(records_markup_speeds) / len(records_markup_speeds),
        step_to_metrics={
            MarkupStep(step): MetricMarkupStep.from_yson(metric)
            for step, metric in metrics_row['step_to_metrics'].items()
        },
        received_at=received_at,
    )


def load_records_data() -> typing.Tuple[
    typing.Dict[str, float],
    typing.Dict[str, MarkupMetadata]
]:
    record_id_to_duration_seconds = {}
    markup_id_to_tags_durations = {}
    for row in yt.read_table('//home/mlcloud/o-gulyaev/ASREXP-839-records'):
        record_id = row['record_id']
        duration_seconds = row['duration_seconds']
        tags = tuple(prepare_tags_for_view(row['tags'], compose=False))
        markup_id = row['markup_id']

        record_id_to_duration_seconds[record_id] = duration_seconds

        if markup_id not in markup_id_to_tags_durations:
            markup_id_to_tags_durations[markup_id] = defaultdict(float)
        markup_id_to_tags_durations[markup_id][tags] += duration_seconds

    markup_id_to_metadata = {}
    for markup_id, tags_to_duration_seconds in markup_id_to_tags_durations.items():
        markup_priority = MarkupPriorities.INTERNAL
        for tags in tags_to_duration_seconds.keys():
            if any(tag.startswith(RecordTagType.MARKUP.value) for tag in tags):
                markup_priority = MarkupPriorities.CLIENT_NORMAL
                break
        markup_id_to_metadata[markup_id] = MarkupMetadata(
            markup_id=markup_id,
            markup_priority=markup_priority,
            tags_stats=[
                TagsMarkupStats(tags=list(tags), duration_seconds=duration_seconds)
                for tags, duration_seconds in tags_to_duration_seconds.items()
            ],
            started_at=None,  # mock for further set
        )

    return record_id_to_duration_seconds, markup_id_to_metadata


def load_assignments_data() -> typing.Dict[str, typing.List[Assignment]]:
    markup_id_to_assignments = defaultdict(list)
    for row in yt.read_table('//home/mlcloud/o-gulyaev/ASREXP-839-assignments'):
        submitted_at = parse_datetime(row['submitted_at'])
        created_at = parse_datetime(row['created_at'])
        markup_id = row['markup_id']
        tasks = [MarkupData.from_yson(task) for task in row['tasks']]

        duration_minutes = (submitted_at - created_at).total_seconds() / 60.0

        markup_id_to_assignments[markup_id].append(
            Assignment(tasks=tasks, duration_minutes=duration_minutes, created_at=created_at)
        )

    return markup_id_to_assignments

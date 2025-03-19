#!/usr/bin/python3

import typing
from collections import defaultdict
from multiprocessing import Process

import nirvana.job_context as nv
import ujson as json

from cloud.ai.speechkit.stt.lib.data.model.dao import MetricEvalRecordData, MetricEvalTagData
from cloud.ai.speechkit.stt.lib.eval.metrics.calc import get_evaluation_targets, get_metrics, calculate_metric
from cloud.ai.speechkit.stt.lib.eval.model import InputVersion, Record, EvaluationTarget, MetricsConfig
from cloud.ai.speechkit.stt.lib.eval.text import transform_evaluation_targets, get_transformation_case_str
from cloud.ai.speechkit.stt.lib.text.slice.application import PreparedSlice


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    outputs = op_ctx.outputs
    params = op_ctx.parameters

    records_path = inputs.get('records.json')
    recognitions_path = inputs.get('recognitions.json')
    if inputs.has('slices.json'):
        with open(inputs.get('slices.json')) as f:
            slices = [PreparedSlice.from_yson(s) for s in json.load(f)]
    else:
        slices = []
    cluster_references_path = inputs.get('cluster_references.json')
    text_comparison_stop_words_path = None
    if inputs.has('text_comparison_stop_words.json'):
        text_comparison_stop_words_path = inputs.get('text_comparison_stop_words.json')
    normalizer_data_path = inputs.get_item('normalizer_data').unpacked_dir() + '/normalizer'

    records_metrics_path = outputs.get('records_metrics.json')
    tags_metrics_path = outputs.get('tags_metrics.json')

    metrics_config = MetricsConfig.from_yson(json.loads(params.get('metrics-config')))
    input_version = InputVersion(params.get('input-version'))

    with open(records_path) as f:
        records = [Record.from_yson(x, input_version) for x in json.load(f)]

    with open(recognitions_path) as f:
        # text per record channel
        record_id_to_recognition = json.load(f)
        if input_version == InputVersion.V1:
            record_id_to_recognition = {
                record_id: [recognition] for record_id, recognition in record_id_to_recognition.items()
            }
        elif input_version != InputVersion.V2:
            raise ValueError(f'Unexpected input version: {input_version}')

    assert {r.id for r in records} == record_id_to_recognition.keys()

    evaluation_targets = get_evaluation_targets(records, record_id_to_recognition, slices)

    evaluation_targets_path = 'source_evaluation_targets.json'
    with open(evaluation_targets_path, 'w') as f:
        f.write(json.dumps([t.to_yson() for t in evaluation_targets], ensure_ascii=False))

    text_transformation_case_to_evaluation_targets_path = transform_evaluation_targets(
        evaluation_targets_path,
        metrics_config,
        normalizer_data_path,
    )

    report_paths = []
    processes = []
    for metric_name, text_transformation_cases in metrics_config.metric_name_to_text_transformation_cases.items():
        for text_transformation_case in text_transformation_cases:
            text_transformation_case_str = get_transformation_case_str(text_transformation_case)
            metric_text_transformation_case_str = f'{metric_name}_{text_transformation_case_str}'
            records_metrics_case_path = f'records_metrics_{metric_text_transformation_case_str}.json'
            tags_metrics_case_path = f'tags_metrics_{metric_text_transformation_case_str}.json'
            report_paths.append((records_metrics_case_path, tags_metrics_case_path))
            evaluation_targets_path = text_transformation_case_to_evaluation_targets_path[
                tuple(text_transformation_case)
            ]
            process = Process(
                target=calculate_metrics_for_text_transformation_case,
                args=(
                    metric_name,
                    text_transformation_case_str,
                    input_version,
                    records_path,
                    evaluation_targets_path,
                    cluster_references_path,
                    text_comparison_stop_words_path,
                    records_metrics_case_path,
                    tags_metrics_case_path,
                ),
            )
            process.start()
            processes.append(process)

    for process in processes:
        process.join()

    records_metrics = []
    tags_metrics = []
    for records_metrics_case_path, tags_metrics_case_path in report_paths:
        with open(records_metrics_case_path) as f:
            records_metrics += json.load(f)
        with open(tags_metrics_case_path) as f:
            tags_metrics += json.load(f)

    with open(records_metrics_path, 'w') as f:
        f.write(json.dumps(records_metrics))

    with open(tags_metrics_path, 'w') as f:
        f.write(json.dumps(tags_metrics))


def calculate_metrics_for_text_transformation_case(
    metric_name: str,
    text_transformation_case_str: str,
    input_version: InputVersion,
    records_path: str,
    evaluation_targets_path: str,
    cluster_references_path: str,
    text_comparison_stop_words_path: typing.Optional[str],
    records_metrics_path: str,
    tags_metrics_path: str,
):
    with open(records_path) as f:
        records = [Record.from_yson(x, input_version) for x in json.load(f)]

    tag_to_records_ids = defaultdict(list)

    for record in records:
        for tag in record.tags:
            tag_to_records_ids[tag].append(record.id)

    with open(evaluation_targets_path) as f:
        transformed_evaluation_targets = [EvaluationTarget.from_yson(r) for r in json.load(f)]

    metric = next(m for m in get_metrics(cluster_references_path) if m.name == metric_name)

    text_comparison_stop_words = set()
    if text_comparison_stop_words_path is not None:
        with open(text_comparison_stop_words_path) as f:
            text_comparison_stop_words = set(json.load(f))

    calculate_metric_result = calculate_metric(
        metric, transformed_evaluation_targets, tag_to_records_ids, text_comparison_stop_words,
    )

    records_metrics = []
    tags_metrics = []

    record_id_channel_to_evaluation_target = {(t.record_id, t.channel): t for t in transformed_evaluation_targets}

    for record_id_channel, evaluation_target in record_id_channel_to_evaluation_target.items():
        record_id, channel = record_id_channel
        records_metrics.append(
            MetricEvalRecordData(
                record_id=record_id,
                channel=channel,
                slices=evaluation_target.slices,
                metric_name=metric_name,
                text_transformations=text_transformation_case_str,
                metric_value=calculate_metric_result.record_id_channel_to_metric_value[record_id_channel],
                metric_data=calculate_metric_result.record_id_channel_to_metric_data[record_id_channel].to_json(),
                hypothesis=evaluation_target.hypothesis,
                reference=evaluation_target.reference,
            )
        )

    for tag_slice, aggregates in calculate_metric_result.tag_slice_to_metric_aggregates.items():
        for aggregate, value in aggregates.items():
            tag, slice = tag_slice
            tags_metrics.append(
                MetricEvalTagData(
                    tag=tag,
                    slice=slice,
                    metric_name=metric_name,
                    aggregation=aggregate,
                    text_transformations=text_transformation_case_str,
                    metric_value=value,
                )
            )

    with open(records_metrics_path, 'w') as f:
        json.dump([m.to_yson() for m in records_metrics], f, indent=4, ensure_ascii=False)

    with open(tags_metrics_path, 'w') as f:
        json.dump([m.to_yson() for m in tags_metrics], f, indent=4, ensure_ascii=False)

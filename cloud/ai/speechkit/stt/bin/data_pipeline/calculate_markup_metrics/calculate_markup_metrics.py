from datetime import datetime
import typing
import ujson as json

import dataforge.feedback_loop as feedback_loop
import nirvana.job_context as nv

from cloud.ai.lib.python.datetime import now
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    MarkupStep, RecordBitMarkup, MarkupAssignment, MarkupPool, MarkupMetadata, Record, RecordTagData,
    MetricMarkup, RecordBit,
)
from cloud.ai.lib.python.datasource.yt.model import generate_json_options_for_table, objects_to_rows, get_table_name
from cloud.ai.speechkit.stt.lib.data.ops.yt import table_metrics_markup_meta
from cloud.ai.speechkit.stt.lib.data_pipeline.markup_metrics import calculate_metrics
from cloud.ai.speechkit.stt.lib.data_pipeline.markup_quality import HoneypotsQuality


def main():
    op_ctx = nv.context()

    params = op_ctx.parameters
    inputs = op_ctx.inputs
    outputs = op_ctx.outputs

    assert \
        inputs.get_named_items('markups_assignments.json').keys() == \
        inputs.get_named_items('markup_pool.json').keys() == \
        inputs.get_named_items('honeypots_quality.json').keys()

    markup_steps_str = inputs.get_named_items('markups_assignments.json').keys()
    markup_step_to_assignments = {}
    markup_step_to_pool = {}
    markup_step_to_honeypots_quality = {}

    for markup_step_str in markup_steps_str:
        markup_step = MarkupStep(markup_step_str)
        with open(inputs.get('markups_assignments.json', link_name=markup_step_str)) as f:
            markup_step_to_assignments[markup_step] = [
                MarkupAssignment.from_yson(m) for m in json.load(f)
            ]
        with open(inputs.get('markup_pool.json', link_name=markup_step_str)) as f:
            pool_json = json.load(f)
            if pool_json == {}:
                pool = None
            else:
                pool = MarkupPool.from_yson(pool_json)
            markup_step_to_pool[markup_step] = pool
        with open(inputs.get('honeypots_quality.json', link_name=markup_step_str)) as f:
            honeypots_quality_json = json.load(f)
            if honeypots_quality_json == {}:
                honeypots_quality = None
            else:
                honeypots_quality = HoneypotsQuality.from_yson(honeypots_quality_json)
            markup_step_to_honeypots_quality[markup_step] = honeypots_quality

    with open(inputs.get('markup_metadata.json')) as f:
        markup_metadata = MarkupMetadata.from_yson(json.load(f))
    with open(inputs.get('records.json')) as f:
        records = [Record.from_yson(r) for r in json.load(f)]
    with open(inputs.get('records_bits.json')) as f:
        records_bits = [RecordBit.from_yson(r) for r in json.load(f)]
    with open(inputs.get('all_records_bits_s3_urls.json')) as f:
        all_records_bits_count = len(json.load(f))
    with open(inputs.get('transcript_records_bits_s3_urls.json')) as f:
        transcript_records_bits_count = len(json.load(f))
    transcript_feedback_loop = None
    if inputs.has('transcript_feedback_loop.json'):
        with open(inputs.get('transcript_feedback_loop.json')) as f:
            data = json.load(f)
            if data != {}:
                transcript_feedback_loop = feedback_loop.Metrics.from_yson(data)
    with open(inputs.get('quality_evaluation_markups.json')) as f:
        quality_evaluation_markups = [RecordBitMarkup.from_yson(m) for m in json.load(f)]
    with open(inputs.get('regular_tags.json')) as f:
        regular_tags = [RecordTagData.from_str(tag) for tag in json.load(f)]

    received_at = now()

    if len(quality_evaluation_markups) == 0:
        write_output(None, received_at, outputs)
        return

    filtered_transcript_bits_ratio = transcript_records_bits_count / all_records_bits_count
    lang = params.get('lang')

    metrics = calculate_metrics(
        markup_metadata,
        lang,
        markup_step_to_assignments,
        markup_step_to_pool,
        markup_step_to_honeypots_quality,
        filtered_transcript_bits_ratio,
        transcript_feedback_loop,
        quality_evaluation_markups,
        records,
        records_bits,
        regular_tags,
        received_at,
        op_ctx,
    )

    write_output(metrics, received_at, outputs)


def write_output(metrics: typing.Optional[MetricMarkup], received_at: datetime, outputs):
    table_name = get_table_name(received_at)
    metrics = [metrics] if metrics is not None else []
    with open(outputs.get('metrics.json'), 'w') as f:
        json.dump(objects_to_rows(metrics), f, indent=4, ensure_ascii=False)
    with open(outputs.get('metrics_table.json'), 'w') as f:
        json.dump(generate_json_options_for_table(meta=table_metrics_markup_meta, name=table_name), f,
                  indent=4, ensure_ascii=False)

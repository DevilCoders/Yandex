from collections import defaultdict
import json
import os
import typing

import dataforge.feedback_loop as feedback_loop
import toloka.client as toloka
import nirvana.job_context as nv

import cloud.ai.lib.python.datetime as datetime
import cloud.ai.speechkit.stt.lib.data_pipeline.transcript_feedback_loop as transcript
import cloud.ai.speechkit.stt.lib.data.model.dao as dao
from cloud.ai.lib.python.datasource.yt.model import (
    generate_json_options_for_table, objects_to_rows, single_object_to_row, get_table_name,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.toloka import create_markup_assignment
from cloud.ai.speechkit.stt.lib.data_pipeline.obfuscate import convert_cmd, reduce_volume_cmd
from cloud.ai.speechkit.stt.lib.data_pipeline.toloka import combine_records_bits_with_markup_data
from cloud.ai.speechkit.stt.lib.data_pipeline.files import get_name_for_record_bit_audio_file_from_record_bit
import cloud.ai.speechkit.stt.lib.data.ops.yt as yt


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    params = op_ctx.parameters
    outputs = op_ctx.outputs

    received_at = datetime.now()

    with open(inputs.get('markup_pool.json')) as f:
        markup_pool_id = json.load(f)['id']
    with open(inputs.get('check_pool.json')) as f:
        check_pool_id = json.load(f)['id']

    if markup_pool_id is None or check_pool_id is None:
        write_output([], [], [], [], None, None, [], None, received_at, outputs)
        return

    with open(inputs.get('user_urls_map.json')) as f:
        users_urls_map = json.load(f)
    with open(inputs.get('markup_metadata.json')) as f:
        markup_metadata = dao.MarkupMetadata.from_yson(json.load(f))
    with open(inputs.get('records_bits.json')) as f:
        records_bits = [dao.RecordBit.from_yson(r) for r in json.load(f)]
    with open(inputs.get('records_bits_s3_urls.json')) as f:
        records_bits_s3_urls = json.load(f)
    with open(inputs.get('obfuscate_data.json')) as f:
        obfuscation_data = json.load(f)

    lang = params.get('lang')

    fb_loop = transcript.construct_feedback_loop(inputs, params, users_urls_map, lang)

    check_assignments, check_bit_markups = load_markups(
        fb_loop.client,
        check_pool_id,
        records_bits,
        records_bits_s3_urls,
        dao.MarkupStep.CHECK_TRANSCRIPT,
        markup_metadata,
        obfuscation_data,
        lang,
        received_at,
    )
    markup_assignments, markup_bit_markups = load_markups(
        fb_loop.client,
        markup_pool_id,
        records_bits,
        records_bits_s3_urls,
        dao.MarkupStep.TRANSCRIPT,
        markup_metadata,
        obfuscation_data,
        lang,
        received_at,
    )

    # this may break because it is done bypassing dataforge logic
    audio_text_to_check_markups = defaultdict(list)
    for markup in check_bit_markups:
        audio_text = markup.markup_data.input.audio_s3_obj.to_https_url(), markup.markup_data.input.text
        audio_text_to_check_markups[audio_text].append(markup.id)
    for markup in markup_bit_markups:
        audio_text = markup.markup_data.input.audio_s3_obj.to_https_url(), markup.markup_data.solution.text
        check_markups = audio_text_to_check_markups[audio_text]
        markup.validation_data = dao.RecordBitMarkupFeedbackLoopValidationData(checks_ids=check_markups)

    toloka_environment = toloka.TolokaClient.Environment[params.get('toloka-environment')]
    markup_pool = dao.MarkupPool(
        id=markup_pool_id,
        markup_id=markup_metadata.markup_id,
        markup_step=dao.MarkupStep.TRANSCRIPT,
        params=fb_loop.client.get_pool(markup_pool_id).unstructure(),
        toloka_environment=toloka_environment.name,
        received_at=received_at,
    )
    check_pool = dao.MarkupPool(
        id=check_pool_id,
        markup_id=markup_metadata.markup_id,
        markup_step=dao.MarkupStep.CHECK_TRANSCRIPT,
        params=fb_loop.client.get_pool(check_pool_id).unstructure(),
        toloka_environment=toloka_environment.name,
        received_at=received_at,
    )

    filename_to_record_bit = {
        get_name_for_record_bit_audio_file_from_record_bit(record_bit): record_bit for record_bit in records_bits
    }
    records_bits_joins = []
    for result in fb_loop.get_results(markup_pool_id, check_pool_id):
        audio: transcript.TolokaAudio = list(result.task)[0]
        audio_filename = os.path.basename(audio.url)  # it works for https:// links
        record_bit = filename_to_record_bit[audio_filename]
        joins = []
        for solution in result.solutions:
            confidence = 0 if solution.evaluation is None else solution.evaluation.confidence
            text: transcript.Text = list(solution.solution)[0]
            joins.append(dao.RecordJoin.create_by_record_id(
                record_id=record_bit.id,
                recognition=dao.RecognitionPlainTranscript(text=text.text),
                join_data=dao.JoinDataFeedbackLoop(
                    confidence=confidence,
                    assignment_accuracy=solution.assignment_accuracy,
                    assignment_evaluation_recall=solution.assignment_evaluation_recall,
                    attempts=len(result.solutions),
                ),
                received_at=received_at,
                multiple_per_moment=len(result.solutions) > 1,
                other=None,
            ))
        records_bits_joins.append(transcript.RecordBitJoins(
            record_bit=record_bit,
            joins=joins,
        ))

    metrics = feedback_loop.calculate_metrics(fb_loop, markup_pool_id, check_pool_id)

    write_output(
        markup_bit_markups,
        check_bit_markups,
        markup_assignments,
        check_assignments,
        markup_pool,
        check_pool,
        records_bits_joins,
        metrics,
        received_at,
        outputs,
    )


def load_markups(
    toloka_client: toloka.TolokaClient,
    pool_id: str,
    records_bits: typing.List[dao.RecordBit],
    records_bits_s3_urls: typing.Dict[str, str],
    markup_step: dao.MarkupStep,
    markup_metadata: dao.MarkupMetadata,
    obfuscation_data: dict,
    lang: str,
    received_at: datetime.datetime,
) -> typing.Tuple[
    typing.List[dao.MarkupAssignment], typing.List[dao.RecordBitMarkup]
]:
    assignment_statuses = [
        toloka.Assignment.ACCEPTED,
        toloka.Assignment.REJECTED,
    ]

    assignments = []
    validation_data = {
        dao.MarkupStep.TRANSCRIPT: dao.FeedbackLoopValidationData(),
        dao.MarkupStep.CHECK_TRANSCRIPT: dao.TolokaAutoValidationData(),
    }[markup_step]
    for assignment_raw in toloka_client.get_assignments(status=assignment_statuses, pool_id=pool_id):
        assignment = create_markup_assignment(
            assignment_raw.unstructure(),
            received_at,
            markup_metadata.markup_id,
            markup_step,
            lang,
        )
        assignment.validation_data.append(validation_data)
        assignments.append(assignment)

    only_accepted = {
        dao.MarkupStep.TRANSCRIPT: False,
        dao.MarkupStep.CHECK_TRANSCRIPT: True,
    }[markup_step]

    record_bit_id_to_markup_data_list = combine_records_bits_with_markup_data(
        assignments, records_bits, records_bits_s3_urls, markup_step, only_accepted,
    )

    markups = []
    for record_bit in records_bits:
        markup_data_list = record_bit_id_to_markup_data_list[record_bit.id]
        bit_name = os.path.splitext(get_name_for_record_bit_audio_file_from_record_bit(record_bit))[0]
        for task, assignment_id in markup_data_list:
            bit_obfuscation_data = obfuscation_data[bit_name]
            record_bit_markup = dao.RecordBitMarkup.create(
                record_bit=record_bit,
                audio_obfuscation_data=dao.SoxEffectsObfuscationDataV2.create(
                    convert_cmd, bit_obfuscation_data, reduce_volume_cmd,
                ),
                markup_id=markup_metadata.markup_id,
                markup_step=markup_step,
                pool_id=pool_id,
                assignment_id=assignment_id,
                markup_data=task,
                received_at=received_at,
            )
            markups.append(record_bit_markup)

    return assignments, markups


def write_output(
    records_bits_markups_transcript: typing.List[dao.RecordBitMarkup],
    records_bits_markups_check: typing.List[dao.RecordBitMarkup],
    markups_assignments_transcript: typing.List[dao.MarkupAssignment],
    markups_assignments_check: typing.List[dao.MarkupAssignment],
    markup_pool_transcript: typing.Optional[dao.MarkupPool],
    markup_pool_check: typing.Optional[dao.MarkupPool],
    records_bits_joins: typing.List[transcript.RecordBitJoins],
    metrics: typing.Optional[feedback_loop.Metrics],
    received_at: datetime,
    outputs,
):
    table_name = get_table_name(received_at)

    with open(outputs.get('markups_assignments_transcript.json'), 'w') as f:
        json.dump(objects_to_rows(markups_assignments_transcript), f, indent=4, ensure_ascii=False)
    with open(outputs.get('markups_assignments_check.json'), 'w') as f:
        json.dump(objects_to_rows(markups_assignments_check), f, indent=4, ensure_ascii=False)
    with open(outputs.get('markups_assignments_table.json'), 'w') as f:
        json.dump(generate_json_options_for_table(meta=yt.table_markups_assignments_meta, name=table_name), f,
                  indent=4, ensure_ascii=False)

    with open(outputs.get('records_bits_markups_transcript.json'), 'w') as f:
        json.dump(objects_to_rows(records_bits_markups_transcript), f, indent=4, ensure_ascii=False)
    with open(outputs.get('records_bits_markups_check.json'), 'w') as f:
        json.dump(objects_to_rows(records_bits_markups_check), f, indent=4, ensure_ascii=False)
    with open(outputs.get('records_bits_markups_table.json'), 'w') as f:
        json.dump(generate_json_options_for_table(meta=yt.table_records_bits_markups_meta, name=table_name), f,
                  indent=4, ensure_ascii=False)

    with open(outputs.get('markup_pool_transcript.json'), 'w') as f:
        json.dump(single_object_to_row(markup_pool_transcript), f, indent=4, ensure_ascii=False)
    with open(outputs.get('markup_pool_check.json'), 'w') as f:
        json.dump(single_object_to_row(markup_pool_check), f, indent=4, ensure_ascii=False)
    with open(outputs.get('markups_pools_table.json'), 'w') as f:
        json.dump(generate_json_options_for_table(meta=yt.table_markups_pools_meta, name=table_name), f,
                  indent=4, ensure_ascii=False)

    with open(outputs.get('records_bits_joins.json'), 'w') as f:
        json.dump([x.to_yson() for x in records_bits_joins], f, indent=4, ensure_ascii=False)

    with open(outputs.get('metrics.json'), 'w') as f:
        json.dump(single_object_to_row(metrics), f, indent=4, ensure_ascii=False)

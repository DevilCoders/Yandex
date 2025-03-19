import os
import typing

import nirvana.job_context as nv
import ujson as json

from cloud.ai.lib.python import datetime
from cloud.ai.lib.python.datasource.yt.model import (
    generate_json_options_for_table, objects_to_rows, get_table_name, single_object_to_row,
)
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    RecordBit,
    RecordBitMarkup,
    SoxEffectsObfuscationDataV2,
    MarkupStep,
    RecordJoin,
    MarkupPool,
    TextComparisonStopWordsArcadiaSource,
    ClusterReferencesInfo,
    MarkupAssignment,
    MarkupMetadata,
)
from cloud.ai.speechkit.stt.lib.data.ops.yt import (
    table_records_bits_markups_meta,
    table_markups_assignments_meta,
    table_markups_pools_meta,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.files import get_name_for_record_bit_audio_file_from_record_bit
from cloud.ai.speechkit.stt.lib.data_pipeline.join import (
    combine_markups_with_bits_data,
    filter_records_bits_to_transcript,
)
import cloud.ai.speechkit.stt.lib.data_pipeline.markup_params as markup_params
from cloud.ai.speechkit.stt.lib.data_pipeline.markup_quality import run_pool_honeypot_acceptance, HoneypotsQuality
from cloud.ai.speechkit.stt.lib.data_pipeline.markup_quality.strategies import (
    TextValidationToolkit, TextComparisonStopWords,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.obfuscate import convert_cmd, reduce_volume_cmd
import cloud.ai.speechkit.stt.lib.data_pipeline.toloka as toloka
from cloud.ai.speechkit.stt.lib.eval.metrics.calc import get_metric_wer


def get_bit_name_by_s3_key(s3_key):
    basename = os.path.basename(s3_key)
    bit_name, _ = os.path.splitext(basename)
    return bit_name


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    outputs = op_ctx.outputs
    params = op_ctx.parameters

    with open(inputs.get('pool.json')) as f:
        pool_id = json.load(f)['id']
    with open(inputs.get('records_bits.json')) as f:
        records_bits = [RecordBit.from_yson(r) for r in json.load(f)]
    with open(inputs.get('obfuscate_data.json')) as f:
        obfuscation_data = json.load(f)
    with open(inputs.get('records_bits_s3_urls.json')) as f:
        records_bits_s3_urls = json.load(f)
    with open(inputs.get('markup_extra_data.json')) as f:
        markup_extra_data = json.load(f)
    with open(inputs.get('text_comparison_stop_words.json')) as f:
        text_comparison_stop_words_list = set(json.load(f))
    with open(inputs.get('text_comparison_stop_words_info.json')) as f:
        text_comparison_stop_words_info = TextComparisonStopWordsArcadiaSource.from_yson(json.load(f))
    with open(inputs.get('cluster_references_info.json')) as f:
        cluster_references_info = ClusterReferencesInfo.from_yson(json.load(f))
    with open(inputs.get('markup_metadata.json')) as f:
        markup_metadata = MarkupMetadata.from_yson(json.load(f))

    markup_step = MarkupStep(params.get('markup-step'))
    lang = params.get('lang')
    toloka_environment = toloka.TolokaEnvironment[params.get('toloka-environment')]
    toloka_token: str = params.get('toloka-token')

    records_bits_markups = []
    markups_assignments = []
    markup_pool = None
    filtered_records_bits_s3_urls = records_bits_s3_urls
    honeypots_quality = None

    if pool_id is None:
        write_output(
            records_bits_markups,
            markups_assignments,
            markup_pool,
            filtered_records_bits_s3_urls,
            honeypots_quality,
            datetime.now(),
            outputs,
        )
        return

    text_validation_toolkit = TextValidationToolkit(
        wer=get_metric_wer(inputs.get('cluster_references.json')),
        cluster_references_info=cluster_references_info,
        stop_words=TextComparisonStopWords(
            source=text_comparison_stop_words_info,
            words=text_comparison_stop_words_list,
        )
    )

    toloka_client = toloka.TolokaClient(oauth_token=toloka_token, lang=lang, environment=toloka_environment)
    extra_data = get_extra_data(markup_step, markup_extra_data, records_bits)

    markups_assignments, honeypots_quality = run_pool_honeypot_acceptance(
        pool_id=pool_id,
        markup_id=markup_metadata.markup_id,
        markup_step=markup_step,
        min_correct_solutions=markup_params.markup_step_to_min_correct_honeypot_solutions[markup_step],
        text_validation_toolkit=text_validation_toolkit,
        toloka_client=toloka_client,
        pull_interval_seconds=30,
        **extra_data,
    )

    record_bit_id_to_markup_data_list = toloka.combine_records_bits_with_markup_data(
        markups_assignments,
        records_bits,
        records_bits_s3_urls,
        markup_step,
    )

    received_at = datetime.now()

    for record_bit in records_bits:
        markup_data_list = record_bit_id_to_markup_data_list[record_bit.id]
        bit_name = os.path.splitext(get_name_for_record_bit_audio_file_from_record_bit(record_bit))[0]
        for task, assignment_id in markup_data_list:
            bit_obfuscation_data = obfuscation_data[bit_name]
            record_bit_markup = RecordBitMarkup.create(
                record_bit=record_bit,
                audio_obfuscation_data=SoxEffectsObfuscationDataV2.create(
                    convert_cmd, bit_obfuscation_data, reduce_volume_cmd,
                ),
                markup_id=markup_metadata.markup_id,
                markup_step=markup_step,
                pool_id=pool_id,
                assignment_id=assignment_id,
                markup_data=task,
                received_at=received_at,
            )
            records_bits_markups.append(record_bit_markup)

    markup_pool = MarkupPool(
        id=pool_id,
        markup_id=markup_metadata.markup_id,
        markup_step=markup_step,
        params=toloka_client.get_pool(pool_id).unstructure(),
        toloka_environment=toloka_environment.name,
        received_at=received_at,
    )

    if markup_step in [MarkupStep.CHECK_ASR_TRANSCRIPT, MarkupStep.CLASSIFICATION]:
        markups_with_bit_data = combine_markups_with_bits_data(records_bits_markups, records_bits)
        filtered_records_bits_s3_urls = filter_records_bits_to_transcript(
            markups_with_bit_data,
            records_bits_s3_urls,
            markup_params.markup_step_to_overlap_strategy[markup_step],
        )

    write_output(
        records_bits_markups,
        markups_assignments,
        markup_pool,
        filtered_records_bits_s3_urls,
        honeypots_quality,
        received_at,
        outputs,
    )


def get_extra_data(
    markup_step: MarkupStep,
    markup_extra_data: typing.Any,
    records_bits: typing.List[RecordBit],
) -> dict:
    if markup_step == MarkupStep.CHECK_ASR_TRANSCRIPT:
        return {'recognition_params': markup_extra_data}
    elif markup_step == MarkupStep.QUALITY_EVALUATION:
        records_joins = [RecordJoin.from_yson(j) for j in markup_extra_data]
        bit_filename_to_join_id = {}
        id_to_bit = {b.id: b for b in records_bits}
        for j in records_joins:
            if j.record_id in id_to_bit:
                bit = id_to_bit[j.record_id]
                bit_filename = get_name_for_record_bit_audio_file_from_record_bit(bit)
                bit_filename_to_join_id[bit_filename] = j.id
        return {'bit_filename_to_join_id': bit_filename_to_join_id}
    else:
        return {}


def write_output(
    records_bits_markups: typing.List[RecordBitMarkup],
    markups_assignments: typing.List[MarkupAssignment],
    markup_pool: typing.Optional[MarkupPool],
    filtered_records_bits_s3_urls: typing.Dict[str, str],
    honeypots_quality: typing.Optional[HoneypotsQuality],
    received_at: datetime,
    outputs,
):
    table_name = get_table_name(received_at)

    with open(outputs.get('records_bits_markups.json'), 'w') as f:
        json.dump(objects_to_rows(records_bits_markups), f, indent=4, ensure_ascii=False)
    with open(outputs.get('records_bits_markups_table.json'), 'w') as f:
        json.dump(generate_json_options_for_table(meta=table_records_bits_markups_meta, name=table_name), f,
                  indent=4, ensure_ascii=False)

    with open(outputs.get('markups_assignments.json'), 'w') as f:
        json.dump(objects_to_rows(markups_assignments), f, indent=4, ensure_ascii=False)
    with open(outputs.get('markups_assignments_table.json'), 'w') as f:
        json.dump(generate_json_options_for_table(meta=table_markups_assignments_meta, name=table_name), f,
                  indent=4, ensure_ascii=False)

    with open(outputs.get('markup_pool.json'), 'w') as f:
        json.dump(single_object_to_row(markup_pool), f, indent=4, ensure_ascii=False)
    with open(outputs.get('markup_pool_table.json'), 'w') as f:
        json.dump(generate_json_options_for_table(meta=table_markups_pools_meta, name=table_name), f,
                  indent=4, ensure_ascii=False)

    with open(outputs.get('filtered_records_bits_s3_urls.json'), 'w') as f:
        json.dump(filtered_records_bits_s3_urls, f, indent=4, ensure_ascii=False)

    with open(outputs.get('honeypots_quality.json'), 'w') as f:
        json.dump(single_object_to_row(honeypots_quality), f, indent=4, ensure_ascii=False)

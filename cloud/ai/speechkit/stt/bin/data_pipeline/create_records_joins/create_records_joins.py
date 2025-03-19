#!/usr/bin/python3

import nirvana.job_context as nv
import ujson as json

from cloud.ai.lib.python.datasource.yt.model import generate_json_options_for_table, objects_to_rows, get_table_name
from cloud.ai.lib.python.datetime import now
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    RecordBit, RecordBitMarkup, SplitDataFixedLengthOffset, get_channels_texts
)
import cloud.ai.speechkit.stt.lib.data_pipeline.transcript_feedback_loop as transcript
from cloud.ai.speechkit.stt.lib.data.ops.yt import table_records_joins_meta
from cloud.ai.speechkit.stt.lib.data_pipeline.join import combine_markups_with_bits_data, join_markups
from cloud.ai.speechkit.stt.lib.data_pipeline.markup_params import markup_step_to_overlap_strategy


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    outputs = op_ctx.outputs

    with open(inputs.get('records_bits.json')) as f:
        records_bits = [RecordBit.from_yson(m) for m in json.load(f)]
    records_bits_markups = []
    for markup_step_str in inputs.get_named_items('records_bits_markups.json').keys():
        with open(inputs.get('records_bits_markups.json', link_name=markup_step_str)) as f:
            records_bits_markups += [RecordBitMarkup.from_yson(m) for m in json.load(f)]
    if inputs.has('external_records_bits_joins.json'):
        with open(inputs.get('external_records_bits_joins.json')) as f:
            external_records_bits_joins = [transcript.RecordBitJoins.from_yson(j) for j in json.load(f)]
    else:
        external_records_bits_joins = []
    with open(inputs.get('split_data.json')) as f:
        split_data = SplitDataFixedLengthOffset.from_yson(json.load(f))
    markup_joiner_executable = inputs.get('markup_joiner_executable')

    bit_offset = split_data.offset_ms

    markups_with_bit_data = combine_markups_with_bits_data(records_bits_markups, records_bits)

    received_at = now()
    records_joins = join_markups(
        markups_with_bit_data, markup_step_to_overlap_strategy, markup_joiner_executable, external_records_bits_joins, bit_offset, received_at,
    )

    validation_entity_ids = set([])
    for markup_with_bit_data in markups_with_bit_data:
        validation_entity_ids.add(markup_with_bit_data[0].record_id)
        validation_entity_ids.add(markup_with_bit_data[0].bit_id)

    joined_entity_ids = set([join.record_id for join in records_joins])

    assert joined_entity_ids.issubset(validation_entity_ids)

    not_joined_entity_ids = validation_entity_ids - joined_entity_ids
    if len(not_joined_entity_ids) > 0:
        print(f'Not joined entities ratio: {len(not_joined_entity_ids) / len(validation_entity_ids):.3f}')
        print(f'Not joined entities list: {not_joined_entity_ids}')

    # for SpeechKit PRO client markup
    with open(outputs.get('records_texts.json'), 'w') as f:
        texts = []
        for record_join in records_joins:
            for i, text in enumerate(get_channels_texts(record_join.recognition)):
                texts.append({
                    'id': record_join.record_id,
                    'text': text,
                    'channel': i + 1,
                })
        json.dump(texts, f, indent=4, ensure_ascii=False)

    table_name = get_table_name(received_at)

    with open(outputs.get('records_joins.json'), 'w') as f:
        json.dump(objects_to_rows(records_joins), f, indent=4, ensure_ascii=False)
    with open(outputs.get('records_joins_table.json'), 'w') as f:
        json.dump(generate_json_options_for_table(meta=table_records_joins_meta, name=table_name), f,
                  indent=4, ensure_ascii=False)

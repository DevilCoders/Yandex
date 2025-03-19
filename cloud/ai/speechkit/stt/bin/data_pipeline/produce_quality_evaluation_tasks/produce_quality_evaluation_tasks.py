#!/usr/bin/python3

import ujson as json
from random import shuffle

import nirvana.job_context as nv

from cloud.ai.speechkit.stt.lib.data.model.dao import RecordBit, RecordJoin
from cloud.ai.speechkit.stt.lib.data_pipeline.files import get_name_for_record_bit_audio_file_from_record_bit
from cloud.ai.speechkit.stt.lib.data_pipeline.markup_params import quality_evaluation_real_tasks_in_assignment


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    outputs = op_ctx.outputs
    params = op_ctx.parameters

    with open(inputs.get('records_bits_s3_urls.json')) as f:
        bit_filename_to_s3_url = json.load(f)
    with open(inputs.get('records_bits.json')) as f:
        records_bits = [RecordBit.from_yson(b) for b in json.load(f)]
    with open(inputs.get('records_joins.json')) as f:
        records_joins = [RecordJoin.from_yson(j) for j in json.load(f)]

    evaluation_bits_ratio = params.get('evaluation-bits-ratio')
    assert 0.0 < evaluation_bits_ratio <= 1.0

    bits_count = len(records_bits)

    bits_count_for_evaluation = max(
        int(bits_count * evaluation_bits_ratio), quality_evaluation_real_tasks_in_assignment,
    )

    shuffle(records_bits)
    evaluation_bits = records_bits[:bits_count_for_evaluation]
    evaluation_bits_ids = {bit.id for bit in evaluation_bits}

    # In common case we have multiple texts per bits (i.e. by feedback loop) and we choose one with best quality
    evaluation_bit_id_to_best_join = {}
    for record_join in records_joins:
        if record_join.record_id not in evaluation_bits_ids:
            continue
        if record_join.record_id not in evaluation_bit_id_to_best_join:
            evaluation_bit_id_to_best_join[record_join.record_id] = record_join
        cur_record_join = evaluation_bit_id_to_best_join[record_join.record_id]
        if record_join.has_better_quality(cur_record_join):
            evaluation_bit_id_to_best_join[record_join.record_id] = record_join

    tasks = []
    for bit in evaluation_bits:
        text = evaluation_bit_id_to_best_join[bit.id].recognition.text
        bit_filename = get_name_for_record_bit_audio_file_from_record_bit(bit)
        s3_url = bit_filename_to_s3_url[bit_filename]
        tasks.append(
            {
                'input_values': {
                    'url': s3_url,
                    'text': text,
                    # TODO(ASREXP-1612): сюда надо передавать настоящий cls, взятый из задания расшифровки
                    #  или из majority vote заданий проверки, чтобы для пустых текстов корректно выводить плашки
                    #  "Нет речи" или "Неразборчивая речь", без этого исправления точность пессимизирована.
                    'cls': 'sp' if text else 'si',
                },
            }
        )

    shuffle(tasks)

    with open(outputs.get('tasks.json'), 'w') as f:
        json.dump(tasks, f, ensure_ascii=False, indent=4)

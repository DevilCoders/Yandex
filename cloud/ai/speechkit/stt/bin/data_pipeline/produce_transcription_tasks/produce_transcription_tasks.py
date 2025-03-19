#!/usr/bin/python3

import ujson as json
from random import shuffle

import nirvana.job_context as nv

from cloud.ai.speechkit.stt.lib.data.model.dao import SplitDataFixedLengthOffset
from cloud.ai.speechkit.stt.lib.data_pipeline.transcription_tasks import get_bits_overlaps


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    outputs = op_ctx.outputs
    params = op_ctx.parameters

    with open(inputs.get('records_bits_s3_urls.json')) as f:
        bit_filename_to_s3_url = json.load(f)
    with open(inputs.get('split_data.json')) as f:
        split_data = SplitDataFixedLengthOffset.from_yson(json.load(f))

    chunk_overlap = params.get('chunk-overlap')
    edge_bits_full_overlap = params.get('edge-bits-full-overlap')

    bit_offset = split_data.offset_ms
    bit_length = split_data.length_ms
    basic_overlap = bit_length // bit_offset

    if chunk_overlap < basic_overlap:
        raise ValueError('Chunk overlap must be greater or equal to bit overlap')

    bit_filename_to_overlap = get_bits_overlaps(
        bit_filename_to_s3_url.keys(), chunk_overlap, bit_offset, basic_overlap, edge_bits_full_overlap,
    )

    tasks = []
    for bit_filename in bit_filename_to_overlap.keys():
        tasks.append(
            {
                'input_values': {
                    'url': bit_filename_to_s3_url[bit_filename],
                },
                'overlap': bit_filename_to_overlap[bit_filename],
            }
        )

    shuffle(tasks)

    with open(outputs.get('tasks.json'), 'w') as f:
        json.dump(tasks, f, ensure_ascii=False, indent=4)

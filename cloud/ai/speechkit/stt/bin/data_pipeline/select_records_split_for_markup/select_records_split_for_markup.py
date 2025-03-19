#!/usr/bin/python3

import ujson as json
import os
import typing

import nirvana.job_context as nv

from cloud.ai.speechkit.stt.lib.data.model.common.id import generate_id
from cloud.ai.speechkit.stt.lib.data_pipeline.files import get_name_for_record_bit_audio_file_from_record_bit
from cloud.ai.speechkit.stt.lib.data.ops.queries import select_records_split_by_tags
from cloud.ai.speechkit.stt.lib.data.model.dao import SoxEffectsObfuscationDataV2, MarkupPriorities


def main():
    op_ctx = nv.context()

    outputs = op_ctx.outputs
    params = op_ctx.parameters

    tags: typing.List[str] = params.get('tags')
    split_at_str: str = params.get('split-at')

    records_bits_with_obfuscation_data = select_records_split_by_tags(tags, split_at_str)

    name_to_record_bit_with_obfuscation_data = {
        get_name_for_record_bit_audio_file_from_record_bit(data.record_bit): data
        for data in records_bits_with_obfuscation_data
    }

    name_to_obfuscated_audio_url = {
        name: data.obfuscated_audio_s3_obj.to_https_url()
        for name, data in name_to_record_bit_with_obfuscation_data.items()
    }

    name_to_obfuscation_data = {}
    for name, data in name_to_record_bit_with_obfuscation_data.items():
        obfuscation_data = data.audio_obfuscation_data
        if not isinstance(obfuscation_data, SoxEffectsObfuscationDataV2):
            raise ValueError(f'Unsupported obfuscation data version')
        name_to_obfuscation_data[os.path.splitext(name)[0]] = obfuscation_data.to_yson()

    split_data = records_bits_with_obfuscation_data[0].record_bit.split_data

    split_data = {
        'bit_length': split_data.length_ms,
        'bit_offset': split_data.offset_ms,
        'split_command': split_data.split_cmd,
        'failed_to_split_records_ids': [],
    }

    with open(outputs.get('records_bits.json'), 'w') as f:
        json.dump(
            [data.record_bit.to_yson() for data in records_bits_with_obfuscation_data], f, indent=4, ensure_ascii=False
        )

    with open(outputs.get('records_bits_s3_urls.json'), 'w') as f:
        json.dump(name_to_obfuscated_audio_url, f, indent=4, ensure_ascii=False)

    with open(outputs.get('obfuscate_data.json'), 'w') as f:
        json.dump(name_to_obfuscation_data, f, indent=4, ensure_ascii=False)

    with open(outputs.get('split_data.json'), 'w') as f:
        json.dump(split_data, f, indent=4, ensure_ascii=False)

    with open(outputs.get('markup_input_params.json'), 'w') as f:
        json.dump(
            {
                'tags': tags,
                'priority': MarkupPriorities.internal.value,
            },
            f,
            indent=4,
            ensure_ascii=False,
        )

    with open(outputs.get('markup_id.txt'), 'w') as f:
        f.write(generate_id())

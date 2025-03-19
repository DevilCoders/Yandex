import nirvana.job_context as nv
import ujson as json

from cloud.ai.speechkit.stt.lib.data.model.dao import RecordBit
from cloud.ai.speechkit.stt.lib.data_pipeline.files import get_name_for_record_bit_audio_file_from_record_bit


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    outputs = op_ctx.outputs
    params = op_ctx.parameters

    with open(inputs.get('records_bits.json')) as f:
        records_bits = [RecordBit.from_yson(m) for m in json.load(f)]
    lang = params.get('lang')

    with open(outputs.get('recognizer_data.json'), 'w') as f:
        json.dump([bit_to_recognizer_input(bit, lang) for bit in records_bits], f, ensure_ascii=False, indent=4)


def bit_to_recognizer_input(record_bit: RecordBit, lang: str) -> dict:
    return {
        'id': get_name_for_record_bit_audio_file_from_record_bit(record_bit),
        's3_obj': record_bit.s3_obj.to_yson(),
        'spec': {
            'audio_encoding': 1,
            'audio_channel_count': 1,
            'sample_rate_hertz': record_bit.audio_params.sample_rate_hertz,
            'lang': lang,
        },
    }

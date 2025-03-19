import nirvana.job_context as nv
import os
import ujson as json

from cloud.ai.lib.python import datetime
from cloud.ai.lib.python.datasource.yt.ops import Table, configure_yt_wrapper
from cloud.ai.speechkit.stt.lib.data_pipeline.files import (
    get_name_for_record_bit_audio_file_from_record_bit,
    unpack_and_list_files,
    bits_dir,
    obfuscated_bits_dir,
)
from cloud.ai.speechkit.stt.lib.data.model.dao import RecordBit, RecordBitAudio, RecordBitMarkupAudio
from cloud.ai.speechkit.stt.lib.data.ops.yt import (
    table_records_bits_audio_meta,
    table_records_bits_markups_audio_meta,
)


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    params = op_ctx.parameters

    with open(inputs.get('records_bits.json')) as f:
        records_bits = [RecordBit.from_yson(r) for r in json.load(f)]

    table_name = Table.get_name(datetime.now())

    obfuscated_audio: float = params.get('obfuscated-audio')
    if obfuscated_audio:
        dao = RecordBitMarkupAudio
        table = Table(table_records_bits_markups_audio_meta, table_name)
        audio_dir = obfuscated_bits_dir
    else:
        dao = RecordBitAudio
        table = Table(table_records_bits_audio_meta, table_name)
        audio_dir = bits_dir

    unpack_and_list_files(archive_path=inputs.get('records_bits_files.tar.gz'), directory_path=audio_dir)

    records_bits_audio = []
    for record_bit in records_bits:
        bit_filename = get_name_for_record_bit_audio_file_from_record_bit(record_bit)
        with open(os.path.join(audio_dir, bit_filename), 'rb') as audio_file:
            audio_bytes = audio_file.read()
        record_bit_audio = dao.create(record_bit, audio_bytes)
        records_bits_audio.append(record_bit_audio)

    configure_yt_wrapper()

    table.append_objects(records_bits_audio)

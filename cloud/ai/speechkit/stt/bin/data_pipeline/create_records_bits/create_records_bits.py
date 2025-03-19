from multiprocessing.pool import ThreadPool
from collections import defaultdict
import typing
import ujson as json
import os

import nirvana.job_context as nv

from cloud.ai.lib.python import datetime
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    Record,
    RecordBit,
    SplitDataFixedLengthOffset,
    BitDataTimeInterval,
    ConvertDataCmd,
)
from cloud.ai.lib.python.datasource.yt.model import generate_json_options_for_table, objects_to_rows, get_table_name
from cloud.ai.speechkit.stt.lib.data.ops.yt import table_records_bits_meta
from cloud.ai.speechkit.stt.lib.data_pipeline.files import (
    get_record_id_and_channel_and_start_ms_and_end_ms_by_record_bit_filename,
    unpack_and_list_files,
    bits_dir,
)
from cloud.ai.speechkit.stt.lib.utils.s3 import create_client


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    outputs = op_ctx.outputs

    records_bits_files_path = inputs.get('records_bits_files.tar.gz')
    with open(inputs.get('records.json')) as f:
        records = [Record.from_yson(r) for r in json.load(f)]
    with open(inputs.get('split_data.json')) as f:
        split_data = SplitDataFixedLengthOffset.from_yson(json.load(f))
    with open(inputs.get('convert_params.json')) as f:
        convert_params = json.load(f)
        sample_rate = convert_params['sample_rate']
        record_id_to_convert_cmd = convert_params['record_id_to_convert_cmd']

    bits = defaultdict(list)
    for filename in unpack_and_list_files(archive_path=records_bits_files_path, directory_path=bits_dir):
        record_id, channel, start_ms, end_ms = \
            get_record_id_and_channel_and_start_ms_and_end_ms_by_record_bit_filename(filename)
        index = start_ms // split_data.offset_ms
        bits[record_id].append((filename, start_ms, end_ms, index, channel))

    received_at = datetime.now()

    records_bits = []
    records_bits_audio = []
    for record in records:
        for filename, start_ms, end_ms, index, channel in bits[record.id]:
            with open(os.path.join(bits_dir, filename), 'rb') as audio_file:
                audio_bytes = audio_file.read()
            record_bit = RecordBit.create(
                record=record,
                split_data=split_data,
                received_at=received_at,
                bit_data=BitDataTimeInterval(start_ms, end_ms, index, channel),
                convert_data=ConvertDataCmd(cmd=record_id_to_convert_cmd[record.id]),
                sample_rate_hertz_native=sample_rate,
            )
            records_bits.append(record_bit)
            records_bits_audio.append(audio_bytes)

    s3 = create_client()

    def upload_bit(bit_with_audio: typing.Tuple[RecordBit, bytes]):
        bit, audio = bit_with_audio
        s3_obj = bit.s3_obj
        s3.put_object(Bucket=s3_obj.bucket, Key=s3_obj.key, Body=audio)

    pool = ThreadPool(processes=16)
    pool.map(upload_bit, zip(records_bits, records_bits_audio))

    table_name = get_table_name(received_at)

    with open(outputs.get('records_bits.json'), 'w') as f:
        json.dump(objects_to_rows(records_bits), f, indent=4, ensure_ascii=False)

    with open(outputs.get('records_bits_table.json'), 'w') as f:
        json.dump(generate_json_options_for_table(meta=table_records_bits_meta, name=table_name), f,
                  indent=4, ensure_ascii=False)

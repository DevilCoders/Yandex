import os
import re
from typing import Tuple, List

from cloud.ai.speechkit.stt.lib.data.model.dao import Record, RecordBit

records_dir = 'records'
bits_dir = 'bits'
obfuscated_bits_dir = 'obfuscated_bits'


def get_name_for_record_audio_file(record: Record) -> str:
    # We count on fact that services proxy logs S3 key is like '.../<record_id>.<ext>'
    return get_name_for_audio_file_from_s3_key(record.s3_obj.key)


def get_name_for_audio_file_from_s3_key(s3_key: str) -> str:
    return os.path.basename(s3_key)


def get_name_for_record_bit_audio_file(record_id: str, channel: int, start_ms: int, end_ms: int) -> str:
    return f'{record_id}_{channel}_{start_ms}-{end_ms}.wav'


def get_name_for_record_bit_audio_file_from_record_bit(record_bit: RecordBit) -> str:
    return get_name_for_record_bit_audio_file(
        record_id=record_bit.record_id,
        channel=record_bit.bit_data.channel,
        start_ms=record_bit.bit_data.start_ms,
        end_ms=record_bit.bit_data.end_ms,
    )


def get_record_id_and_channel_and_start_ms_and_end_ms_by_record_bit_filename(
    record_bit_filename: str,
) -> Tuple[str, int, int, int]:
    channel = 1
    if record_bit_filename.count('_') < 2:
        # Old style bit names before multichannel markup
        record_id, start_ms, end_ms = re.findall('(.*)_([0-9]+)-([0-9]+).wav', record_bit_filename)[0]
    else:
        record_id, channel, start_ms, end_ms = re.findall('(.*)_([0-9]+)_([0-9]+)-([0-9]+).wav', record_bit_filename)[0]
    return record_id, int(channel), int(start_ms), int(end_ms)


# Works also for old bit filenames without channels.
# Needed because of honeypots, which can be generated from old markup.
def get_start_ms_and_end_ms_by_record_bit_filename(
    record_bit_filename: str,
) -> Tuple[int, int]:
    filename_without_text, _ = os.path.splitext(record_bit_filename)
    range_part = filename_without_text.split('_')[-1]
    start_ms, end_ms = range_part.split('-')
    return int(start_ms), int(end_ms)


def unpack_and_list_files(archive_path, directory_path) -> List[str]:
    os.system(f'tar xzf {archive_path}')
    return os.listdir(directory_path)

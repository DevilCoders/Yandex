import io
import json
import os
import re

import yt.wrapper as yt
import yt.yson as yson
from pydub import AudioSegment

from cloud.ai.speechkit.stt.lib.data.model.dao import Record, RecordJoin, AudioEncoding
from cloud.ai.speechkit.stt.lib.data_pipeline.files import (
    records_dir,
    unpack_and_list_files,
    get_name_for_record_audio_file,
)

model_native_sample_rate_hertz = 16000
model_native_sample_width = 2  # 16 bit


def main():
    yt.config['proxy']['url'] = 'hahn'

    with open('/Users/o-gulyaev/tmp/utt_split/records.json') as f:
        records = [Record.from_yson(r) for r in json.load(f)]

    records_files_path = '/Users/o-gulyaev/tmp/utt_split/records_files.tar.gz'

    unpack_and_list_files(archive_path=records_files_path, directory_path=records_dir)

    with open('/Users/o-gulyaev/tmp/utt_split/records_joins.json') as f:
        records_joins = [RecordJoin.from_yson(r) for r in json.load(f)]

    record_id_to_join = {j.record_id: j for j in records_joins}

    rows = []
    filtered_records = [r for r in records if r.audio_params.duration_seconds < 300.0]

    for record in filtered_records:
        process_record(record, record_id_to_join[record.id], rows)

    table_path = '//home/mlcloud/o-gulyaev/ASREXP-881/aligner-input-2020-01-28'
    yt.create(
        'table',
        table_path,
        attributes=yson.json_to_yson({
            'schema': {
                '$value': [
                    {'name': 'record_id', 'type': 'string'},
                    {'name': 'channel', 'type': 'int64'},
                    {'name': 'duration_seconds', 'type': 'double'},
                    {'name': 'wav', 'type': 'string'},
                    {'name': 'text', 'type': 'string'},
                ],
                '$attributes': {'strict': True},
            }
        }),
    )
    yt.write_table(table_path, rows)


# Copy-paste from convert_and_split_to_bits
def process_record(record: Record, record_join: RecordJoin, rows: list):
    record_file_path = os.path.join(records_dir, get_name_for_record_audio_file(record))
    audio_encoding = record.req_params.get_audio_encoding()

    if audio_encoding == AudioEncoding.LPCM:
        source_audio = AudioSegment.from_raw(record_file_path,
                                             channels=record.audio_params.channel_count,
                                             sample_width=2,
                                             frame_rate=record.req_params.get_sample_rate_hertz())
    elif audio_encoding == AudioEncoding.OGG_OPUS:
        source_audio = AudioSegment.from_ogg(record_file_path)
    else:
        raise ValueError(f'Unexpected record encoding: {audio_encoding}')

    assert record.audio_params.channel_count == source_audio.channels
    assert source_audio.channels <= 16

    conv_audio = source_audio.set_sample_width(model_native_sample_width).set_frame_rate(model_native_sample_rate_hertz)

    for i, conv_mono_audio in enumerate(conv_audio.split_to_mono()):
        channel = i + 1
        if source_audio.channels > 1:
            channel_text = record_join.recognition.channels[channel - 1].recognition.text
        else:
            channel_text = record_join.recognition.text

        wav_content = io.BytesIO()

        conv_mono_audio.export(wav_content, format='wav')
        wav_content.seek(0)

        rows.append({
            'record_id': record.id,
            'channel': channel,
            'duration_seconds': float(record.audio_params.duration_seconds),
            'wav': wav_content.read(),
            'text': sanitize_text(channel_text),
        })


def sanitize_text(text):
    return re.sub(' +', ' ', text.replace('?', ''))

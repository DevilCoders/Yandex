import requests
import pytz
import io
import yt.wrapper as yt
import nirvana.job_context as nv

from datetime import datetime
from pydub import AudioSegment
from pydub.exceptions import CouldntDecodeError

from cloud.ai.speechkit.stt.lib.data.model.dao import (
    Record,
    RecordAudio,
    S3Object,
    HashVersion,
    RecordSourceImport,
    YandexCallCenterImportData,
    RecordRequestParams,
    RecordAudioParams,
    RecordTag,
    RecordTagData,
    RecordTagType,
    ImportSource,
)
from cloud.ai.speechkit.stt.lib.data.model.common.id import generate_id
from cloud.ai.speechkit.stt.lib.data.model.common.hash import crc32
from cloud.ai.speechkit.stt.lib.data.model.common import s3_consts
from cloud.ai.lib.python.datetime import now
from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.data.ops.collect_records import assign_mark
from cloud.ai.speechkit.stt.lib.data.ops.yt import (
    table_records_meta,
    table_records_audio_meta,
    table_records_tags_meta,
)
from cloud.ai.speechkit.stt.lib.utils.s3 import create_client

AUDIO_FRAME_RATE = 8000


class CallCenterData:
    def __init__(self, call_reason, phone, duration, timestamp, url, call_time, status):
        self.call_reason = call_reason
        self.phone = phone
        self.duration = duration
        self.timestamp = timestamp
        self.url = url
        self.call_time = call_time
        self.status = status


def main():
    job_context = nv.context()
    parameters = job_context.get_parameters()

    records_table_path = parameters.get('yt_input_table')
    received_at = now()

    call_center_data = []

    for row in yt.read_table(records_table_path):
        call_center_data.append(
            CallCenterData(
                row['call_reason'],
                row['phone'],
                row['call_duration'],
                row['create_time'],
                row['call_url'],
                row['call_time'],
                row['call_status'],
            )
        )
    print("Collected " + str(len(call_center_data)) + " call_center_data.")

    if len(call_center_data) == 0:
        return

    audio_data_list = []
    not_decoded_source_records = 0

    for cc_data in call_center_data:
        source_file = cc_data.url.rsplit('/', 1)[1]
        print(source_file)
        audio_data = requests.get(cc_data.url).content
        try:
            audio_segment_stereo = AudioSegment.from_mp3(io.BytesIO(audio_data))
            assert audio_segment_stereo.channels == 2
            assert audio_segment_stereo.sample_width == 2
            assert audio_segment_stereo.frame_rate == AUDIO_FRAME_RATE
            for idx, audio_segment_mono in enumerate(audio_segment_stereo.split_to_mono()):
                data = audio_segment_mono.set_frame_rate(AUDIO_FRAME_RATE)
                out_f = io.BytesIO()
                data.export(out_f=out_f, format='wav')
                out_f.seek(0)
                audio_segment = AudioSegment.from_file(out_f)
                assert audio_segment.channels == 1
                assert audio_segment.sample_width == 2
                assert audio_segment.frame_rate == AUDIO_FRAME_RATE
                audio_duration_seconds = audio_segment.duration_seconds
                print(f'{source_file}: {audio_duration_seconds}')
                audio_data_list.append(
                    {
                        'source_file': source_file,
                        'raw_data': audio_segment.raw_data,
                        'duration_seconds': float(audio_duration_seconds),
                        'data': cc_data,
                        'channel': idx + 1,
                    }
                )
        except CouldntDecodeError:
            print('Source record %s couldn\'t be decoded, skip' % source_file)
            not_decoded_source_records += 1
            continue

    print('Not decoded source records ratio: %.3f' % (float(not_decoded_source_records) / float(len(call_center_data))))

    s3 = create_client()

    records = []
    records_audio = []
    records_tags = []

    tags_data_list = [
        RecordTagData(type=RecordTagType.IMPORT, value=ImportSource.YANDEX_CALL_CENTER.value),
        RecordTagData.create_period(received_at),
        RecordTagData.create_lang_ru(),
    ]

    # Create Record and RecordTag objects, upload .raw to S3
    for audio_data in audio_data_list:
        record_id = generate_id()
        duration_seconds = audio_data['duration_seconds']
        audio = audio_data['raw_data']
        data = audio_data['data']
        channel = audio_data['channel']

        s3_obj = S3Object(
            endpoint=s3_consts.cloud_endpoint,
            bucket=s3_consts.data_bucket,
            key=f'Speechkit/STT/Data/{received_at.year}/{received_at.month:02d}/{received_at.day:02d}/{record_id}.raw',
        )

        record = Record(
            id=record_id,
            s3_obj=s3_obj,
            mark=assign_mark(),
            source=RecordSourceImport.create_yandex_call_center(
                YandexCallCenterImportData(
                    yt_table_path=records_table_path,
                    source_audio_url=data.url,
                    channel=channel,
                    call_reason=data.call_reason,
                    phone=data.phone,
                    call_time=data.call_time,
                    call_duration=data.duration,
                    call_status=data.status,
                    create_time=parse_datetime_from_unix_timestamp(data.timestamp),
                ),
            ),
            req_params=RecordRequestParams(
                recognition_spec={
                    'audio_encoding': 1,
                    'language_code': 'ru-RU',
                    'sample_rate_hertz': AUDIO_FRAME_RATE,
                },
            ),
            audio_params=RecordAudioParams(
                acoustic='phone',
                duration_seconds=duration_seconds,
                size_bytes=len(audio),
                channel_count=1,
            ),
            received_at=received_at,
            other=None,
        )
        record_audio = RecordAudio(
            record_id=record_id,
            audio=None,
            hash=crc32(audio),
            hash_version=HashVersion.CRC_32_BZIP2,
        )

        s3.put_object(Bucket=record.s3_obj.bucket, Key=record.s3_obj.key, Body=audio)

        records.append(record)
        records_audio.append(record_audio)

        for tag_data in tags_data_list:
            records_tags.append(RecordTag.add(record=record, data=tag_data, received_at=received_at))

    table_name = Table.get_name(received_at)
    table_records = Table(meta=table_records_meta, name=table_name)
    table_records_audio = Table(meta=table_records_audio_meta, name=table_name)
    table_records_tags = Table(meta=table_records_tags_meta, name=table_name)

    table_records.append_objects(records)
    table_records_audio.append_objects(records_audio)
    table_records_tags.append_objects(records_tags)


def parse_datetime_from_unix_timestamp(timestamp):
    return pytz.utc.localize(datetime.fromtimestamp(timestamp / 1000))


if __name__ == "__main__":
    main()

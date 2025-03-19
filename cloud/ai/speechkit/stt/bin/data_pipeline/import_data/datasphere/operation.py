import re
import sys
import tarfile
import logging
from pathlib import Path
from multiprocessing.pool import ThreadPool
from typing import Optional

from nope import JobProcessorOperation
from nope.endpoints import *
from nope.parameters import *

import yt.wrapper as yt
import pandas as pd
import numpy as np
from boto3.s3.transfer import TransferConfig

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.lib.python.datetime import now
from cloud.ai.speechkit.stt.lib.data.model.common import s3_consts
from cloud.ai.speechkit.stt.lib.data.model.common.hash import fast_crc32
from cloud.ai.speechkit.stt.lib.data.ops.collect_records import assign_mark
from cloud.ai.speechkit.stt.lib.data_pipeline.import_data.records import process_file_wav
from cloud.ai.speechkit.stt.lib.utils.s3 import create_client
from cloud.ai.speechkit.stt.lib.data.ops.yt import (
    table_records_meta,
    table_records_audio_meta,
    table_records_tags_meta,
    table_records_joins_meta,
)
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    Mark,
    Record,
    RecordAudio,
    HashVersion,
    RecordRequestParams,
    RecordAudioParams,
    S3Object,
    RecordSourceImport,
    DatasphereImportData,
    RecordJoin,
    RecognitionPlainTranscript,
    JoinDataExemplar,
    RecordTag,
    RecordTagData,
    RecordTagType,
    ImportSource,
)


logging.getLogger('boto3').setLevel(logging.CRITICAL)
logging.getLogger('botocore').setLevel(logging.CRITICAL)
logging.getLogger('s3transfer').setLevel(logging.CRITICAL)
logging.getLogger('urllib3').setLevel(logging.CRITICAL)


class ImportFromDatasphere(JobProcessorOperation):
    name = 'Import from datasphere'
    owner = 'cloud-ml'
    description = 'Import from datasphere to s3'

    class Parameters(JobProcessorOperation.Parameters):
        dataset_name = StringParameter(nirvana_name='dataset-name', required=True)
        folder_id = StringParameter(nirvana_name='folder-id', required=True)
        mark = EnumParameter(nirvana_name='mark', required=True, enum_values=['train', 'test', 'val', 'auto'])
        language = EnumParameter(nirvana_name='language', required=True, enum_values=['ru-RU'])
        s3_bucket = StringParameter(nirvana_name='s3-bucket', required=True)
        s3_key = StringParameter(nirvana_name='s3-key', required=True)
        store_to_hume = BooleanParameter(nirvana_name='store-hume', required=False)
        aws_access_key_id = StringParameter(nirvana_name='aws-access-key-id', required=True)
        aws_secret_access_key = SecretParameter(nirvana_name='aws-secret-access-key', required=True)

    class Inputs(JobProcessorOperation.Inputs):
        pass

    class Outputs(JobProcessorOperation.Outputs):
        report = TextOutput(nirvana_name='report')

    class Ports(JobProcessorOperation.Ports):
        pass

    def __init__(self):
        super().__init__()
        self.lang_regex = {
            'ru-RU': re.compile('[^ а-яё]')
        }

    def write_error_and_fail(self, message: str, exception: Optional[Exception]):
        Path(self.Outputs.report.get_path()).write_text(message)
        if exception:
            raise exception
        sys.exit(0)

    def upload_batch(self, s3, batch):
        pool = ThreadPool(processes=16)
        received_at = now()
        mark = Mark(str(self.Parameters.mark).upper()) if self.Parameters.mark != 'auto' else None

        table_name = Table.get_name(received_at)
        audio_s3_dir = table_name.replace('-', '/')

        def get_s3_key(record_id: str):
            return f'Speechkit/STT/Data/{audio_s3_dir}/{record_id}.raw'

        def put_audio_to_s3(record_data: dict):
            if not self.Parameters.store_to_hume:
                s3.put_object(
                    Bucket=s3_consts.data_bucket,
                    Key=get_s3_key(record_data['audio_data'].record_id),
                    Body=record_data['audio_data'].raw_data,
                )

        pool.map(put_audio_to_s3, batch)

        records, records_audio, records_tags, records_joins = [], [], [], []

        tag_data_list = [
            RecordTagData(
                type=RecordTagType.IMPORT,
                value=ImportSource.DATASPHERE.value,
            ),
            RecordTagData.create_period(received_at),
            RecordTagData.create_lang(self.Parameters.language),
            RecordTagData(type=RecordTagType.FOLDER, value=self.Parameters.folder_id),
            RecordTagData(type=RecordTagType.DATASET, value=self.Parameters.dataset_name)
        ]

        for record_data in batch:
            audio_data = record_data['audio_data']
            record_id = audio_data.record_id
            audio = audio_data.raw_data
            wav_file = record_data['wav_file']
            text = record_data['text']

            record = Record(
                id=record_id,
                s3_obj=S3Object(
                    endpoint=s3_consts.cloud_endpoint,
                    bucket=s3_consts.data_bucket,
                    key=get_s3_key(record_id),
                ),
                mark=mark or assign_mark(),
                source=RecordSourceImport.create_datasphere(
                    DatasphereImportData(
                        source_archive_s3_obj=S3Object(
                            endpoint=s3_consts.cloud_endpoint,
                            bucket=self.Parameters.s3_bucket,
                            key=self.Parameters.s3_key
                        ),
                        folder_id=str(self.Parameters.folder_id),
                        dataset_name=str(self.Parameters.dataset_name),
                        wav_file=wav_file
                    ),
                ),
                req_params=RecordRequestParams(
                    recognition_spec={
                        'audio_encoding': 1,
                        'language_code': self.Parameters.language,
                        'sample_rate_hertz': audio_data.sample_rate_hertz,
                    },
                ),
                audio_params=RecordAudioParams(
                    acoustic='unknown',
                    duration_seconds=audio_data.duration_seconds,
                    size_bytes=len(audio),
                    channel_count=audio_data.channel_count,
                ),
                received_at=received_at,
                other=None,
            )
            record_audio = RecordAudio(
                record_id=record_id,
                audio=None,
                hash=fast_crc32(audio),
                hash_version=HashVersion.CRC_32,
            )
            records.append(record)
            records_audio.append(record_audio)

            records_joins.append(
                RecordJoin.create_by_record(
                    record=record,
                    recognition=RecognitionPlainTranscript(text=text),
                    join_data=JoinDataExemplar(),
                    received_at=received_at,
                    other=None,
                )
            )

            records_tags.extend(
                RecordTag.add(record=record, data=tag_data, received_at=received_at) for tag_data in tag_data_list
            )

        table_name = Table.get_name(received_at)
        if self.Parameters.store_to_hume:
            print('Creating YT tables in Hume')
            yt.config['proxy']['url'] = 'hume'
        else:
            yt.config['proxy']['url'] = 'hahn'

        table_records = Table(meta=table_records_meta, name=table_name)
        table_records_audio = Table(meta=table_records_audio_meta, name=table_name)
        table_records_tags = Table(meta=table_records_tags_meta, name=table_name)
        table_records_joins = Table(meta=table_records_joins_meta, name=table_name)

        table_records.append_objects(records)
        table_records_audio.append_objects(records_audio)
        table_records_tags.append_objects(records_tags)
        table_records_joins.append_objects(records_joins)

        print(f'Tag: {tag_data_list[0].to_str()}')

    def try_upload_batch(self, s3, batch):
        try:
            self.upload_batch(s3, batch)
        except Exception as e:
            self.write_error_and_fail('Internal error occurred while uploading dataset', e)

    def exec(self):
        folder_id = str(self.Parameters.folder_id)
        assert folder_id.startswith('b1g') or folder_id == 'mlcloud'

        s3 = create_client(
            access_key_id=self.Parameters.aws_access_key_id,
            secret_access_key=self.Parameters.aws_secret_access_key
        )

        s3.download_file(
            Bucket=self.Parameters.s3_bucket,
            Key=self.Parameters.s3_key,
            Filename='archive.tar.gz',
            Config=TransferConfig(
                multipart_threshold=1024*25,
                max_concurrency=16,
                multipart_chunksize=1024*25,
                use_threads=True
            )
        )

        if not tarfile.is_tarfile('archive.tar.gz'):
            self.write_error_and_fail('input file is not a .tar.gz archive', None)
        with tarfile.open('archive.tar.gz', 'r:gz') as tar:
            tar.extractall()

        texts_path, audio_path = Path('texts.tsv'), Path('audio')
        if not texts_path.is_file():
            self.write_error_and_fail('File `texts.tsv` was not found', None)
        if not audio_path.is_dir():
            self.write_error_and_fail('Directory `audio` was not found', None)

        df = pd.read_csv('texts.tsv', sep='\t')
        for column in ['text', 'wav_file']:
            if column not in df:
                self.write_error_and_fail(f'Column "{column}" was not found', None)

        batch = []
        lang_regex = self.lang_regex[self.Parameters.language]
        df = df.replace(np.nan, '', regex=True)

        for i, row in df.iterrows():
            text, wav_file = row['text'], row['wav_file']

            invalid_letters = set(lang_regex.findall(text))
            if invalid_letters:
                self.write_error_and_fail(f'Text "{text}" contains invalid symbols: {invalid_letters}', None)

            wav_path = audio_path / wav_file
            if not wav_path.exists():
                self.write_error_and_fail(f'Wav {wav_path} was not found', None)

            try:
                audio_data = process_file_wav(str(wav_path))
            except Exception as e:
                self.write_error_and_fail(f'Failed to parse wav {wav_path}: {e}', e)

            batch.append({
                'wav_file': wav_file,
                'audio_data': audio_data,
                'text': text
            })

            if len(batch) >= 50000:
                self.try_upload_batch(s3, batch)
                batch = []

        if len(batch) > 0:
            self.try_upload_batch(s3, batch)

        Path(self.Outputs.report.get_path()).write_text('Success!')

    def run(self):
        self.exec()

import unittest
from datetime import timezone

from cloud.ai.speechkit.stt.lib.data.model.dao import *


class TestSerialization(unittest.TestCase):
    def test_common(self):
        obj = S3Object(
            endpoint='https://storage.yandexcloud.net',
            bucket='cloud-ai-data',
            key='foo.wav',
        )
        yson = {
            'endpoint': 'https://storage.yandexcloud.net',
            'bucket': 'cloud-ai-data',
            'key': 'foo.wav',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(S3Object.from_yson(yson), obj)

    def test_records(self):
        obj = RecordRequestParams(
            recognition_spec={
                'sample_rate_hertz': 8000,
                'language_code': 'ru-RU',
            },
        )
        yson = {
            'recognition_spec': {
                'sample_rate_hertz': 8000,
                'language_code': 'ru-RU',
            },
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(RecordRequestParams.from_yson(yson), obj)

        obj = RecordAudioParams(
            acoustic='unknown',
            duration_seconds=123.456,
            size_bytes=440000,
            channel_count=1,
        )
        yson = {
            'acoustic': 'unknown',
            'duration_seconds': 123.456,
            'size_bytes': 440000,
            'channel_count': 1,
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(RecordAudioParams.from_yson(yson), obj)

        obj = RecordSourceCloudMethod(
            name='long',
            version='v2',
        )
        yson = {
            'name': 'long',
            'version': 'v2',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(RecordSourceCloudMethod.from_yson(yson), obj)

        obj = RecordSourceCloud(
            folder_id='b1g0gu61j6fvcih43l9l',
            method=RecordSourceCloudMethod(
                name='long',
                version='v2',
            ),
        )
        yson = {
            'folder_id': 'b1g0gu61j6fvcih43l9l',
            'method': {
                'name': 'long',
                'version': 'v2',
            },
            'version': 'cloud',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(RecordSourceCloud.from_yson(yson), obj)

        yson = {
            'version': 'exemplar'
        }
        obj = RecordSourceExemplar()
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(RecordSourceExemplar.from_yson(yson), obj)

        obj = RecordSourceMarkup(
            source_file_name='foo.wav',
            folder_id='b1g0gu61j6fvcih43l9l',
            billing_units=10,
        )
        yson = {
            'source_file_name': 'foo.wav',
            'folder_id': 'b1g0gu61j6fvcih43l9l',
            'billing_units': 10,
            'version': 'markup',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(RecordSourceMarkup.from_yson(yson), obj)

        obj = VoiceRecorderMarkupData(
            assignment_id='123',
            worker_id='abc',
            task_id='xxx',
        )
        yson = {
            'assignment_id': '123',
            'worker_id': 'abc',
            'task_id': 'xxx',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(VoiceRecorderMarkupData.from_yson(yson), obj)

        obj = VoiceRecorderEvaluationData(
            hypothesis='алиса',
            metric_name='WER',
            metric_value=0.5,
            text_transformations='none',
        )
        yson = {
            'hypothesis': 'алиса',
            'metric_name': 'WER',
            'metric_value': 0.5,
            'text_transformations': 'none',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(VoiceRecorderEvaluationData.from_yson(yson), obj)

        obj = VoiceRecorderImportDataV1(
            markup_data=VoiceRecorderMarkupData(
                assignment_id='123',
                worker_id='abc',
                task_id='xxx',
            ),
            evaluation_data=VoiceRecorderEvaluationData(
                hypothesis='алиса',
                metric_name='WER',
                metric_value=0.5,
                text_transformations='none',
            ),
            duration=42.5,
            asr_mark='OK',
            text='алло',
            uuid='123-55-dd',
        )
        yson = {
            'markup_data': {
                'assignment_id': '123',
                'worker_id': 'abc',
                'task_id': 'xxx',
            },
            'evaluation_data': {
                'hypothesis': 'алиса',
                'metric_name': 'WER',
                'metric_value': 0.5,
                'text_transformations': 'none',
            },
            'duration': 42.5,
            'asr_mark': 'OK',
            'text': 'алло',
            'uuid': '123-55-dd',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(VoiceRecorderImportDataV1.from_yson(yson), obj)

        obj = VoicetableImportData(
            other_info={
                'foo': 'bar',
            },
            original_info={
                'sample_rate': 8000,
            },
            language='en-US',
            text_ru='эйсидиси',
            text_orig='acdc',
            application=None,
            acoustic='mobile',
            date='2020-01-01',
            dataset_id=None,
            speaker_info='oksana',
            duration=43.21,
            uttid='aaabbccc',
            mark='TEST',
        )
        yson = {
            'other_info': {
                'foo': 'bar',
            },
            'original_info': {
                'sample_rate': 8000,
            },
            'language': 'en-US',
            'text_ru': 'эйсидиси',
            'text_orig': 'acdc',
            'application': None,
            'acoustic': 'mobile',
            'date': '2020-01-01',
            'dataset_id': None,
            'speaker_info': 'oksana',
            'duration': 43.21,
            'uttid': 'aaabbccc',
            'mark': 'TEST',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(VoicetableImportData.from_yson(yson), obj)

        obj = YandexCallCenterImportData(
            yt_table_path='//tmp/junk-12321',
            source_audio_url='https://foo.org/123.mp3',
            channel=2,
            call_reason='BUG',
            phone='+71234567890',
            call_time='2020-01-15',
            call_duration=60,
            call_status='OK',
            create_time=datetime(year=1998, month=12, day=31, tzinfo=timezone.utc),
        )
        yson = {
            'yt_table_path': '//tmp/junk-12321',
            'source_audio_url': 'https://foo.org/123.mp3',
            'channel': 2,
            'call_reason': 'BUG',
            'phone': '+71234567890',
            'call_time': '2020-01-15',
            'call_duration': 60,
            'call_status': 'OK',
            'create_time': '1998-12-31T00:00:00+00:00',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(YandexCallCenterImportData.from_yson(yson), obj)

        obj = YTTableImportData(
            yt_table_path='//home/mlcloud/lol',
            ticket='CLOUD-777',
            folder_id=None,
            original_uuid=None,
        )
        yson = {
            'yt_table_path': '//home/mlcloud/lol',
            'ticket': 'CLOUD-777',
            'folder_id': None,
            'original_uuid': None,
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(YTTableImportData.from_yson(yson), obj)

        obj = FilesImportData(
            ticket='CLOUD-888',
            source_file_name='foo.wav',
            folder_id=None,
        )
        yson = {
            'ticket': 'CLOUD-888',
            'source_file_name': 'foo.wav',
            'folder_id': None,
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(FilesImportData.from_yson(yson), obj)

        obj = RecordSourceImport(
            source=ImportSource.FILES,
            data=FilesImportData(
                ticket='CLOUD-888',
                source_file_name='foo.wav',
                folder_id=None,
            ),
        )
        yson = {
            'version': 'import',
            'source': 'files',
            'data': {
                'ticket': 'CLOUD-888',
                'source_file_name': 'foo.wav',
                'folder_id': None,
            },
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(RecordSourceImport.from_yson(yson), obj)

        obj = RecordSourceImport(
            source=ImportSource.S3_BUCKET,
            data=S3BucketImportData(
                source_s3_obj=S3Object(
                    endpoint='s3.mds.yandex.net',
                    bucket='talks',
                    key='12345',
                ),
                last_modified=datetime(year=1998, month=12, day=31, tzinfo=timezone.utc),
                tag='taxi-p2p',
            ),
        )
        yson = {
            'version': 'import',
            'source': 's3-bucket',
            'data': {
                'source_s3_obj': {
                    'endpoint': 's3.mds.yandex.net',
                    'bucket': 'talks',
                    'key': '12345',
                },
                'last_modified': '1998-12-31T00:00:00+00:00',
                'tag': 'taxi-p2p',
            },
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(RecordSourceImport.from_yson(yson), obj)

        with self.assertRaises(ValueError):
            RecordSourceImport.from_yson({
                'source': 'foo',
                'field': 'bar',
            })

        obj = Record(
            id='01399a9c-2e1a-4312-8648-9cfdca8e181e',
            s3_obj=S3Object(
                endpoint='https://storage.yandexcloud.net',
                bucket='cloud-ai-data',
                key='foo.wav',
            ),
            mark=Mark.VAL,
            source=RecordSourceImport(
                source=ImportSource.FILES,
                data=FilesImportData(
                    ticket='CLOUD-888',
                    source_file_name='foo.wav',
                    folder_id=None,
                ),
            ),
            req_params=RecordRequestParams(
                recognition_spec={
                    'sample_rate_hertz': 8000,
                    'language_code': 'ru-RU',
                },
            ),
            audio_params=RecordAudioParams(
                acoustic='unknown',
                duration_seconds=123.456,
                size_bytes=440000,
                channel_count=1,
            ),
            received_at=datetime(year=1999, month=12, day=31, tzinfo=timezone.utc),
            other=None,
        )
        yson = {
            'id': '01399a9c-2e1a-4312-8648-9cfdca8e181e',
            's3_obj': {
                'endpoint': 'https://storage.yandexcloud.net',
                'bucket': 'cloud-ai-data',
                'key': 'foo.wav',
            },
            'mark': 'VAL',
            'source': {
                'version': 'import',
                'source': 'files',
                'data': {
                    'ticket': 'CLOUD-888',
                    'source_file_name': 'foo.wav',
                    'folder_id': None,
                },
            },
            'req_params': {
                'recognition_spec': {
                    'sample_rate_hertz': 8000,
                    'language_code': 'ru-RU',
                },
            },
            'audio_params': {
                'duration_seconds': 123.456,
                'size_bytes': 440000,
                'acoustic': 'unknown',
                'channel_count': 1,
            },
            'received_at': '1999-12-31T00:00:00+00:00',
            'other': None,
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(Record.from_yson(yson), obj)

        obj = RecordAudio(
            record_id='01399a9c-2e1a-4312-8648-9cfdca8e181e',
            audio=b'112233',
            hash=b'wejwg2',
            hash_version=HashVersion.CRC_32_BZIP2,
        )
        yson = {
            'record_id': '01399a9c-2e1a-4312-8648-9cfdca8e181e',
            'audio': b'112233',
            'hash': b'wejwg2',
            'hash_version': 'CRC_32_BZIP2',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(RecordAudio.from_yson(yson), obj)

    def test_records_tags(self):
        obj = RecordTagData(
            type=RecordTagType.PERIOD,
            value='2020-01',
        )
        yson = {
            'type': 'PERIOD',
            'value': '2020-01',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(RecordTagData.from_yson(yson), obj)

        obj = RecordTag(
            id='0756592e-6e9e-45ee-ae85-c9a70713a4a7',
            action=RecordTagAction.ADD,
            record_id='01399a9c-2e1a-4312-8648-9cfdca8e181e',
            data=RecordTagData(
                type=RecordTagType.PERIOD,
                value='2020-01',
            ),
            received_at=datetime(year=1999, month=12, day=31, tzinfo=timezone.utc),
        )
        yson = {
            'id': '0756592e-6e9e-45ee-ae85-c9a70713a4a7',
            'action': 'ADD',
            'record_id': '01399a9c-2e1a-4312-8648-9cfdca8e181e',
            'data': {
                'type': 'PERIOD',
                'value': '2020-01',
            },
            'received_at': '1999-12-31T00:00:00+00:00',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(RecordTag.from_yson(yson), obj)

    def test_records_bits(self):
        obj = SplitDataFixedLengthOffset(
            length_ms=9000,
            offset_ms=3000,
            split_cmd='pydub bla',
        )
        yson = {
            'version': 'fixed-length-offset',
            'length_ms': 9000,
            'offset_ms': 3000,
            'split_cmd': 'pydub bla',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(SplitDataFixedLengthOffset.from_yson(yson), obj)

        obj = BitDataTimeInterval(
            start_ms=12000,
            end_ms=15030,
            index=4,
            channel=1,
        )
        self.assertEqual(
            {
                'start_ms': 12000,
                'end_ms': 15030,
                'index': 4,
                'channel': 1,
            },
            obj.to_yson(),
        )
        self.assertEqual(
            BitDataTimeInterval.from_yson({
                'start_ms': 12000,
                'end_ms': 15030,
                'index': 4,
            }),
            obj,
        )

        obj = ConvertDataCmd(
            cmd='sox -i lol -f bar',
        )
        yson = {
            'cmd': 'sox -i lol -f bar',
            'version': 'cmd',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(ConvertDataCmd.from_yson(yson), obj)

        obj = RecordBitAudioParams(
            language_code='kk-KK',
            sample_rate_hertz=16000,
            acoustic='unknown',
        )
        yson = {
            'language_code': 'kk-KK',
            'sample_rate_hertz': 16000,
            'acoustic': 'unknown',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(RecordBitAudioParams.from_yson(yson), obj)

        obj = RecordBit(
            id='01399a9c-2e1a-4312-8648-9cfdca8e181e/1999-12-31T00:00:00+00:00/4',
            record_id='01399a9c-2e1a-4312-8648-9cfdca8e181e',
            split_id='01399a9c-2e1a-4312-8648-9cfdca8e181e/1999-12-31T00:00:00+00:00',
            split_data=SplitDataFixedLengthOffset(
                length_ms=9000,
                offset_ms=3000,
                split_cmd='pydub bla',
            ),
            bit_data=BitDataTimeInterval(
                start_ms=12000,
                end_ms=15030,
                index=4,
                channel=2,
            ),
            convert_data=ConvertDataCmd(
                cmd='sox -i lol -f bar',
            ),
            mark=Mark.TEST,
            s3_obj=S3Object(
                endpoint='https://storage.yandexcloud.net',
                bucket='cloud-ai-data',
                key='bits/topor.wav',
            ),
            audio_params=RecordBitAudioParams(
                language_code='kk-KK',
                sample_rate_hertz=16000,
                acoustic='unknown',
            ),
            received_at=datetime(year=1999, month=12, day=31, tzinfo=timezone.utc),
            other=None,
        )
        yson = {
            'id': '01399a9c-2e1a-4312-8648-9cfdca8e181e/1999-12-31T00:00:00+00:00/4',
            'record_id': '01399a9c-2e1a-4312-8648-9cfdca8e181e',
            'split_id': '01399a9c-2e1a-4312-8648-9cfdca8e181e/1999-12-31T00:00:00+00:00',
            'split_data': {
                'version': 'fixed-length-offset',
                'length_ms': 9000,
                'offset_ms': 3000,
                'split_cmd': 'pydub bla',
            },
            'bit_data': {
                'start_ms': 12000,
                'end_ms': 15030,
                'index': 4,
                'channel': 2,
            },
            'convert_data': {
                'cmd': 'sox -i lol -f bar',
                'version': 'cmd',
            },
            'mark': 'TEST',
            's3_obj': {
                'endpoint': 'https://storage.yandexcloud.net',
                'bucket': 'cloud-ai-data',
                'key': 'bits/topor.wav',
            },
            'audio_params': {
                'language_code': 'kk-KK',
                'sample_rate_hertz': 16000,
                'acoustic': 'unknown',
            },
            'received_at': '1999-12-31T00:00:00+00:00',
            'other': None,
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(RecordBit.from_yson(yson), obj)

        obj = RecordBitAudio(
            bit_id='01399a9c-2e1a-4312-8648-9cfdca8e181e/1999-12-31T00:00:00+00:00/4',
            audio=b'00xxff',
            hash=b'sfsjlf39',
            hash_version=HashVersion.CRC_32_BZIP2,
        )
        yson = {
            'bit_id': '01399a9c-2e1a-4312-8648-9cfdca8e181e/1999-12-31T00:00:00+00:00/4',
            'audio': b'00xxff',
            'hash': b'sfsjlf39',
            'hash_version': 'CRC_32_BZIP2',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(RecordBitAudio.from_yson(yson), obj)

    def test_records_bits_markups(self):
        obj = RecordBitMarkupFeedbackLoopValidationData(
            checks_ids=['111', '999', 'abc'],
        )
        yson = {
            'checks_ids': ['111', '999', 'abc'],
            'type': 'feedback-loop',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, RecordBitMarkupFeedbackLoopValidationData.from_yson(yson))

        obj = RecordBitMarkup(
            id='80002301-c10f-4433-97e8-b8095e76b9a2/2021-07-10T10:15:22.900423+00:00/01/000025/56c159f1-f2bf-4f9d-8d91-4b8b5bb81600',
            bit_id='80002301-c10f-4433-97e8-b8095e76b9a2/2021-07-10T10:15:22.900423+00:00/01/000025',
            record_id='80002301-c10f-4433-97e8-b8095e76b9a2',
            audio_obfuscation_data=SoxEffectsObfuscationDataV2(
                convert_cmd='sox {old_filename} {new_filename} --norm pitch {pitch}',
                dbfs_before=-15.9157450388,
                dbfs_after=-14.3346049955,
                pitch=-206,
                reduce_volume_cmd=None,
                dbfs_after_reduced=None,
            ),
            audio_params=RecordBitAudioParams(
                acoustic='unknown',
                language_code='ru-RU',
                sample_rate_hertz=16000,
            ),
            markup_id='19868742-7451-4a69-b24f-68db7c46d2d5',
            markup_step=MarkupStep.TRANSCRIPT,
            pool_id='25828907',
            assignment_id='540d3c22-d421-4b3b-b37a-fbd1768b6471',
            markup_data=MarkupData(
                version=MarkupDataVersions.TRANSCRIPT_AND_TYPE,
                input=AudioURLInput(
                    audio_s3_obj=S3Object(
                        endpoint='storage.yandexcloud.net',
                        bucket='cloud-ai-data',
                        key='Speechkit/STT/Toloka/Assignments/2021/07/09/02667abf-79f6-4c74-9a1e-30180103ffd5_2_30000-39000.wav',
                    ),
                ),
                solution=MarkupSolutionTranscriptAndType(
                    text='привет',
                    type=MarkupTranscriptType.SPEECH,
                ),
                known_solutions=[],
                task_id='000188b4a3--60e8964444a89020d568fa0d',
                overlap=1,
                raw_data=None,
                created_at=datetime(year=1999, month=12, day=31, tzinfo=timezone.utc),
            ),
            validation_data=RecordBitMarkupFeedbackLoopValidationData(
                checks_ids=['111', '999', 'abc'],
            ),
            received_at=datetime(year=1999, month=12, day=31, tzinfo=timezone.utc),
            other=None,
        )
        yson = {
            'id': '80002301-c10f-4433-97e8-b8095e76b9a2/2021-07-10T10:15:22.900423+00:00/01/000025/56c159f1-f2bf-4f9d-8d91-4b8b5bb81600',
            'bit_id': '80002301-c10f-4433-97e8-b8095e76b9a2/2021-07-10T10:15:22.900423+00:00/01/000025',
            'record_id': '80002301-c10f-4433-97e8-b8095e76b9a2',
            'audio_obfuscation_data': {
                'convert_cmd': 'sox {old_filename} {new_filename} --norm pitch {pitch}',
                'dbfs_before': -15.9157450388,
                'dbfs_after': -14.3346049955,
                'pitch': -206,
                'version': 'sox-effects-v2',
            },
            'audio_params': {
                'acoustic': 'unknown',
                'language_code': 'ru-RU',
                'sample_rate_hertz': 16000,
            },
            'markup_id': '19868742-7451-4a69-b24f-68db7c46d2d5',
            'markup_step': 'transcript',
            'pool_id': '25828907',
            'assignment_id': '540d3c22-d421-4b3b-b37a-fbd1768b6471',
            'markup_data': {
                'version': 'transcript-and-type',
                'input': {
                    'audio_s3_obj': {
                        'endpoint': 'storage.yandexcloud.net',
                        'bucket': 'cloud-ai-data',
                        'key': 'Speechkit/STT/Toloka/Assignments/2021/07/09/02667abf-79f6-4c74-9a1e-30180103ffd5_2_30000-39000.wav',
                    },
                },
                'solution': {
                    'text': 'привет',
                    'type': 'speech',
                },
                'known_solutions': [],
                'task_id': '000188b4a3--60e8964444a89020d568fa0d',
                'overlap': 1,
                'raw_data': None,
                'created_at': '1999-12-31T00:00:00+00:00',
            },
            'validation_data': {
                'checks_ids': ['111', '999', 'abc'],
                'type': 'feedback-loop',
            },
            'received_at': '1999-12-31T00:00:00+00:00',
            'other': None,
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, RecordBitMarkup.from_yson(yson))

    def test_markups_assignments(self):
        obj = TextComparisonStopWordsArcadiaSource(
            topic='general',
            lang='ru-RU',
            revision=112233,
        )
        yson = {
            'topic': 'general',
            'lang': 'ru-RU',
            'revision': 112233,
            'source': 'arcadia',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(TextComparisonStopWordsArcadiaSource.from_yson(yson), obj)

    def test_markups_pools(self):
        obj = MarkupPool(
            id='25184896',
            markup_id='7b6deb61-1631-41ee-83a0-27ff79c958f3',
            markup_step=MarkupStep.CHECK_ASR_TRANSCRIPT,
            params={
                'assignment_max_duration_seconds': 2187,
                'assignments_issuing_config': {
                    'issue_task_suites_in_creation_order': False,
                },
            },
            toloka_environment='PRODUCTION',
            received_at=datetime(year=1999, month=12, day=31, tzinfo=timezone.utc),
        )
        yson = {
            'id': '25184896',
            'markup_id': '7b6deb61-1631-41ee-83a0-27ff79c958f3',
            'markup_step': 'check-asr-transcript',
            'params': {
                'assignment_max_duration_seconds': 2187,
                'assignments_issuing_config': {
                    'issue_task_suites_in_creation_order': False,
                },
            },
            'toloka_environment': 'PRODUCTION',
            'received_at': '1999-12-31T00:00:00+00:00',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(MarkupPool.from_yson(yson), obj)

    def test_records_joins(self):
        obj = RecognitionPlainTranscript(text='алло')
        yson = {
            'text': 'алло',
            'version': 'plain-transcript',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, RecognitionPlainTranscript.from_yson(yson))

        obj = RecognitionRawText(text='алло')
        yson = {
            'text': 'алло',
            'version': 'raw-text',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, RecognitionRawText.from_yson(yson))

        obj = ChannelRecognition(
            channel=2,
            recognition=RecognitionRawText(text='алло'),
        )
        yson = {
            'channel': 2,
            'recognition': {
                'text': 'алло',
                'version': 'raw-text',
            },
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, ChannelRecognition.from_yson(yson))

        obj = MultiChannelRecognition(
            channels=[
                ChannelRecognition(
                    channel=1,
                    recognition=RecognitionPlainTranscript(text='вы позвонили в мега центр'),
                ),
                ChannelRecognition(
                    channel=2,
                    recognition=RecognitionRawText(text='алло'),
                )
            ]
        )
        yson = {
            'channels': [
                {
                    'channel': 1,
                    'recognition': {
                        'text': 'вы позвонили в мега центр',
                        'version': 'plain-transcript',
                    },
                },
                {
                    'channel': 2,
                    'recognition': {
                        'text': 'алло',
                        'version': 'raw-text',
                    },
                }
            ],
            'multichannel': True,
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, MultiChannelRecognition.from_yson(yson))

        obj = SimpleStaticOverlapStrategy(
            min_majority=2,
            overall=3,
        )
        yson = {
            'min_majority': 2,
            'overall': 3,
            'overlap_type': 'static',
            'strategy': 'simple',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, SimpleStaticOverlapStrategy.from_yson(yson))

        obj = JoinDataNoSpeechTranscriptMV(
            votes=MajorityVote(
                majority=2,
                overall=3,
            ),
            overlap_strategy=SimpleStaticOverlapStrategy(
                min_majority=1,
                overall=3,
            ),
        )
        yson = {
            'votes': {
                'majority': 2,
                'overall': 3,
            },
            'overlap_strategy': {
                'min_majority': 1,
                'overall': 3,
                'overlap_type': 'static',
                'strategy': 'simple',
            },
            'version': 'no-speech-ts-mv',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, JoinDataNoSpeechTranscriptMV.from_yson(yson))

        obj = JoinDataFeedbackLoop(
            confidence=0.5,
            assignment_accuracy=0.6,
            assignment_evaluation_recall=1.0,
            attempts=2,
        )
        yson = {
            'confidence': 0.5,
            'assignment_accuracy': 0.6,
            'assignment_evaluation_recall': 1.0,
            'attempts': 2,
            'version': 'feedback-loop-v0.0',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, JoinDataFeedbackLoop.from_yson(yson))

        obj = ChannelJoinData(
            channel=2,
            join_data=JoinDataNotOKCheckTranscriptNoSpeechMV(
                votes=MajorityVote(
                    majority=2,
                    overall=3,
                ),
                overlap_strategy=None,
            ),
        )
        yson = {
            'channel': 2,
            'join_data': {
                'votes': {
                    'majority': 2,
                    'overall': 3,
                },
                'version': 'ct-not-ok-no-speech-mv',
            }
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, ChannelJoinData.from_yson(yson))

        obj = MultiChannelJoinData(
            channels=[
                ChannelJoinData(
                    channel=1,
                    join_data=JoinDataVoicetableReferenceRu(),
                ),
                ChannelJoinData(
                    channel=2,
                    join_data=JoinDataVoiceRecorder(),
                )
            ]
        )
        yson = {
            'channels': [
                {
                    'channel': 1,
                    'join_data': {
                        'version': 'voicetable-ref-ru',
                    },
                },
                {
                    'channel': 2,
                    'join_data': {
                        'version': 'voice-recorder',
                    },
                },
            ],
            'multichannel': True,
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, MultiChannelJoinData.from_yson(yson))

        obj = RecordJoin(
            record_id='3a3d2c27-49c8-4a9f-8afb-74256fa4c67d',
            id='3a3d2c27-49c8-4a9f-8afb-74256fa4c67d/2021-07-18T04:53:03.480515+00:00',
            recognition=MultiChannelRecognition(
                channels=[
                    ChannelRecognition(
                        channel=1,
                        recognition=RecognitionPlainTranscript('алло'),
                    ),
                    ChannelRecognition(
                        channel=2,
                        recognition=RecognitionPlainTranscript('вы позвонили в центр-пентр'),
                    ),
                ],
            ),
            join_data=MultiChannelJoinData(
                channels=[
                    ChannelJoinData(
                        channel=1,
                        join_data=JoinDataDpM3NaiveDev(),
                    ),
                    ChannelJoinData(
                        channel=2,
                        join_data=JoinDataFeedbackLoop(
                            confidence=0.8,
                            assignment_accuracy=0.9,
                            assignment_evaluation_recall=0.5,
                            attempts=1,
                        ),
                    )
                ],
            ),
            received_at=datetime(year=1999, month=12, day=31, tzinfo=timezone.utc),
            other=None,
        )
        yson = {
            'record_id': '3a3d2c27-49c8-4a9f-8afb-74256fa4c67d',
            'id': '3a3d2c27-49c8-4a9f-8afb-74256fa4c67d/2021-07-18T04:53:03.480515+00:00',
            'recognition': {
                'multichannel': True,
                'channels': [
                    {
                        'channel': 1,
                        'recognition': {
                            'text': 'алло',
                            'version': 'plain-transcript',
                        },
                    },
                    {
                        'channel': 2,
                        'recognition': {
                            'text': 'вы позвонили в центр-пентр',
                            'version': 'plain-transcript',
                        },
                    },
                ],
            },
            'join_data': {
                'multichannel': True,
                'channels': [
                    {
                        'channel': 1,
                        'join_data': {
                            'version': 'dp-m3-naive:dev',
                        },
                    },
                    {
                        'channel': 2,
                        'join_data': {
                            'confidence': 0.8,
                            'assignment_accuracy': 0.9,
                            'assignment_evaluation_recall': 0.5,
                            'attempts': 1,
                            'version': 'feedback-loop-v0.0',
                        },
                    },
                ],
            },
            'received_at': '1999-12-31T00:00:00+00:00',
            'other': None,
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(obj, RecordJoin.from_yson(yson))

    def test_eval(self):
        obj = ClusterReferencesArcadia(
            topic='common',
            lang='ru-RU',
            revision=778899,
        )
        yson = {
            'topic': 'common',
            'lang': 'ru-RU',
            'revision': 778899,
            'source': 'arcadia',
        }
        self.assertEqual(yson, obj.to_yson())
        self.assertEqual(ClusterReferencesArcadia.from_yson(yson), obj)

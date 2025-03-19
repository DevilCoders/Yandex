import typing
import unittest

from mock import patch

from cloud.ai.speechkit.stt.lib.data.model.dao import MarkupTranscriptType
from cloud.ai.speechkit.stt.lib.data.ops.queries import TranscriptHoneypotCandidateRow
from cloud.ai.speechkit.stt.lib.data_pipeline.honeypots.transcript import (
    get_honeypot_toloka_data,
    filter_honeypots_candidates_by_words_blacklist,
    generate_honeypots,
)


class TestTranscriptHoneypotsGeneration(unittest.TestCase):
    def test_honeypot_toloka_data(self):
        self.assertEqual(
            {
                'input_values': {
                    'url': 'https://storage.yandexcloud.net/cloud-ai-data/some-dir/bit.wav',
                },
                'known_solutions': [
                    {
                        'output_values': {
                            'text': 'алло ну пока удобно',
                            'cls': 'sp',
                        },
                        'correctness_weight': 1,
                    },
                    {
                        'output_values': {
                            'text': 'алло пока удобно',
                            'cls': 'sp',
                        },
                        'correctness_weight': 1,
                    },
                ],
            },
            get_honeypot_toloka_data(
                audio_s3_key='some-dir/bit.wav',
                texts=['алло ну пока удобно', 'алло пока удобно'],
                type=MarkupTranscriptType.SPEECH,
            ),
        )

        self.assertEqual(
            {
                'input_values': {
                    'url': 'https://storage.yandexcloud.net/cloud-ai-data/some-dir/bit-no-speech.wav',
                },
                'known_solutions': [
                    {
                        'output_values': {
                            'text': '',
                            'cls': 'si',
                        },
                        'correctness_weight': 1,
                    },
                ],
            },
            get_honeypot_toloka_data(
                audio_s3_key='some-dir/bit-no-speech.wav',
                texts=[''],
                type=MarkupTranscriptType.NO_SPEECH,
            ),
        )

        with self.assertRaises(ValueError):
            get_honeypot_toloka_data(
                audio_s3_key='some-dir/bit.wav',
                texts=['', 'алло'],
                type=MarkupTranscriptType.NO_SPEECH,
            )

        with self.assertRaises(ValueError):
            get_honeypot_toloka_data(
                audio_s3_key='some-dir/bit.wav',
                texts=['алло', '', 'ага'],
                type=MarkupTranscriptType.SPEECH,
            )

    def test_filter_honeypots_candidates_by_words_blacklist(self):
        def generate_honeypots_candidates(solutions_texts: typing.List[typing.List[str]]):
            return [TranscriptHoneypotCandidateRow(audio_s3_key='foo', texts=texts) for texts in solutions_texts]

        self.assertEqual(
            generate_honeypots_candidates(
                [
                    ['автоматически подключается или как', 'автоматически подключается или'],
                    ['не нужно пожалуйста не нужно ничего подключать'],
                    ['вы ж меня слышите нет что я вам говорю слышите'],
                ]
            ),
            filter_honeypots_candidates_by_words_blacklist(
                honeypots_candidates=generate_honeypots_candidates(
                    [
                        ['автоматически подключается или как', 'автоматически подключается или'],
                        ['ну пока удобно', 'алло ну пока удобно'],
                        ['номер недоступен оставьте сообщение на автоответчик'],
                        ['не нужно пожалуйста не нужно ничего подключать'],
                        ['номер не отвечает оставьте сообщение на автоответчик'],
                        ['але здравствуйте компания мегафон'],
                        ['вы ж меня слышите нет что я вам говорю слышите'],
                        ['не надо мне ничего спасибо до свидания'],
                    ]
                ),
                words_blacklist=[
                    'алло',
                    'але',
                    'недоступен',
                    'автоответчик',
                ],
                limit=3,
            )[0],
        )

        self.assertEqual(
            generate_honeypots_candidates(
                [
                    ['вы ж меня слышите нет что я вам говорю слышите'],
                ]
            ),
            filter_honeypots_candidates_by_words_blacklist(
                honeypots_candidates=generate_honeypots_candidates(
                    [
                        ['алло ну пока удобно'],
                        ['вы ж меня слышите нет что я вам говорю слышите'],
                        ['номер недоступен оставьте сообщение на автоответчик'],
                        ['але здравствуйте компания мегафон'],
                    ]
                ),
                words_blacklist=[
                    'алло',
                    'але',
                    'недоступен',
                    'автоответчик',
                ],
                limit=100,
            )[0],
        )

    @patch(
        'cloud.ai.speechkit.stt.lib.data_pipeline.honeypots.transcript.generate.select_transcript_honeypots_candidates'
    )
    def test_not_enough_honeypots_candidates(self, select_honeypots_candidates):
        def call_generate_honeypots():
            return generate_honeypots(
                markups_table_name_ge=None,
                random_sample_fraction=0.25,
                text_min_length=3,
                text_min_words=1,
                solution_min_overlap=1,
                max_unique_texts=1,
                from_honeypots=False,
                honeypots_with_speech_count=3,
                words_blacklist=[
                    'алло',
                    'недоступен',
                ],
                lang='ru-RU',
            )

        # Not enough rows from YQL query
        select_honeypots_candidates.return_value = []
        with self.assertRaises(RuntimeError):
            call_generate_honeypots()

        # Enough YQL query rows, but not texts not filtered by words blacklist
        select_honeypots_candidates.return_value = [
            TranscriptHoneypotCandidateRow(audio_s3_key='some-dir/speech1.wav', texts=['алло ну пока удобно']),
            TranscriptHoneypotCandidateRow(
                audio_s3_key='some-dir/speech2.wav', texts=['вы ж меня слышите нет что я вам говорю слышите'],
            ),
            TranscriptHoneypotCandidateRow(
                audio_s3_key='some-dir/speech3.wav', texts=['автоматически подключается или как'],
            ),
            TranscriptHoneypotCandidateRow(
                audio_s3_key='some-dir/speech4.wav', texts=['номер недоступен оставьте сообщение на автоответчик'],
            ),
        ]
        with self.assertRaises(RuntimeError):
            call_generate_honeypots()

        # OK, enough unique texts, filtered by word blacklist
        select_honeypots_candidates.return_value = [
            TranscriptHoneypotCandidateRow(audio_s3_key=f'some-dir/speech{i}.wav', texts=[f'подключать {i}'])
            for i in range(4)
        ]
        result = call_generate_honeypots()
        self.assertEqual(3, len(result))

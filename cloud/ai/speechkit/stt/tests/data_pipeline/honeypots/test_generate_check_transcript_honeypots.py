from collections import OrderedDict
import unittest
from cloud.ai.speechkit.stt.lib.data.model.dao import MarkupTranscriptType
from cloud.ai.speechkit.stt.lib.data_pipeline.honeypots.check_transcript import (
    get_check_toloka_dict_from_check_honeypot,
    extract_solution_types,
    CheckTranscriptSolutionType,
    CheckTranscriptHoneypotCandidateRow,
    generate_mutated_honeypots,
    get_check_honeypot_from_toloka_transcript_dict,
)


class TestTranscriptHoneypotsGeneration(unittest.TestCase):
    def test_honeypot_toloka_data(self):
        self.assertEqual(
            {
                'input_values': {
                    'url': 'https://storage.yandexcloud.net/cloud-ai-data/some-dir/bit.wav',
                    'text': 'алло ну пока удобно',
                },
                'known_solutions': [
                    {
                        'output_values': {
                            'ok': True,
                            'cls': 'sp',
                        },
                        'correctness_weight': 1,
                    }
                ],
            },
            get_check_toloka_dict_from_check_honeypot(CheckTranscriptHoneypotCandidateRow(
                audio_s3_key='some-dir/bit.wav', text='алло ну пока удобно',
                ok=True, type=MarkupTranscriptType.SPEECH)),
        )

        self.assertEqual(
            {
                'input_values': {
                    'url': 'https://storage.yandexcloud.net/cloud-ai-data/some-dir/bit.wav',
                    'text': 'алиса',
                },
                'known_solutions': [
                    {
                        'output_values': {
                            'ok': False,
                            'cls': 'si',
                        },
                        'correctness_weight': 1,
                    }
                ],
            },
            get_check_toloka_dict_from_check_honeypot(CheckTranscriptHoneypotCandidateRow(
                audio_s3_key='some-dir/bit.wav', text='алиса',
                ok=False, type=MarkupTranscriptType.NO_SPEECH)),
        )

    def test_extract_solution_types(self):
        solution_type_to_count = OrderedDict(
            {
                CheckTranscriptSolutionType.OK_SPEECH: 2,
                CheckTranscriptSolutionType.OK_NO_SPEECH: 2,
                CheckTranscriptSolutionType.NOT_OK_SPEECH: 2,
                CheckTranscriptSolutionType.NOT_OK_NO_SPEECH: 2,
            }
        )

        honeypots_candidates = []

        with self.assertRaises(ValueError):
            extract_solution_types(honeypots_candidates, solution_type_to_count)

        honeypots_candidates += [
            # OK_SPEECH
            CheckTranscriptHoneypotCandidateRow(
                audio_s3_key='01.wav', text='да', ok=True, type=MarkupTranscriptType.SPEECH
            ),
            # OK_NO_SPEECH
            CheckTranscriptHoneypotCandidateRow(
                audio_s3_key='02.wav', text='', ok=True, type=MarkupTranscriptType.NO_SPEECH
            ),
            # <none>
            CheckTranscriptHoneypotCandidateRow(
                audio_s3_key='03.wav', text='нет', ok=False, type=MarkupTranscriptType.UNCLEAR
            ),
            # OK_SPEECH
            CheckTranscriptHoneypotCandidateRow(
                audio_s3_key='04.wav', text='до свидания', ok=True, type=MarkupTranscriptType.SPEECH
            ),
            # OK_NO_SPEECH
            CheckTranscriptHoneypotCandidateRow(
                audio_s3_key='05.wav', text='', ok=True, type=MarkupTranscriptType.NO_SPEECH
            ),
            # NOT_OK_SPEECH
            CheckTranscriptHoneypotCandidateRow(
                audio_s3_key='06.wav', text='алиса', ok=False, type=MarkupTranscriptType.SPEECH
            ),
            # OK_SPEECH
            CheckTranscriptHoneypotCandidateRow(
                audio_s3_key='07.wav', text='абонент не отвечает', ok=True, type=MarkupTranscriptType.SPEECH
            ),
            # NOT_OK_NO_SPEECH
            CheckTranscriptHoneypotCandidateRow(
                audio_s3_key='08.wav', text='диван', ok=False, type=MarkupTranscriptType.NO_SPEECH
            ),
            # <none>
            CheckTranscriptHoneypotCandidateRow(
                audio_s3_key='09.wav', text='где', ok=False, type=MarkupTranscriptType.UNCLEAR
            ),
            # OK_NO_SPEECH
            CheckTranscriptHoneypotCandidateRow(
                audio_s3_key='10.wav', text='', ok=True, type=MarkupTranscriptType.NO_SPEECH
            ),
            # NOT_OK_NO_SPEECH
            CheckTranscriptHoneypotCandidateRow(
                audio_s3_key='11.wav', text='алиса', ok=False, type=MarkupTranscriptType.NO_SPEECH
            ),
        ]

        with self.assertRaises(ValueError):
            extract_solution_types(honeypots_candidates, solution_type_to_count)

        honeypots_candidates += [
            # NOT_OK_SPEECH
            CheckTranscriptHoneypotCandidateRow(
                audio_s3_key='12.wav', text='', ok=False, type=MarkupTranscriptType.SPEECH
            ),
        ]

        self.assertEqual(
            [
                CheckTranscriptHoneypotCandidateRow(
                    audio_s3_key='01.wav', text='да', ok=True, type=MarkupTranscriptType.SPEECH
                ),
                CheckTranscriptHoneypotCandidateRow(
                    audio_s3_key='04.wav', text='до свидания', ok=True, type=MarkupTranscriptType.SPEECH
                ),
                CheckTranscriptHoneypotCandidateRow(
                    audio_s3_key='02.wav', text='', ok=True, type=MarkupTranscriptType.NO_SPEECH
                ),
                CheckTranscriptHoneypotCandidateRow(
                    audio_s3_key='05.wav', text='', ok=True, type=MarkupTranscriptType.NO_SPEECH
                ),
                CheckTranscriptHoneypotCandidateRow(
                    audio_s3_key='06.wav', text='алиса', ok=False, type=MarkupTranscriptType.SPEECH
                ),
                CheckTranscriptHoneypotCandidateRow(
                    audio_s3_key='12.wav', text='', ok=False, type=MarkupTranscriptType.SPEECH
                ),
                CheckTranscriptHoneypotCandidateRow(
                    audio_s3_key='08.wav', text='диван', ok=False, type=MarkupTranscriptType.NO_SPEECH
                ),
                CheckTranscriptHoneypotCandidateRow(
                    audio_s3_key='11.wav', text='алиса', ok=False, type=MarkupTranscriptType.NO_SPEECH
                ),
            ],
            extract_solution_types(honeypots_candidates, solution_type_to_count),
        )

        self.assertEqual(
            [
                CheckTranscriptHoneypotCandidateRow(
                    audio_s3_key='08.wav', text='диван', ok=False, type=MarkupTranscriptType.NO_SPEECH
                ),
            ],
            extract_solution_types(honeypots_candidates, {CheckTranscriptSolutionType.NOT_OK_NO_SPEECH: 1}),
        )

    def test_get_honeypot_from_toloka_dict(self):
        transcript_honeypots_data = [
            {
                'input_values': {
                    'url': 'https://storage.yandexcloud.net/cloud-ai-data/Speechkit/STT/Toloka/Assignments/2020/08/07/04e8403d-de06-49d7-a43b-d4c24fa6a1d2_0-1810.wav'
                },
                'known_solutions': [{'correctness_weight': 1, 'output_values': {'text': 'о господи', 'cls': 'sp'}}],
            },
            {
                'input_values': {
                    'url': 'https://storage.yandexcloud.net/cloud-ai-data/Speechkit/STT/Toloka/Assignments/2020/06/23/57fb95ca-9b1d-44a7-9cfe-bc2a84796bd3_0-5720.wav'
                },
                'known_solutions': [{'correctness_weight': 1, 'output_values': {'text': '', 'cls': 'si'}}],
            },
        ]

        honeypots = [get_check_honeypot_from_toloka_transcript_dict(hd) for hd in transcript_honeypots_data]

        self.assertEqual(honeypots, [
            CheckTranscriptHoneypotCandidateRow(
                audio_s3_key='Speechkit/STT/Toloka/Assignments/2020/08/07/04e8403d-de06-49d7-a43b-d4c24fa6a1d2_0-1810.wav',
                ok=True,
                text='о господи',
                type=MarkupTranscriptType.SPEECH,
            ),
            CheckTranscriptHoneypotCandidateRow(
                audio_s3_key='Speechkit/STT/Toloka/Assignments/2020/06/23/57fb95ca-9b1d-44a7-9cfe-bc2a84796bd3_0-5720.wav',
                ok=True,
                text='',
                type=MarkupTranscriptType.NO_SPEECH,
            ),
        ])

    def test_generate_mutated_honeypots(self):
        transcript_honeypots_data = [
            {
                'input_values': {
                    'url': 'https://storage.yandexcloud.net/cloud-ai-data/Speechkit/STT/Toloka/Assignments/2020/08/07/04e8403d-de06-49d7-a43b-d4c24fa6a1d2_0-1810.wav'
                },
                'known_solutions': [
                    {'correctness_weight': 1, 'output_values': {'text': 'о господи', 'cls': 'sp'}}
                ],
            },
            {
                'input_values': {
                    'url': 'https://storage.yandexcloud.net/cloud-ai-data/Speechkit/STT/Toloka/Assignments/2020/08/04/d404b0bf-36f8-40f2-9500-20bf7b26cd95_0-4310.wav'
                },
                'known_solutions': [
                    {
                        'correctness_weight': 1,
                        'output_values': {'text': 'у меня почему то на карточке исчезают деньги', 'cls': 'sp'},
                    }
                ],
            },
            {
                'input_values': {
                    'url': 'https://storage.yandexcloud.net/cloud-ai-data/Speechkit/STT/Toloka/Assignments/2020/07/30/ca9218f8-3155-4201-bb05-cd095e37058d_0-6260.wav'
                },
                'known_solutions': [
                    {'correctness_weight': 1, 'output_values': {'text': 'извините нет мне некогда', 'cls': 'sp'}}
                ],
            },
            {
                'input_values': {
                    'url': 'https://storage.yandexcloud.net/cloud-ai-data/Speechkit/STT/Toloka/Assignments/2020/08/04/021e3e54-dc73-4037-b763-d5d191540765_0-4810.wav'
                },
                'known_solutions': [
                    {
                        'correctness_weight': 1,
                        'output_values': {'text': 'перевести на консультанта не онлайн', 'cls': 'sp'},
                    }
                ],
            },
            {
                'input_values': {
                    'url': 'https://storage.yandexcloud.net/cloud-ai-data/Speechkit/STT/Toloka/Assignments/2020/08/04/9460b10c-48a1-402f-ac80-3153d486fa48_0-2210.wav'
                },
                'known_solutions': [
                    {'correctness_weight': 1, 'output_values': {'text': 'разговор с оператором', 'cls': 'sp'}}
                ],
            },
            {
                'input_values': {
                    'url': 'https://storage.yandexcloud.net/cloud-ai-data/Speechkit/STT/Toloka/Assignments/2020/07/29/b6363c4e-620a-4572-a1ba-613dbd50ddcf_0-9000.wav'
                },
                'known_solutions': [
                    {'correctness_weight': 1, 'output_values': {'text': 'четыре девять семь три', 'cls': 'sp'}}
                ],
            },
            {
                'input_values': {
                    'url': 'https://storage.yandexcloud.net/cloud-ai-data/Speechkit/STT/Toloka/Assignments/2020/07/06/2170bb3e-b956-4ec2-b79e-45761cf177df_0-9000.wav'
                },
                'known_solutions': [
                    {
                        'correctness_weight': 1,
                        'output_values': {
                            'text': 'двадцать ноль восемь тысяча девятьсот восемьдесят четвёртый',
                            'cls': 'sp',
                        },
                    }
                ],
            },
            {
                'input_values': {
                    'url': 'https://storage.yandexcloud.net/cloud-ai-data/Speechkit/STT/Toloka/Assignments/2020/08/04/e4164c9f-b823-4b9b-aa68-5184bf64913a_0-2830.wav'
                },
                'known_solutions': [
                    {
                        'correctness_weight': 1,
                        'output_values': {'text': 'как мне досрочно погасить кредит', 'cls': 'sp'},
                    }
                ],
            },
            {
                'input_values': {
                    'url': 'https://storage.yandexcloud.net/cloud-ai-data/Speechkit/STT/Toloka/Assignments/2020/07/19/e7d7eacc-b78c-4885-83bb-d3d12acbc2ab_0-9000.wav'
                },
                'known_solutions': [
                    {'correctness_weight': 1, 'output_values': {'text': 'семь девять восемь ноль', 'cls': 'sp'}}
                ],
            },
            {
                'input_values': {
                    'url': 'https://storage.yandexcloud.net/cloud-ai-data/Speechkit/STT/Toloka/Assignments/2020/06/26/763dccd9-1524-40ea-9f1d-46b0204e79e1_0-9000.wav'
                },
                'known_solutions': [
                    {
                        'correctness_weight': 1,
                        'output_values': {
                            'text': 'девушка что нужно для того чтобы с вами расторгнуть договор и уйти от вас',
                            'cls': 'sp',
                        },
                    }
                ],
            },
        ]
        original_honeypots = [get_check_honeypot_from_toloka_transcript_dict(hd) for hd in transcript_honeypots_data]

        mutated_honeypots = generate_mutated_honeypots(
            original_honeypots, 0.5, from_overlap=False, lang='ru-RU',
        )
        self.assertEqual(10, len(mutated_honeypots))
        not_ok_count = 0

        for mutated_honeypot, original_honeypot in zip(mutated_honeypots, original_honeypots):
            if mutated_honeypot.ok:
                self.assertEqual(mutated_honeypot, original_honeypot)
            else:
                not_ok_count += 1
                self.assertNotEqual(mutated_honeypot.text, original_honeypot.text)
                self.assertEqual(mutated_honeypot.type, MarkupTranscriptType.SPEECH)
                self.assertEqual(mutated_honeypot.audio_s3_key, original_honeypot.audio_s3_key)

        self.assertEqual(5, not_ok_count)

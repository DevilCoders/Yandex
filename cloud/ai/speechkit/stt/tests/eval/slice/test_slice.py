import unittest
from datetime import datetime, timezone

import requests_mock

from cloud.ai.lib.python.datetime import now
from cloud.ai.speechkit.stt.lib.data.model.dao import TextComparisonStopWordsArcadiaSource
from cloud.ai.speechkit.stt.lib.text.slice.application import infer_slices
from cloud.ai.speechkit.stt.lib.text.slice.generation import (
    PredicateContainTexts,
    PredicateLength,
    PredicateCustom,
    PredicateExpression,
    SliceDescriptor
)


class TestMetricsCalculation(unittest.TestCase):
    arcadia_token = 'mock'

    expression = PredicateExpression(
        operator='or',
        predicates=[
            PredicateLength(
                min_length=5,
                max_length=None,
            ),
            PredicateExpression(
                operator='and',
                predicates=[
                    PredicateCustom(
                        eval_str='not text.startswith("й")'
                    ),
                    PredicateLength(
                        min_length=None,
                        max_length=20,
                    ),
                ],
            ),
        ],
    )

    @requests_mock.mock()
    def test_predicate_contains_texts(self, m):
        m.get('https://a.yandex-team.ru/api/tree/blob/trunk/'
              'arcadia/cloud/ai/speechkit/stt/lib/text/resources/text_comparison_stop_words/ru-RU/general.json?rev=100',
              text='[]')

        with self.assertRaises(AssertionError):
            PredicateContainTexts(
                texts_source=TextComparisonStopWordsArcadiaSource(
                    topic='general',
                    lang='ru-RU',
                    revision=100,
                ),
                words=True,
                negation=True,
            ).to_eval_str(self.arcadia_token)

        m.get('https://a.yandex-team.ru/api/tree/blob/trunk/'
              'arcadia/cloud/ai/speechkit/stt/lib/text/resources/text_comparison_stop_words/ru-RU/autoresponder.json?rev=123',
              text='["абонент", "недоступен"]')

        self.assertEqual(
            'not any(word in {"абонент", "недоступен", } for word in text.split(" "))',
            PredicateContainTexts(
                texts_source=TextComparisonStopWordsArcadiaSource(
                    topic='autoresponder',
                    lang='ru-RU',
                    revision=123,
                ),
                words=True,
                negation=True,
            ).to_eval_str(self.arcadia_token),
        )

        m.get('https://a.yandex-team.ru/api/tree/blob/trunk/'
              'arcadia/cloud/ai/speechkit/stt/lib/text/resources/text_comparison_stop_words/ru-RU/autoresponder-phrases.json?rev=123',
              text='["голосового сигнала", "оставьте сообщение"]')

        self.assertEqual(
            'any(t in text for t in ("голосового сигнала", "оставьте сообщение", ))',
            PredicateContainTexts(
                texts_source=TextComparisonStopWordsArcadiaSource(
                    topic='autoresponder-phrases',
                    lang='ru-RU',
                    revision=123,
                ),
                words=False,
                negation=False,
            ).to_eval_str(self.arcadia_token),
        )

    def test_predicate_length(self):
        with self.assertRaises(AssertionError):
            PredicateLength(
                min_length=None,
                max_length=None,
            ).to_eval_str(self.arcadia_token)

        with self.assertRaises(AssertionError):
            PredicateLength(
                min_length=4,
                max_length=3,
            ).to_eval_str(self.arcadia_token)

        self.assertEqual(
            'len(text) >= 2',
            PredicateLength(min_length=2, max_length=None).to_eval_str(self.arcadia_token))

        self.assertEqual(
            'len(text) <= 6',
            PredicateLength(min_length=None, max_length=6).to_eval_str(self.arcadia_token))

        self.assertEqual(
            'len(text) >= 2 and len(text) <= 6',
            PredicateLength(min_length=2, max_length=6).to_eval_str(self.arcadia_token))

    def test_predicate_custom(self):
        self.assertEqual('text != "алиса"', PredicateCustom(eval_str='text != "алиса"').to_eval_str(self.arcadia_token))

    def test_predicate_expression(self):
        with self.assertRaises(AssertionError):
            PredicateExpression(operator='and', predicates=[]).to_eval_str(self.arcadia_token)

        self.assertEqual(
            '(len(text) >= 5 or (not text.startswith("й") and len(text) <= 20))',
            self.expression.to_eval_str(self.arcadia_token),
        )

    def test_serialization(self):
        yson = {
            'name': 'foo',
            'predicate': {
                'operator': 'or',
                'predicates': [
                    {
                        'max_length': None,
                        'min_length': 5,
                        'type': 'length',
                    },
                    {
                        'operator': 'and',
                        'predicates': [
                            {
                                'eval_str': 'not text.startswith("й")',
                                'type': 'custom',
                            },
                            {
                                'max_length': 20,
                                'min_length': None,
                                'type': 'length',
                            },
                        ],
                        'type': 'expression'
                    },
                ],
                'type': 'expression',
            },
            'received_at': '1999-12-31T00:00:00+00:00',
        }

        slice_desc = SliceDescriptor(
            name='foo',
            predicate=self.expression,
            received_at=datetime(year=1999, month=12, day=31, tzinfo=timezone.utc),
        )

        self.assertEqual(yson, slice_desc.to_yson())
        self.assertEqual(slice_desc, SliceDescriptor.from_yson(yson))

    @requests_mock.mock()
    def test_application(self, m):
        m.get('https://a.yandex-team.ru/api/tree/blob/trunk/'
              'arcadia/cloud/ai/speechkit/stt/lib/text/resources/text_comparison_stop_words/ru-RU/autoresponder.json?rev=123',
              text='["абонент", "недоступен"]')

        m.get('https://a.yandex-team.ru/api/tree/blob/trunk/'
              'arcadia/cloud/ai/speechkit/stt/lib/text/resources/text_comparison_stop_words/ru-RU/autoresponder-phrases.json?rev=123',
              text='["голосового сигнала", "оставьте сообщение"]')

        slice_1 = SliceDescriptor(
            name='not-autoresponder-not-empty',
            predicate=PredicateExpression(
                operator='and',
                predicates=[
                    PredicateContainTexts(
                        texts_source=TextComparisonStopWordsArcadiaSource(
                            topic='autoresponder',
                            lang='ru-RU',
                            revision=123,
                        ),
                        words=True,
                        negation=True,
                    ),
                    PredicateContainTexts(
                        texts_source=TextComparisonStopWordsArcadiaSource(
                            topic='autoresponder-phrases',
                            lang='ru-RU',
                            revision=123,
                        ),
                        words=False,
                        negation=True,
                    ),
                    PredicateLength(min_length=1, max_length=None),
                ],
            ),
            received_at=now(),
        )

        slice_2 = SliceDescriptor(
            name='with-digits',
            predicate=PredicateCustom(
                eval_str=r'len(re.findall("\d+", text)) > 0',
            ),
            received_at=now(),
        )

        m.get('https://a.yandex-team.ru/api/tree/blob/trunk/'
              'arcadia/cloud/ai/speechkit/stt/lib/text/resources/text_comparison_stop_words/ru-RU/alice.json?rev=123',
              text='["алиса"]')

        m.get('https://a.yandex-team.ru/api/tree/blob/trunk/'
              'arcadia/cloud/ai/speechkit/stt/lib/text/resources/text_comparison_stop_words/ru-RU/alice-phrases.json?rev=123',
              text='["включи музыку"]')

        slice_3 = SliceDescriptor(
            name='alice',
            predicate=PredicateExpression(
                operator='or',
                predicates=[
                    PredicateContainTexts(
                        texts_source=TextComparisonStopWordsArcadiaSource(
                            topic='alice',
                            lang='ru-RU',
                            revision=123,
                        ),
                        words=True,
                        negation=False,
                    ),
                    PredicateContainTexts(
                        texts_source=TextComparisonStopWordsArcadiaSource(
                            topic='alice-phrases',
                            lang='ru-RU',
                            revision=123,
                        ),
                        words=False,
                        negation=False,
                    ),
                ],
            ),
            received_at=now(),
        )

        prep_slices = [slice.prepare(self.arcadia_token) for slice in (slice_1, slice_2, slice_3)]

        for text, expected_slices in (
            ('добрый день', ['not-autoresponder-not-empty']),
            ('', []),
            ('добрый 42 день', ['not-autoresponder-not-empty', 'with-digits']),
            ('алло абонент до свидания', []),
            ('приве7 недоступен', ['with-digits']),
            ('после голосового сигнала', []),
            ('прием алиса', ['not-autoresponder-not-empty', 'alice']),
            ('3й включи музыку', ['not-autoresponder-not-empty', 'with-digits', 'alice']),
        ):
            self.assertEqual(expected_slices, infer_slices(text, prep_slices),
                             f'{text} should have slices {expected_slices}')

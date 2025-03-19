import unittest

from cloud.ai.speechkit.stt.lib.data.model.dao import RecordTagData, RecordTagType
from cloud.ai.speechkit.stt.lib.data.model.tags import (
    filter_tags_by_tag_conjunction, prepare_tags_for_view, decompose_tag, RecordTagDataRequest,
)


class TestTagView(unittest.TestCase):
    def test_filter_tags_by_tag_conjunction(self):
        tag_conjunctions = [
            {
                RecordTagDataRequest(type=RecordTagType.CLIENT, value='mtt-u16', negation=False),
                RecordTagDataRequest(type=RecordTagType.MODE, value='stream', negation=False),
                RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-05', negation=False),
            },
            {
                RecordTagDataRequest(type=RecordTagType.IMPORT, value='yandex-call-center', negation=False),
                RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-03', negation=False),
            },
        ]
        for tags, filtered_tags in (
            (
                ['IMPORT:yandex-call-center', 'PERIOD:2020-03', 'LANG:ru-RU'],
                {'IMPORT:yandex-call-center', 'PERIOD:2020-03'},
            ),
            (
                ['CLIENT:mtt-u16', 'MODE:stream', 'LANG:ru-RU', 'PERIOD:2020-05'],
                {'CLIENT:mtt-u16', 'MODE:stream', 'PERIOD:2020-05'},
            ),
            (
                ['IMPORT:yandex-call-center', 'PERIOD:2020-03'],
                {'IMPORT:yandex-call-center', 'PERIOD:2020-03'},
            ),
            (
                ['CLIENT:mtt-u16', 'MODE:stream', 'PERIOD:2020-05', 'OTHER:foo'],
                {'CLIENT:mtt-u16', 'MODE:stream', 'PERIOD:2020-05'},
            ),
        ):
            self.assertEqual(filtered_tags, filter_tags_by_tag_conjunction(tags, tag_conjunctions))

        with self.assertRaises(ValueError):
            filter_tags_by_tag_conjunction(
                ['IMPORT:yandex-call-center', 'LANG:en-US'],
                [
                    {
                        RecordTagDataRequest(type=RecordTagType.CLIENT, value='mtt-u16', negation=False),
                        RecordTagDataRequest(type=RecordTagType.MODE, value='stream', negation=False),
                        RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-05', negation=False),
                    },
                    {
                        RecordTagDataRequest(type=RecordTagType.IMPORT, value='yandex-call-center', negation=False),
                        RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-03', negation=False),
                    },
                ],
            )

        with self.assertRaises(ValueError):
            filter_tags_by_tag_conjunction(
                ['CLIENT:mtt-u16', 'MODE:stream', 'PERIOD:2020-05'],
                [
                    {
                        RecordTagDataRequest(type=RecordTagType.CLIENT, value='mtt-u16', negation=False),
                        RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-05', negation=False),
                    },
                    {
                        RecordTagDataRequest(type=RecordTagType.CLIENT, value='mtt-u16', negation=False),
                        RecordTagDataRequest(type=RecordTagType.MODE, value='stream', negation=False),
                    },
                ],
            )

        with self.assertRaises(NotImplementedError):
            filter_tags_by_tag_conjunction(
                ['CLIENT:mtt-u16', 'MODE:stream', 'PERIOD:2020-05'],
                [
                    {
                        RecordTagDataRequest(type=RecordTagType.CLIENT, value='mtt-u16', negation=False),
                        RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-05', negation=True),
                    },
                ],
            )

        with self.assertRaises(NotImplementedError):
            filter_tags_by_tag_conjunction(
                ['CLIENT:mtt-u16', 'MODE:stream', 'PERIOD:2020-05'],
                [
                    {
                        RecordTagDataRequest(type=RecordTagType.CLIENT, value=None, negation=False),
                        RecordTagDataRequest(type=RecordTagType.PERIOD, value='2020-05', negation=False),
                    },
                ],
            )

    def test_prepare_tags_for_view(self):
        tags = [
            'LANG:en-US',
            'PERIOD:2020-03',
            'CLIENT:mtt-u16',
        ]

        self.assertEqual(
            [
                'CLIENT:mtt-u16',
                'LANG:en-US',
                'PERIOD:2020-03',
            ],
            prepare_tags_for_view(tags, False),
        )

        self.assertEqual(
            [
                'CLIENT-LANG-PERIOD:mtt-u16__en-US__2020-03',
            ],
            prepare_tags_for_view(tags, True),
        )

    def test_decompose_tag(self):
        self.assertEqual(
            [
                RecordTagData(type=RecordTagType.CLIENT, value='web-banker-ke3'),
                RecordTagData(type=RecordTagType.MODE, value='stream'),
                RecordTagData(type=RecordTagType.PERIOD, value='2020-08'),
            ],
            decompose_tag('CLIENT-MODE-PERIOD:web-banker-ke3__stream__2020-08')
        )

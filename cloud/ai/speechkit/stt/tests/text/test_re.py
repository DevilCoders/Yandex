import unittest

import cloud.ai.speechkit.stt.lib.text.re as re


class TestRE(unittest.TestCase):
    def test_expression(self):
        regexp = re.lang_to_regexp['kk-KK']
        self.assertEqual("re.compile('^[а-яёіғқңүұһәө ]*$')", str(regexp.match_re))
        self.assertEqual("re.compile('^[а-яёіғқңүұһәө ?]*$')", str(regexp.toloka_match_re))
        self.assertEqual("re.compile('[^а-яёіғқңүұһәө ]')", str(regexp.clean_re))
        self.assertEqual("re.compile('[^а-яёіғқңүұһәө ?]')", str(regexp.toloka_clean_re))

    def test_match(self):
        for text, toloka, expected_ok in (
            (' аы абқң  ё ', False, True),
            (' аы абҚң  ё ', False, False),
            (' аы абқң  1 ', False, False),
            (' аы абқң  z ', False, False),
            (' аы абқң  ? ', False, False),
            (' аы абқң  ? ', True, True),
        ):
            actual_ok, _ = re.match(text, 'kk-KK', toloka)
            self.assertEqual(expected_ok, actual_ok)

    def test_clean(self):
        for source_text, expected_cleaned_text, toloka in (
            ('  аЫ  абҚң  Ё ', 'аы абқң ё', False),
            ('  аЫ,  абҚң  Топор?  12 ё .', 'аы абқң топор ё', False),
            ('  аЫ,  абҚң  Топор?  12 ё .', 'аы абқң топор? ё', True),
        ):
            actual_cleaned_text, _ = re.clean(source_text, 'kk-KK', toloka)
            self.assertEqual(expected_cleaned_text, actual_cleaned_text)

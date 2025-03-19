import unittest

from cloud.ai.speechkit.stt.lib.data.model.dao import RecordTag


class TestTagSanitize(unittest.TestCase):
    def test_empty(self):
        with self.assertRaises(ValueError):
            RecordTag.sanitize_value("")

    def test_ru_letters(self):
        with self.assertRaises(ValueError):
            RecordTag.sanitize_value("ннн")

    def test_ru_letters_digits(self):
        with self.assertRaises(ValueError):
            RecordTag.sanitize_value("9нн9н999")

    def test_symbols(self):
        with self.assertRaises(ValueError):
            RecordTag.sanitize_value("*_*")

    def test_symbols_2(self):
        with self.assertRaises(ValueError):
            RecordTag.sanitize_value("%%$##@")

    def test_sanitize_capital(self):
        self.assertEqual("vvv", RecordTag.sanitize_value("VVv"))

    def test_sanitize_space(self):
        with self.assertRaises(ValueError):
            self.assertEqual("--", RecordTag.sanitize_value("  "))

    def test_sanitize_overall(self):
        self.assertEqual("vvv-_123-456---__end", RecordTag.sanitize_value("VVv-_123 456---__END"))

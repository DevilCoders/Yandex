import unittest
from cloud.ai.speechkit.stt.lib.eval.text.punctuation import PunctuationCleaner


class TestPunctuationCleaner(unittest.TestCase):
    def setUp(self):
        self.punctuation_cleaner = PunctuationCleaner()

    def test_simple(self):
        self.assertEqual(
            "Привет Андрей",
            self.punctuation_cleaner.transform("Привет, Андрей"),
        )

    def test_different_punct(self):
        self.assertEqual(
            "Привет Андрей Как дела у меня нормально",
            self.punctuation_cleaner.transform("Привет,,, Андрей?!! Как дела: -- у меня нормально."),
        )

    def test_strip(self):
        self.assertEqual(
            "Привет Андрей Как дела у меня нормально",
            self.punctuation_cleaner.transform("   Привет,,,   Андрей?!!   Как дела: --    у меня нормально."),
        )

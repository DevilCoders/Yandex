import unittest
from cloud.ai.speechkit.stt.lib.eval.text.lemmatizer import Lemmatizer


class TestLemmatizer(unittest.TestCase):
    def setUp(self):
        self.lemmatizer = Lemmatizer()

    def test_simple(self):
        self.assertEqual(
            "привет, андрей",
            self.lemmatizer.transform("Привет, Андрей"),
        )

    def test_long(self):
        self.assertEqual(
            "здравствуйте что вы делать сегодня день целый день на работа",
            self.lemmatizer.transform("здравствуйте что вы делали сегодня днем целый день на работе"),
        )

    def test_caps(self):
        self.assertEqual(
            "здравствуйте что вы делать сегодня день целый день на работа",
            self.lemmatizer.transform("здравствуйте ЧТО вы делали СЕГОДНЯ днем целый день на РАБОТЕ"),
        )

    def test_caps_with_punct(self):
        self.assertEqual(
            "здравствуйте, что вы делать сегодня день целый день на работе?",
            self.lemmatizer.transform("Здравствуйте, ЧТО вы делали СЕГОДНЯ днем целый день на РАБОТЕ?"),
        )

import unittest

from cloud.ai.speechkit.stt.lib.eval.metrics.calc import get_metric_mer_1_0
from cloud.ai.speechkit.stt.lib.eval.metrics.mer import MERData


class TestMERCalculation(unittest.TestCase):
    def setUp(self):
        self.MER = get_metric_mer_1_0()

    def test_equal_case_insensitive(self):
        self.assertEqual(
            MERData(
                score=0.17604,
            ),
            self.MER.get_metric_data(
                hyp='Мама мыла раму',
                ref='мама мыла Раму',
            ),
        )

    def test_insertion(self):
        self.assertEqual(
            MERData(
                score=0.34025,
            ),
            self.MER.get_metric_data(
                hyp='мама мыла красивую раму',
                ref='мама мыла раму',
            ),
        )

    def test_deletion(self):
        self.assertEqual(
            MERData(
                score=0.34025,
            ),
            self.MER.get_metric_data(
                hyp='мама мыла раму',
                ref='мама мыла красивую раму',
            ),
        )

    def test_substitution(self):
        self.assertEqual(
            MERData(
                score=0.34381,
            ),
            self.MER.get_metric_data(
                hyp='мама мыла красивую раму',
                ref='мама мыла ужасную раму',
            ),
        )

    def test_multiple_errors(self):
        self.assertEqual(
            MERData(
                score=0.46983,
            ),
            self.MER.get_metric_data(
                hyp='моя мама мыла ужасную раму',
                ref='мама мыла красивую раму',
            ),
        )

    def test_NO_errors(self):
        self.assertEqual(
            MERData(
                score=0.33358,
            ),
            self.MER.get_metric_data(
                hyp='мама не мыла раму',
                ref='мама мыла раму',
            ),
        )

    def test_YES_errors(self):
        self.assertEqual(
            MERData(
                score=0.46389,
            ),
            self.MER.get_metric_data(
                hyp='Нет, мама мыла не раму',
                ref='Да, мама мыла раму',
            ),
        )

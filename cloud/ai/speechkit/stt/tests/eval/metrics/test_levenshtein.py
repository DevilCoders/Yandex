import unittest

from cloud.ai.speechkit.stt.lib.eval.metrics.calc import get_metric_lev
from cloud.ai.speechkit.stt.lib.eval.metrics.levenshtein import LevenshteinData


class TestLevenshteinCalculation(unittest.TestCase):
    def setUp(self):
        self.LEV = get_metric_lev()

    def test_equal_case_insensitive(self):
        self.assertEqual(
            LevenshteinData(
                score=0.14286,
            ),
            self.LEV.get_metric_data(
                hyp='Мама мыла раму',
                ref='мама мыла Раму',
            ),
        )

    def test_insertion(self):
        self.assertEqual(
            LevenshteinData(
                score=0.3913,
            ),
            self.LEV.get_metric_data(
                hyp='мама мыла красивую раму',
                ref='мама мыла раму',
            ),
        )

    def test_deletion(self):
        self.assertEqual(
            LevenshteinData(
                score=0.3913,
            ),
            self.LEV.get_metric_data(
                hyp='мама мыла раму',
                ref='мама мыла красивую раму',
            ),
        )

    def test_substitution(self):
        self.assertEqual(
            LevenshteinData(
                score=0.17391,
            ),
            self.LEV.get_metric_data(
                hyp='мама мыла красивую раму',
                ref='мама мыла ужасную раму',
            ),
        )

    def test_multiple_errors(self):
        self.assertEqual(
            LevenshteinData(
                score=0.30769,
            ),
            self.LEV.get_metric_data(
                hyp='моя мама мыла ужасную раму',
                ref='мама мыла красивую раму',
            ),
        )

    def test_NO_errors(self):
        self.assertEqual(
            LevenshteinData(
                score=0.17647,
            ),
            self.LEV.get_metric_data(
                hyp='мама не мыла раму',
                ref='мама мыла раму',
            ),
        )

    def test_YES_errors(self):
        self.assertEqual(
            LevenshteinData(
                score=0.27273,
            ),
            self.LEV.get_metric_data(
                hyp='Нет, мама мыла не раму',
                ref='Да, мама мыла раму',
            ),
        )

    def test_empty(self):
        self.assertEqual(
            LevenshteinData(
                score=0.0,
            ),
            self.LEV.get_metric_data(
                hyp='',
                ref='',
            ),
        )

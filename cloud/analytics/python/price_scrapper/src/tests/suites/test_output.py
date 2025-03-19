from unittest import TestCase
from pandas import read_excel, DataFrame

from scripts.main import run_compute_scrapping
from pandas.util.testing import assert_frame_equal


class TestScrapperOutput(TestCase):

    def test_compute(self):
        expected_compute_table = read_excel("data/Expected compute tariffs.xlsx")
        expected_compute_table = expected_compute_table.set_index(expected_compute_table["Unnamed: 0"])
        del expected_compute_table['Unnamed: 0']
        expected_compute_table.index.name = None
        actual_table = run_compute_scrapping()
        # expected_compute_table.to_excel("expected_table.xlsx")
        # actual_table.to_excel("actual table.xlsx")
        assert_frame_equal(expected_compute_table, actual_table, atol=0.005)

    def test_two_dfs(self):
        expected_frame = self._get_sample_frame()
        actual_frame = expected_frame.copy()
        assert_frame_equal(expected_frame, actual_frame, atol=0.02)

    def test_frames_not_eq(self):
        expected_frame = self._get_sample_frame()
        actual_frame = expected_frame.copy()
        actual_frame.loc["IBM"] = [1000, 101]
        with self.assertRaises(AssertionError):
            assert_frame_equal(expected_frame, actual_frame, atol=0.02)

    def _get_sample_frame(self) -> DataFrame:
        df = DataFrame(columns=["Revenue", "Net income"],
                       index=["IBM", "MSFT", "INTC"])
        df.loc["IBM"] = [1001.2, 100.5]
        df.loc["MSFT"] = [1200.3, 120.6]
        df.loc["INTC"] = [800.4, 200.7]
        return df

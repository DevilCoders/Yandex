import os

import experiment_pool.pool_helpers as pool_helpers
import yaqutils.test_helpers as utest
from reports.gp_metric_scores import calculate_scores_main


def get_file_path(filename, data_path):
    dirname = os.path.join(data_path, "gp_metric_scores/")
    return os.path.join(dirname, filename)


def try_scores(pool_name, expected_tsv_name, expected_json_name, tmpdir, data_path):
    pool_path = get_file_path(pool_name, data_path)
    expected_tsv_path = get_file_path(expected_tsv_name, data_path)
    expected_json_path = get_file_path(expected_json_name, data_path)

    tmp_output_tsv = str(tmpdir.join("tmp.tsv"))
    tmp_output_json = str(tmpdir.join("tmp.json"))

    pool = pool_helpers.load_pool(pool_path)
    calculate_scores_main(pool, tmp_output_tsv, tmp_output_json)

    # TODO: more flexible check (it's ok to change float precision a bit, for example)
    utest.fuzzy_diff_tsv_files(expected_file=expected_tsv_path, test_file=tmp_output_tsv, precision=1e-8)
    utest.fuzzy_diff_json(expected_file=expected_json_path, test_file=tmp_output_json)


# noinspection PyClassHasNoInit
class TestGPMetricScores:
    def test_excel_pool(self, tmpdir, data_path):
        try_scores("pool.json", "gp-output.tsv", "gp-output.json", tmpdir, data_path)

    def test_new_pool(self, tmpdir, data_path):
        try_scores("single_obs_pool.json", "single-output.tsv", "single-output.json", tmpdir, data_path)

    def test_good_pool(self, tmpdir, data_path):
        try_scores("single_obs_pool_good.json", "single-output-good.tsv", "single-output-good.json", tmpdir, data_path)

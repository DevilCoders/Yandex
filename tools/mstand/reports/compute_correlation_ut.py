import os

import reports.compute_correlation as comp_corr
from correlations import CorrelationPearson
from experiment_pool import pool_helpers as phelp
from reports import CorrCalcContext, CorrOutContext, CriteriaPair
from user_plugins import PluginKey


def run_calc_correlation(correlation, pool_name, criteria_left, criteria_right, tmpdir, adminka_session):
    pool = phelp.load_pool(pool_name)

    criteria_pair = CriteriaPair(criteria_left=criteria_left, criteria_right=criteria_right)

    main_metric = PluginKey("ATv2")

    out_file = str(tmpdir.join("corr-test-out.json"))
    out_file_tsv = str(tmpdir.join("corr-test-out.tsv"))

    out_dir = str(tmpdir.join("corr-out-dir"))

    os.mkdir(out_dir)

    corr_out_ctx = CorrOutContext(output_file=out_file,
                                  output_tsv=out_file_tsv,
                                  save_to_dir=out_dir)

    corr_calc_ctx = CorrCalcContext(criteria_pair=criteria_pair,
                                    main_metric_key=main_metric,
                                    min_pvalue=0.0001)

    comp_corr.calc_correlation_main(correlation=correlation,
                                    pool=pool,
                                    corr_calc_ctx=corr_calc_ctx,
                                    corr_out_ctx=corr_out_ctx,
                                    adminka_session=adminka_session)

    corr_out_ctx_all = CorrOutContext(output_file=out_file,
                                      output_tsv=None,
                                      save_to_dir=None)

    comp_corr.calc_correlation_main(correlation=correlation,
                                    pool=pool,
                                    corr_calc_ctx=corr_calc_ctx,
                                    corr_out_ctx=corr_out_ctx_all,
                                    adminka_session=adminka_session)


# noinspection PyClassHasNoInit
class TestCorrelation:
    def test_correlation_regular(self, tmpdir, session, data_path):
        pool_name = os.path.join(data_path, "corr_test_pool.json")

        correlation = CorrelationPearson()
        criteria_left = PluginKey(name="criterias.TTest")
        criteria_right = PluginKey(name="criterias.TTest")

        run_calc_correlation(correlation, pool_name, criteria_left, criteria_right, tmpdir, session)

    def test_correlation_full_auto_criteria(self, tmpdir, session, data_path):
        pool_name = os.path.join(data_path, "corr_test_pool.json")

        correlation = CorrelationPearson()
        criteria_left = None
        criteria_right = None

        run_calc_correlation(correlation, pool_name, criteria_left, criteria_right, tmpdir, session)

    def test_correlation_semi_auto_criteria(self, tmpdir, session, data_path):
        pool_name = os.path.join(data_path, "corr_test_pool.json")

        correlation = CorrelationPearson()
        criteria_left = None
        criteria_right = PluginKey(name="criterias.TTest")

        run_calc_correlation(correlation, pool_name, criteria_left, criteria_right, tmpdir, session)

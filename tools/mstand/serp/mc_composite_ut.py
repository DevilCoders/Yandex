import logging
import os
import pytest
import shutil

import serp.mc_composite as smc

from user_plugins import PluginContainer
from user_plugins import PluginSource
from user_plugins import PluginBatch

from serp import RawSerpDataStorage
from serp import ParsedSerpDataStorage
from serp import MetricDataStorage

from serp import SerpFetchParams
from serp import McCalcOptions

import yaqutils.file_helpers as ufile
import yaqutils.json_helpers as ujson
import yaqutils.misc_helpers as umisc


# noinspection PyClassHasNoInit
@pytest.mark.usefixtures("make_silent_run_command")
class TestMcComposite:
    def test_offline_mc_composite_main(self, tmpdir, data_path):
        # run composite block as Metrics does

        good_metric_one = PluginSource(module_name="sample_metrics.offline.offline_test_metrics",
                                       class_name="OfflineTestMetricRelSum",
                                       alias="McSimpleMetric")

        good_metric_two = PluginSource(module_name="sample_metrics.offline.offline_test_metrics",
                                       class_name="OfflineTestMetricDetailedWithDepth",
                                       alias="McDetailedMetric")

        bad_metric = PluginSource(module_name="sample_metrics.offline.offline_test_metrics",
                                  class_name="OfflineTestMetricWithError",
                                  alias="McBadMetric")

        null_metric = PluginSource(module_name="sample_metrics.offline.offline_test_metrics",
                                   class_name="OfflineTestMetricAlwaysNull",
                                   alias="McNullMetric")

        batch = PluginBatch(plugin_sources=[good_metric_one, good_metric_two, bad_metric, null_metric])

        metric_container = PluginContainer(plugin_batch=batch)

        # prepare serpset archive
        raw_serpset_dir = os.path.join(data_path, "composite")
        # It needs to run the test in arcadia only
        tmp_raw_serpset_dir = str(tmpdir.join("composite"))
        shutil.copytree(raw_serpset_dir, tmp_raw_serpset_dir)
        umisc.run_command(["chmod", "-R", "+w", tmp_raw_serpset_dir])

        mc_serpsets_tar = str(tmpdir.join("serpsets.tgz"))
        ufile.tar_directory(path_to_pack=tmp_raw_serpset_dir, tar_name=mc_serpsets_tar, dir_content_only=True)

        mc_output_file = str(tmpdir.join("metrics-external.tgz"))
        mc_broken_metrics_file = str(tmpdir.join("broken-metrics.json"))
        save_to_tar = str(tmpdir.join("metrics-internal.tgz"))
        pool_output_file = str(tmpdir.join("pool.json"))

        temp_root_dir = str(tmpdir)

        fetch_params = SerpFetchParams(use_external_convertor=False)

        raw_serp_storage = RawSerpDataStorage(root_dir=temp_root_dir, use_cache=True)
        parsed_serp_storage = ParsedSerpDataStorage(root_dir=temp_root_dir, use_cache=True)
        metric_storage = MetricDataStorage(root_dir=temp_root_dir, use_cache=True)

        # no internal out

        mc_calc_options = McCalcOptions(skip_metric_errors=True, collect_scale_stats=False,
                                        mc_alias_prefix="metric_prefix.", mc_error_alias_prefix="metric_error_prefix.",
                                        skip_failed_serps=True, allow_no_position=True, allow_broken_components=True,
                                        remove_raw_serpsets=False, mc_output_file=mc_output_file,
                                        mc_output_mr_table=None,
                                        mc_broken_metrics_file=mc_broken_metrics_file)
        mc_calc_options.use_internal_output = False

        smc.mc_composite_main(pool=None,
                              metric_container=metric_container,
                              fetch_params=fetch_params,
                              mc_serpsets_tar=mc_serpsets_tar,
                              threads=2,
                              raw_serp_storage=raw_serp_storage,
                              parsed_serp_storage=parsed_serp_storage,
                              metric_storage=metric_storage,
                              pool_output=pool_output_file,
                              mc_calc_options=mc_calc_options,
                              save_to_tar=save_to_tar)

        # with internal out
        mc_calc_options.use_internal_output = True
        smc.mc_composite_main(pool=None,
                              metric_container=metric_container,
                              fetch_params=fetch_params,
                              mc_serpsets_tar=mc_serpsets_tar,
                              threads=2,
                              raw_serp_storage=raw_serp_storage,
                              parsed_serp_storage=parsed_serp_storage,
                              metric_storage=metric_storage,
                              pool_output=pool_output_file,
                              mc_calc_options=mc_calc_options,
                              save_to_tar=save_to_tar)

        mc_results_dir = str(tmpdir.join("mc_results"))
        os.mkdir(mc_results_dir)
        ufile.untar_directory(tar_name=mc_output_file, dest_dir=mc_results_dir)
        res_one = os.path.join(mc_results_dir, "100500.json")
        res_two = os.path.join(mc_results_dir, "9001.json")

        logging.info("result serpset 1: %s", res_one)
        logging.info("result serpset 2: %s", res_two)
        assert ufile.is_file_or_link(res_one)
        assert ufile.is_file_or_link(res_two)

        mc_results_one = ujson.load_from_file(res_one)
        # 2 queries
        assert len(mc_results_one) == 2
        assert "metric_prefix.McSimpleMetric" in mc_results_one[1]
        mc_results_two = ujson.load_from_file(res_two)

        # 1 good query, 1 failed query
        assert len(mc_results_two) == 2
        assert mc_results_two[0]["metric_prefix.McDetailedMetric"] == 3300
        comps = mc_results_two[0]["components"]
        assert comps[0]["metric_prefix.McDetailedMetric"] == 10
        assert comps[1]["metric_prefix.McDetailedMetric"] == 100
        # serp_depth = 2 => no value at pos 3
        assert "metric_prefix.McDetailedMetric" not in comps[2]
        # no metrics
        assert list(mc_results_two[1].keys()) == ["query"]

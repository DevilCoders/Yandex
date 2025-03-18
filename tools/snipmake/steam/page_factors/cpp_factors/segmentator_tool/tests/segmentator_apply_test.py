# coding: utf-8

from yatest import common


def test_segmentator_apply():
    return common.canonical_execute(common.binary_path("tools/snipmake/steam/page_factors/cpp_factors/segmentator_tool/segmentator_tool"),
        [
            "-s", common.data_path("segmentator_tests_data/segmentator2015/tests_data/docs"),
            "-c", "utf-8", "-m", "apply",
        ], check_exit_code=False, timeout=200)

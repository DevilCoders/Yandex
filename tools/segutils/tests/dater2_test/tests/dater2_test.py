# coding: utf-8

from yatest import common


def test_dater2():
    with open(common.data_path("segmentator_tests_data/filelist.txt")) as stdin:
        return common.canonical_execute(common.binary_path("tools/segutils/tests/dater2_test/dater2_test"),
            [
                "-f", common.data_path("segmentator_tests_data/files"), "-h", common.source_path("yweb/common/roboconf/htparser.ini"),
                "-d", common.data_path("recognize/dict.dict"), "-l", common.source_path("yweb/urlrules/2ld.list"),
                "-i", "01.07.2009", "-M", "adar",
            ], stdin=stdin,
            check_exit_code=False, timeout=220)

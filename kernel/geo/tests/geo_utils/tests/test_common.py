# coding: utf-8

from yatest import common


def get_testname(name):
    if "test_" == name[:5]:
        name = name[5:]      # cut test_-prefix
    if "Test" == name[4:-4]:
        name = name[:-4]     # cut Test-suffix
    return name

def get_testdata(fname):
    if not len(fname):
        return ""

    if "sbr://" == fname[:6]:
        return fname[6:]

    slash_idx = fname.rfind('/')
    if -1 != slash_idx:
        fname = fname[slash_idx + 1:]
    fname = fname.replace('.py', '.txt')
    return common.data_path("geo_utils/" + fname)

def do_test(testname, testdata_file = ""):
    # TODO(anyone@) we can do call this function without hardcoded values:
    # do_test(sys._getframe().f_code.co_name, sys._getframe().f_code.co_filename, 80),

    return common.canonical_execute(
        common.binary_path("kernel/geo/tests/geo_utils/geo_utils_test"),
        [
            "--" + get_testname(testname), get_testdata(testdata_file),
            "--incexc_regions", common.data_path("geo_utils/inc_exc_regions.txt"),
            "--geodata", "geodata5.bin",
            "--relev_reg", common.data_path("geo_utils/geodata/relev_regions.txt"),
            "--ipreg", common.data_path("geo/ipreg"),
        ],
        check_exit_code = False)

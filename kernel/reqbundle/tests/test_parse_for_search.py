import yatest.common
import sys


BINARY_PARSE_FOR_SEARCH = yatest.common.binary_path("kernel/reqbundle/tools/parse_for_search/parse_for_search")


def test_parse_for_search():
    inputPath = "./factor_requests_qbundles_100.tsv"
    outputPath = "./parsed_qbundles.txt"

    command = [BINARY_PARSE_FOR_SEARCH, "-i", inputPath]
    sys.stderr.write("Execute: {}\n".format(command))
    yatest.common.execute(
        command,
        stdout=open(outputPath, "w"))

    return [yatest.common.canonical_file(outputPath)]

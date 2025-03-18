# coding=utf-8

import yatest.common


def test_text_norm():
    command = [
        yatest.common.binary_path("tools/test_norm/test_norm"),
        "-g", yatest.common.data_path("wizard/synnorm/synnorm.gzt.bin"),
        "-s", yatest.common.data_path("wizard/language/stopword.lst"),
        "-l", yatest.common.data_path("wizard/language/langdiscr.lst"),
        "-m", yatest.common.data_path("wizard/language/morphfixlist.txt"),
    ]

    inp_path = yatest.common.source_path("tools/test_norm/tests/test_norm_reqs.txt")
    out_path = "test_norm_reqs.txt.out"
    with open(inp_path) as inp:
        with open(out_path, "w") as out:
            yatest.common.execute(command, stdin=inp, stdout=out)
    return yatest.common.canonical_file(out_path)


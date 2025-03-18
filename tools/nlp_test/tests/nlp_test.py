# -*- coding=utf-8 -*-
import os

import yatest.common


def test_defcfg():
    dir_path = yatest.common.data_path("nlp_test/html")
    canonical_files = []
    for fname in sorted(os.listdir(dir_path)):
        out_path = fname + "_defcfg.out"
        command = [
            yatest.common.binary_path("tools/nlp_test/nlp_test"),
            "-p", "html",
            "-o", out_path,
            os.path.join(dir_path, fname),
        ]
        yatest.common.execute(command)
        canonical_files.append(yatest.common.canonical_file(out_path))

    return canonical_files


def test_robotcfg():
    dir_path = yatest.common.data_path("nlp_test/html")
    canonical_files = []
    for fname in sorted(os.listdir(dir_path)):
        out_path = fname + "_robotcfg.out"
        command = [
            yatest.common.binary_path("tools/nlp_test/nlp_test"),
            "-p", "html",
            "-c", yatest.common.source_path("tools/nlp_test/tests/htparser.ini"),
            "-o", out_path,
            os.path.join(dir_path, fname),
        ]
        yatest.common.execute(command)
        canonical_files.append(yatest.common.canonical_file(out_path))

    return canonical_files


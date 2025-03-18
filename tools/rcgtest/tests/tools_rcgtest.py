# coding=utf-8
import os

import yatest.common


def run_rcgtest(args, inp_path):
    command = [
        yatest.common.binary_path("tools/rcgtest/tools_rcgtest"),
        yatest.common.data_path("recognize/dict.dict"),
    ]
    command.extend(args)

    out_path = inp_path + ".out"
    inp_path = yatest.common.data_path("recognize/parser_tests_data/" + inp_path)
    with open(inp_path) as inp, open(out_path, "w") as out:
        yatest.common.execute(command, stdin=inp, stdout=out)

    return yatest.common.canonical_file(out_path)


def test_example():
    return run_rcgtest(["-ap"], "example.html")


def test_twitter():
    return run_rcgtest(["-ap", "-y", "twitter.com"], "twitter.tur.html")


def test_youtube():
    return run_rcgtest(["-ap", "-y", "youtube.com"], "youtube.tur.html")


def test_generic():
    command = [
        yatest.common.binary_path("tools/rcgtest/tools_rcgtest"),
        "-dpaf",
        yatest.common.data_path("recognize/dict.dict"),
    ]

    test_dir = yatest.common.data_path("recognize/tests_data")

    tmp_inp = "tmp_inp.txt"
    with open(tmp_inp, "w") as tmp_out:
        for root, dirs, files in sorted(os.walk(test_dir)):
            for fname in sorted(files):
                inp_path = os.path.join(root, fname)
                print >>tmp_out, inp_path

    with open(tmp_inp) as inp:
        result = yatest.common.execute(command, stdin=inp)
    os.remove(tmp_inp)

    # prettify output results
    out_path = "out.txt"
    with open(out_path, "w") as out:
        for line in result.std_out.splitlines():
            ind = line.rfind("arcadia_tests_data")
            line = line[ind:]
            out.write(line + "\n")

    return yatest.common.canonical_file(out_path)




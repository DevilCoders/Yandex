# coding=utf-8

"""Тест printrichnode.
Передает на вход программе printrichnode тестовые данные из файла test_req.txt,
результат записывается в out_req.txt и сравнивается с каноническим результатом.

Тест утверждён Ольга Рябченкова (limburg@)

"""

import yatest.common
import os


def run_printrichnode(args=tuple(), fname="test_req.txt"):
    out_path = "".join(arg for arg in args if "morphfixlist" not in arg) + ".out"
    inp_path = yatest.common.source_path(os.path.join("tools/printrichnode/tests", fname))

    command = [
        yatest.common.binary_path("tools/printrichnode/printrichnode"),
        inp_path,
    ]
    command.extend(args)

    with open(out_path, "w") as out:
        yatest.common.execute(command, stdout=out)
    return yatest.common.canonical_file(out_path)


def test_default():
    return run_printrichnode([
        "-f", yatest.common.build_path("search/wizard/data/wizard/language/morphfixlist.txt"),
    ])


def test_s():
    return run_printrichnode([
        "-s",
        "-f", yatest.common.build_path("search/wizard/data/wizard/language/morphfixlist.txt"),
    ])


def test_su():
    return run_printrichnode([
        "-s",
        "-u",
        "-f", yatest.common.build_path("search/wizard/data/wizard/language/morphfixlist.txt"),
    ])


def test_wizard():
    return run_printrichnode([
        "--wizard",
    ])


def test_extented_syntax():
    return run_printrichnode([
        "-x",
        "-f", yatest.common.build_path("search/wizard/data/wizard/language/morphfixlist.txt"),
    ])


def test_strange():
    return run_printrichnode(fname="test_strange.txt")


def test_strange_wizard():
    return run_printrichnode([
        "--wizard",
    ], fname="test_strange.txt")

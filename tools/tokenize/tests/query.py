import yatest.common
import difflib
import pytest


def run_query(inp_path, args=tuple(), out_postprocess=yatest.common.canonical_file):
    out_path = inp_path + ".out"
    inp_path = yatest.common.source_path("tools/tokenize/tests/" + inp_path)

    command = [
        yatest.common.binary_path("tools/tokenize/tokenize"),
        "query",
        "--left-d",
        "[",
        "--right-d",
        "] ",
    ]
    command.extend(args)

    with open(out_path, "w") as out:
        yatest.common.execute(command, stdout=out, stdin=open(inp_path, "r"))
    return out_postprocess(out_path)


INPUT_FILES = ["manual.txt", "special_symbols.txt", "queries_sample.txt"]


def test_queries():
    return run_query("queries_sample.txt", ("--original", ))


def test_errors():
    return run_query("errors.txt", ("--not-stop", "--original", ))


def test_random():
    return run_query("random.txt", ("--original", ))


@pytest.mark.parametrize("input_file, version", [(input_file, version) for input_file in INPUT_FILES for version in [1, 2, 3, 4, 5]])
def test_query_tokenizer(input_file, version):
    return run_query(input_file, ("--not-stop", "--version", str(version), "--original", ))

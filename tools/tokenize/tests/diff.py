import yatest.common
import difflib

def run_diff(inp_path, args=tuple(), out_postprocess=yatest.common.canonical_file):
    out_path_query = inp_path + ".query.out"
    out_path_document = inp_path + ".document.out"
    inp_path = yatest.common.source_path("tools/tokenize/tests/" + inp_path)
    command_document = [
        yatest.common.binary_path("tools/tokenize/tokenize"),
        "document",
        "-i",
        inp_path,
        "-u",
    ]
    command_document.extend(args)

    with open(out_path_document, "w") as out:
        yatest.common.execute(command_document, stdout=out)

    command_query = [
        yatest.common.binary_path("tools/tokenize/tokenize"),
        "query",
        "-i",
        inp_path,
        "-u",
    ]
    command_query.extend(args)

    with open(out_path_query, "w") as out:
        yatest.common.execute(command_query, stdout=out)

    diff = difflib.ndiff(open(out_path_document).readlines(), open(out_path_query).readlines())
    return ' '.join(diff)


def test_no_diff():
    return run_diff("queries.txt")


def test_with_diff():
    return run_diff("queries_diff.txt")


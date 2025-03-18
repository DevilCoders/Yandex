import yatest.common
import difflib
import pytest


def run_document(inp_path, args=tuple(), out_postprocess=yatest.common.canonical_file):
    out_path = inp_path + ".out"
    inp_path = yatest.common.source_path("tools/tokenize/tests/" + inp_path)

    command = [
        yatest.common.binary_path("tools/tokenize/tokenize"),
        "document",
        "-i",
        inp_path,
        "--left-d",
        " \"",
        "--right-d",
        "\"",
    ]
    command.extend(args)

    with open(out_path, "w") as out:
        yatest.common.execute(command, stdout=out)
    return out_postprocess(out_path)


INPUT_FILES = ["in.txt", "abbreviations.txt", "chinese.txt", "japanese.txt", "korean.txt", "spain.txt", "other_languages.txt", "tables.txt", "manual.txt"]


@pytest.mark.parametrize("input_file, version", [(input_file, version) for input_file in INPUT_FILES for version in [2, 3]])
def test_document_tokenizer(input_file, version):
    return run_document(input_file, ["--version", str(version)])


def count_words(fname):
    return len([w for w in open(fname, 'r') if w.startswith('NLP_WORD')])


def test_special():
    return run_document("special_symbols.txt", [], count_words)

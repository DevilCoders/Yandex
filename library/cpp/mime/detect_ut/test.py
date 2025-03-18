import os

import yatest.common

binary = "tools/showmime/showmime"


def get_input_files(data_path):
    res = []
    for root, dirs, files in os.walk(data_path):
        for f in files:
            res.append(f)
    return sorted(res)


def do_test(data_path, mime):
    out_path = yatest.common.output_path(mime + ".out")
    data_path = os.path.join(os.getcwd(), data_path)
    with open(out_path, "w") as out:
        for input_file in get_input_files(data_path):
            result = yatest.common.execute(
                [yatest.common.binary_path(binary), "-e", os.path.join(data_path, input_file)],
                check_exit_code=False  # some tests are expected to fail
            )
            print >> out, "%s, no hint: code=%s, output=%s" % (input_file, result.returncode, result.std_out.strip())
            result = yatest.common.execute(
                [yatest.common.binary_path(binary), "-e", "-m", mime, os.path.join(data_path, input_file)]
            )
            print >> out, "%s, hint=%s: code=%s, output=%s" % (input_file, mime, result.returncode, result.std_out.strip())
    return yatest.common.canonical_file(out_path)


def test_all():
    formats = [
        "text",
        "html",
        "pdf",
        "rtf",
        "doc",
        "xml",
        "swf",
        "xls",
        "ppt",
        "jpg",
        "png",
        "gif",
        "docx",
        "odt",
        "odp",
        "ods",
        "xmlhtml",
        "bmp",
        "wav",
        "archive",
        "exe",
        "odg",
        "gzip",
        "xlsx",
        "pptx",
        "js",
        "epub",
        "json",
        "apk",
        "webp",
        "djvu",
        "chm",
        "tex",
        "tiff",
        "pnm",
        "svg",
        "ico",
        ]
    result = {}
    for mime in formats:
        result[mime] = do_test("mime_test_files/" + mime, mime)
    return result

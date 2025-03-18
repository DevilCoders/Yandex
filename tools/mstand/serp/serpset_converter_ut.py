import gzip
import os
import re
import subprocess
import logging

import yaqutils.test_helpers as utest
import yaqutils.file_helpers as ufile

import serp.serpset_converter as ss_conv


def is_gunzip_old():
    # return False by default to not xfail the test if version extracting
    # will go wrong
    try:
        output = subprocess.check_output(['gunzip', '--version'])
    except subprocess.CalledProcessError:
        return False

    version_re = re.compile(r'gunzip \(gzip\) ([\d.]+)')

    match = version_re.match(str(output))
    if not match:
        return False

    version_parts = match.group(1).split('.')

    try:
        version = [int(p) for p in version_parts]
    except ValueError:
        return False

    return version < [1, 5]


# noinspection PyClassHasNoInit
class TestConvertSerpset:
    def test_convert_serpset_unpacked(self, tmpdir, data_path):
        serpset_id = "100500"
        src_file = os.path.join(data_path, "fetcher/test-serpset.json")
        dst_file = str(tmpdir.join("serpset-converted.jsonl"))
        ss_conv.convert_serpset_to_jsonlines(serpset_id, raw_file=src_file, jsonlines_file=dst_file,
                                             do_unpack=False, sort_keys=True, remove_original=False,
                                             use_external_convertor=False)

        expected_file = os.path.join(data_path, "fetcher/serpset-converted-unpacked.jsonl")
        utest.diff_files(test_file=dst_file, expected_file=expected_file)

    def test_convert_serpset_packed(self, tmpdir, data_path):
        if is_gunzip_old():
            logging.warning("too old system gunzip, cannot run test 'test_convert_serpset_packed'")
            return

        serpset_id = "100500"
        src_file = os.path.join(data_path, "fetcher/test-serpset.json")
        src_file_gz = str(tmpdir.join("test-serpset.json.gz"))

        with gzip.open(src_file_gz, "wb") as gfd:
            serpset_content = ufile.read_text_file(src_file)
            gfd.write(serpset_content.encode("utf-8"))

        dst_file = str(tmpdir.join("serpset-converted.jsonl"))
        ss_conv.convert_serpset_to_jsonlines(serpset_id, raw_file=src_file_gz, jsonlines_file=dst_file,
                                             do_unpack=True, sort_keys=False, use_external_convertor=False, remove_original=False)

        expected_file = os.path.join(data_path, "fetcher/serpset-converted.jsonl")
        utest.diff_files(test_file=dst_file, expected_file=expected_file)

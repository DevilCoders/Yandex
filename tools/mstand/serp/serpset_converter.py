import logging
import os
import time
from typing import Optional

import yaqutils.misc_helpers as umisc
import yaqutils.file_helpers as ufile
import yaqutils.json_helpers as ujson


def convert_serpset_to_jsonlines(serpset_id: Optional[str], raw_file: str, jsonlines_file: str, do_unpack: bool = False,
                                 sort_keys: bool = False, use_external_convertor: bool = False, remove_original: bool = False):
    assert raw_file
    assert jsonlines_file

    start_time = time.time()
    logging.info("Converting serpset %s to jsonlines (%s to %s)", serpset_id, raw_file, jsonlines_file)

    assert raw_file != jsonlines_file, "convert_serpset_to_jsonlines: src file is same as dst file"

    # cleanup destination
    ufile.remove_if_exists(jsonlines_file)

    if do_unpack:
        unpacked_file = unpack_raw_serp_file(raw_file)
    else:
        unpacked_file = raw_file

    if unpacked_file == jsonlines_file:
        logging.info("Seems that unpacked file %s already have jsonlines format, skipping conversion", unpacked_file)
    else:
        if use_external_convertor:
            convert_json_file_to_jsonlines_external(json_file=unpacked_file, jsonlines_file=jsonlines_file)
        else:
            convert_json_file_to_jsonlines_internal(json_file=unpacked_file, jsonlines_file=jsonlines_file,
                                                    sort_keys=sort_keys)

        if do_unpack:
            logging.info("removing unpacked file %s", unpacked_file)
            os.unlink(unpacked_file)

    if remove_original:
        logging.info("removing original raw serpset %s after conversion", raw_file)
        os.unlink(raw_file)

    umisc.log_elapsed(start_time, "Serpset %s conversion done", serpset_id)


def unpack_raw_serp_file(raw_file):
    logging.info("Unpacking %s", raw_file)
    gz_suffix = ".gz"
    assert raw_file.endswith(gz_suffix)
    umisc.run_command(["gunzip", "--keep", "--force", raw_file])
    unpacked_file = raw_file[:-len(gz_suffix)]
    assert unpacked_file
    return unpacked_file


def convert_json_file_to_jsonlines_internal(json_file: str, jsonlines_file: str, sort_keys: bool = False):
    # this method is left for unit tests only.
    # production should use fast and low-memory external C++ convertor from <arcadia>/tools/json_to_jsonlines
    assert json_file != jsonlines_file, "convert_json_file_to_jsonlines_internal: src file is same as dst file"
    logging.info("Converting file %s to %s [internal method]", json_file, jsonlines_file)

    temp_jsonlines_file = jsonlines_file + ".tmp"
    ujson.json_to_jsonlines(input_file=json_file, output_file=temp_jsonlines_file, sort_keys=sort_keys)
    os.rename(temp_jsonlines_file, jsonlines_file)


def convert_json_file_to_jsonlines_external(json_file: str, jsonlines_file: str):
    assert json_file != jsonlines_file, "convert_json_file_to_jsonlines_external: src file is same as dst file"
    logging.info("Converting file %s to %s [external method]", json_file, jsonlines_file)

    temp_jsonlines_file = jsonlines_file + ".tmp"
    conv_binary_path = "./json_to_jsonlines"
    if not ufile.is_file_or_link(conv_binary_path):
        conv_hint = "Please build '<arcadia>/tools/json_to_jsonlines' target and make symlink of binary to mstand's root folder."
        raise Exception("Convertor json_to_jsonlines not found. {}".format(conv_hint))

    convert_cmd = [conv_binary_path, "-i", json_file, "-o", temp_jsonlines_file]
    umisc.run_command(convert_cmd)
    os.rename(temp_jsonlines_file, jsonlines_file)

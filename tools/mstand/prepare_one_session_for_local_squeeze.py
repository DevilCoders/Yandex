#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

import argparse
import os
import shutil

import yaqutils.file_helpers as ufile
import yaqutils.json_helpers as ujson
import yaqutils.time_helpers as utime

LOCAL_FOLDER = "sample_sessions"


def parse_args():
    parser = argparse.ArgumentParser(description="Prepares file system structure for squeezing local user-sessions")
    parser.add_argument(
        "-s",
        "--user-session",
        required=True,
        help="path to file with user_session's records",
    )
    parser.add_argument(
        "-j",
        "--json",
        action="store_true",
        help="Do not convert session from tsv (yamr) to json (yt) format. Default: tsv (yamr) format",
    )
    parser.add_argument(
        "-o",
        "--overwrite",
        action="store_true",
        help="Overwrite results or print error, if results already exists (default)",
    )
    return parser.parse_args()


def convert_tsv_to_json(input_file, output_file):
    with ufile.fopen_read(input_file, use_unicode=False) as inp:
        with ufile.fopen_write(output_file, use_unicode=False) as out:
            for line in inp:
                parts = parse_tsv_line(line)
                data = {
                    "key": parts[0],
                    "subkey": parts[1],
                    "value": parts[2]
                }
                ujson.dump_to_fd(data, out)
                out.write("\n")


def create_pool(path, date):
    """
    :type path: str
    :type date: str
    :return:
    """
    pool = {
        "version": 1,
        "observations": [
            {
                "observation_id": None,
                "control": {"testid": "all"},
                "date_from": date,
                "date_to": date
            }
        ]
    }

    ujson.dump_to_file(pool, path)


def parse_tsv_line(line):
    line = line.rstrip()
    if "\t" not in line:
        raise Exception("No tab character found in source file line. Seems that you forget '-j' option. ")
    tokens = line.split("\t", 2)
    if len(tokens) != 3:
        raise Exception("TSV should have exactly 3 columns: key, subkey, value.")
    return tokens


def main():
    cli_args = parse_args()

    with ufile.fopen_read(cli_args.user_session) as session_fd:
        line = session_fd.readline().rstrip()

    if cli_args.json:
        timestamp = int(ujson.load_from_str(line)["subkey"])
    else:
        tokens = parse_tsv_line(line)
        timestamp = int(tokens[1])

    session_date = utime.format_date(utime.timestamp_to_datetime_msk(timestamp), pretty=True)

    day_folder = os.path.join(LOCAL_FOLDER, "search", "daily", session_date)

    if os.path.isdir(day_folder):
        if cli_args.overwrite:
            shutil.rmtree(day_folder)
            os.makedirs(day_folder)
        else:
            raise Exception("Error: folder {} already exists".format(day_folder))
    else:
        ufile.make_dirs(day_folder)

    dest_dir = os.path.join(day_folder, "clean")
    if cli_args.json:
        shutil.copyfile(cli_args.user_session, dest_dir)
    else:
        convert_tsv_to_json(cli_args.user_session, dest_dir)

    squeeze_date = utime.format_date(utime.timestamp_to_datetime_msk(timestamp))

    pool_path = os.path.join(day_folder, "pool.json")
    create_pool(pool_path, squeeze_date)

    squeeze_params = "-i {pool} --squeeze-path {base_folder} --sessions-path {base_folder}".format(
        folder=day_folder, base_folder=LOCAL_FOLDER, pool=pool_path)
    calc_params = "-i {pool} --squeeze-path {base_folder} -c SPU -m metrics.online".format(
        folder=day_folder, base_folder=LOCAL_FOLDER, pool=pool_path)

    print """Done.

Squeeze local user_session:
./session_squeeze_local.py {squeeze_params} --services <services>

Calc metric on local squeeze:
./session_calc_metric_local.py {calc_params} --services <services>
    """.format(squeeze_params=squeeze_params, calc_params=calc_params)


if __name__ == "__main__":
    main()

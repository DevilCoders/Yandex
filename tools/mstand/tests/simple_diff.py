#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

import argparse
import logging
import subprocess
import yaqutils.misc_helpers as umisc


def main():
    umisc.configure_logger(verbose=0)
    parser = argparse.ArgumentParser(description="Unix diff wrapper")
    parser.add_argument(
        "--orig",
        help="path to the original data"
    )

    parser.add_argument(
        "--test",
        help="path to the test data"
    )

    cli_args = parser.parse_args()

    command = [
        "diff",
        "-ub",
        cli_args.orig,
        cli_args.test,
    ]

    diff_process = subprocess.Popen(command, stdout=subprocess.PIPE)

    lines = ""
    for line in iter(diff_process.stdout.readline, ""):
        lines += line
    if lines != "":
        logging.error(lines)

    diff_process.wait()

    ret_code = diff_process.returncode
    if ret_code != 0:
        raise Exception("Files are diferent. Retcode {}.".format(ret_code))

if __name__ == "__main__":
    main()

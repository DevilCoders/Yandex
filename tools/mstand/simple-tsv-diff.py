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

    delim = "\t"
    #delim = " "

    orig_f = open(cli_args.orig, "rt")
    test_f = open(cli_args.test, "rt")

    orig_line = next(orig_f, None)
    test_line = next(test_f, None)
    while bool(orig_line and test_line):
        same_lines = (orig_line == test_line)
        if same_lines:
            orig_line = next(orig_f, None)
            test_line = next(test_f, None)
        else:
            orig = orig_line.strip().split(delim)
            test = test_line.strip().split(delim)

            orig_iter = iter(orig)
            test_iter = iter(test)

            orig_token = next(orig_iter, None)
            test_token = next(test_iter, None)
            while bool(orig_token and test_token):
                if orig_token == test_token:
                    pass
                else:
                    raise Exception("Files are diferent. Test {} Orig {}.".format(orig_token, test_token))
                orig_token = next(orig_iter, None)
                test_token = next(test_iter, None)



                # raise Exception("Files are diferent. Retcode {}.".format(ret_code))

if __name__ == "__main__":
    main()

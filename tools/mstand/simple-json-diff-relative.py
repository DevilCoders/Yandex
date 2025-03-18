#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import logging
import math
import numbers

import yaqutils.args_helpers as uargs
import yaqutils.json_helpers as ujson
import yaqutils.misc_helpers as umisc


def is_zero(test, zero_abs_threshold):
    return math.fabs(test) <= zero_abs_threshold


def are_float_relative_differs(orig, test, rel_abs_threshold, zero_abs_threshold):
    if is_zero(orig, zero_abs_threshold):
        return not is_zero(test, zero_abs_threshold)
    else:
        rel_diff = (orig - test) / orig
        return math.fabs(rel_diff) > rel_abs_threshold


def cmp_json_list(orig, test, rel_threshold, zero_abs_threshold, blacklist):
    if len(orig) != len(test):
        if len(orig) > len(test):
            logging.error("Additional orig elemenst:")
            logging.error(orig[len(test) :])
        else:
            logging.error("Additional test elemenst:")
            logging.error(test[len(orig):])
        raise Exception(
            "Jsons are different: orig node has {} elements but test node has {} elements".format(
                len(orig), len(test)))
    else:
        for orig_item, test_item in zip(orig, test):
            cmp_json(orig_item, test_item, rel_threshold, zero_abs_threshold, blacklist)


def cmp_json_dict(left, right, rel_threshold, zero_abs_threshold, blacklist):
    left_keys = set(left.keys())
    right_keys = set(right.keys())

    if left_keys != right_keys:
        raise Exception(
            "Jsons are different: left node has\n{}\nkeys, but right node has\n{}\nkeys".format(
                sorted(left_keys), sorted(right_keys)))
    else:
        for key in left_keys:
            if key not in blacklist:
                cmp_json(left[key], right[key], rel_threshold, zero_abs_threshold, blacklist)


def is_float_node(node):
    return isinstance(node, numbers.Real)


def cmp_float_nodes(original, test, rel_threshold, zero_abs_threshold):
    floats_differs = are_float_relative_differs(original, test, rel_threshold, zero_abs_threshold)
    if floats_differs:
        raise Exception(
            "Jsons are different:\nleft node\n{}\nright node\n{}\n".format(
                original, test))


def cmp_json(original, test, rel_threshold, zero_abs_threshold, blacklist):
    if type(original) != type(test):
        raise Exception("Jsons are different: original node type is {} but test node is {}".format(type(original), type(test)))
    if isinstance(original, list):
        return cmp_json_list(original, test, rel_threshold, zero_abs_threshold, blacklist)
    elif isinstance(original, dict):
        return cmp_json_dict(original, test, rel_threshold, zero_abs_threshold, blacklist)
    else:
        if is_float_node(original):
            cmp_float_nodes(original, test, rel_threshold, zero_abs_threshold)
        elif original != test:

            raise Exception(
                "Jsons are different:\noriginal node\n{}\ntest node\n{}\n".format(
                    original, test))


def parse_args():
    parser = argparse.ArgumentParser(description="Unix diff wrapper")
    uargs.add_verbosity(parser)
    parser.add_argument(
        "--orig",
        help="Path to the original data"
    )
    parser.add_argument(
        "--test",
        help="Path to the test data"
    )
    parser.add_argument(
        "--rel-threshold",
        help="Relative threshold for float comparsion.",
        type=float,
        default=1.e-3,
    )
    parser.add_argument(
        "--zero-abs-threshold",
        help="Absolute threshold for float to be treated as zero.",
        type=float,
        default=1.e-7,
    )
    parser.add_argument(
        "--blacklist",
        nargs="+",
        help="List of keys to ignore.",
    )
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    orig = ujson.load_from_file(cli_args.orig)
    test = ujson.load_from_file(cli_args.test)

    if orig != test:
        cmp_json(orig, test, cli_args.rel_threshold, cli_args.zero_abs_threshold, cli_args.blacklist or [])


if __name__ == "__main__":
    main()

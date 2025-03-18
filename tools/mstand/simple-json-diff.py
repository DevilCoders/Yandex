#!/usr/bin/env python3

import argparse
import logging
import yaqutils.misc_helpers as umisc
import json
import numbers
import math


def cmp_json_list(orig, test, abs_threshold):
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
            cmp_json(orig_item, test_item, abs_threshold)


def cmp_json_dict(left, right, abs_threshold):
    left_keys = set(left.keys())
    right_keys = set(right.keys())

    if left_keys != right_keys:
        raise Exception(
            "Jsons are different: left node has\n{}\nkeys, but right node has\n{}\nkeys".format(
                sorted(left_keys), sorted(right_keys)))
    else:
        for key in left_keys:
            cmp_json(left[key], right[key], abs_threshold)


def is_float_node(node):
    return isinstance(node, numbers.Real)


def cmp_float_nodes(left, right, abs_threshold):
    diff = math.fabs(left - right)
    is_ok = diff < abs_threshold
    if not is_ok:
        raise Exception(
            "Jsons are different:\nleft node\n{}\nright node\n{}\n".format(
                left, right))


def cmp_json(left, right, abs_threshold):
    if type(left) != type(right):
        raise Exception("Jsons are different: left node type is {} but right node is {}".format(type(left), type(right)))
    if type(left) == type([]):
        return cmp_json_list(left, right, abs_threshold)
    elif type(left) == type({}):
        return cmp_json_dict(left, right, abs_threshold)
    else:
        if is_float_node(left):
            cmp_float_nodes(left, right, abs_threshold)
        elif left != right:

            raise Exception(
                "Jsons are different:\nleft node\n{}\nright node\n{}\n".format(
                    left, right))


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

    parser.add_argument(
        "--abs-threshold",
        help="absolute threshold for float comparsion.",
        type=float,
        default=1.e-7,
    )

    cli_args = parser.parse_args()

    orig = json.load(file(cli_args.orig, "rt"))
    test = json.load(file(cli_args.test, "rt"))
    if orig != test:
        cmp_json(orig, test, cli_args.abs_threshold)

if __name__ == "__main__":
    main()

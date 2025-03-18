#!/usr/bin/env python3
import logging
import sys

import yaqutils.json_helpers as ujson
import yaqutils.misc_helpers as umisc


class StatComparator(object):
    def __init__(self, threshold=1.0):
        self.threshold = threshold

    def compare(self, left, right):
        return self._compare_structures(left, right)

    def _compare_values(self, left, right, key):
        if isinstance(left, int) and isinstance(right, int):
            diff = right - left
            if diff == 0:
                return True
            if left != 0:
                rel_diff = 100.0 * float(diff) / float(left)
                if abs(rel_diff) > self.threshold:
                    logging.info("%s: %s vs %s, rel diff: %.4f%%".format(key, left, right, rel_diff))
            else:
                logging.info("%s: %s vs %s, abs diff: %s".format(key, left, right, diff))
            return True
        return False  # not processed

    def _compare_structures(self, left, right, parent_key=""):
        if isinstance(left, dict) and isinstance(right, dict):
            for key in left:
                if key not in right:
                    logging.warning("%s: >>>> No key '%s' on the right side".format(parent_key, key))
                    continue
                left_value = left[key]
                right_value = right[key]

                rec_key = "{}.{}".format(parent_key, key)
                if not self._compare_values(left_value, right_value, rec_key):
                    self._compare_structures(left_value, right_value, rec_key)
        elif isinstance(left, list) and isinstance(right, list):
            for index, left_value in enumerate(left):
                if index > len(right) - 1:
                    logging.warning("%s: >>>> No index '%s' on the right side".format(parent_key, index))
                    continue
                right_value = right[index]

                rec_key = "{}.[{}]".format(parent_key, index)
                if not self._compare_values(left_value, right_value, rec_key):
                    self._compare_structures(left_value, right_value, rec_key)
        else:
            raise RuntimeError("Unsupported types for compare. Left: {}, Right: {}", type(left), type(right))


def main():
    umisc.configure_logger()
    left_input = sys.argv[1]
    right_input = sys.argv[2]

    threshold = float(sys.argv[3]) if len(sys.argv) > 3 else 1.0

    sc = StatComparator(threshold)

    left = ujson.load_from_file(left_input)
    right = ujson.load_from_file(right_input)
    sc.compare(left, right)


if __name__ == "__main__":
    main()

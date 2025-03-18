#!/usr/bin/env python3

import csv
import json
import os
import time

# noinspection PyUnresolvedReferences,PyPackageRequirements
import ujson
import simplejson


def read_csv(path, func):
    with open(path, "r", buffering=80 * 1024 * 1024) as fd:
        reader = csv.reader(fd, delimiter='\t')
        for row in reader:
            yield [func(x) for x in row]


def try_except(s):
    try:
        return int(s)
    except ValueError:
        try:
            return float(s)
        except ValueError:
            return json.loads(s)


NUMBERS_FILE_PATH = "json_numbers_read_test_numbers.txt"


def measure(func, name):
    print("{} started".format(name))
    time_start = time.time()
    for _ in read_csv(NUMBERS_FILE_PATH, func):
        pass
    time_end = time.time()
    print("{} ended in {}".format(name, time_end - time_start))


def prepare():
    if not os.path.isfile(NUMBERS_FILE_PATH):
        print("generating")
        with open(NUMBERS_FILE_PATH, "w") as fd:
            for x in range(50000000):
                fd.write("{}\t{}\n".format(float(x) / 3.0, x))
        print("done")


def main():
    prepare()
    measure(float, "float")
    measure(ujson.loads, "ujson")
    measure(simplejson.loads, "simplejson")
    measure(try_except, "try_except")
    measure(json.loads, "json.loads")


if __name__ == "__main__":
    main()

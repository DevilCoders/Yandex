#!/usr/bin/env python3

import csv
import itertools
import os
import time

import numpy
import pandas


def read_numpy(path):
    return numpy.fromfile(path, numpy.float, sep=' ')


def read_pandas(path):
    return pandas.read_csv(path, dtype=numpy.float, sep='\t', header=None, na_filter=False)


def iter_csv(path):
    with open(path, "r", buffering=80 * 1024 * 1024) as fd:
        reader = csv.reader(fd, delimiter='\t')
        for row in reader:
            yield [float(x) for x in row]


def read_iter(path):
    return list(iter_csv(path))


def read_numpy_fromiter(path):
    return numpy.fromiter(itertools.chain.from_iterable(iter_csv(path)), numpy.float)


def parse_line(line):
    return [float(x) for x in line.split("\t")]


def read_pandas_convert(path):
    return pandas.read_csv(path, sep="\n", dtype=numpy.object, header=None, squeeze=True, converters={0: parse_line})


def read_iter_array(path):
    return numpy.array(list(iter_csv(path)), dtype=numpy.object)


NUMBERS_FILE_PATH = "array_read_test_numbers.txt"


def measure(func):
    start = time.time()
    print("started with ", func)
    arr = func(NUMBERS_FILE_PATH)
    print(len(arr))
    print(func, " ended in ", time.time() - start)


def prepare():
    if not os.path.isfile(NUMBERS_FILE_PATH):
        print("generating")
        with open(NUMBERS_FILE_PATH, "w") as fd:
            for x in range(50000000):
                fd.write("{}\t{}\n".format(float(x) / 3.0, x))
        print("done")


prepare()
measure(read_pandas)
measure(read_numpy)
measure(read_numpy_fromiter)
measure(read_iter)
measure(read_iter_array)
measure(read_pandas_convert)

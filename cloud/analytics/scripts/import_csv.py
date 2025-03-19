#!/usr/bin/env python2
#
# We need to use python2 because of yandex-yt-yson-bindings on Mac OS X

from __future__ import unicode_literals
from backports import csv
import sys
import io
import logging

logging.basicConfig(level=logging.INFO)

import yt.wrapper as yt

DST_CLUSTER = 'hume'

INFILE = sys.argv[1]
TABLE_NAME = sys.argv[2]
YT_TO_PYTHON_TYPES = {
    'string': unicode,
    'double': float,
    'uint64': long,
    'boolean': bool,
    'uint16': int,
    'uint8': int,
}

yt.config["proxy"]["url"] = DST_CLUSTER

dst_table = '//projects/marketo-testing/export/{}'.format(TABLE_NAME)
schema = yt.get(dst_table+'/@')['schema']
logging.info('Got schema from yt: {}'.format(schema))

def yt_python_type(key):
    for field in schema:
        if field['name'] == key:
            return YT_TO_PYTHON_TYPES[field['type']]
    else:
        raise RuntimeError("Cannot find type mapping for key: {}".format(key))

def parse_data(data):
    for line in data:
        for key in line.keys():
            cast_type = yt_python_type(key)
            if line[key] == 'null':
                line[key] = None
            elif cast_type is float:
                line[key] = line[key].replace(',', '.')
            if line[key]:
                line[key] = cast_type(line[key])
        yield line


with io.open(INFILE, encoding='cp1251') as csvfile:
     csvreader = csv.DictReader(csvfile, delimiter=";")
     yt.write_table(dst_table, parse_data(csvreader))

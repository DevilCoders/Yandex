#!/usr/bin/env python2
import json
# noinspection PyPackageRequirements,PyUnresolvedReferences
import libra


class Record(object):
    def __init__(self, json_record):
        self.key = json_record["key"]
        self.subkey = json_record["subkey"]
        self.value = json_record["value"].encode("utf-8")

    @staticmethod
    def from_string(string):
        return Record(json.loads(string))


def read_file(filename):
    with open(filename) as fd:
        for line in fd:
            yield Record.from_string(line)


def parse_file(filename):
    return libra.ParseSession(read_file(filename), "blockstat.dict")


for request in parse_file("y6043383171459086372.sjson"):
    print(request)

#!/usr/bin/env python3
import re

from log_parser import LogParser


class MongoDBLogParser(LogParser):
    def __init__(self):
        super().__init__()
        self.origin = None
        # self.input_date_format = '%Y-%m-%dT%H:%M:%S.%f%z'
        self.re = re.compile(
            r'^(?P<key>\d+;\d+;\d+;)(?P<datetime>[^\s]+)\s+'
            r'(?P<severity>[^\s])\s+(?P<component>[^\s]+)\s+\[(?P<context>[^\]]+)\](\s+)?(?P<message>.*)'
        )

    def parse_record(self, record: str):
        match_obj = self.re.match(record)
        if match_obj:
            return match_obj.groupdict()


def main():
    parser = MongoDBLogParser()
    parser.run()


if __name__ == '__main__':
    main()

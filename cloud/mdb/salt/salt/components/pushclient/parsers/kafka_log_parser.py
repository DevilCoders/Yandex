#!/usr/bin/env python3
import re

from log_parser import LogParser


class KafkaLogParser(LogParser):
    def __init__(self):
        super().__init__()
        self.origin = 'kafka'
        self.re = re.compile(
            r'^(?P<key>\d+;\d+;\d+;)\[(?P<datetime>\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}.\d{3})\]\s+(?P<severity>\S+)\s+(?P<message>.*)'
        )

    def is_record_start(self, line):
        match = self.re.match(line)
        return match is not None

    def parse_record(self, record: str):
        match_obj = self.re.match(record)
        if match_obj:
            return match_obj.groupdict()


def main():
    parser = KafkaLogParser()
    parser.run()


if __name__ == '__main__':
    main()

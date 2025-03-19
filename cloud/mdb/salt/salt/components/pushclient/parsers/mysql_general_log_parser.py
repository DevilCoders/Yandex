#!/usr/bin/env python3
import re

from log_parser import LogParser


class GeneralLogParser(LogParser):
    def __init__(self):
        super().__init__()
        self.origin = 'mysql_general'
        # self.input_date_format = '%Y-%m-%dT%H:%M:%S.%f'
        self.re = re.compile(
            r'^(?P<key>\d+;\d+;\d+;)(?P<datetime>\S+)\s+(?P<id>\d+) (?P<command>\S+)\s+(?P<argument>.*)'
        )
        self.line_continuation_pattern = re.compile(r'^(?P<key>\d+;\d+;\d+;)\t\t')

    def is_record_start(self, s):
        return not self.line_continuation_pattern.match(s)

    def parse_record(self, record: str):
        match_obj = self.re.match(record)
        if not match_obj:
            return None

        result = match_obj.groupdict()
        result.setdefault('argument', '')

        # handle multiline entries:
        lines = record.split('\n')
        is_first_line = True
        for line in lines:
            if not line:
                continue
            if not is_first_line:
                result['argument'] += '\n' + self.cut_key(line)
            is_first_line = False

        result['raw'] = self.extract_raw(record)
        return result


def main():
    parser = GeneralLogParser()
    parser.run()


if __name__ == '__main__':
    main()

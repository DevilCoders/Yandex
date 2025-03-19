#!/usr/bin/env python3
import re
import time

from log_parser import LogParser


class ErrorLogParser(LogParser):
    # example: 2018-09-28T08:56:40.672148+03:00 2 [Note] WSREP: Setting wsrep_ready to true
    def __init__(self):
        super().__init__()
        self.fake_timestamp = time.time()
        self.origin = 'mysql_error'
        # self.input_date_format = '%Y-%m-%dT%H:%M:%S.%f'
        self.re_main = re.compile(
            r'^(?P<key>\d+;\d+;\d+;)(?P<datetime>\S+)\s+(?P<id>\d+)\s+(?P<status>\[.*?\])\s+(?P<message>.*)'
        )
        # innodb monitoring output
        self.re_inno = re.compile(
            r'^(?P<key>\d+;\d+;\d+;)(?P<datetime>\S+\s+\S+)\s+\S+\s+(?P<message>INNODB MONITOR OUTPUT.*)'
        )
        # deadlock monitoring output
        self.re_lock = re.compile(r'^(?P<key>\d+;\d+;\d+;)(?P<message>TRANSACTION\s+\d+,.*)')
        # tail of multiline message
        self.re_short = re.compile(r'^(?P<key>\d+;\d+;\d+;)(?P<message>.*)')

    def match_first_line(self, line):
        return self.re_main.match(line) or self.re_lock.match(line) or self.re_inno.match(line)

    def is_record_start(self, line):
        return self.match_first_line(line) is not None

    def parse_record(self, record: str) -> dict:
        lines = record.split('\n')
        match_obj = self.match_first_line(lines[0])
        if match_obj:
            result = match_obj.groupdict()
            for line in lines[1:]:
                match_obj_short = self.re_short.match(line)
                if not match_obj_short:
                    continue
                line_dict = match_obj_short.groupdict()
                result['key'] = line_dict['key']
                result['message'] += '\n' + line_dict['message']
            result.setdefault('id', 0)
            result.setdefault('status', '')
            result['status'] = result['status'].replace('[', '').replace(']', '').strip().title()
            result['raw'] = self.extract_raw(record)
            return result

    def parse_timestamp(self, dt):
        ts = super().parse_timestamp(dt)
        if not ts:
            ts = self.fake_timestamp
        self.fake_timestamp = ts + (int(ts % 1 * 1000) + 1.1) / 1000
        return ts


def main():
    parser = ErrorLogParser()
    parser.run()


if __name__ == '__main__':
    main()

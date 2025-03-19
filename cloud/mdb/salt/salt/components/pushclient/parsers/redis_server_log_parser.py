#!/usr/bin/env python3
import re
import time
from datetime import timezone, timedelta

from log_parser import LogParser

LOCAL_TZ = timezone(timedelta(seconds=-1 * (time.altzone if time.daylight else time.timezone)))


class ServerLogParser(LogParser):
    def __init__(self):
        super().__init__()
        self.origin = 'redis'
        # self.input_date_format = '%d %b %Y %H:%M:%S.%f'
        self.re = re.compile(
            r'^(?P<key>\d+;\d+;\d+;)(?P<pid>\d+):(?P<role>\S+) (?P<datetime>\S+ \S+ \S+ \S+)\s+[#\*\-]\s+(?P<message>.*)'
        )

    def parse_record(self, record: str):
        match_obj = self.re.match(record)
        if match_obj:
            return match_obj.groupdict()


def main():
    parser = ServerLogParser()
    parser.run()


if __name__ == '__main__':
    main()

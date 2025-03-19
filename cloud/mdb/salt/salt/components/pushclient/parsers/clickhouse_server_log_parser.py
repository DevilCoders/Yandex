#!/usr/bin/env python3
import re
import time
from datetime import timezone, timedelta

from log_parser import LogParser

LOCAL_TZ = timezone(timedelta(seconds=-1 * (time.altzone if time.daylight else time.timezone)))


class ServerLogParser(LogParser):
    def __init__(self):
        super().__init__()
        self.origin = 'clickhouse'
        # self.input_date_format = '%Y.%m.%d %H:%M:%S.%f'
        self.re = re.compile(
            r'^(?P<key>\d+;\d+;\d+;)(?P<datetime>\S+ \S+)\s+\[ (?P<thread>\S+) \]\s+{(?P<query_id>\S+)?}\s+'
            r'<(?P<severity>\S+)>\s+(?P<component>.*?(?=:\s)):\s+(?P<message>\S.*)'
        )
        self.re_message = re.compile(r'(?P<key>\d+;\d+;\d+;)(?P<message>.*)')

    def is_record_start(self, line):
        match = self.re.match(line)
        return match is not None

    def parse_record(self, record: str):
        lines = record.split('\n')
        match_obj = self.re.match(lines[0])
        if not match_obj:
            return
        result = match_obj.groupdict()
        for i in range(1, len(lines)):
            message_part_match = self.re_message.match(lines[i])
            if not message_part_match:
                continue
            message_part = message_part_match.groupdict()
            result['message'] += r'\n' + message_part['message']
            result['key'] = message_part['key']
            # TODO: remove after deploying a new logsdb shard
            if result['severity'] == 'Trace':
                return
        return result


def main():
    parser = ServerLogParser()
    parser.run()


if __name__ == '__main__':
    main()

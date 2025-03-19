#!/usr/bin/env python3
import json

from log_parser import LogParser

REDACTED = 'If you have a new license, please update it. Otherwise, please reach out to'


class ElasticsearchLogParser(LogParser):
    def __init__(self):
        super().__init__()
        self.origin = None

    def parse_record(self, record: str):
        if REDACTED in record:
            return

        parts = record.split(';', 3)
        if len(parts) != 4:
            return

        parsed = json.loads(parts[3])
        if parsed:
            parsed['key'] = ';'.join(parts[:3]) + ';'
            parsed['datetime'] = parsed['timestamp']
            return parsed


def main():
    parser = ElasticsearchLogParser()
    parser.run()


if __name__ == '__main__':
    main()

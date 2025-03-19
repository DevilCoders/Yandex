#!/usr/bin/env python3
import json

from log_parser import LogParser


class MongoDBLogParser(LogParser):
    def __init__(self):
        super().__init__()
        self.origin = None

    def parse_record(self, record: str):
        parts = record.split(';', 3)
        if len(parts) != 4:
            return

        parsed = json.loads(parts[3])
        if parsed:
            return {
                'key': ';'.join(parts[:3]) + ';',
                'datetime': parsed['t']['$date'],
                'severity': parsed['s'],
                'component': parsed['c'],
                'context': parsed['ctx'],
                'message': parts[3],
            }


def main():
    parser = MongoDBLogParser()
    parser.run()


if __name__ == '__main__':
    main()

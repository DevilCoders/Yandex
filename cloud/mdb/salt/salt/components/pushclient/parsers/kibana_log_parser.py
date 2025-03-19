#!/usr/bin/env python3
import json

from log_parser import LogParser


class KibanaLogParser(LogParser):
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
                'datetime': parsed['@timestamp'],
                # We don't have log types in kibana, but we can't get rid of this line because 'type' field is
                # required for the database, so we make it a common 'log'
                'type': "log",
                'message': parts[3],
            }


def main():
    parser = KibanaLogParser()
    parser.run()


if __name__ == '__main__':
    main()

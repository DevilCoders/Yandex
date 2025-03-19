#!/usr/bin/env python3
import re
import json
from log_parser import LogParser

result_template = {
    "name": "",
    "record": "",
    "timestamp": "",
    "command_class": "",
    "connection_id": "",
    "connection_type": "",
    "db": "",
    "host": "",
    "ip": "",
    "mysql_version": "",
    "os_login": "",
    "os_version": "",
    "priv_user": "",
    "proxy_user": "",
    "server_id": "",
    "sqltext": "",
    "startup_optionsi": "",
    "status": "",
    "status_code": "",
    "user": "",
    "version": "",
}


class AuditLogParser(LogParser):
    def __init__(self):
        super().__init__()
        self.origin = 'mysql_audit'
        # self.input_date_format = '%Y-%m-%dT%H:%M:%S.%f'
        self.re = re.compile(r'^(?P<key>\d+;\d+;\d+;)(?P<object>.*)')

    def parse_record(self, record: str) -> dict:
        match_obj = self.re.match(record)
        if match_obj is not None:
            record_dict = match_obj.groupdict()
            result = json.loads(record_dict['object'])['audit_record']
            result['key'] = record_dict['key']
            allElements = result_template.copy()
            allElements.update(result)

            allElements['datetime'] = allElements['timestamp']
            allElements.pop('timestamp', None)

            allElements['hostname'] = allElements['host']
            allElements.pop('host', None)

            allElements['raw'] = self.extract_raw(record)
            return allElements


def main():
    parser = AuditLogParser()
    parser.run()


if __name__ == '__main__':
    main()
